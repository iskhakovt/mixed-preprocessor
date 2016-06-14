// Copyright (c) Timur Iskhakov.
// Distributed under the terms of the GNU GPL v3 License.


#ifndef MIXED_PREPROCESSOR_MIXEDCOMPUTATIONS_HPP
#define MIXED_PREPROCESSOR_MIXEDCOMPUTATIONS_HPP


// #include "MacroDependency.hpp"
#include "MixedComputationsPPCallbacks.hpp"
#include "MixedMacroArgs.hpp"
#include "MixedToken.hpp"

#include "clang/Lex/MacroInfo.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Lex/PPCallbacks.h"

#include <unordered_map>
#include <unordered_set>

using namespace clang;


class MacroDependency;
class MixedMacroArgs;
class MixedToken;
typedef std::shared_ptr<MixedToken> MixedToken_ptr_t;


class MixedComputations : PPCallbacks {
    Preprocessor &PP;
    // std::unique_ptr<MacroDependency> Dependency;

    std::unordered_map<const MacroInfo *, std::vector<MixedToken_ptr_t>> Definitions;
    std::unordered_map<const MacroInfo *, std::vector<MixedToken_ptr_t>> PreComputed;

    std::vector<MixedToken_ptr_t> ExpandedCache;
    std::vector<MixedToken_ptr_t>::const_iterator ExpandedCacheIter;

    void LexMacro(Token &MacroName, MacroInfo *MI);

    bool isNextPPTokenLParen();

    void PreCompute(const MacroInfo *MI);

public:
    MixedComputations(Preprocessor &PP);

    bool isDefined(const MacroInfo *MI);

    void MacroDefined(const Token &MacroNameTok, const MacroDirective *MD);
    void MacroUndefined(const Token &MacroNameTok, const MacroDefinition &MD);

    std::vector<MixedToken_ptr_t> Preprocess(
            const MacroInfo *MI,
            std::vector<MixedToken_ptr_t>::const_iterator &TokenIt,
            MixedMacroArgs &MA,
            const std::unordered_set<const MacroInfo *> &ExpansionStack,
            bool inArgument);

    bool PasteTokens(const Token &LHS, const Token &RHS, Token &Tok);

    std::vector<MixedToken_ptr_t> ExpandMacro(
            const Token &Tok,
            const MacroInfo *MI,
            std::vector<MixedToken_ptr_t>::const_iterator &TokenIt,
            const std::unordered_set<const MacroInfo *> &ExpansionStack,
            const MacroInfo *ParentMI,
            MixedMacroArgs &ParentArgs);

    void Lex(Token &Tok);
};

#endif //MIXED_PREPROCESSOR_MIXEDCOMPUTATIONS_HPP
