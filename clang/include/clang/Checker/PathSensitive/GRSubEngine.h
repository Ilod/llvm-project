//== GRSubEngine.h - Interface of the subengine of GRCoreEngine ----*- C++ -*-//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the interface of a subengine of the GRCoreEngine.
//
//===----------------------------------------------------------------------===//
#ifndef LLVM_CLANG_ANALYSIS_GRSUBENGINE_H
#define LLVM_CLANG_ANALYSIS_GRSUBENGINE_H

#include "clang/Checker/PathSensitive/SVals.h"

namespace clang {

class Stmt;
class CFGBlock;
class CFGElement;
class ExplodedNode;
class GRState;
class GRStateManager;
class GRBlockCounter;
class GRStmtNodeBuilder;
class GRBranchNodeBuilder;
class GRIndirectGotoNodeBuilder;
class GRSwitchNodeBuilder;
class GREndPathNodeBuilder;
class GRCallEnterNodeBuilder;
class GRCallExitNodeBuilder;
class LocationContext;

class GRSubEngine {
public:
  virtual ~GRSubEngine() {}

  virtual const GRState* getInitialState(const LocationContext *InitLoc) = 0;

  virtual AnalysisManager &getAnalysisManager() = 0;

  virtual GRStateManager &getStateManager() = 0;

  /// Called by GRCoreEngine. Used to generate new successor
  /// nodes by processing the 'effects' of a block-level statement.
  virtual void ProcessStmt(const CFGElement E, GRStmtNodeBuilder& builder) = 0;

  /// Called by GRCoreEngine when start processing
  /// a CFGBlock.  This method returns true if the analysis should continue
  /// exploring the given path, and false otherwise.
  virtual bool ProcessBlockEntrance(const CFGBlock* B, const ExplodedNode *Pred,
                                    GRBlockCounter BC) = 0;

  /// Called by GRCoreEngine.  Used to generate successor
  ///  nodes by processing the 'effects' of a branch condition.
  virtual void ProcessBranch(const Stmt* Condition, const Stmt* Term,
                             GRBranchNodeBuilder& builder) = 0;

  /// Called by GRCoreEngine.  Used to generate successor
  /// nodes by processing the 'effects' of a computed goto jump.
  virtual void ProcessIndirectGoto(GRIndirectGotoNodeBuilder& builder) = 0;

  /// Called by GRCoreEngine.  Used to generate successor
  /// nodes by processing the 'effects' of a switch statement.
  virtual void ProcessSwitch(GRSwitchNodeBuilder& builder) = 0;

  /// ProcessEndPath - Called by GRCoreEngine.  Used to generate end-of-path
  ///  nodes when the control reaches the end of a function.
  virtual void ProcessEndPath(GREndPathNodeBuilder& builder) = 0;

  // Generate the entry node of the callee.
  virtual void ProcessCallEnter(GRCallEnterNodeBuilder &builder) = 0;

  // Generate the first post callsite node.
  virtual void ProcessCallExit(GRCallExitNodeBuilder &builder) = 0;
  
  /// Called by ConstraintManager. Used to call checker-specific
  /// logic for handling assumptions on symbolic values.
  virtual const GRState* ProcessAssume(const GRState *state,
                                       SVal cond, bool assumption) = 0;
  
  /// Called by GRCoreEngine when the analysis worklist is either empty or the
  //  maximum number of analysis steps have been reached.
  virtual void ProcessEndWorklist(bool hasWorkRemaining) = 0;
};
}

#endif
