//===--- CorrectIncludeNameCheck.cpp - clang-tidy -------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "CorrectIncludeNameCheck.h"
#include "clang/Lex/PPCallbacks.h"
#include "clang/Lex/Preprocessor.h"

using namespace clang::ast_matchers;

namespace clang::tidy::misc {

namespace {
class IncludeOrderPPCallbacks : public PPCallbacks {
public:
  explicit IncludeOrderPPCallbacks(ClangTidyCheck &Check,
                                   const SourceManager &SM)
      : Check(Check), SM(SM) {}

  void InclusionDirective(SourceLocation HashLoc, const Token &IncludeTok,
                          StringRef FileName, bool IsAngled,
                          CharSourceRange FilenameRange,
                          OptionalFileEntryRef File, StringRef SearchPath,
                          StringRef RelativePath, const Module *Imported,
                          SrcMgr::CharacteristicKind FileType) override;
  void EndOfMainFile() override;

private:
  struct IncludeDirective {
    SourceLocation Loc;    ///< '#' location in the include directive
    CharSourceRange Range; ///< SourceRange for the file name
    std::string Filename;  ///< Filename as a string
    bool IsAngled;         ///< true if this was an include with angle brackets
    bool IsMainModule;     ///< true if this was the first include in a file
  };

  ClangTidyCheck &Check;
  const SourceManager &SM;
};
} // namespace

void CorrectIncludeNameCheck::registerPPCallbacks(
    const SourceManager &SM, Preprocessor *PP, Preprocessor *ModuleExpanderPP) {
  PP->addPPCallbacks(::std::make_unique<IncludeOrderPPCallbacks>(*this, SM));
}

static int getPriority(StringRef Filename, bool IsAngled, bool IsMainModule) {
  // We leave the main module header at the top.
  if (IsMainModule)
    return 0;

  // LLVM and clang headers are in the penultimate position.
  if (Filename.startswith("llvm/") || Filename.startswith("llvm-c/") ||
      Filename.startswith("clang/") || Filename.startswith("clang-c/"))
    return 2;

  // Put these between system and llvm headers to be consistent with LLVM
  // clang-format style.
  if (Filename.startswith("gtest/") || Filename.startswith("gmock/"))
    return 3;

  // System headers are sorted to the end.
  if (IsAngled)
    return 4;

  // Other headers are inserted between the main module header and LLVM headers.
  return 1;
}

void IncludeOrderPPCallbacks::InclusionDirective(
    SourceLocation HashLoc, const Token &IncludeTok, StringRef FileName,
    bool IsAngled, CharSourceRange FilenameRange, OptionalFileEntryRef File,
    StringRef SearchPath, StringRef RelativePath, const Module *Imported,
    SrcMgr::CharacteristicKind FileType) {
  // We recognize the first include as a special main module header and want
  // to leave it in the top position.
  IncludeDirective ID = {HashLoc, FilenameRange, std::string(FileName),
                         IsAngled, false};
  if (!File) {
    // How can this happen?
  }
  auto name = File->getName();
  auto request = File->getNameAsRequested();
}

void IncludeOrderPPCallbacks::EndOfMainFile() {}
} // namespace clang::tidy::misc
