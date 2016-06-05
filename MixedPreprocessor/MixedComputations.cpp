// Copyright (c) Timur Iskhakov.
// Distributed under the terms of the GNU GPL v3 License.


#include "MixedComputations.hpp"

#include "clang/Lex/LexDiagnostic.h"


bool MixedComputations::isNextPPTokenLParen() {
    Token Tok;

    PP.EnableBacktrackAtThisPos();
    PP.LexUnexpandedNonComment(Tok);
    PP.Backtrack();

    return Tok.is(tok::l_paren);
}

MixedComputations::MixedComputations(Preprocessor &PP) : PP(PP) {
    PP.addPPCallbacks(llvm::make_unique<MixedComputationsPPCallbacks>(*this));
    Dependency = llvm::make_unique<MacroDependency>(*this);
}

bool MixedComputations::isDefined(const MacroInfo *MI) {
    return true;
}

void MixedComputations::MacroDefined(const Token &MacroNameTok, const MacroDirective *MD) {
    Dependency->Update(PP.getMacroInfo(MacroNameTok.getIdentifierInfo()));
}

void MixedComputations::MacroUndefined(const Token &MacroNameTok, const MacroDefinition &MD) {
    Dependency->Update(PP.getMacroInfo(MacroNameTok.getIdentifierInfo()));
}

std::unordered_set<const MacroInfo *> MixedComputations::Preprocess(const MacroInfo *MI) {
    assert(isDefined(MI));
    return {};
}

std::unordered_set<const MacroInfo *> MixedComputations::Compute(const MacroInfo *MI) {
    assert(isDefined(MI));
    return {};
}

std::vector<Token> MixedComputations::ExpandMacro(Token &MacroName, MacroInfo *MI) {
    // SourceLocation MacroEnd;
    // MacroArgs *MA = MI->isFunctionLike() ? ReadFunctionLikeMacroArgs(MacroName, MI, MacroEnd) : nullptr;

    return {MacroName};
}

std::vector<Token> MixedComputations::ExpandToken(Token &Tok) {
    if (!Tok.isAnyIdentifier()) {
        return {Tok};
    }

    IdentifierInfo *II = Tok.getIdentifierInfo();

    if (MacroInfo *MI = PP.getMacroInfo(II)) {
        if (!Tok.isExpandDisabled() && MI->isEnabled()) {
            // C99 6.10.3p10: If the preprocessing token immediately after the
            // macro name isn't a '(', this macro should not be expanded.
            if (!MI->isFunctionLike() || isNextPPTokenLParen())
                return ExpandMacro(Tok, MI);
        } else {
             // C99 6.10.3.4p2 says that a disabled macro may never again be
             // expanded, even if it's in a context where it could be expanded in the
             // future.
             Tok.setFlag(Token::DisableExpand);
             if (MI->isObjectLike() || isNextPPTokenLParen())
                 PP.Diag(Tok, diag::pp_disabled_macro_expansion);
        }
    }

    return {Tok};
}
