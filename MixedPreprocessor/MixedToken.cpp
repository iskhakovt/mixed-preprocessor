// Copyright (c) Timur Iskhakov.
// Distributed under the terms of the GNU GPL v3 License.


#include "MixedToken.hpp"


std::vector<MixedToken_ptr_t> CommonToken::getExpanded(const MacroInfo *MI, MixedMacroArgs &Args) const {
    return {std::make_shared<CommonToken>(getTok(), isExpanded())};
}

std::vector<MixedToken_ptr_t> CommonToken::getUnexpanded(MixedMacroArgs &Args) const {
    return {std::make_shared<CommonToken>(getTok(), true)};
}

std::vector<MixedToken_ptr_t> IdentifierArgToken::getExpanded(const MacroInfo *MI, MixedMacroArgs &Args) const {
    auto NewExpansionStack = ExpansionStack;
    NewExpansionStack.insert(MI);

    return {std::make_shared<IdentifierArgToken>(getTok(), true, NewExpansionStack)};
}

std::vector<MixedToken_ptr_t> IdentifierArgToken::getUnexpanded(MixedMacroArgs &Args) const {
    return {std::make_shared<IdentifierArgToken>(getTok(), isExpanded(), ExpansionStack)};
}

std::vector<MixedToken_ptr_t> MixedArgToken::getExpanded(const MacroInfo *MI, MixedMacroArgs &Args) const {
    return Args.getExpanded(ArgNum, ExpansionStack);
}

std::vector<MixedToken_ptr_t> MixedArgToken::getUnexpanded(MixedMacroArgs &Args) const {
    return Args.getUnexpanded(ArgNum);
}

/*
std::vector<MixedToken_ptr_t> StringifyToken::getExpanded(MixedMacroArgs &Args) {
    return {Args.getStringified(ArgNum, Charify)};
}
*/
