// Copyright (c) Timur Iskhakov.
// Distributed under the terms of the GNU GPL v3 License.


#ifndef MIXED_PREPROCESSOR_MIXEDCOMPUTATIONS_HPP
#define MIXED_PREPROCESSOR_MIXEDCOMPUTATIONS_HPP


#include <clang/Lex/MacroInfo.h>
#include "MacroDependency.hpp"
#include "MixedComputationsPPCallbacks.hpp"

#include "clang/Lex/Preprocessor.h"
#include "clang/Lex/PPCallbacks.h"

#include <unordered_set>

using namespace clang;


class MacroDependency;


class MixedComputations : PPCallbacks {
    Preprocessor &PP;
    std::unique_ptr<MacroDependency> Dependency;

    bool isNextPPTokenLParen();

    MacroArgs * ReadFunctionLikeMacroArgs(Token &MacroName,
                                          MacroInfo *MI,
                                          SourceLocation &MacroEnd);

public:
    MixedComputations(Preprocessor &PP);

    bool isDefined(const MacroInfo *MI);

    void MacroDefined(const Token &MacroNameTok, const MacroDirective *MD);
    void MacroUndefined(const Token &MacroNameTok, const MacroDefinition &MD);

    std::unordered_set<const MacroInfo *> Preprocess(const MacroInfo *MI);
    std::unordered_set<const MacroInfo *> Compute(const MacroInfo *MI);

    std::vector<Token> ExpandMacro(Token &Tok, MacroInfo *MI);
    std::vector<Token> ExpandToken(Token &Tok);
};

#endif //MIXED_PREPROCESSOR_MIXEDCOMPUTATIONS_HPP
