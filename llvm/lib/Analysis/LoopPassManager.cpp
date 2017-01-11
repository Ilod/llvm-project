//===- LoopPassManager.cpp - Loop pass management -------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/Analysis/LoopPassManager.h"
#include "llvm/Analysis/BasicAliasAnalysis.h"
#include "llvm/Analysis/GlobalsModRef.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionAliasAnalysis.h"
#include "llvm/IR/Dominators.h"

using namespace llvm;

// Explicit template instantiations and specialization defininitions for core
// template typedefs.
namespace llvm {
template class AllAnalysesOn<Loop>;
template class AnalysisManager<Loop, LoopStandardAnalysisResults &>;
template class PassManager<Loop, LoopAnalysisManager,
                           LoopStandardAnalysisResults &, LPMUpdater &>;
template class InnerAnalysisManagerProxy<LoopAnalysisManager, Function>;
template class OuterAnalysisManagerProxy<FunctionAnalysisManager, Loop,
                                         LoopStandardAnalysisResults &>;

/// Explicitly specialize the pass manager's run method to handle loop nest
/// structure updates.
template <>
PreservedAnalyses
PassManager<Loop, LoopAnalysisManager, LoopStandardAnalysisResults &,
            LPMUpdater &>::run(Loop &L, LoopAnalysisManager &AM,
                               LoopStandardAnalysisResults &AR, LPMUpdater &U) {
  PreservedAnalyses PA = PreservedAnalyses::all();

  if (DebugLogging)
    dbgs() << "Starting Loop pass manager run.\n";

  for (auto &Pass : Passes) {
    if (DebugLogging)
      dbgs() << "Running pass: " << Pass->name() << " on " << L;

    PreservedAnalyses PassPA = Pass->run(L, AM, AR, U);

    // If the loop was deleted, abort the run and return to the outer walk.
    if (U.skipCurrentLoop()) {
      PA.intersect(std::move(PassPA));
      break;
    }

    // Update the analysis manager as each pass runs and potentially
    // invalidates analyses.
    AM.invalidate(L, PassPA);

    // Finally, we intersect the final preserved analyses to compute the
    // aggregate preserved set for this pass manager.
    PA.intersect(std::move(PassPA));

    // FIXME: Historically, the pass managers all called the LLVM context's
    // yield function here. We don't have a generic way to acquire the
    // context and it isn't yet clear what the right pattern is for yielding
    // in the new pass manager so it is currently omitted.
    // ...getContext().yield();
  }

  // Invalidation for the current loop should be handled above, and other loop
  // analysis results shouldn't be impacted by runs over this loop. Therefore,
  // the remaining analysis results in the AnalysisManager are preserved. We
  // mark this with a set so that we don't need to inspect each one
  // individually.
  // FIXME: This isn't correct! This loop and all nested loops' analyses should
  // be preserved, but unrolling should invalidate the parent loop's analyses.
  PA.preserveSet<AllAnalysesOn<Loop>>();

  if (DebugLogging)
    dbgs() << "Finished Loop pass manager run.\n";

  return PA;
}

bool LoopAnalysisManagerFunctionProxy::Result::invalidate(
    Function &F, const PreservedAnalyses &PA,
    FunctionAnalysisManager::Invalidator &Inv) {
  // First compute the sequence of IR units covered by this proxy. We will want
  // to visit this in postorder, but because this is a tree structure we can do
  // this by building a preorder sequence and walking it in reverse.
  SmallVector<Loop *, 4> PreOrderLoops, PreOrderWorklist;
  // Note that we want to walk the roots in reverse order because we will end
  // up reversing the preorder sequence. However, it happens that the loop nest
  // roots are in reverse order within the LoopInfo object. So we just walk
  // forward here.
  // FIXME: If we change the order of LoopInfo we will want to add a reverse
  // here.
  for (Loop *RootL : *LI) {
    assert(PreOrderWorklist.empty() &&
           "Must start with an empty preorder walk worklist.");
    PreOrderWorklist.push_back(RootL);
    do {
      Loop *L = PreOrderWorklist.pop_back_val();
      PreOrderWorklist.append(L->begin(), L->end());
      PreOrderLoops.push_back(L);
    } while (!PreOrderWorklist.empty());
  }

  // If this proxy or the loop info is going to be invalidated, we also need
  // to clear all the keys coming from that analysis. We also completely blow
  // away the loop analyses if any of the standard analyses provided by the
  // loop pass manager go away so that loop analyses can freely use these
  // without worrying about declaring dependencies on them etc.
  // FIXME: It isn't clear if this is the right tradeoff. We could instead make
  // loop analyses declare any dependencies on these and use the more general
  // invalidation logic below to act on that.
  auto PAC = PA.getChecker<LoopAnalysisManagerFunctionProxy>();
  if (!(PAC.preserved() || PAC.preservedSet<AllAnalysesOn<Function>>()) ||
      Inv.invalidate<AAManager>(F, PA) ||
      Inv.invalidate<AssumptionAnalysis>(F, PA) ||
      Inv.invalidate<DominatorTreeAnalysis>(F, PA) ||
      Inv.invalidate<LoopAnalysis>(F, PA) ||
      Inv.invalidate<ScalarEvolutionAnalysis>(F, PA)) {
    // Note that the LoopInfo may be stale at this point, however the loop
    // objects themselves remain the only viable keys that could be in the
    // analysis manager's cache. So we just walk the keys and forcibly clear
    // those results. Note that the order doesn't matter here as this will just
    // directly destroy the results without calling methods on them.
    for (Loop *L : PreOrderLoops)
      InnerAM->clear(*L);

    // We also need to null out the inner AM so that when the object gets
    // destroyed as invalid we don't try to clear the inner AM again. At that
    // point we won't be able to reliably walk the loops for this function and
    // only clear results associated with those loops the way we do here.
    // FIXME: Making InnerAM null at this point isn't very nice. Most analyses
    // try to remain valid during invalidation. Maybe we should add an
    // `IsClean` flag?
    InnerAM = nullptr;

    // Now return true to indicate this *is* invalid and a fresh proxy result
    // needs to be built. This is especially important given the null InnerAM.
    return true;
  }

  // Directly check if the relevant set is preserved so we can short circuit
  // invalidating loops.
  bool AreLoopAnalysesPreserved =
      PA.allAnalysesInSetPreserved<AllAnalysesOn<Loop>>();

  // Since we have a valid LoopInfo we can actually leave the cached results in
  // the analysis manager associated with the Loop keys, but we need to
  // propagate any necessary invalidation logic into them. We'd like to
  // invalidate things in roughly the same order as they were put into the
  // cache and so we walk the preorder list in reverse to form a valid
  // postorder.
  for (Loop *L : reverse(PreOrderLoops)) {
    Optional<PreservedAnalyses> InnerPA;

    // Check to see whether the preserved set needs to be adjusted based on
    // function-level analysis invalidation triggering deferred invalidation
    // for this loop.
    if (auto *OuterProxy =
            InnerAM->getCachedResult<FunctionAnalysisManagerLoopProxy>(*L))
      for (const auto &OuterInvalidationPair :
           OuterProxy->getOuterInvalidations()) {
        AnalysisKey *OuterAnalysisID = OuterInvalidationPair.first;
        const auto &InnerAnalysisIDs = OuterInvalidationPair.second;
        if (Inv.invalidate(OuterAnalysisID, F, PA)) {
          if (!InnerPA)
            InnerPA = PA;
          for (AnalysisKey *InnerAnalysisID : InnerAnalysisIDs)
            InnerPA->abandon(InnerAnalysisID);
        }
      }

    // Check if we needed a custom PA set. If so we'll need to run the inner
    // invalidation.
    if (InnerPA) {
      InnerAM->invalidate(*L, *InnerPA);
      continue;
    }

    // Otherwise we only need to do invalidation if the original PA set didn't
    // preserve all Loop analyses.
    if (!AreLoopAnalysesPreserved)
      InnerAM->invalidate(*L, PA);
  }

  // Return false to indicate that this result is still a valid proxy.
  return false;
}

template <>
LoopAnalysisManagerFunctionProxy::Result
LoopAnalysisManagerFunctionProxy::run(Function &F,
                                      FunctionAnalysisManager &AM) {
  return Result(*InnerAM, AM.getResult<LoopAnalysis>(F));
}
}

PreservedAnalyses llvm::getLoopPassPreservedAnalyses() {
  PreservedAnalyses PA;
  PA.preserve<AssumptionAnalysis>();
  PA.preserve<DominatorTreeAnalysis>();
  PA.preserve<LoopAnalysis>();
  PA.preserve<LoopAnalysisManagerFunctionProxy>();
  PA.preserve<ScalarEvolutionAnalysis>();
  // TODO: What we really want to do here is preserve an AA category, but that
  // concept doesn't exist yet.
  PA.preserve<AAManager>();
  PA.preserve<BasicAA>();
  PA.preserve<GlobalsAA>();
  PA.preserve<SCEVAA>();
  return PA;
}

PrintLoopPass::PrintLoopPass() : OS(dbgs()) {}
PrintLoopPass::PrintLoopPass(raw_ostream &OS, const std::string &Banner)
    : OS(OS), Banner(Banner) {}

PreservedAnalyses PrintLoopPass::run(Loop &L, LoopAnalysisManager &,
                                     LoopStandardAnalysisResults &,
                                     LPMUpdater &) {
  printLoop(L, OS, Banner);
  return PreservedAnalyses::all();
}
