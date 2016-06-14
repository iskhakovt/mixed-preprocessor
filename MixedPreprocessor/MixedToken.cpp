// Copyright (c) Timur Iskhakov.
// Distributed under the terms of the GNU GPL v3 License.


#include "MixedToken.hpp"


std::vector<MixedToken_ptr_t> CommonToken::getExpanded(MixedMacroArgs &Args) const {
    return {std::make_shared<CommonToken>(getTok(), isExpanded())};
}

std::vector<MixedToken_ptr_t> CommonToken::getUnexpanded(MixedMacroArgs &Args) const {
    return {std::make_shared<CommonToken>(getTok(), true)};
}

void CommonToken::addExpansionStack(const std::unordered_set<const MacroInfo *> &Stack) {

}

std::vector<MixedToken_ptr_t> IdentifierArgToken::getExpanded(MixedMacroArgs &Args) const {
    return {std::make_shared<IdentifierArgToken>(getTok(), true, ExpansionStack)};
}

std::vector<MixedToken_ptr_t> IdentifierArgToken::getUnexpanded(MixedMacroArgs &Args) const {
    return {std::make_shared<IdentifierArgToken>(getTok(), isExpanded(), ExpansionStack)};
}

void IdentifierArgToken::addExpansionStack(const std::unordered_set<const MacroInfo *> &Stack) {
    for (auto MI : Stack) {
        ExpansionStack.insert(MI);
    }
}

std::vector<MixedToken_ptr_t> MixedArgToken::getExpanded(MixedMacroArgs &Args) const {
    return Args.getExpanded(ArgNum, ExpansionStack);
}

std::vector<MixedToken_ptr_t> MixedArgToken::getUnexpanded(MixedMacroArgs &Args) const {
    return Args.getUnexpanded(ArgNum);
}

void MixedArgToken::addExpansionStack(const std::unordered_set<const MacroInfo *> &Stack) {
    for (auto MI : Stack) {
        ExpansionStack.insert(MI);
    }
}

/*
std::vector<MixedToken_ptr_t> StringifyToken::getExpanded(MixedMacroArgs &Args) {
    return {Args.getStringified(ArgNum, Charify)};
}
*/
