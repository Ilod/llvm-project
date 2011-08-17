#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"
#include "clang/StaticAnalyzer/Core/CheckerRegistry.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"

using namespace clang;
using namespace ento;

namespace {
class MainCallChecker : public Checker < check::PreStmt<CallExpr> > {
  mutable llvm::OwningPtr<BugType> BT;

public:
  void checkPreStmt(const CallExpr *CE, CheckerContext &C) const;
};
} // end anonymous namespace

void MainCallChecker::checkPreStmt(const CallExpr *CE, CheckerContext &C) const {
  const ProgramState *state = C.getState();
  const Expr *Callee = CE->getCallee();
  const FunctionDecl *FD = state->getSVal(Callee).getAsFunctionDecl();

  if (!FD)
    return;

  // Get the name of the callee.
  IdentifierInfo *II = FD->getIdentifier();
  if (!II)   // if no identifier, not a simple C function
    return;

  if (II->isStr("main")) {
    ExplodedNode *N = C.generateSink();
    if (!N)
      return;

    if (!BT)
      BT.reset(new BugType("call to main", "example analyzer plugin"));

    RangedBugReport *report = new RangedBugReport(*BT, BT->getName(), N);
    report->addRange(Callee->getSourceRange());
    C.EmitReport(report);    
  }
}

// Register plugin!
extern "C"
void clang_registerCheckers (CheckerRegistry &registry) {
  registry.addChecker<MainCallChecker>("example.MainCallChecker", "Disallows calls to functions called main");
}

extern "C"
const char clang_analyzerAPIVersionString[] = CLANG_ANALYZER_API_VERSION_STRING;
