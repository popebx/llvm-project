//===--- ReplaceCstringSlicingCheck.cpp - clang-tidy ----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ReplaceCstringSlicingCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecordLayout.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/Lex/Lexer.h"

using namespace clang::ast_matchers;

namespace clang::tidy::misc {

void ReplaceCstringSlicingCheck::registerMatchers(MatchFinder *Finder) {


  const auto isCStringEx =
      anyOf(hasType(cxxRecordDecl(hasName("CStringEx"))),
            hasType(references(cxxRecordDecl(hasName("CStringEx")))));
  const auto isCstringT = ofClass(cxxRecordDecl(
      hasName("CStringT"), classTemplateSpecializationDecl(hasTemplateArgument(
                               0, refersToType(asString("char"))))));

  const auto sliceInAssignment =
      callExpr(callee(cxxMethodDecl(anyOf(isCopyAssignmentOperator(),
                                          isMoveAssignmentOperator()),
                                    isCstringT)),
               hasArgument(1, isCStringEx),
               has(implicitCastExpr(has(
                   implicitCastExpr(has(declRefExpr().bind("variable")))))));
  const auto sliceInConstructor = cxxConstructExpr(
      hasDeclaration(cxxConstructorDecl(
          anyOf(isCopyConstructor(), isMoveConstructor()), isCstringT)),
      hasArgument(0, isCStringEx),
      has(implicitCastExpr(
          has(implicitCastExpr(has(declRefExpr().bind("variable")))))));
  Finder->addMatcher(traverse(TK_AsIs, expr(sliceInConstructor)),
                     this);
  Finder->addMatcher(traverse(TK_AsIs, expr(sliceInAssignment)),
                     this);
}

void ReplaceCstringSlicingCheck::check(const MatchFinder::MatchResult &Result) {  
  const auto *variable = Result.Nodes.getNodeAs<DeclRefExpr>("variable");

  assert(variable != nullptr);
 
  auto &SM = Result.Context->getSourceManager();
  auto &LO = Result.Context->getLangOpts();
  auto charRange = Lexer::getAsCharRange(variable->getSourceRange(), SM, LO);

  diag(charRange.getEnd(), "Slicing CStringEx to CString!")
      << FixItHint::CreateInsertion(charRange.getEnd(), ".GetString()");
}

} // namespace clang::tidy::misc
