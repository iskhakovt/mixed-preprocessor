// Copyright (c) Timur Iskhakov.
// Distributed under the terms of the GNU GPL v3 License.


#ifndef MIXED_PREPROCESSOR_MIXEDTOKEN_HPP
#define MIXED_PREPROCESSOR_MIXEDTOKEN_HPP


#include "MixedMacroArgs.hpp"

#include "clang/Lex/MacroInfo.h"
#include "clang/Lex/Token.h"
#include "clang/Basic/TokenKinds.h"

#include <memory>
#include <vector>
#include <unordered_set>

using namespace clang;


class MixedMacroArgs;
class MixedToken;
typedef std::shared_ptr<MixedToken> MixedToken_ptr_t;


class MixedToken {
protected:
    bool Expanded;

public:
    MixedToken(bool Expanded) : Expanded(Expanded) {}
    virtual ~MixedToken() {}

    virtual std::vector<MixedToken_ptr_t> getExpanded(MixedMacroArgs &Args) const = 0;
    virtual std::vector<MixedToken_ptr_t> getUnexpanded(MixedMacroArgs &Args) const = 0;
    virtual void addExpansionStack(const std::unordered_set<const MacroInfo *> &Stack) = 0;

    bool isExpanded() const { return Expanded; }
    void setExpanded() { Expanded = true; }

    virtual bool isAnyIdentifier() const = 0;
    virtual IdentifierInfo * getIdentifierInfo() const = 0;

    virtual bool is(tok::TokenKind K) const = 0;
    virtual bool isNot(tok::TokenKind K) const = 0;
    virtual bool isOneOf(tok::TokenKind K1, tok::TokenKind K2) const = 0;

    virtual bool isCommonToken() const = 0;
};

class CommonToken : public MixedToken {
    const Token Tok;

public:
    CommonToken(const Token &Tok, bool Expanded) : MixedToken(Expanded), Tok(Tok) {}

    std::vector<MixedToken_ptr_t> getExpanded(MixedMacroArgs &Args) const override;
    std::vector<MixedToken_ptr_t> getUnexpanded(MixedMacroArgs &Args) const override;
    void addExpansionStack(const std::unordered_set<const MacroInfo *> &Stack) override;

    bool isAnyIdentifier() const override { return Tok.isAnyIdentifier(); }
    IdentifierInfo * getIdentifierInfo() const override { return Tok.getIdentifierInfo(); }

    bool is(tok::TokenKind K) const override { return Tok.is(K); }
    bool isNot(tok::TokenKind K) const override { return Tok.isNot(K); }
    bool isOneOf(tok::TokenKind K1, tok::TokenKind K2) const override { return Tok.isOneOf(K1, K2); }

    bool isCommonToken() const override { return true; };

    const Token & getTok() const { return Tok; }
};

class IdentifierArgToken : public CommonToken {
    std::unordered_set<const MacroInfo *> ExpansionStack;

public:
    IdentifierArgToken(const Token &Tok, bool Expanded,
                       const std::unordered_set<const MacroInfo *> &ExpansionStack) :
            CommonToken(Tok, Expanded), ExpansionStack(ExpansionStack) {
        assert(Tok.getIdentifierInfo());
    }

    std::vector<MixedToken_ptr_t> getExpanded(MixedMacroArgs &Args) const override;
    std::vector<MixedToken_ptr_t> getUnexpanded(MixedMacroArgs &Args) const override;
    void addExpansionStack(const std::unordered_set<const MacroInfo *> &Stack) override;

    bool isAnyIdentifier() const override { return true; }
};

class MixedArgToken : public MixedToken {
    const unsigned ArgNum;
    std::unordered_set<const MacroInfo *> ExpansionStack;

public:
    MixedArgToken(unsigned getArgNum, bool Expanded,
                  const std::unordered_set<const MacroInfo *> &ExpansionStack) :
            MixedToken(Expanded), ArgNum(getArgNum), ExpansionStack(ExpansionStack) {}

    std::vector<MixedToken_ptr_t> getExpanded(MixedMacroArgs &Args) const override;
    std::vector<MixedToken_ptr_t> getUnexpanded(MixedMacroArgs &Args) const override;
    void addExpansionStack(const std::unordered_set<const MacroInfo *> &Stack) override;

    bool isAnyIdentifier() const override { return false; }
    IdentifierInfo * getIdentifierInfo() const override { return nullptr; }

    bool is(tok::TokenKind K) const override { return false; }
    bool isNot(tok::TokenKind K) const override { return false; }
    bool isOneOf(tok::TokenKind K1, tok::TokenKind K2) const override { return false; }

    bool isCommonToken() const override { return false; }
};

/*
class StringifyToken : public MixedToken {
    std::vector<MixedToken_ptr_t> Tokens;
    const bool Charify;
};
*/


#endif //MIXED_PREPROCESSOR_MIXEDTOKEN_HPP
