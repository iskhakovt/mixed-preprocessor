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
        if (It->isAnyIdentifier()) {
            Tokens.emplace_back(std::make_shared<CommonToken>(*It, false));
        } else {
            std::unordered_set<const MacroInfo *> ExpansionStack = {MI};
            Tokens.emplace_back(std::make_shared<IdentifierArgToken>(*It, false, ExpansionStack));
        }
    }

    Definitions[MI] = Tokens;
}

void MixedComputations::MacroUndefined(const Token &MacroNameTok, const MacroDefinition &MD) {
    const MacroInfo *MI = PP.getMacroInfo(MacroNameTok.getIdentifierInfo());

    Definitions.erase(MI);
}

std::vector<MixedToken_ptr_t> MixedComputations::ExpandMacro(
        const Token &MacroName,
        MacroInfo *MI,
        std::vector<MixedToken_ptr_t>::const_iterator &Begin,
        const std::unordered_set<const MacroInfo *> &ExpansionStack,
        MixedMacroArgs &ParentArgs) {
    llvm::errs() << "ExpandMacro: " << PP.getSpelling(MacroName) << "\n";

    size_t numArgs = MI->getNumArgs();

    assert((*Begin)->is(tok::l_paren));
    ++Begin;

    std::vector<std::vector<MixedToken_ptr_t>> Args;

    if (MI->isFunctionLike()) {
        std::vector<MixedToken_ptr_t> Arg = Preprocess(MI, Begin, ParentArgs, ExpansionStack, true);

        if (Arg.empty() || Arg.back()->isOneOf(tok::eof, tok::eod)) {
            return {};
        } else if (Arg.back()->is(tok::comma)) {
            Token Tok = reinterpret_cast<CommonToken *>(Arg.back().get())->getTok();
            Tok.setKind(tok::eof);
            Arg.back() = std::make_shared<CommonToken>(Tok, Arg.back()->isExpanded());

            Args.push_back(Arg);
        } else {
            assert(Arg.back()->is(tok::r_paren));

            Token Tok = reinterpret_cast<CommonToken *>(Arg.back().get())->getTok();
            Tok.setKind(tok::eof);
            Arg.back() = std::make_shared<CommonToken>(Tok, Arg.back()->isExpanded());

            Args.push_back(Arg);
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

    while (1) {
        PP.LexUnexpandedNonComment(Tok);

        if (Tok.isOneOf(tok::eof, tok::eod)) {
            ExpandedCache = Tokens;
            ExpandedCacheIter = ExpandedCache.begin();
            return;
        }

        if (Tok.is(tok::l_paren)) {
            ++NumParens;
        } else if (Tok.is(tok::r_paren)) {
            --NumParens;

            if (!NumParens) {
                break;
            }
        } else {
            if (Tok.isAnyIdentifier()) {
                Tokens.emplace_back(std::make_shared<CommonToken>(Tok, false));
            } else {
                std::unordered_set<const MacroInfo *> ExpansionStack = {MI};
                Tokens.emplace_back(std::make_shared<IdentifierArgToken>(Tok, false, ExpansionStack));
            }
        }
    }

    std::vector<MixedToken_ptr_t>::const_iterator Iter = Tokens.cbegin();

    std::vector<std::vector<MixedToken_ptr_t>> Args;
    std::unordered_set<const MacroInfo *> ExpansionStack;
    MixedMacroArgs emptyMA(*this, nullptr, Args);

    ExpandedCache = ExpandMacro(MacroName, MI, Iter, ExpansionStack, emptyMA);
    ExpandedCacheIter = ExpandedCache.begin();
}

void MixedComputations::PreCompute(const MacroInfo *MI) {
    unsigned numArgs = MI->getNumArgs();

    std::vector<std::vector<MixedToken_ptr_t>> Args(numArgs);
    for (unsigned i = 0; i != numArgs; ++i) {
        Args[i] = {std::make_shared<MixedArgToken>(i, false, std::unordered_set<const MacroInfo *>())};
    }

    MixedMacroArgs MA(*this, MI, Args);

    assert(Definitions.find(MI) != Definitions.end());

    std::vector<MixedToken_ptr_t>::const_iterator Iter = Definitions[MI].cbegin();
    PreComputed[MI] = Preprocess(MI, Iter, MA, {}, false);
}
