//===--- ReplaceCstringSlicingCheck.h - clang-tidy --------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MISC_REPLACECSTRINGSLICINGCHECK_H
#define LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MISC_REPLACECSTRINGSLICINGCHECK_H

#include "../ClangTidyCheck.h"

namespace clang::tidy::misc {

/// FIXME: Write a short description.
///
/// For the user-facing documentation see:
/// http://clang.llvm.org/extra/clang-tidy/checks/misc/replace-cstring-slicing.html
class ReplaceCstringSlicingCheck : public ClangTidyCheck {
public:
  ReplaceCstringSlicingCheck(StringRef Name, ClangTidyContext *Context)
      : ClangTidyCheck(Name, Context) {}
  void registerMatchers(ast_matchers::MatchFinder *Finder) override;
  void check(const ast_matchers::MatchFinder::MatchResult &Result) override;
  void diagnoseSlicedOverriddenMethods(const Expr &Call,
                                       const CXXRecordDecl &DerivedDecl,
                                       const CXXRecordDecl &BaseDecl);

};

} // namespace clang::tidy::misc

#endif // LLVM_CLANG_TOOLS_EXTRA_CLANG_TIDY_MISC_REPLACECSTRINGSLICINGCHECK_H
