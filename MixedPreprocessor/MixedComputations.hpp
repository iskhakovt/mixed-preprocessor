// Copyright (c) Timur Iskhakov.
// Distributed under the terms of the GNU GPL v3 License.


#ifndef MIXED_PREPROCESSOR_MIXEDCOMPUTATIONS_HPP
#define MIXED_PREPROCESSOR_MIXEDCOMPUTATIONS_HPP


#include "MixedComputationsPPCallbacks.hpp"

#include "clang/Lex/Preprocessor.h"
#include "clang/Lex/PPCallbacks.h"

using namespace clang;


class MixedComputations : PPCallbacks {
    Preprocessor &PP;

public:
    MixedComputations(Preprocessor &PP);

    void MacroDefined(const Token &MacroNameTok, const MacroDirective *MD);
    void MacroUndefined(const Token &MacroNameTok, const MacroDefinition &MD);

    ArrayRef<Token> expandToken(Token &Tok);
};

#endif //MIXED_PREPROCESSOR_MIXEDCOMPUTATIONS_HPP
