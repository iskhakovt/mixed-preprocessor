// Copyright (c) Timur Iskhakov.
// Distributed under the terms of the GNU GPL v3 License.


#ifndef MIXED_PREPROCESSOR_MIXEDMACROARGS_HPP
#define MIXED_PREPROCESSOR_MIXEDMACROARGS_HPP


#include "MixedComputations.hpp"
#include "MixedToken.hpp"

#include "clang/Lex/Preprocessor.h"

#include <map>
#include <vector>
#include <unordered_map>

using namespace clang;


class MixedComputations;
class MixedToken;
typedef std::shared_ptr<MixedToken> MixedToken_ptr_t;


class MixedMacroArgs {
    MixedComputations &MC;
    Preprocessor &PP;
    const MacroInfo *MI;

    std::vector<std::vector<MixedToken_ptr_t>> Args;
    
    // std::map<std::pair<unsigned, std::unordered_set<const MacroInfo *>>,
    //        std::vector<MixedToken_ptr_t>> ExpandedArgs;

    // std::unordered_map<unsigned, Token> CharifiedArgs;
    // std::unordered_map<unsigned, Token> StringifiedArgs;

public:
    MixedMacroArgs(MixedComputations &MC, Preprocessor &PP, const MacroInfo *MI,
                   const std::vector<std::vector<MixedToken_ptr_t>> &Args) :
            MC(MC), PP(PP), MI(MI), Args(Args) {}


    std::vector<MixedToken_ptr_t> getExpanded(
            unsigned ArgNum,
            const std::unordered_set<const MacroInfo *> &ExpansionStack);


    std::vector<MixedToken_ptr_t> getUnexpanded(unsigned ArgNum);
};


#endif //MIXED_PREPROCESSOR_MIXEDMACROARGS_HPP
