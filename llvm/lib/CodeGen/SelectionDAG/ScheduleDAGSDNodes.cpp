//===--- ScheduleDAGSDNodes.cpp - Implement the ScheduleDAGSDNodes class --===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This implements the ScheduleDAG class, which is a base class used by
// scheduling implementation classes.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "pre-RA-sched"
#include "SDNodeDbgValue.h"
#include "ScheduleDAGSDNodes.h"
#include "InstrEmitter.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetInstrInfo.h"
#include "llvm/Target/TargetLowering.h"
#include "llvm/Target/TargetRegisterInfo.h"
#include "llvm/Target/TargetSubtarget.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;

STATISTIC(LoadsClustered, "Number of loads clustered together");

// This allows latency based scheduler to notice high latency instructions
// without a target itinerary. The choise if number here has more to do with
// balancing scheduler heursitics than with the actual machine latency.
static cl::opt<int> HighLatencyCycles(
  "sched-high-latency-cycles", cl::Hidden, cl::init(10),
  cl::desc("Roughly estimate the number of cycles that 'long latency'"
           "instructions take for targets with no itinerary"));

ScheduleDAGSDNodes::ScheduleDAGSDNodes(MachineFunction &mf)
  : ScheduleDAG(mf),
    InstrItins(mf.getTarget().getInstrItineraryData()) {}

/// Run - perform scheduling.
///
void ScheduleDAGSDNodes::Run(SelectionDAG *dag, MachineBasicBlock *bb,
                             MachineBasicBlock::iterator insertPos) {
  DAG = dag;
  ScheduleDAG::Run(bb, insertPos);
}

/// NewSUnit - Creates a new SUnit and return a ptr to it.
///
SUnit *ScheduleDAGSDNodes::NewSUnit(SDNode *N) {
#ifndef NDEBUG
  const SUnit *Addr = 0;
  if (!SUnits.empty())
    Addr = &SUnits[0];
#endif
  SUnits.push_back(SUnit(N, (unsigned)SUnits.size()));
  assert((Addr == 0 || Addr == &SUnits[0]) &&
         "SUnits std::vector reallocated on the fly!");
  SUnits.back().OrigNode = &SUnits.back();
  SUnit *SU = &SUnits.back();
  const TargetLowering &TLI = DAG->getTargetLoweringInfo();
  if (!N ||
      (N->isMachineOpcode() &&
       N->getMachineOpcode() == TargetOpcode::IMPLICIT_DEF))
    SU->SchedulingPref = Sched::None;
  else
    SU->SchedulingPref = TLI.getSchedulingPreference(N);
  return SU;
}

SUnit *ScheduleDAGSDNodes::Clone(SUnit *Old) {
  SUnit *SU = NewSUnit(Old->getNode());
  SU->OrigNode = Old->OrigNode;
  SU->Latency = Old->Latency;
  SU->isVRegCycle = Old->isVRegCycle;
  SU->isCall = Old->isCall;
  SU->isCallOp = Old->isCallOp;
  SU->isTwoAddress = Old->isTwoAddress;
  SU->isCommutable = Old->isCommutable;
  SU->hasPhysRegDefs = Old->hasPhysRegDefs;
  SU->hasPhysRegClobbers = Old->hasPhysRegClobbers;
  SU->isScheduleHigh = Old->isScheduleHigh;
  SU->isScheduleLow = Old->isScheduleLow;
  SU->SchedulingPref = Old->SchedulingPref;
  Old->isCloned = true;
  return SU;
}

/// CheckForPhysRegDependency - Check if the dependency between def and use of
/// a specified operand is a physical register dependency. If so, returns the
/// register and the cost of copying the register.
static void CheckForPhysRegDependency(SDNode *Def, SDNode *User, unsigned Op,
                                      const TargetRegisterInfo *TRI,
                                      const TargetInstrInfo *TII,
                                      unsigned &PhysReg, int &Cost) {
  if (Op != 2 || User->getOpcode() != ISD::CopyToReg)
    return;

  unsigned Reg = cast<RegisterSDNode>(User->getOperand(1))->getReg();
  if (TargetRegisterInfo::isVirtualRegister(Reg))
    return;

  unsigned ResNo = User->getOperand(2).getResNo();
  if (Def->isMachineOpcode()) {
    const TargetInstrDesc &II = TII->get(Def->getMachineOpcode());
    if (ResNo >= II.getNumDefs() &&
        II.ImplicitDefs[ResNo - II.getNumDefs()] == Reg) {
      PhysReg = Reg;
      const TargetRegisterClass *RC =
        TRI->getMinimalPhysRegClass(Reg, Def->getValueType(ResNo));
      Cost = RC->getCopyCost();
    }
  }
}

static void AddGlue(SDNode *N, SDValue Glue, bool AddGlue, SelectionDAG *DAG) {
  SmallVector<EVT, 4> VTs;
  SDNode *GlueDestNode = Glue.getNode();

  // Don't add glue from a node to itself.
  if (GlueDestNode == N) return;

  // Don't add glue to something which already has glue.
  if (N->getValueType(N->getNumValues() - 1) == MVT::Glue) return;

  for (unsigned I = 0, E = N->getNumValues(); I != E; ++I)
    VTs.push_back(N->getValueType(I));

  if (AddGlue)
    VTs.push_back(MVT::Glue);

  SmallVector<SDValue, 4> Ops;
  for (unsigned I = 0, E = N->getNumOperands(); I != E; ++I)
    Ops.push_back(N->getOperand(I));

  if (GlueDestNode)
    Ops.push_back(Glue);

  SDVTList VTList = DAG->getVTList(&VTs[0], VTs.size());
  MachineSDNode::mmo_iterator Begin = 0, End = 0;
  MachineSDNode *MN = dyn_cast<MachineSDNode>(N);

  // Store memory references.
  if (MN) {
    Begin = MN->memoperands_begin();
    End = MN->memoperands_end();
  }

  DAG->MorphNodeTo(N, N->getOpcode(), VTList, &Ops[0], Ops.size());

  // Reset the memory references
  if (MN)
    MN->setMemRefs(Begin, End);
}

/// ClusterNeighboringLoads - Force nearby loads together by "gluing" them.
/// This function finds loads of the same base and different offsets. If the
/// offsets are not far apart (target specific), it add MVT::Glue inputs and
/// outputs to ensure they are scheduled together and in order. This
/// optimization may benefit some targets by improving cache locality.
void ScheduleDAGSDNodes::ClusterNeighboringLoads(SDNode *Node) {
  SDNode *Chain = 0;
  unsigned NumOps = Node->getNumOperands();
  if (Node->getOperand(NumOps-1).getValueType() == MVT::Other)
    Chain = Node->getOperand(NumOps-1).getNode();
  if (!Chain)
    return;

  // Look for other loads of the same chain. Find loads that are loading from
  // the same base pointer and different offsets.
  SmallPtrSet<SDNode*, 16> Visited;
  SmallVector<int64_t, 4> Offsets;
  DenseMap<long long, SDNode*> O2SMap;  // Map from offset to SDNode.
  bool Cluster = false;
  SDNode *Base = Node;
  for (SDNode::use_iterator I = Chain->use_begin(), E = Chain->use_end();
       I != E; ++I) {
    SDNode *User = *I;
    if (User == Node || !Visited.insert(User))
      continue;
    int64_t Offset1, Offset2;
    if (!TII->areLoadsFromSameBasePtr(Base, User, Offset1, Offset2) ||
        Offset1 == Offset2)
      // FIXME: Should be ok if they addresses are identical. But earlier
      // optimizations really should have eliminated one of the loads.
      continue;
    if (O2SMap.insert(std::make_pair(Offset1, Base)).second)
      Offsets.push_back(Offset1);
    O2SMap.insert(std::make_pair(Offset2, User));
    Offsets.push_back(Offset2);
    if (Offset2 < Offset1)
      Base = User;
    Cluster = true;
  }

  if (!Cluster)
    return;

  // Sort them in increasing order.
  std::sort(Offsets.begin(), Offsets.end());

  // Check if the loads are close enough.
  SmallVector<SDNode*, 4> Loads;
  unsigned NumLoads = 0;
  int64_t BaseOff = Offsets[0];
  SDNode *BaseLoad = O2SMap[BaseOff];
  Loads.push_back(BaseLoad);
  for (unsigned i = 1, e = Offsets.size(); i != e; ++i) {
    int64_t Offset = Offsets[i];
    SDNode *Load = O2SMap[Offset];
    if (!TII->shouldScheduleLoadsNear(BaseLoad, Load, BaseOff, Offset,NumLoads))
      break; // Stop right here. Ignore loads that are further away.
    Loads.push_back(Load);
    ++NumLoads;
  }

  if (NumLoads == 0)
    return;

  // Cluster loads by adding MVT::Glue outputs and inputs. This also
  // ensure they are scheduled in order of increasing addresses.
  SDNode *Lead = Loads[0];
  AddGlue(Lead, SDValue(0, 0), true, DAG);

  SDValue InGlue = SDValue(Lead, Lead->getNumValues() - 1);
  for (unsigned I = 1, E = Loads.size(); I != E; ++I) {
    bool OutGlue = I < E - 1;
    SDNode *Load = Loads[I];

    AddGlue(Load, InGlue, OutGlue, DAG);

    if (OutGlue)
      InGlue = SDValue(Load, Load->getNumValues() - 1);

    ++LoadsClustered;
  }
}

/// ClusterNodes - Cluster certain nodes which should be scheduled together.
///
void ScheduleDAGSDNodes::ClusterNodes() {
  for (SelectionDAG::allnodes_iterator NI = DAG->allnodes_begin(),
       E = DAG->allnodes_end(); NI != E; ++NI) {
    SDNode *Node = &*NI;
    if (!Node || !Node->isMachineOpcode())
      continue;

    unsigned Opc = Node->getMachineOpcode();
    const TargetInstrDesc &TID = TII->get(Opc);
    if (TID.mayLoad())
      // Cluster loads from "near" addresses into combined SUnits.
      ClusterNeighboringLoads(Node);
  }
}

void ScheduleDAGSDNodes::BuildSchedUnits() {
  // During scheduling, the NodeId field of SDNode is used to map SDNodes
  // to their associated SUnits by holding SUnits table indices. A value
  // of -1 means the SDNode does not yet have an associated SUnit.
  unsigned NumNodes = 0;
  for (SelectionDAG::allnodes_iterator NI = DAG->allnodes_begin(),
       E = DAG->allnodes_end(); NI != E; ++NI) {
    NI->setNodeId(-1);
    ++NumNodes;
  }

  // Reserve entries in the vector for each of the SUnits we are creating.  This
  // ensure that reallocation of the vector won't happen, so SUnit*'s won't get
  // invalidated.
  // FIXME: Multiply by 2 because we may clone nodes during scheduling.
  // This is a temporary workaround.
  SUnits.reserve(NumNodes * 2);

  // Add all nodes in depth first order.
  SmallVector<SDNode*, 64> Worklist;
  SmallPtrSet<SDNode*, 64> Visited;
  Worklist.push_back(DAG->getRoot().getNode());
  Visited.insert(DAG->getRoot().getNode());

  SmallVector<SUnit*, 8> CallSUnits;
  while (!Worklist.empty()) {
    SDNode *NI = Worklist.pop_back_val();

    // Add all operands to the worklist unless they've already been added.
    for (unsigned i = 0, e = NI->getNumOperands(); i != e; ++i)
      if (Visited.insert(NI->getOperand(i).getNode()))
        Worklist.push_back(NI->getOperand(i).getNode());

    if (isPassiveNode(NI))  // Leaf node, e.g. a TargetImmediate.
      continue;

    // If this node has already been processed, stop now.
    if (NI->getNodeId() != -1) continue;

    SUnit *NodeSUnit = NewSUnit(NI);

    // See if anything is glued to this node, if so, add them to glued
    // nodes.  Nodes can have at most one glue input and one glue output.  Glue
    // is required to be the last operand and result of a node.

    // Scan up to find glued preds.
    SDNode *N = NI;
    while (N->getNumOperands() &&
           N->getOperand(N->getNumOperands()-1).getValueType() == MVT::Glue) {
      N = N->getOperand(N->getNumOperands()-1).getNode();
      assert(N->getNodeId() == -1 && "Node already inserted!");
      N->setNodeId(NodeSUnit->NodeNum);
      if (N->isMachineOpcode() && TII->get(N->getMachineOpcode()).isCall())
        NodeSUnit->isCall = true;
    }

    // Scan down to find any glued succs.
    N = NI;
    while (N->getValueType(N->getNumValues()-1) == MVT::Glue) {
      SDValue GlueVal(N, N->getNumValues()-1);

      // There are either zero or one users of the Glue result.
      bool HasGlueUse = false;
      for (SDNode::use_iterator UI = N->use_begin(), E = N->use_end();
           UI != E; ++UI)
        if (GlueVal.isOperandOf(*UI)) {
          HasGlueUse = true;
          assert(N->getNodeId() == -1 && "Node already inserted!");
          N->setNodeId(NodeSUnit->NodeNum);
          N = *UI;
          if (N->isMachineOpcode() && TII->get(N->getMachineOpcode()).isCall())
            NodeSUnit->isCall = true;
          break;
        }
      if (!HasGlueUse) break;
    }

    if (NodeSUnit->isCall)
      CallSUnits.push_back(NodeSUnit);

    // Schedule zero-latency TokenFactor below any nodes that may increase the
    // schedule height. Otherwise, ancestors of the TokenFactor may appear to
    // have false stalls.
    if (NI->getOpcode() == ISD::TokenFactor)
      NodeSUnit->isScheduleLow = true;

    // If there are glue operands involved, N is now the bottom-most node
    // of the sequence of nodes that are glued together.
    // Update the SUnit.
    NodeSUnit->setNode(N);
    assert(N->getNodeId() == -1 && "Node already inserted!");
    N->setNodeId(NodeSUnit->NodeNum);

    // Compute NumRegDefsLeft. This must be done before AddSchedEdges.
    InitNumRegDefsLeft(NodeSUnit);

    // Assign the Latency field of NodeSUnit using target-provided information.
    ComputeLatency(NodeSUnit);
  }

  // Find all call operands.
  while (!CallSUnits.empty()) {
    SUnit *SU = CallSUnits.pop_back_val();
    for (const SDNode *SUNode = SU->getNode(); SUNode;
         SUNode = SUNode->getGluedNode()) {
      if (SUNode->getOpcode() != ISD::CopyToReg)
        continue;
      SDNode *SrcN = SUNode->getOperand(2).getNode();
      if (isPassiveNode(SrcN)) continue;   // Not scheduled.
      SUnit *SrcSU = &SUnits[SrcN->getNodeId()];
      SrcSU->isCallOp = true;
    }
  }
}

void ScheduleDAGSDNodes::AddSchedEdges() {
  const TargetSubtarget &ST = TM.getSubtarget<TargetSubtarget>();

  // Check to see if the scheduler cares about latencies.
  bool UnitLatencies = ForceUnitLatencies();

  // Pass 2: add the preds, succs, etc.
  for (unsigned su = 0, e = SUnits.size(); su != e; ++su) {
    SUnit *SU = &SUnits[su];
    SDNode *MainNode = SU->getNode();

    if (MainNode->isMachineOpcode()) {
      unsigned Opc = MainNode->getMachineOpcode();
      const TargetInstrDesc &TID = TII->get(Opc);
      for (unsigned i = 0; i != TID.getNumOperands(); ++i) {
        if (TID.getOperandConstraint(i, TOI::TIED_TO) != -1) {
          SU->isTwoAddress = true;
          break;
        }
      }
      if (TID.isCommutable())
        SU->isCommutable = true;
    }

    // Find all predecessors and successors of the group.
    for (SDNode *N = SU->getNode(); N; N = N->getGluedNode()) {
      if (N->isMachineOpcode() &&
          TII->get(N->getMachineOpcode()).getImplicitDefs()) {
        SU->hasPhysRegClobbers = true;
        unsigned NumUsed = InstrEmitter::CountResults(N);
        while (NumUsed != 0 && !N->hasAnyUseOfValue(NumUsed - 1))
          --NumUsed;    // Skip over unused values at the end.
        if (NumUsed > TII->get(N->getMachineOpcode()).getNumDefs())
          SU->hasPhysRegDefs = true;
      }

      for (unsigned i = 0, e = N->getNumOperands(); i != e; ++i) {
        SDNode *OpN = N->getOperand(i).getNode();
        if (isPassiveNode(OpN)) continue;   // Not scheduled.
        SUnit *OpSU = &SUnits[OpN->getNodeId()];
        assert(OpSU && "Node has no SUnit!");
        if (OpSU == SU) continue;           // In the same group.

        EVT OpVT = N->getOperand(i).getValueType();
        assert(OpVT != MVT::Glue && "Glued nodes should be in same sunit!");
        bool isChain = OpVT == MVT::Other;

        unsigned PhysReg = 0;
        int Cost = 1;
        // Determine if this is a physical register dependency.
        CheckForPhysRegDependency(OpN, N, i, TRI, TII, PhysReg, Cost);
        assert((PhysReg == 0 || !isChain) &&
               "Chain dependence via physreg data?");
        // FIXME: See ScheduleDAGSDNodes::EmitCopyFromReg. For now, scheduler
        // emits a copy from the physical register to a virtual register unless
        // it requires a cross class copy (cost < 0). That means we are only
        // treating "expensive to copy" register dependency as physical register
        // dependency. This may change in the future though.
        if (Cost >= 0 && !StressSched)
          PhysReg = 0;

        // If this is a ctrl dep, latency is 1.
        unsigned OpLatency = isChain ? 1 : OpSU->Latency;
        // Special-case TokenFactor chains as zero-latency.
        if(isChain && OpN->getOpcode() == ISD::TokenFactor)
          OpLatency = 0;

        const SDep &dep = SDep(OpSU, isChain ? SDep::Order : SDep::Data,
                               OpLatency, PhysReg);
        if (!isChain && !UnitLatencies) {
          ComputeOperandLatency(OpN, N, i, const_cast<SDep &>(dep));
          ST.adjustSchedDependency(OpSU, SU, const_cast<SDep &>(dep));
        }

        if (!SU->addPred(dep) && !dep.isCtrl() && OpSU->NumRegDefsLeft > 1) {
          // Multiple register uses are combined in the same SUnit. For example,
          // we could have a set of glued nodes with all their defs consumed by
          // another set of glued nodes. Register pressure tracking sees this as
          // a single use, so to keep pressure balanced we reduce the defs.
          //
          // We can't tell (without more book-keeping) if this results from
          // glued nodes or duplicate operands. As long as we don't reduce
          // NumRegDefsLeft to zero, we handle the common cases well.
          --OpSU->NumRegDefsLeft;
        }
      }
    }
  }
}

/// BuildSchedGraph - Build the SUnit graph from the selection dag that we
/// are input.  This SUnit graph is similar to the SelectionDAG, but
/// excludes nodes that aren't interesting to scheduling, and represents
/// glued together nodes with a single SUnit.
void ScheduleDAGSDNodes::BuildSchedGraph(AliasAnalysis *AA) {
  // Cluster certain nodes which should be scheduled together.
  ClusterNodes();
  // Populate the SUnits array.
  BuildSchedUnits();
  // Compute all the scheduling dependencies between nodes.
  AddSchedEdges();
}

// Initialize NumNodeDefs for the current Node's opcode.
void ScheduleDAGSDNodes::RegDefIter::InitNodeNumDefs() {
  // Check for phys reg copy.
  if (!Node)
    return;

  if (!Node->isMachineOpcode()) {
    if (Node->getOpcode() == ISD::CopyFromReg)
      NodeNumDefs = 1;
    else
      NodeNumDefs = 0;
    return;
  }
  unsigned POpc = Node->getMachineOpcode();
  if (POpc == TargetOpcode::IMPLICIT_DEF) {
    // No register need be allocated for this.
    NodeNumDefs = 0;
    return;
  }
  unsigned NRegDefs = SchedDAG->TII->get(Node->getMachineOpcode()).getNumDefs();
  // Some instructions define regs that are not represented in the selection DAG
  // (e.g. unused flags). See tMOVi8. Make sure we don't access past NumValues.
  NodeNumDefs = std::min(Node->getNumValues(), NRegDefs);
  DefIdx = 0;
}

// Construct a RegDefIter for this SUnit and find the first valid value.
ScheduleDAGSDNodes::RegDefIter::RegDefIter(const SUnit *SU,
                                           const ScheduleDAGSDNodes *SD)
  : SchedDAG(SD), Node(SU->getNode()), DefIdx(0), NodeNumDefs(0) {
  InitNodeNumDefs();
  Advance();
}

// Advance to the next valid value defined by the SUnit.
void ScheduleDAGSDNodes::RegDefIter::Advance() {
  for (;Node;) { // Visit all glued nodes.
    for (;DefIdx < NodeNumDefs; ++DefIdx) {
      if (!Node->hasAnyUseOfValue(DefIdx))
        continue;
      ValueType = Node->getValueType(DefIdx);
      ++DefIdx;
      return; // Found a normal regdef.
    }
    Node = Node->getGluedNode();
    if (Node == NULL) {
      return; // No values left to visit.
    }
    InitNodeNumDefs();
  }
}

void ScheduleDAGSDNodes::InitNumRegDefsLeft(SUnit *SU) {
  assert(SU->NumRegDefsLeft == 0 && "expect a new node");
  for (RegDefIter I(SU, this); I.IsValid(); I.Advance()) {
    assert(SU->NumRegDefsLeft < USHRT_MAX && "overflow is ok but unexpected");
    ++SU->NumRegDefsLeft;
  }
}

void ScheduleDAGSDNodes::ComputeLatency(SUnit *SU) {
  SDNode *N = SU->getNode();

  // TokenFactor operands are considered zero latency, and some schedulers
  // (e.g. Top-Down list) may rely on the fact that operand latency is nonzero
  // whenever node latency is nonzero.
  if (N && N->getOpcode() == ISD::TokenFactor) {
    SU->Latency = 0;
    return;
  }

  // Check to see if the scheduler cares about latencies.
  if (ForceUnitLatencies()) {
    SU->Latency = 1;
    return;
  }

  if (!InstrItins || InstrItins->isEmpty()) {
    if (N && N->isMachineOpcode() &&
        TII->isHighLatencyDef(N->getMachineOpcode()))
      SU->Latency = HighLatencyCycles;
    else
      SU->Latency = 1;
    return;
  }

  // Compute the latency for the node.  We use the sum of the latencies for
  // all nodes glued together into this SUnit.
  SU->Latency = 0;
  for (SDNode *N = SU->getNode(); N; N = N->getGluedNode())
    if (N->isMachineOpcode())
      SU->Latency += TII->getInstrLatency(InstrItins, N);
}

void ScheduleDAGSDNodes::ComputeOperandLatency(SDNode *Def, SDNode *Use,
                                               unsigned OpIdx, SDep& dep) const{
  // Check to see if the scheduler cares about latencies.
  if (ForceUnitLatencies())
    return;

  if (dep.getKind() != SDep::Data)
    return;

  unsigned DefIdx = Use->getOperand(OpIdx).getResNo();
  if (Use->isMachineOpcode())
    // Adjust the use operand index by num of defs.
    OpIdx += TII->get(Use->getMachineOpcode()).getNumDefs();
  int Latency = TII->getOperandLatency(InstrItins, Def, DefIdx, Use, OpIdx);
  if (Latency > 1 && Use->getOpcode() == ISD::CopyToReg &&
      !BB->succ_empty()) {
    unsigned Reg = cast<RegisterSDNode>(Use->getOperand(1))->getReg();
    if (TargetRegisterInfo::isVirtualRegister(Reg))
      // This copy is a liveout value. It is likely coalesced, so reduce the
      // latency so not to penalize the def.
      // FIXME: need target specific adjustment here?
      Latency = (Latency > 1) ? Latency - 1 : 1;
  }
  if (Latency >= 0)
    dep.setLatency(Latency);
}

void ScheduleDAGSDNodes::dumpNode(const SUnit *SU) const {
  if (!SU->getNode()) {
    dbgs() << "PHYS REG COPY\n";
    return;
  }

  SU->getNode()->dump(DAG);
  dbgs() << "\n";
  SmallVector<SDNode *, 4> GluedNodes;
  for (SDNode *N = SU->getNode()->getGluedNode(); N; N = N->getGluedNode())
    GluedNodes.push_back(N);
  while (!GluedNodes.empty()) {
    dbgs() << "    ";
    GluedNodes.back()->dump(DAG);
    dbgs() << "\n";
    GluedNodes.pop_back();
  }
}

namespace {
  struct OrderSorter {
    bool operator()(const std::pair<unsigned, MachineInstr*> &A,
                    const std::pair<unsigned, MachineInstr*> &B) {
      return A.first < B.first;
    }
  };
}

/// ProcessSDDbgValues - Process SDDbgValues associated with this node.
static void ProcessSDDbgValues(SDNode *N, SelectionDAG *DAG,
                               InstrEmitter &Emitter,
                    SmallVector<std::pair<unsigned, MachineInstr*>, 32> &Orders,
                            DenseMap<SDValue, unsigned> &VRBaseMap,
                            unsigned Order) {
  if (!N->getHasDebugValue())
    return;

  // Opportunistically insert immediate dbg_value uses, i.e. those with source
  // order number right after the N.
  MachineBasicBlock *BB = Emitter.getBlock();
  MachineBasicBlock::iterator InsertPos = Emitter.getInsertPos();
  ArrayRef<SDDbgValue*> DVs = DAG->GetDbgValues(N);
  for (unsigned i = 0, e = DVs.size(); i != e; ++i) {
    if (DVs[i]->isInvalidated())
      continue;
    unsigned DVOrder = DVs[i]->getOrder();
    if (!Order || DVOrder == ++Order) {
      MachineInstr *DbgMI = Emitter.EmitDbgValue(DVs[i], VRBaseMap);
      if (DbgMI) {
        Orders.push_back(std::make_pair(DVOrder, DbgMI));
        BB->insert(InsertPos, DbgMI);
      }
      DVs[i]->setIsInvalidated();
    }
  }
}

// ProcessSourceNode - Process nodes with source order numbers. These are added
// to a vector which EmitSchedule uses to determine how to insert dbg_value
// instructions in the right order.
static void ProcessSourceNode(SDNode *N, SelectionDAG *DAG,
                           InstrEmitter &Emitter,
                           DenseMap<SDValue, unsigned> &VRBaseMap,
                    SmallVector<std::pair<unsigned, MachineInstr*>, 32> &Orders,
                           SmallSet<unsigned, 8> &Seen) {
  unsigned Order = DAG->GetOrdering(N);
  if (!Order || !Seen.insert(Order)) {
    // Process any valid SDDbgValues even if node does not have any order
    // assigned.
    ProcessSDDbgValues(N, DAG, Emitter, Orders, VRBaseMap, 0);
    return;
  }

  MachineBasicBlock *BB = Emitter.getBlock();
  if (Emitter.getInsertPos() == BB->begin() || BB->back().isPHI()) {
    // Did not insert any instruction.
    Orders.push_back(std::make_pair(Order, (MachineInstr*)0));
    return;
  }

  Orders.push_back(std::make_pair(Order, prior(Emitter.getInsertPos())));
  ProcessSDDbgValues(N, DAG, Emitter, Orders, VRBaseMap, Order);
}


/// EmitSchedule - Emit the machine code in scheduled order.
MachineBasicBlock *ScheduleDAGSDNodes::EmitSchedule() {
  InstrEmitter Emitter(BB, InsertPos);
  DenseMap<SDValue, unsigned> VRBaseMap;
  DenseMap<SUnit*, unsigned> CopyVRBaseMap;
  SmallVector<std::pair<unsigned, MachineInstr*>, 32> Orders;
  SmallSet<unsigned, 8> Seen;
  bool HasDbg = DAG->hasDebugValues();

  // If this is the first BB, emit byval parameter dbg_value's.
  if (HasDbg && BB->getParent()->begin() == MachineFunction::iterator(BB)) {
    SDDbgInfo::DbgIterator PDI = DAG->ByvalParmDbgBegin();
    SDDbgInfo::DbgIterator PDE = DAG->ByvalParmDbgEnd();
    for (; PDI != PDE; ++PDI) {
      MachineInstr *DbgMI= Emitter.EmitDbgValue(*PDI, VRBaseMap);
      if (DbgMI)
        BB->insert(InsertPos, DbgMI);
    }
  }

  for (unsigned i = 0, e = Sequence.size(); i != e; i++) {
    SUnit *SU = Sequence[i];
    if (!SU) {
      // Null SUnit* is a noop.
      EmitNoop();
      continue;
    }

    // For pre-regalloc scheduling, create instructions corresponding to the
    // SDNode and any glued SDNodes and append them to the block.
    if (!SU->getNode()) {
      // Emit a copy.
      EmitPhysRegCopy(SU, CopyVRBaseMap);
      continue;
    }

    SmallVector<SDNode *, 4> GluedNodes;
    for (SDNode *N = SU->getNode()->getGluedNode(); N;
         N = N->getGluedNode())
      GluedNodes.push_back(N);
    while (!GluedNodes.empty()) {
      SDNode *N = GluedNodes.back();
      Emitter.EmitNode(GluedNodes.back(), SU->OrigNode != SU, SU->isCloned,
                       VRBaseMap);
      // Remember the source order of the inserted instruction.
      if (HasDbg)
        ProcessSourceNode(N, DAG, Emitter, VRBaseMap, Orders, Seen);
      GluedNodes.pop_back();
    }
    Emitter.EmitNode(SU->getNode(), SU->OrigNode != SU, SU->isCloned,
                     VRBaseMap);
    // Remember the source order of the inserted instruction.
    if (HasDbg)
      ProcessSourceNode(SU->getNode(), DAG, Emitter, VRBaseMap, Orders,
                        Seen);
  }

  // Insert all the dbg_values which have not already been inserted in source
  // order sequence.
  if (HasDbg) {
    MachineBasicBlock::iterator BBBegin = BB->getFirstNonPHI();

    // Sort the source order instructions and use the order to insert debug
    // values.
    std::sort(Orders.begin(), Orders.end(), OrderSorter());

    SDDbgInfo::DbgIterator DI = DAG->DbgBegin();
    SDDbgInfo::DbgIterator DE = DAG->DbgEnd();
    // Now emit the rest according to source order.
    unsigned LastOrder = 0;
    for (unsigned i = 0, e = Orders.size(); i != e && DI != DE; ++i) {
      unsigned Order = Orders[i].first;
      MachineInstr *MI = Orders[i].second;
      // Insert all SDDbgValue's whose order(s) are before "Order".
      if (!MI)
        continue;
      for (; DI != DE &&
             (*DI)->getOrder() >= LastOrder && (*DI)->getOrder() < Order; ++DI) {
        if ((*DI)->isInvalidated())
          continue;
        MachineInstr *DbgMI = Emitter.EmitDbgValue(*DI, VRBaseMap);
        if (DbgMI) {
          if (!LastOrder)
            // Insert to start of the BB (after PHIs).
            BB->insert(BBBegin, DbgMI);
          else {
            // Insert at the instruction, which may be in a different
            // block, if the block was split by a custom inserter.
            MachineBasicBlock::iterator Pos = MI;
            MI->getParent()->insert(llvm::next(Pos), DbgMI);
          }
        }
      }
      LastOrder = Order;
    }
    // Add trailing DbgValue's before the terminator. FIXME: May want to add
    // some of them before one or more conditional branches?
    while (DI != DE) {
      MachineBasicBlock *InsertBB = Emitter.getBlock();
      MachineBasicBlock::iterator Pos= Emitter.getBlock()->getFirstTerminator();
      if (!(*DI)->isInvalidated()) {
        MachineInstr *DbgMI= Emitter.EmitDbgValue(*DI, VRBaseMap);
        if (DbgMI)
          InsertBB->insert(Pos, DbgMI);
      }
      ++DI;
    }
  }

  BB = Emitter.getBlock();
  InsertPos = Emitter.getInsertPos();
  return BB;
}
