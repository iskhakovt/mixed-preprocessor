// Copyright (c) Timur Iskhakov.
// Distributed under the terms of the GNU GPL v3 License.


#include "MixedComputations.hpp"


MixedComputations::MixedComputations(Preprocessor &PP) : PP(PP) {
    PP.addPPCallbacks(llvm::make_unique<MixedComputationsPPCallbacks>(*this));
    Dependency = llvm::make_unique<MacroDependency>(*this);
}

bool MixedComputations::isDedined(const MacroInfo *MI) {
    return true;
}

void MixedComputations::MacroDefined(const Token &MacroNameTok, const MacroDirective *MD) {
    Dependency->Update(PP.getMacroInfo(MacroNameTok.getIdentifierInfo()));
}

void MixedComputations::MacroUndefined(const Token &MacroNameTok, const MacroDefinition &MD) {
    Dependency->Update(PP.getMacroInfo(MacroNameTok.getIdentifierInfo()));
}

std::unordered_set<const MacroInfo *> MixedComputations::Preprocess(const MacroInfo *MI) {
    assert(isDedined(MI));
    return {};
}

std::unordered_set<const MacroInfo *> MixedComputations::Compute(const MacroInfo *MI) {
    assert(isDedined(MI));
    return {};
}

std::vector<Token> MixedComputations::ExpandMacro(const MacroInfo *MI, const MacroArgs *MA) {
    assert(isDedined(MI));
    return {};
}

std::vector<Token> MixedComputations::ExpandToken(Token &Tok) {
    return {};
}
