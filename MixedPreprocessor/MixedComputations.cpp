// Copyright (c) Timur Iskhakov.
// Distributed under the terms of the GNU GPL v3 License.


#include "MixedComputations.hpp"

#include "clang/Lex/LexDiagnostic.h"
#include "clang/Lex/MacroArgs.h"


bool MixedComputations::isNextPPTokenLParen() {
    Token Tok;

    PP.EnableBacktrackAtThisPos();
    PP.LexUnexpandedNonComment(Tok);
    PP.Backtrack();

    return Tok.is(tok::l_paren);
}

MixedComputations::MixedComputations(Preprocessor &PP) : PP(PP) {
    PP.addPPCallbacks(llvm::make_unique<MixedComputationsPPCallbacks>(*this));
    // Dependency = llvm::make_unique<MacroDependency>(*this);
    ExpandedCacheIter = ExpandedCache.begin();
}

bool MixedComputations::isDefined(const MacroInfo *MI) {
    return Definitions.find(MI) != Definitions.end();
}

void MixedComputations::MacroDefined(const Token &MacroNameTok, const MacroDirective *MD) {
    const MacroInfo *MI = PP.getMacroInfo(MacroNameTok.getIdentifierInfo());

    std::vector<MixedToken_ptr_t> Tokens;

    for (auto It = MI->tokens_begin(); It != MI->tokens_end(); ++It) {
        if (!It->isAnyIdentifier()) {
            Tokens.emplace_back(std::make_shared<CommonToken>(*It, true));
        } else if (MI->getArgumentNum(It->getIdentifierInfo()) == -1){
            std::unordered_set<const MacroInfo *> ExpansionStack = {MI};
            Tokens.emplace_back(std::make_shared<IdentifierArgToken>(*It, true, ExpansionStack));
        } else {
            unsigned ArgNum = MI->getArgumentNum(It->getIdentifierInfo());
            std::unordered_set<const MacroInfo *> ExpansionStack = {MI};
            Tokens.emplace_back(std::make_shared<MixedArgToken>(ArgNum, true, ExpansionStack));
        }
    }

    Token Tok;
    Tok.startToken();
    Tok.setKind(tok::eof);
    Tokens.emplace_back(std::make_shared<CommonToken>(Tok, false));

    Definitions[MI] = Tokens;
}

void MixedComputations::MacroUndefined(const Token &MacroNameTok, const MacroDefinition &MD) {
    const MacroInfo *MI = PP.getMacroInfo(MacroNameTok.getIdentifierInfo());
    Definitions.erase(MI);
}

std::vector<MixedToken_ptr_t> MixedComputations::ExpandMacro(
        const Token &MacroName,
        const MacroInfo *MI,
        std::vector<MixedToken_ptr_t>::const_iterator &Begin,
        const std::unordered_set<const MacroInfo *> &ExpansionStack,
        const MacroInfo *ParentMI,
        MixedMacroArgs &ParentArgs) {
    size_t numArgs = MI->getNumArgs();
    std::vector<std::vector<MixedToken_ptr_t>> Args;

    if (MI->isFunctionLike()) {
        assert((*Begin)->is(tok::l_paren));
        ++Begin;

        if (MI->isFunctionLike()) {
            while (1) {
                std::vector<MixedToken_ptr_t> Arg = Preprocess(ParentMI, Begin, ParentArgs, ExpansionStack, true);

                if (Arg.empty() || Arg.back()->isOneOf(tok::eof, tok::eod)) {
                    return {};
                } else if (Arg.back()->is(tok::comma)) {
                    Token Tok;
                    Tok.startToken();
                    Tok.setKind(tok::eof);
                    Arg.back() = std::make_shared<CommonToken>(Tok, false);

                    Args.push_back(Arg);
                } else {
                    assert(Arg.back()->is(tok::r_paren));

                    Token Tok;
                    Tok.startToken();
                    Tok.setKind(tok::eof);
                    Arg.back() = std::make_shared<CommonToken>(Tok, false);

                    Args.push_back(Arg);
                    break;
                }
            }
        }
    }

    if (Args.size() != numArgs) {
        return {};
    }

    MixedMacroArgs MixedMA(*this, MI, Args);

    std::unordered_set<const MacroInfo *> NexExpansionStack = ExpansionStack;
    NexExpansionStack.insert(MI);

    if (PreComputed.find(MI) == PreComputed.end()) {
        PreCompute(MI);
    }

    std::vector<MixedToken_ptr_t>::const_iterator Iter = PreComputed[MI].cbegin();
    return Preprocess(MI, Iter, MixedMA, NexExpansionStack, false);
}

void MixedComputations::Lex(Token &Tok) {
    while(1) {
        if (ExpandedCacheIter != ExpandedCache.end()) {
            assert((*ExpandedCacheIter)->isCommonToken());

            Tok = reinterpret_cast<CommonToken *>(ExpandedCacheIter->get())->getTok();
            ++ExpandedCacheIter;

            if (Tok.isOneOf(tok::eof, tok::eod)) continue;
            return;
        }

        ExpandedCache.clear();
        ExpandedCacheIter = ExpandedCache.begin();

        PP.LexUnexpandedNonComment(Tok);

        if (!Tok.isAnyIdentifier()) {
            return;
        }

        IdentifierInfo *II = Tok.getIdentifierInfo();

        if (MacroInfo *MI = PP.getMacroInfo(II)) {
            if (!Tok.isExpandDisabled() && MI->isEnabled()) {
                // C99 6.10.3p10: If the preprocessing token immediately after the
                // macro name isn't a '(', this macro should not be expanded.
                if (!MI->isFunctionLike() || isNextPPTokenLParen()) {
                    LexMacro(Tok, MI);
                    continue;
                }
            } else {
                // C99 6.10.3.4p2 says that a disabled macro may never again be
                // expanded, even if it's in a context where it could be expanded in the
                // future.
                Tok.setFlag(Token::DisableExpand);
                if (MI->isObjectLike() || isNextPPTokenLParen())
                    PP.Diag(Tok, diag::pp_disabled_macro_expansion);
            }
        }

        return;
    }
}

void MixedComputations::LexMacro(Token &MacroName, MacroInfo *MI) {
    std::vector<MixedToken_ptr_t> Tokens;
    Token Tok;

    unsigned NumParens = 0;

    if (MI->isFunctionLike()) {
        while (1) {
            PP.LexUnexpandedNonComment(Tok);

            if (Tok.isOneOf(tok::eof, tok::eod)) {
                ExpandedCache = Tokens;
                ExpandedCacheIter = ExpandedCache.begin();
                return;
            }

            if (!Tok.isAnyIdentifier()) {
                Tokens.emplace_back(std::make_shared<CommonToken>(Tok, false));
            } else {
                std::unordered_set<const MacroInfo *> ExpansionStack = {MI};
                Tokens.emplace_back(std::make_shared<IdentifierArgToken>(Tok, false, ExpansionStack));
            }

            if (Tok.is(tok::l_paren)) {
                ++NumParens;
            } else if (Tok.is(tok::r_paren)) {
                --NumParens;

                if (!NumParens) {
                    break;
                }
            }
        }
    }

    std::vector<MixedToken_ptr_t>::const_iterator Iter = Tokens.cbegin();

    std::vector<std::vector<MixedToken_ptr_t>> Args;
    std::unordered_set<const MacroInfo *> ExpansionStack;
    MixedMacroArgs emptyMA(*this, nullptr, Args);

    ExpandedCache = ExpandMacro(MacroName, MI, Iter, ExpansionStack, nullptr, emptyMA);
    ExpandedCacheIter = ExpandedCache.begin();
}

void MixedComputations::PreCompute(const MacroInfo *MI) {
    unsigned numArgs = MI->getNumArgs();

    std::vector<std::vector<MixedToken_ptr_t>> Args(numArgs);
    for (unsigned i = 0; i != numArgs; ++i) {
        Args[i] = {std::make_shared<MixedArgToken>(i, false, std::unordered_set<const MacroInfo *>())};

        Token Tok;
        Tok.startToken();
        Tok.setKind(tok::eof);
        Args[i].push_back(std::make_shared<CommonToken>(Tok, false));
    }

    MixedMacroArgs MA(*this, MI, Args);

    assert(Definitions.find(MI) != Definitions.end());

    std::vector<MixedToken_ptr_t>::const_iterator Iter = Definitions[MI].cbegin();
    auto Tokens = Preprocess(MI, Iter, MA, {}, false);

    for (auto &TokenPtr : Tokens) {
        if (!TokenPtr->isCommonToken()) {
            TokenPtr->setExpanded();
        }
    }

    PreComputed[MI] = Tokens;
}
