//===-- AlphaISelLowering.h - Alpha DAG Lowering Interface ------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the interfaces that Alpha uses to lower LLVM code into a
// selection DAG.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TARGET_ALPHA_ALPHAISELLOWERING_H
#define LLVM_TARGET_ALPHA_ALPHAISELLOWERING_H

#include "llvm/ADT/VectorExtras.h"
#include "llvm/Target/TargetLowering.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "Alpha.h"

namespace llvm {

  namespace AlphaISD {
    enum NodeType {
      // Start the numbering where the builting ops and target ops leave off.
      FIRST_NUMBER = ISD::BUILTIN_OP_END,
      //These corrospond to the identical Instruction
      CVTQT_, CVTQS_, CVTTQ_,

      /// GPRelHi/GPRelLo - These represent the high and low 16-bit
      /// parts of a global address respectively.
      GPRelHi, GPRelLo, 

      /// RetLit - Literal Relocation of a Global
      RelLit,

      /// GlobalRetAddr - used to restore the return address
      GlobalRetAddr,
      
      /// CALL - Normal call.
      CALL,

      /// DIVCALL - used for special library calls for div and rem
      DivCall,
      
      /// return flag operand
      RET_FLAG,

      /// CHAIN = COND_BRANCH CHAIN, OPC, (G|F)PRC, DESTBB [, INFLAG] - This
      /// corresponds to the COND_BRANCH pseudo instruction.  
      /// *PRC is the input register to compare to zero,
      /// OPC is the branch opcode to use (e.g. Alpha::BEQ),
      /// DESTBB is the destination block to branch to, and INFLAG is
      /// an optional input flag argument.
      COND_BRANCH_I, COND_BRANCH_F

    };
  }

  class AlphaTargetLowering : public TargetLowering {
    int VarArgsOffset;  // What is the offset to the first vaarg
    int VarArgsBase;    // What is the base FrameIndex
    bool useITOF;
  public:
    explicit AlphaTargetLowering(TargetMachine &TM);
    
    /// getSetCCResultType - Get the SETCC result ValueType
    virtual MVT::SimpleValueType getSetCCResultType(MVT VT) const;

    /// LowerOperation - Provide custom lowering hooks for some operations.
    ///
    virtual SDValue LowerOperation(SDValue Op, SelectionDAG &DAG);

    /// ReplaceNodeResults - Replace the results of node with an illegal result
    /// type with new values built out of custom code.
    ///
    virtual void ReplaceNodeResults(SDNode *N, SmallVectorImpl<SDValue>&Results,
                                    SelectionDAG &DAG);

    // Friendly names for dumps
    const char *getTargetNodeName(unsigned Opcode) const;

    SDValue LowerCallResult(SDValue Chain, SDValue InFlag,
                            unsigned CallConv, bool isVarArg,
                            const SmallVectorImpl<ISD::InputArg> &Ins,
                            DebugLoc dl, SelectionDAG &DAG,
                            SmallVectorImpl<SDValue> &InVals);

    ConstraintType getConstraintType(const std::string &Constraint) const;

    std::vector<unsigned> 
      getRegClassForInlineAsmConstraint(const std::string &Constraint,
                                        MVT VT) const;

    bool hasITOF() { return useITOF; }

    MachineBasicBlock *EmitInstrWithCustomInserter(MachineInstr *MI,
                                                   MachineBasicBlock *BB) const;

    virtual bool isOffsetFoldingLegal(const GlobalAddressSDNode *GA) const;

    /// getFunctionAlignment - Return the Log2 alignment of this function.
    virtual unsigned getFunctionAlignment(const Function *F) const;

  private:
    // Helpers for custom lowering.
    void LowerVAARG(SDNode *N, SDValue &Chain, SDValue &DataPtr,
                    SelectionDAG &DAG);

    virtual SDValue
      LowerFormalArguments(SDValue Chain,
                           unsigned CallConv, bool isVarArg,
                           const SmallVectorImpl<ISD::InputArg> &Ins,
                           DebugLoc dl, SelectionDAG &DAG,
                           SmallVectorImpl<SDValue> &InVals);

    virtual SDValue
      LowerCall(SDValue Chain, SDValue Callee,
                unsigned CallConv, bool isVarArg, bool isTailCall,
                const SmallVectorImpl<ISD::OutputArg> &Outs,
                const SmallVectorImpl<ISD::InputArg> &Ins,
                DebugLoc dl, SelectionDAG &DAG,
                SmallVectorImpl<SDValue> &InVals);

    virtual SDValue
      LowerReturn(SDValue Chain,
                  unsigned CallConv, bool isVarArg,
                  const SmallVectorImpl<ISD::OutputArg> &Outs,
                  DebugLoc dl, SelectionDAG &DAG);
  };
}

#endif   // LLVM_TARGET_ALPHA_ALPHAISELLOWERING_H
