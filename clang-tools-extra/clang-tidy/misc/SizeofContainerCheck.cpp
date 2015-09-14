//===--- SizeofContainerCheck.cpp - clang-tidy-----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "SizeofContainerCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

using namespace clang::ast_matchers;

namespace clang {
namespace tidy {

void SizeofContainerCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(
      expr(unless(isInTemplateInstantiation()),
           expr(sizeOfExpr(has(expr(hasType(hasCanonicalType(hasDeclaration(
                    recordDecl(matchesName("^(::std::|::string)"),
                               unless(matchesName("^::std::(bitset|array)$")),
                               hasMethod(methodDecl(hasName("size"), isPublic(),
                                                    isConst()))))))))))
               .bind("sizeof"),
           // Ignore ARRAYSIZE(<array of containers>) pattern.
           unless(hasAncestor(binaryOperator(
               anyOf(hasOperatorName("/"), hasOperatorName("%")),
               hasLHS(ignoringParenCasts(sizeOfExpr(expr()))),
               hasRHS(ignoringParenCasts(equalsBoundNode("sizeof"))))))),
      this);
}

void SizeofContainerCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *SizeOf =
      Result.Nodes.getNodeAs<UnaryExprOrTypeTraitExpr>("sizeof");

  auto Diag =
      diag(SizeOf->getLocStart(), "sizeof() doesn't return the size of the "
                                  "container; did you mean .size()?");
}

} // namespace tidy
} // namespace clang

