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
  constexpr std::string_view baseClassName = "CStringT<char, StrTraitMFC<>>";
  constexpr std::string_view derivedClassName = "CStringEx";
  const auto OfBaseClass = ofClass(cxxRecordDecl().bind("BaseDecl"));
  const auto IsDerivedFromBaseDecl =
      cxxRecordDecl(hasName(derivedClassName),
                    isDerivedFrom(equalsBoundNode("BaseDecl")))
          .bind("DerivedDecl");
  const auto HasTypeDerivedFromBaseDecl =
      anyOf(hasType(IsDerivedFromBaseDecl),
            hasType(references(IsDerivedFromBaseDecl)));
  const auto IsCallToBaseClass = hasParent(cxxConstructorDecl(
      ofClass(isSameOrDerivedFrom(equalsBoundNode("DerivedDecl"))),
      hasAnyConstructorInitializer(allOf(
          isBaseInitializer(), withInitializer(equalsBoundNode("Call"))))));

  // Assignment slicing: "a = b;" and "a = std::move(b);" variants.
  const auto SlicesObjectInAssignment =
      callExpr(expr().bind("Call"),
               callee(cxxMethodDecl(anyOf(isCopyAssignmentOperator(),
                                          isMoveAssignmentOperator()),
                                    OfBaseClass)),
               hasArgument(1, HasTypeDerivedFromBaseDecl));

  // Construction slicing: "A a{b};" and "f(b);" variants. Note that in case of
  // slicing the letter will create a temporary and therefore call a ctor.
  const auto SlicesObjectInCtor = cxxConstructExpr(
      expr().bind("Call"),
      hasDeclaration(cxxConstructorDecl(
          anyOf(isCopyConstructor(), isMoveConstructor()), OfBaseClass)),
      hasArgument(0, HasTypeDerivedFromBaseDecl),
      // We need to disable matching on the call to the base copy/move
      // constructor in DerivedDecl's constructors.
      unless(IsCallToBaseClass));

  const auto construction = cxxConstructExpr(
      hasDeclaration(
          cxxConstructorDecl(anyOf(isCopyConstructor(), isMoveConstructor()),
                             hasName("CString"))
              .bind("BaseClass")),
      hasArgument(
          0, anyOf(hasType(cxxRecordDecl(hasName("CStringEx")).bind("Derived")),
                   hasType(references(
                       cxxRecordDecl(hasName("CStringEx")).bind("Derived"))))),
      has(implicitCastExpr(
          has(implicitCastExpr(has(declRefExpr().bind("variable")))))));

  Finder->addMatcher(
      traverse(TK_AsIs, expr(SlicesObjectInAssignment).bind("Call")), this);
  Finder->addMatcher(traverse(TK_AsIs, construction), this);
}

void ReplaceCstringSlicingCheck::check(const MatchFinder::MatchResult &Result) {
  const auto *BaseDecl = Result.Nodes.getNodeAs<CXXRecordDecl>("BaseDecl");
  const auto *DerivedDecl =
      Result.Nodes.getNodeAs<CXXRecordDecl>("DerivedDecl");
  const auto *Call = Result.Nodes.getNodeAs<CXXConstructExpr>("Call");
  const auto *variable = Result.Nodes.getNodeAs<DeclRefExpr>("variable");

  assert(variable != nullptr);
  /* assert(BaseDecl != nullptr);
   assert(DerivedDecl != nullptr);
   assert(Call != nullptr);*/

  // Warn when slicing member variables.
  // const auto &BaseLayout =
  //    BaseDecl->getASTContext().getASTRecordLayout(BaseDecl);
  // const auto &DerivedLayout =
  //    DerivedDecl->getASTContext().getASTRecordLayout(DerivedDecl);
  // const CharUnits StateSize =
  //    DerivedLayout.getDataSize() - BaseLayout.getDataSize();
  // if (StateSize.isPositive()) {
  //    auto *arg = Call->getArg(0);
  
  //  auto sourceRange = Call->getSourceRange();
  auto &SM = Result.Context->getSourceManager();
  auto &LO = Result.Context->getLangOpts();
  auto charRange = Lexer::getAsCharRange(variable->getSourceRange(), SM, LO);
  
  /*diag(Call->getExprLoc(), "slicing object from type %0 to %1 discards "
                           "%2 bytes of state")
      << DerivedDecl << BaseDecl <<
     static_cast<int>(StateSize.getQuantity());*/
  diag(charRange.getEnd(), "Slicing CStringEx to CString!")
      << FixItHint::CreateInsertion(charRange.getEnd(), ".GetString()");
  // }
}

} // namespace clang::tidy::misc
