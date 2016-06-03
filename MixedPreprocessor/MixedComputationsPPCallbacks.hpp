// Copyright (c) Timur Iskhakov.
// Distributed under the terms of the GNU GPL v3 License.


#ifndef MIXED_PREPROCESSOR_MIXEDCOMPUTATIONSPPCALLBACKS_HPP
#define MIXED_PREPROCESSOR_MIXEDCOMPUTATIONSPPCALLBACKS_HPP


#include "MixedComputations.hpp"

#include "clang/Lex/PPCallbacks.h"

using namespace clang;


class MixedComputations;


class MixedComputationsPPCallbacks : public PPCallbacks {
    MixedComputations &MC;

public:
    MixedComputationsPPCallbacks(MixedComputations &MC) : MC(MC) {}

    void MacroDefined(const Token &MacroNameTok, const MacroDirective *MD) override;
    void MacroUndefined(const Token &MacroNameTok, const MacroDefinition &MD) override;
};


#endif //MIXED_PREPROCESSOR_MIXEDCOMPUTATIONSPPCALLBACKS_HPP
