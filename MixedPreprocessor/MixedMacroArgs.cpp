// Copyright (c) Timur Iskhakov.
// Distributed under the terms of the GNU GPL v3 License.


#include "MixedMacroArgs.hpp"

#include "clang/Lex/MacroArgs.h"


std::vector<MixedToken_ptr_t> MixedMacroArgs::getExpanded(
        unsigned ArgNum, const std::unordered_set<const MacroInfo *> &ExpansionStack) {
    assert(ArgNum < Args.size());

    std::unordered_set<const MacroInfo *> NewExpansionStack = ExpansionStack;
    NewExpansionStack.insert(MI);

    std::vector<MixedToken_ptr_t>::const_iterator Iter = getUnexpanded(ArgNum).cbegin();
    return MC.Preprocess(MI, Iter, *this, NewExpansionStack, false);
}

std::vector<MixedToken_ptr_t> MixedMacroArgs::getUnexpanded(unsigned ArgNum) {
    assert(ArgNum < Args.size());
    return Args[ArgNum];
}

/*
MixedToken_ptr_t MixedMacroArgs::getStringified(unsigned ArgNum, bool Charify) {
    assert(ArgNum < Args.size());

    std::vector<MixedToken_ptr_t> Arg = Args[ArgNum];

    if (!std::all_of(Arg.begin(), Arg.end(),
            [](MixedToken_ptr_t TokenPtr) { return TokenPtr->isCommonToken(); })) {
        return std::make_shared<StringifyToken>(ArgNum, Charify);
    }

    std::unordered_map<unsigned, Token> &CachedRes = Charify ?
                                                     CharifiedArgs :
                                                     StringifiedArgs;

    if (CachedRes.find(ArgNum) == CachedRes.end()) {
        Token *Tokens = new Token[Arg.size()];
        for (size_t i = 0; i != Arg.size(); ++i) {
            // I am sure it is correct
            Tokens[i] = reinterpret_cast<CommonToken *>(Arg[i].get())->getTok();
        }

        CachedRes[ArgNum] = MacroArgs::StringifyArgument(
                Tokens, PP, Charify,
                Tokens[0].getLocation(), Tokens[Arg.size() - 1].getLocation());
    }

    return std::make_shared<CommonToken>(CachedRes[ArgNum]);
}
*/
