// Copyright (c) Timur Iskhakov.
// Distributed under the terms of the GNU GPL v3 License.


#include "MixedComputationsPPCallbacks.hpp"


void MixedComputationsPPCallbacks::MacroDefined(const Token &MacroNameTok, const MacroDirective *MD) {
    MC.MacroDefined(MacroNameTok, MD);
}

void MixedComputationsPPCallbacks::MacroUndefined(const Token &MacroNameTok, const MacroDefinition &MD) {
    MC.MacroUndefined(MacroNameTok, MD);
}
