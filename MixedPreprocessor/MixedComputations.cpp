// Copyright (c) Timur Iskhakov.
// Distributed under the terms of the GNU GPL v3 License.


#include "MixedComputations.hpp"


MixedComputations::MixedComputations(Preprocessor &PP) : PP(PP) {
    PP.addPPCallbacks(llvm::make_unique<MixedComputationsPPCallbacks>(*this));
}


void MixedComputations::MacroDefined(const Token &MacroNameTok, const MacroDirective *MD) {

}

void MixedComputations::MacroUndefined(const Token &MacroNameTok, const MacroDefinition &MD) {

}

ArrayRef<Token> MixedComputations::expandToken(Token &Tok) {
    return ArrayRef<Token>();
}
