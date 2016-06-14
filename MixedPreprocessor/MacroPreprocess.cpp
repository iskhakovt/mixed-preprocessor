// Copyright (c) Timur Iskhakov.
// Distributed under the terms of the GNU GPL v3 License.


#include "MixedComputations.hpp"

#include "clang/Lex/LexDiagnostic.h"



MixedToken_ptr_t NextToken(std::vector<MixedToken_ptr_t>::const_iterator TokenIt,
                           const std::list<MixedToken_ptr_t> &List,
                           std::list<MixedToken_ptr_t>::iterator ListIt) {
    if (List.end() == std::next(ListIt)) {
        return *TokenIt;
    }
    return *std::next(ListIt);
}

void EraseNextToken(std::vector<MixedToken_ptr_t>::const_iterator &TokenIt,
                    std::list<MixedToken_ptr_t> &List,
                    std::list<MixedToken_ptr_t>::iterator ListIt) {
    if (List.end() == std::next(ListIt)) {
        ++TokenIt;
    } else {
        List.erase(std::next(ListIt));
    }
}


std::vector<MixedToken_ptr_t> MixedComputations::Preprocess(
        const MacroInfo *MI,
        std::vector<MixedToken_ptr_t>::const_iterator &TokenIt,
        MixedMacroArgs &MA,
        const std::unordered_set<const MacroInfo *> &ExpansionStack,
        bool inArgument) {
    assert(!MI || isDefined(MI));

    std::list<MixedToken_ptr_t> res;
    std::list<MixedToken_ptr_t>::iterator to_proceed = res.begin();

    unsigned NumParens = 0;

    while (1) {
        if (to_proceed == res.end()) {
            to_proceed = res.insert(res.end(), *(TokenIt++));
        }

        if ((*to_proceed)->isOneOf(tok::eof, tok::eof)) {
            break;
        }

        if (!(*to_proceed)->isCommonToken() && (*to_proceed)->isExpanded()) {
            std::vector<MixedToken_ptr_t> Expanded = (*to_proceed)->getExpanded(MA);

            while (!Expanded.empty() && (*Expanded.back()).isOneOf(tok::eof, tok::eod)) {
                Expanded.pop_back();
            }

            if (Expanded.size() == 0) {
                auto Next = std::next(to_proceed);
                res.erase(to_proceed);
                to_proceed = Next;
            } else {
                auto Next = res.insert(to_proceed, Expanded.begin(), Expanded.end());
                res.erase(to_proceed);
                to_proceed = Next;
            }

            continue;
        }

        if ((*to_proceed)->is(tok::l_paren)) {
            ++NumParens;
        } else if ((*to_proceed)->is(tok::r_paren)) {
            if (!NumParens) {
                break;
            }

            --NumParens;
        } else if ((*to_proceed)->is(tok::comma)) {
            if (!NumParens && inArgument) {
                break;
            }
        }

        if ((*to_proceed)->isAnyIdentifier() /*&& (*to_proceed)->isExpanded()*/) {
            if (NextToken(TokenIt, res, to_proceed)->is(tok::hashhash)) {
                continue;
            }

            IdentifierInfo *II = (*to_proceed)->getIdentifierInfo();

            // If this is a macro to be expanded, do it.
            if (MacroInfo *currMI = PP.getMacroInfo(II)) {
                if (/*!to_proceed->isExpandDisabled() &&*/ currMI->isEnabled()) {
                    // C99 6.10.3p10: If the preprocessing token immediately after the
                    // macro name isn't a '(', this macro should not be expanded.
                    if (!currMI->isFunctionLike() || NextToken(TokenIt, res, to_proceed)->is(tok::l_paren)) {

                        CommonToken *TokPtr = reinterpret_cast<CommonToken *>(to_proceed->get());

                        std::vector<MixedToken_ptr_t> Expanded = ExpandMacro(
                                TokPtr->getTok(), currMI, TokenIt, ExpansionStack, MI, MA);

                        while (!Expanded.empty() && Expanded.back()->isOneOf(tok::eof, tok::eod)) {
                            Expanded.pop_back();
                        }

                        auto Next = res.insert(to_proceed, Expanded.begin(), Expanded.end());

                        res.erase(to_proceed);
                        to_proceed = Next;

                        continue;
                    }
                } else {
                    // C99 6.10.3.4p2 says that a disabled macro may never again be
                    // expanded, even if it's in a context where it could be expanded in the
                    // future.
                    /*
                    to_proceed->setFlag(Token::DisableExpand);
                    if (currMI->isObjectLike() || isNextPPTokenLParen())
                        PP.Diag(*to_proceed, diag::pp_disabled_macro_expansion);
                    */
                }
            }

            // to_proceed->setFlag(Token::DisableExpand);
            ++to_proceed;
        } else if ((*to_proceed)->is(tok::hash) || (*to_proceed)->is(tok::hashat)) {
            assert(false && "Stringify and Charify are not supported");
        } else if ((*to_proceed)->is(tok::hashhash)) {
            if (to_proceed == res.begin() || NextToken(TokenIt, res, to_proceed)->isOneOf(tok::eof, tok::eof)) {
                // ill-formed, ignore hashhash
                ++to_proceed;
                continue;
            }

            std::vector<MixedToken_ptr_t> left = (*std::prev(to_proceed))->getUnexpanded(MA);
            std::vector<MixedToken_ptr_t> right = NextToken(TokenIt, res, to_proceed)->getUnexpanded(MA);

            res.erase(std::prev(to_proceed));
            EraseNextToken(TokenIt, res, to_proceed);

            res.insert(to_proceed, left.begin(), left.end());
            res.insert(std::next(to_proceed), right.begin(), right.end());



            if (to_proceed != res.begin() &&
                    (*std::prev(to_proceed))->isCommonToken() &&
                     (*std::next(to_proceed))->isCommonToken()) {
                CommonToken *LHS = reinterpret_cast<CommonToken *>(std::prev(to_proceed)->get());
                CommonToken *RHS = reinterpret_cast<CommonToken *>(std::next(to_proceed)->get());

                Token Tok;

                // TODO: Result might be meaningful
                PasteTokens(LHS->getTok(), RHS->getTok(), Tok);

                res.erase(std::prev(to_proceed));
                res.erase(std::next(to_proceed));

                if (Tok.isAnyIdentifier()) {
                    *to_proceed = std::make_shared<IdentifierArgToken>(Tok, false, ExpansionStack);
                } else {
                    *to_proceed = std::make_shared<CommonToken>(Tok, false);
                }
            } else {
                ++to_proceed;
                ++to_proceed;
            }

        } else {
            ++to_proceed;
        }
    }

    return std::vector<MixedToken_ptr_t>(res.begin(), res.end());
}

// PasteTokens - Tok is the LHS of a ## operator, and CurToken is the ##
/// operator.  Read the ## and RHS, and paste the LHS/RHS together. Return the result as Tok.
/// If this returns true, the caller should immediately return the token.
bool MixedComputations::PasteTokens(const Token &LHS, const Token &RHS, Token &Tok) {
    SmallString<128> Buffer;
    const char *ResultTokStrPtr = nullptr;
    // SourceLocation StartLoc = LHS.getLocation();
    SourceLocation PasteOpLoc;

    // Consume the ## operator if any.
    PasteOpLoc = LHS.getLocation();

    // Allocate space for the result token.  This is guaranteed to be enough for
    // the two tokens.
    Buffer.resize(RHS.getLength() + RHS.getLength());

    // Get the spelling of the LHS token in Buffer.
    const char *BufPtr = &Buffer[0];
    bool Invalid = false;
    unsigned LHSLen = PP.getSpelling(LHS, BufPtr, &Invalid);
    if (BufPtr != &Buffer[0])   // Really, we want the chars in Buffer!
        memcpy(&Buffer[0], BufPtr, LHSLen);
    if (Invalid)
        return true;

    BufPtr = Buffer.data() + LHSLen;
    unsigned RHSLen = PP.getSpelling(RHS, BufPtr, &Invalid);
    if (Invalid)
        return true;
    if (RHSLen && BufPtr != &Buffer[LHSLen])
        // Really, we want the chars in Buffer!
        memcpy(&Buffer[LHSLen], BufPtr, RHSLen);

    // Trim excess space.
    Buffer.resize(LHSLen+RHSLen);

    // Plop the pasted result (including the trailing newline and null) into a
    // scratch buffer where we can lex it.
    Token ResultTokTmp;
    ResultTokTmp.startToken();

    // Claim that the tmp token is a string_literal so that we can get the
    // character pointer back from CreateString in getLiteralData().
    ResultTokTmp.setKind(tok::string_literal);
    PP.CreateString(Buffer, ResultTokTmp);
    SourceLocation ResultTokLoc = ResultTokTmp.getLocation();
    ResultTokStrPtr = ResultTokTmp.getLiteralData();

    // Lex the resultant pasted token into Result.
    Token Result;

    if (LHS.isAnyIdentifier() && RHS.isAnyIdentifier()) {
        // Common paste case: identifier+identifier = identifier.  Avoid creating
        // a lexer and other overhead.
        PP.IncrementPasteCounter(true);
        Result.startToken();
        Result.setKind(tok::raw_identifier);
        Result.setRawIdentifierData(ResultTokStrPtr);
        Result.setLocation(ResultTokLoc);
        Result.setLength(LHSLen+RHSLen);
    } else {
        PP.IncrementPasteCounter(false);

        assert(ResultTokLoc.isFileID() &&
               "Should be a raw location into scratch buffer");
        SourceManager &SourceMgr = PP.getSourceManager();
        FileID LocFileID = SourceMgr.getFileID(ResultTokLoc);

        bool Invalid = false;
        const char *ScratchBufStart
                = SourceMgr.getBufferData(LocFileID, &Invalid).data();
        if (Invalid)
            return false;

        // Make a lexer to lex this string from.  Lex just this one token.
        // Make a lexer object so that we lex and expand the paste result.
        Lexer TL(SourceMgr.getLocForStartOfFile(LocFileID),
                 PP.getLangOpts(), ScratchBufStart,
                 ResultTokStrPtr, ResultTokStrPtr+LHSLen+RHSLen);

        // Lex a token in raw mode.  This way it won't look up identifiers
        // automatically, lexing off the end will return an eof token, and
        // warnings are disabled.  This returns true if the result token is the
        // entire buffer.
        bool isInvalid = !TL.LexFromRawLexer(Result);

        // If we got an EOF token, we didn't form even ONE token.  For example, we
        // did "/ ## /" to get "//".
        isInvalid |= Result.is(tok::eof);

        // If pasting the two tokens didn't form a full new token, this is an
        // error.  This occurs with "x ## +"  and other stuff.  Return with Tok
        // unmodified and with RHS as the next token to lex.
        if (isInvalid) {
            // Explicitly convert the token location to have proper expansion
            // information so that the user knows where it came from.
            SourceManager &SM = PP.getSourceManager();
            SourceLocation Loc =
                    SM.createExpansionLoc(PasteOpLoc, LHS.getLocation(), RHS.getLocation(), 2);

            // Do not emit the error when preprocessing assembler code.
            if (!PP.getLangOpts().AsmPreprocessor) {
                // If we're in microsoft extensions mode, downgrade this from a hard
                // error to an extension that defaults to an error.  This allows
                // disabling it.
                PP.Diag(Loc, PP.getLangOpts().MicrosoftExt ? diag::ext_pp_bad_paste_ms
                                                           : diag::err_pp_bad_paste)
                << Buffer;
            }
        }

        // Turn ## into 'unknown' to avoid # ## # from looking like a paste
        // operator.
        if (Result.is(tok::hashhash))
            Result.setKind(tok::unknown);
    }

    // Transfer properties of the LHS over the Result.
    Result.setFlagValue(Token::StartOfLine , Tok.isAtStartOfLine());
    Result.setFlagValue(Token::LeadingSpace, Tok.hasLeadingSpace());

    Tok = Result;

    // Now that we got the result token, it will be subject to expansion.  Since
    // token pasting re-lexes the result token in raw mode, identifier information
    // isn't looked up.  As such, if the result is an identifier, look up id info.
    if (Tok.is(tok::raw_identifier)) {
        // Look up the identifier info for the token.  We disabled identifier lookup
        // by saying we're skipping contents, so we need to do this manually.
        PP.LookUpIdentifierInfo(Tok);
    }
    return false;
}
