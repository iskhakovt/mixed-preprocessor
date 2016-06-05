// Copyright (c) Timur Iskhakov.
// Distributed under the terms of the GNU GPL v3 License.

// Is mostly a copy of Preprocessor private functions
// defined in MacroExpansion.cpp

#include "MixedComputations.hpp"

#include "clang/Lex/LexDiagnostic.h"
#include "clang/Lex/MacroArgs.h"


enum Bracket {
    Brace,
    Paren
};

/// CheckMatchedBrackets - Returns true if the braces and parentheses in the
/// token vector are properly nested.
static bool CheckMatchedBrackets(const SmallVectorImpl<Token> &Tokens) {
    SmallVector<Bracket, 8> Brackets;
    for (SmallVectorImpl<Token>::const_iterator I = Tokens.begin(),
                 E = Tokens.end();
         I != E; ++I) {
        if (I->is(tok::l_paren)) {
            Brackets.push_back(Paren);
        } else if (I->is(tok::r_paren)) {
            if (Brackets.empty() || Brackets.back() == Brace)
                return false;
            Brackets.pop_back();
        } else if (I->is(tok::l_brace)) {
            Brackets.push_back(Brace);
        } else if (I->is(tok::r_brace)) {
            if (Brackets.empty() || Brackets.back() == Paren)
                return false;
            Brackets.pop_back();
        }
    }
    return Brackets.empty();
}

/// GenerateNewArgTokens - Returns true if OldTokens can be converted to a new
/// vector of tokens in NewTokens.  The new number of arguments will be placed
/// in NumArgs and the ranges which need to surrounded in parentheses will be
/// in ParenHints.
/// Returns false if the token stream cannot be changed.  If this is because
/// of an initializer list starting a macro argument, the range of those
/// initializer lists will be place in InitLists.
static bool GenerateNewArgTokens(Preprocessor &PP,
                                 SmallVectorImpl<Token> &OldTokens,
                                 SmallVectorImpl<Token> &NewTokens,
                                 unsigned &NumArgs,
                                 SmallVectorImpl<SourceRange> &ParenHints,
                                 SmallVectorImpl<SourceRange> &InitLists) {
    if (!CheckMatchedBrackets(OldTokens))
        return false;

    // Once it is known that the brackets are matched, only a simple count of the
    // braces is needed.
    unsigned Braces = 0;

    // First token of a new macro argument.
    SmallVectorImpl<Token>::iterator ArgStartIterator = OldTokens.begin();

    // First closing brace in a new macro argument.  Used to generate
    // SourceRanges for InitLists.
    SmallVectorImpl<Token>::iterator ClosingBrace = OldTokens.end();
    NumArgs = 0;
    Token TempToken;
    // Set to true when a macro separator token is found inside a braced list.
    // If true, the fixed argument spans multiple old arguments and ParenHints
    // will be updated.
    bool FoundSeparatorToken = false;
    for (SmallVectorImpl<Token>::iterator I = OldTokens.begin(),
                 E = OldTokens.end();
         I != E; ++I) {
        if (I->is(tok::l_brace)) {
            ++Braces;
        } else if (I->is(tok::r_brace)) {
            --Braces;
            if (Braces == 0 && ClosingBrace == E && FoundSeparatorToken)
                ClosingBrace = I;
        } else if (I->is(tok::eof)) {
            // EOF token is used to separate macro arguments
            if (Braces != 0) {
                // Assume comma separator is actually braced list separator and change
                // it back to a comma.
                FoundSeparatorToken = true;
                I->setKind(tok::comma);
                I->setLength(1);
            } else { // Braces == 0
                // Separator token still separates arguments.
                ++NumArgs;

                // If the argument starts with a brace, it can't be fixed with
                // parentheses.  A different diagnostic will be given.
                if (FoundSeparatorToken && ArgStartIterator->is(tok::l_brace)) {
                    InitLists.push_back(
                            SourceRange(ArgStartIterator->getLocation(),
                                        PP.getLocForEndOfToken(ClosingBrace->getLocation())));
                    ClosingBrace = E;
                }

                // Add left paren
                if (FoundSeparatorToken) {
                    TempToken.startToken();
                    TempToken.setKind(tok::l_paren);
                    TempToken.setLocation(ArgStartIterator->getLocation());
                    TempToken.setLength(0);
                    NewTokens.push_back(TempToken);
                }

                // Copy over argument tokens
                NewTokens.insert(NewTokens.end(), ArgStartIterator, I);

                // Add right paren and store the paren locations in ParenHints
                if (FoundSeparatorToken) {
                    SourceLocation Loc = PP.getLocForEndOfToken((I - 1)->getLocation());
                    TempToken.startToken();
                    TempToken.setKind(tok::r_paren);
                    TempToken.setLocation(Loc);
                    TempToken.setLength(0);
                    NewTokens.push_back(TempToken);
                    ParenHints.push_back(SourceRange(ArgStartIterator->getLocation(),
                                                     Loc));
                }

                // Copy separator token
                NewTokens.push_back(*I);

                // Reset values
                ArgStartIterator = I + 1;
                FoundSeparatorToken = false;
            }
        }
    }

    return !ParenHints.empty() && InitLists.empty();
}

/// ReadFunctionLikeMacroArgs - After reading "MACRO" and knowing that the next
/// token is the '(' of the macro, this method is invoked to read all of the
/// actual arguments specified for the macro invocation.  This returns null on
/// error.
MacroArgs * MixedComputations::ReadFunctionLikeMacroArgs(Token &MacroName,
                                                         MacroInfo *MI,
                                                         SourceLocation &MacroEnd) {
    // The number of fixed arguments to parse.
    unsigned NumFixedArgsLeft = MI->getNumArgs();
    bool isVariadic = MI->isVariadic();

    // Outer loop, while there are more arguments, keep reading them.
    Token Tok;

    // Read arguments as unexpanded tokens.  This avoids issues, e.g., where
    // an argument value in a macro could expand to ',' or '(' or ')'.
    PP.LexUnexpandedToken(Tok);
    assert(Tok.is(tok::l_paren) && "Error computing l-paren-ness?");

    // ArgTokens - Build up a list of tokens that make up each argument.  Each
    // argument is separated by an EOF token.  Use a SmallVector so we can avoid
    // heap allocations in the common case.
    SmallVector<Token, 64> ArgTokens;
    bool ContainsCodeCompletionTok = false;

    SourceLocation TooManyArgsLoc;

    unsigned NumActuals = 0;
    while (Tok.isNot(tok::r_paren)) {
        if (ContainsCodeCompletionTok && Tok.isOneOf(tok::eof, tok::eod))
            break;

        assert(Tok.isOneOf(tok::l_paren, tok::comma) &&
               "only expect argument separators here");

        unsigned ArgTokenStart = ArgTokens.size();
        SourceLocation ArgStartLoc = Tok.getLocation();

        // C99 6.10.3p11: Keep track of the number of l_parens we have seen.  Note
        // that we already consumed the first one.
        unsigned NumParens = 0;

        while (1) {
            // Read arguments as unexpanded tokens.  This avoids issues, e.g., where
            // an argument value in a macro could expand to ',' or '(' or ')'.
            PP.LexUnexpandedToken(Tok);

            if (Tok.isOneOf(tok::eof, tok::eod)) { // "#if f(<eof>" & "#if f(\n"
                if (!ContainsCodeCompletionTok) {
                    PP.Diag(MacroName, diag::err_unterm_macro_invoc);
                    PP.Diag(MI->getDefinitionLoc(), diag::note_macro_here)
                        << MacroName.getIdentifierInfo();
                    // Do not lose the EOF/EOD.  Return it to the client.
                    MacroName = Tok;
                    return nullptr;
                } else {
                    // Do not lose the EOF/EOD.
                    Token *Toks = new Token[1];
                    Toks[0] = Tok;
                    PP.EnterTokenStream(Toks, 1, true, true);
                    break;
                }
            } else if (Tok.is(tok::r_paren)) {
                // If we found the ) token, the macro arg list is done.
                if (NumParens-- == 0) {
                    MacroEnd = Tok.getLocation();
                    break;
                }
            } else if (Tok.is(tok::l_paren)) {
                ++NumParens;
            } else if (Tok.is(tok::comma) && NumParens == 0 &&
                       !(Tok.getFlags() & Token::IgnoredComma)) {
                // In Microsoft-compatibility mode, single commas from nested macro
                // expansions should not be considered as argument separators. We test
                // for this with the IgnoredComma token flag above.

                // Comma ends this argument if there are more fixed arguments expected.
                // However, if this is a variadic macro, and this is part of the
                // variadic part, then the comma is just an argument token.
                if (!isVariadic) break;
                if (NumFixedArgsLeft > 1)
                    break;
            } /* else if (Tok.is(tok::comment) && !PP.KeepMacroComments) {
                // If this is a comment token in the argument list and we're just in
                // -C mode (not -CC mode), discard the comment.
                continue;
            } */ else if (!Tok.isAnnotation() && Tok.getIdentifierInfo() != nullptr) {
                // Reading macro arguments can cause macros that we are currently
                // expanding from to be popped off the expansion stack.  Doing so causes
                // them to be reenabled for expansion.  Here we record whether any
                // identifiers we lex as macro arguments correspond to disabled macros.
                // If so, we mark the token as noexpand.  This is a subtle aspect of
                // C99 6.10.3.4p2.
                if (MacroInfo *MI = PP.getMacroInfo(Tok.getIdentifierInfo()))
                if (!MI->isEnabled())
                    Tok.setFlag(Token::DisableExpand);
            } else if (Tok.is(tok::code_completion)) {
                assert("ReadFunctionLikeMacroArgs Code Completion Token not supported");

                /*
                ContainsCodeCompletionTok = true;
                if (PP.CodeComplete)
                    PP.CodeComplete->CodeCompleteMacroArgument(MacroName.getIdentifierInfo(),
                                                               MI, NumActuals);
                // Don't mark that we reached the code-completion point because the
                // parser is going to handle the token and there will be another
                // code-completion callback.
                */
            }

            ArgTokens.push_back(Tok);
        }

        // If this was an empty argument list foo(), don't add this as an empty
        // argument.
        if (ArgTokens.empty() && Tok.getKind() == tok::r_paren)
            break;

        // If this is not a variadic macro, and too many args were specified, emit
        // an error.
        if (!isVariadic && NumFixedArgsLeft == 0 && TooManyArgsLoc.isInvalid()) {
            if (ArgTokens.size() != ArgTokenStart)
                TooManyArgsLoc = ArgTokens[ArgTokenStart].getLocation();
            else
                TooManyArgsLoc = ArgStartLoc;
        }

        // Empty arguments are standard in C99 and C++0x, and are supported as an
        // extension in other modes.
        if (ArgTokens.size() == ArgTokenStart && !PP.getLangOpts().C99)
            PP.Diag(Tok, PP.getLangOpts().CPlusPlus11 ?
                      diag::warn_cxx98_compat_empty_fnmacro_arg :
                      diag::ext_empty_fnmacro_arg);

        // Add a marker EOF token to the end of the token list for this argument.
        Token EOFTok;
        EOFTok.startToken();
        EOFTok.setKind(tok::eof);
        EOFTok.setLocation(Tok.getLocation());
        EOFTok.setLength(0);
        ArgTokens.push_back(EOFTok);
        ++NumActuals;
        if (!ContainsCodeCompletionTok && NumFixedArgsLeft != 0)
            --NumFixedArgsLeft;
    }

    // Okay, we either found the r_paren.  Check to see if we parsed too few
    // arguments.
    unsigned MinArgsExpected = MI->getNumArgs();

    // If this is not a variadic macro, and too many args were specified, emit
    // an error.
    if (!isVariadic && NumActuals > MinArgsExpected &&
        !ContainsCodeCompletionTok) {
        // Emit the diagnostic at the macro name in case there is a missing ).
        // Emitting it at the , could be far away from the macro name.
        PP.Diag(TooManyArgsLoc, diag::err_too_many_args_in_macro_invoc);
        PP.Diag(MI->getDefinitionLoc(), diag::note_macro_here)
            << MacroName.getIdentifierInfo();

        // Commas from braced initializer lists will be treated as argument
        // separators inside macros.  Attempt to correct for this with parentheses.
        // TODO: See if this can be generalized to angle brackets for templates
        // inside macro arguments.

        SmallVector<Token, 4> FixedArgTokens;
        unsigned FixedNumArgs = 0;
        SmallVector<SourceRange, 4> ParenHints, InitLists;
        if (!GenerateNewArgTokens(PP, ArgTokens, FixedArgTokens, FixedNumArgs,
                                  ParenHints, InitLists)) {
            if (!InitLists.empty()) {
                DiagnosticBuilder DB =
                        PP.Diag(MacroName,
                             diag::note_init_list_at_beginning_of_macro_argument);
                for (SourceRange Range : InitLists)
                    DB << Range;
            }
            return nullptr;
        }
        if (FixedNumArgs != MinArgsExpected)
            return nullptr;

        DiagnosticBuilder DB = PP.Diag(MacroName, diag::note_suggest_parens_for_macro);
        for (SourceRange ParenLocation : ParenHints) {
            DB << FixItHint::CreateInsertion(ParenLocation.getBegin(), "(");
            DB << FixItHint::CreateInsertion(ParenLocation.getEnd(), ")");
        }
        ArgTokens.swap(FixedArgTokens);
        NumActuals = FixedNumArgs;
    }

    // See MacroArgs instance var for description of this.
    bool isVarargsElided = false;

    if (ContainsCodeCompletionTok) {
        // Recover from not-fully-formed macro invocation during code-completion.
        Token EOFTok;
        EOFTok.startToken();
        EOFTok.setKind(tok::eof);
        EOFTok.setLocation(Tok.getLocation());
        EOFTok.setLength(0);
        for (; NumActuals < MinArgsExpected; ++NumActuals)
            ArgTokens.push_back(EOFTok);
    }

    if (NumActuals < MinArgsExpected) {
        // There are several cases where too few arguments is ok, handle them now.
        if (NumActuals == 0 && MinArgsExpected == 1) {
            // #define A(X)  or  #define A(...)   ---> A()

            // If there is exactly one argument, and that argument is missing,
            // then we have an empty "()" argument empty list.  This is fine, even if
            // the macro expects one argument (the argument is just empty).
            isVarargsElided = MI->isVariadic();
        } else if (MI->isVariadic() &&
                   (NumActuals+1 == MinArgsExpected ||  // A(x, ...) -> A(X)
                    (NumActuals == 0 && MinArgsExpected == 2))) {// A(x,...) -> A()
            // Varargs where the named vararg parameter is missing: OK as extension.
            //   #define A(x, ...)
            //   A("blah")
            //
            // If the macro contains the comma pasting extension, the diagnostic
            // is suppressed; we know we'll get another diagnostic later.
            if (!MI->hasCommaPasting()) {
                PP.Diag(Tok, diag::ext_missing_varargs_arg);
                PP.Diag(MI->getDefinitionLoc(), diag::note_macro_here)
                    << MacroName.getIdentifierInfo();
            }

            // Remember this occurred, allowing us to elide the comma when used for
            // cases like:
            //   #define A(x, foo...) blah(a, ## foo)
            //   #define B(x, ...) blah(a, ## __VA_ARGS__)
            //   #define C(...) blah(a, ## __VA_ARGS__)
            //  A(x) B(x) C()
            isVarargsElided = true;
        } else if (!ContainsCodeCompletionTok) {
            // Otherwise, emit the error.
            PP.Diag(Tok, diag::err_too_few_args_in_macro_invoc);
            PP.Diag(MI->getDefinitionLoc(), diag::note_macro_here)
                << MacroName.getIdentifierInfo();
            return nullptr;
        }

        // Add a marker EOF token to the end of the token list for this argument.
        SourceLocation EndLoc = Tok.getLocation();
        Tok.startToken();
        Tok.setKind(tok::eof);
        Tok.setLocation(EndLoc);
        Tok.setLength(0);
        ArgTokens.push_back(Tok);

        // If we expect two arguments, add both as empty.
        if (NumActuals == 0 && MinArgsExpected == 2)
            ArgTokens.push_back(Tok);

    } else if (NumActuals > MinArgsExpected && !MI->isVariadic() &&
               !ContainsCodeCompletionTok) {
        // Emit the diagnostic at the macro name in case there is a missing ).
        // Emitting it at the , could be far away from the macro name.
        PP.Diag(MacroName, diag::err_too_many_args_in_macro_invoc);
        PP.Diag(MI->getDefinitionLoc(), diag::note_macro_here)
            << MacroName.getIdentifierInfo();
        return nullptr;
    }

    return MacroArgs::create(MI, ArgTokens, isVarargsElided, PP);
}
