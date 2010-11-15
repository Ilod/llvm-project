//===-- PPCCodeEmitter.cpp - JIT Code Emitter for PowerPC32 -------*- C++ -*-=//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the PowerPC 32-bit CodeEmitter and associated machinery to
// JIT-compile bitcode to native PowerPC.
//
//===----------------------------------------------------------------------===//

#include "PPCTargetMachine.h"
#include "PPCRelocations.h"
#include "PPC.h"
#include "llvm/Module.h"
#include "llvm/PassManager.h"
#include "llvm/CodeGen/JITCodeEmitter.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetOptions.h"
using namespace llvm;

namespace {
  class PPCCodeEmitter : public MachineFunctionPass {
    TargetMachine &TM;
    JITCodeEmitter &MCE;
    MachineModuleInfo *MMI;
    
    void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.addRequired<MachineModuleInfo>();
      MachineFunctionPass::getAnalysisUsage(AU);
    }
    
    static char ID;
    
    /// MovePCtoLROffset - When/if we see a MovePCtoLR instruction, we record
    /// its address in the function into this pointer.
    void *MovePCtoLROffset;
  public:
    
    PPCCodeEmitter(TargetMachine &tm, JITCodeEmitter &mce)
      : MachineFunctionPass(ID), TM(tm), MCE(mce) {}

    /// getBinaryCodeForInstr - This function, generated by the
    /// CodeEmitterGenerator using TableGen, produces the binary encoding for
    /// machine instructions.
    unsigned getBinaryCodeForInstr(const MachineInstr &MI) const;

    
    MachineRelocation GetRelocation(const MachineOperand &MO,
                                    unsigned RelocID) const;
    
    /// getMachineOpValue - evaluates the MachineOperand of a given MachineInstr
    unsigned getMachineOpValue(const MachineInstr &MI,
                               const MachineOperand &MO) const;

    unsigned get_crbitm_encoding(const MachineInstr &MI, unsigned OpNo) const;
    unsigned getCallTargetEncoding(const MachineInstr &MI, unsigned OpNo) const;
    
    const char *getPassName() const { return "PowerPC Machine Code Emitter"; }

    /// runOnMachineFunction - emits the given MachineFunction to memory
    ///
    bool runOnMachineFunction(MachineFunction &MF);

    /// emitBasicBlock - emits the given MachineBasicBlock to memory
    ///
    void emitBasicBlock(MachineBasicBlock &MBB);
  };
}

char PPCCodeEmitter::ID = 0;

/// createPPCCodeEmitterPass - Return a pass that emits the collected PPC code
/// to the specified MCE object.
FunctionPass *llvm::createPPCJITCodeEmitterPass(PPCTargetMachine &TM,
                                                JITCodeEmitter &JCE) {
  return new PPCCodeEmitter(TM, JCE);
}

bool PPCCodeEmitter::runOnMachineFunction(MachineFunction &MF) {
  assert((MF.getTarget().getRelocationModel() != Reloc::Default ||
          MF.getTarget().getRelocationModel() != Reloc::Static) &&
         "JIT relocation model must be set to static or default!");

  MMI = &getAnalysis<MachineModuleInfo>();
  MCE.setModuleInfo(MMI);
  do {
    MovePCtoLROffset = 0;
    MCE.startFunction(MF);
    for (MachineFunction::iterator BB = MF.begin(), E = MF.end(); BB != E; ++BB)
      emitBasicBlock(*BB);
  } while (MCE.finishFunction(MF));

  return false;
}

void PPCCodeEmitter::emitBasicBlock(MachineBasicBlock &MBB) {
  MCE.StartMachineBasicBlock(&MBB);

  for (MachineBasicBlock::iterator I = MBB.begin(), E = MBB.end(); I != E; ++I){
    const MachineInstr &MI = *I;
    MCE.processDebugLoc(MI.getDebugLoc(), true);
    switch (MI.getOpcode()) {
    default:
      MCE.emitWordBE(getBinaryCodeForInstr(MI));
      break;
    case TargetOpcode::PROLOG_LABEL:
    case TargetOpcode::EH_LABEL:
      MCE.emitLabel(MI.getOperand(0).getMCSymbol());
      break;
    case TargetOpcode::IMPLICIT_DEF:
    case TargetOpcode::KILL:
      break; // pseudo opcode, no side effects
    case PPC::MovePCtoLR:
    case PPC::MovePCtoLR8:
      assert(TM.getRelocationModel() == Reloc::PIC_);
      MovePCtoLROffset = (void*)MCE.getCurrentPCValue();
      MCE.emitWordBE(0x48000005);   // bl 1
      break;
    }
    MCE.processDebugLoc(MI.getDebugLoc(), false);
  }
}

unsigned PPCCodeEmitter::get_crbitm_encoding(const MachineInstr &MI,
                                             unsigned OpNo) const {
  const MachineOperand &MO = MI.getOperand(OpNo);
  assert((MI.getOpcode() == PPC::MTCRF || MI.getOpcode() == PPC::MFOCRF) &&
         (MO.getReg() >= PPC::CR0 && MO.getReg() <= PPC::CR7));
  return 0x80 >> PPCRegisterInfo::getRegisterNumbering(MO.getReg());
}

MachineRelocation PPCCodeEmitter::GetRelocation(const MachineOperand &MO, 
                                                unsigned RelocID) const {
  if (MO.isGlobal())
    return MachineRelocation::getGV(MCE.getCurrentPCOffset(), RelocID,
                                    const_cast<GlobalValue *>(MO.getGlobal()),0,
                                    isa<Function>(MO.getGlobal()));
  if (MO.isSymbol())
    return MachineRelocation::getExtSym(MCE.getCurrentPCOffset(),
                                        RelocID, MO.getSymbolName(), 0);
  if (MO.isCPI())
    return MachineRelocation::getConstPool(MCE.getCurrentPCOffset(),
                                           RelocID, MO.getIndex(), 0);

  if (MO.isMBB())
    MCE.addRelocation(MachineRelocation::getBB(MCE.getCurrentPCOffset(),
                                               RelocID, MO.getMBB()));
  
  assert(MO.isJTI());
  return MachineRelocation::getJumpTable(MCE.getCurrentPCOffset(),
                                         RelocID, MO.getIndex(), 0);
}

unsigned PPCCodeEmitter::getCallTargetEncoding(const MachineInstr &MI,
                                               unsigned OpNo) const {
  const MachineOperand &MO = MI.getOperand(OpNo);
  if (MO.isReg() || MO.isImm()) return getMachineOpValue(MI, MO);
  
  MCE.addRelocation(GetRelocation(MO, PPC::reloc_pcrel_bx));
  return 0;
}


unsigned PPCCodeEmitter::getMachineOpValue(const MachineInstr &MI,
                                           const MachineOperand &MO) const {

  if (MO.isReg()) {
    assert(MI.getOpcode() != PPC::MTCRF && MI.getOpcode() != PPC::MFOCRF);
    return PPCRegisterInfo::getRegisterNumbering(MO.getReg());
  }
  
  if (MO.isImm())
    return MO.getImm();
  
  if (MO.isGlobal() || MO.isSymbol() || MO.isCPI() || MO.isJTI()) {
    unsigned Reloc = 0;
    assert((TM.getRelocationModel() != Reloc::PIC_ || MovePCtoLROffset) &&
           "MovePCtoLR not seen yet?");
    switch (MI.getOpcode()) {
    default: MI.dump(); llvm_unreachable("Unknown instruction for relocation!");
    case PPC::LIS:
    case PPC::LIS8:
    case PPC::ADDIS:
    case PPC::ADDIS8:
      Reloc = PPC::reloc_absolute_high;       // Pointer to symbol
      break;
    case PPC::LI:
    case PPC::LI8:
    case PPC::LA:
    // Loads.
    case PPC::LBZ:
    case PPC::LBZ8:
    case PPC::LHA:
    case PPC::LHA8:
    case PPC::LHZ:
    case PPC::LHZ8:
    case PPC::LWZ:
    case PPC::LWZ8:
    case PPC::LFS:
    case PPC::LFD:

    // Stores.
    case PPC::STB:
    case PPC::STB8:
    case PPC::STH:
    case PPC::STH8:
    case PPC::STW:
    case PPC::STW8:
    case PPC::STFS:
    case PPC::STFD:
      Reloc = PPC::reloc_absolute_low;
      break;

    case PPC::LWA:
    case PPC::LD:
    case PPC::STD:
    case PPC::STD_32:
      Reloc = PPC::reloc_absolute_low_ix;
      break;
    }

    MachineRelocation R = GetRelocation(MO, Reloc);

    // If in PIC mode, we need to encode the negated address of the
    // 'movepctolr' into the unrelocated field.  After relocation, we'll have
    // &gv-&movepctolr-4 in the imm field.  Once &movepctolr is added to the imm
    // field, we get &gv.  This doesn't happen for branch relocations, which are
    // always implicitly pc relative.
    if (TM.getRelocationModel() == Reloc::PIC_) {
      assert(MovePCtoLROffset && "MovePCtoLR not seen yet?");
      R.setConstantVal(-(intptr_t)MovePCtoLROffset - 4);
    }
    MCE.addRelocation(R);

  } else if (MO.isMBB()) {
    unsigned Reloc = 0;
    unsigned Opcode = MI.getOpcode();
    if (Opcode == PPC::B)
      Reloc = PPC::reloc_pcrel_bx;
    else // BCC instruction
      Reloc = PPC::reloc_pcrel_bcx;

    MCE.addRelocation(MachineRelocation::getBB(MCE.getCurrentPCOffset(),
                                               Reloc, MO.getMBB()));
  } else {
#ifndef NDEBUG
    errs() << "ERROR: Unknown type of MachineOperand: " << MO << "\n";
#endif
    llvm_unreachable(0);
  }

  return 0;
}

#include "PPCGenCodeEmitter.inc"
