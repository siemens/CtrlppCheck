
#include "calculationssimplifier.h"

#include "errorlogger.h"
#include "mathlib.h"
#include "settings.h"
#include "token.h"
#include "tokenize.h"
#include "tokenlist.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <map>
#include <stack>
#include <utility>

static bool isLowerThanLogicalAnd(const Token *lower)
{
    return lower->isAssignmentOp() || Token::Match(lower, "}|;|(|[|]|)|,|?|:|%oror%|return|throw|case");
}

static bool isLowerThanOr(const Token* lower)
{
    return isLowerThanLogicalAnd(lower) || lower->str() == "&&";
}

static bool isLowerThanXor(const Token* lower)
{
    return isLowerThanOr(lower) || lower->str() == "|";
}

static bool isLowerThanAnd(const Token* lower)
{
    return isLowerThanXor(lower) || lower->str() == "^";
}

static bool isLowerThanShift(const Token* lower)
{
    return isLowerThanAnd(lower) || lower->str() == "&";
}

static bool isLowerThanPlusMinus(const Token* lower)
{
    return isLowerThanShift(lower) || Token::Match(lower, "%comp%|<<|>>");
}

static bool isLowerThanMulDiv(const Token* lower)
{
    return isLowerThanPlusMinus(lower) || Token::Match(lower, "+|-");
}

static bool isLowerEqualThanMulDiv(const Token* lower)
{
    return isLowerThanMulDiv(lower) || Token::Match(lower, "[*/%]");
}

CalculationsSimplifier::CalculationsSimplifier(Tokenizer *tokenizer)
    : mTokenizer(tokenizer), mTokenList(tokenizer->list)
{
}

CalculationsSimplifier::~CalculationsSimplifier()
{
}

void CalculationsSimplifier::eraseTokens(Token *begin, const Token *end)
{
    if (!begin || begin == end)
        return;

    while (begin->next() && begin->next() != end) {
        begin->deleteNext();
    }
}

bool CalculationsSimplifier::simplifyNumericCalculations(Token *tok)
{
    bool ret = false;
    // (1-2)
    while (tok->tokAt(3) && tok->isNumber() && tok->tokAt(2)->isNumber()) { // %any% %num% %any% %num% %any%
        const Token *before = tok->previous();
        if (!before)
            break;
        const Token* op = tok->next();
        const Token* after = tok->tokAt(3);
        const std::string &num1 = op->previous()->str();
        const std::string &num2 = op->next()->str();
        if (Token::Match(before, "* %num% /") && (num2 != "0") && num1 == MathLib::multiply(num2, MathLib::divide(num1, num2))) {
            // Division where result is a whole number
        } else if (!((op->str() == "*" && (isLowerThanMulDiv(before) || before->str() == "*") && isLowerEqualThanMulDiv(after)) || // associative
                     (Token::Match(op, "[/%]") && isLowerThanMulDiv(before) && isLowerEqualThanMulDiv(after)) || // NOT associative
                     (Token::Match(op, "[+-]") && isLowerThanMulDiv(before) && isLowerThanMulDiv(after)) || // Only partially (+) associative, but handled later
                     (Token::Match(op, ">>|<<") && isLowerThanShift(before) && isLowerThanPlusMinus(after)) || // NOT associative
                     (op->str() == "&" && isLowerThanShift(before) && isLowerThanShift(after)) || // associative
                     (op->str() == "^" && isLowerThanAnd(before) && isLowerThanAnd(after)) || // associative
                     (op->str() == "|" && isLowerThanXor(before) && isLowerThanXor(after)) || // associative
                     (op->str() == "&&" && isLowerThanOr(before) && isLowerThanOr(after)) ||
                     (op->str() == "||" && isLowerThanLogicalAnd(before) && isLowerThanLogicalAnd(after))))
            break;

        // Don't simplify "%num% / 0"
        if (Token::Match(op, "[/%] 0"))
            break;

        // Integer operations
        if (Token::Match(op, ">>|<<|&|^|%or%")) {
            // Don't simplify if operand is negative, shifting with negative
            // operand is UB. Bitmasking with negative operand is implementation
            // defined behaviour.
            if (MathLib::isNegative(num1) || MathLib::isNegative(num2))
                break;

            const MathLib::value v1(num1);
            const MathLib::value v2(num2);

            if (!v1.isInt() || !v2.isInt())
                break;

            switch (op->str()[0]) {
            case '<':
                tok->str((v1 << v2).str());
                break;
            case '>':
                tok->str((v1 >> v2).str());
                break;
            case '&':
                tok->str((v1 & v2).str());
                break;
            case '|':
                tok->str((v1 | v2).str());
                break;
            case '^':
                tok->str((v1 ^ v2).str());
                break;
            };
        }

        // Logical operations
        else if (Token::Match(op, "%oror%|&&")) {
            const bool op1 = !MathLib::isNullValue(num1);
            const bool op2 = !MathLib::isNullValue(num2);
            const bool result = (op->str() == "||") ? (op1 || op2) : (op1 && op2);
            tok->str(result ? "1" : "0");
        }

        else if (Token::Match(tok->previous(), "- %num% - %num%"))
            tok->str(MathLib::add(num1, num2));
        else if (Token::Match(tok->previous(), "- %num% + %num%"))
            tok->str(MathLib::subtract(num1, num2));
        else {
            try {
                tok->str(MathLib::calculate(num1, num2, op->str()[0]));
            } catch (InternalError &e) {
                e.token = tok;
                throw;
            }
        }

        tok->deleteNext(2);

        ret = true;
    }

    return ret;
}

// TODO: This is not the correct class for simplifyCalculations(), so it
// should be moved away.
bool CalculationsSimplifier::simplifyCalculations(Token* frontToken, Token *backToken)
{
    bool ret = false;
    if (!frontToken) {
        frontToken = mTokenList.front();
    }
    for (Token *tok = frontToken; tok != backToken; tok = tok->next()) {
        // Remove parentheses around variable..
        // keep parentheses here: dynamic_cast<Fred *>(p);
        // keep parentheses here: A operator * (int);
        // keep parentheses here: int ( * ( * f ) ( ... ) ) (int) ;
        // keep parentheses here: int ( * * ( * compilerHookVector ) (void) ) ( ) ;
        // keep parentheses here: operator new [] (size_t);
        // keep parentheses here: Functor()(a ... )
        // keep parentheses here: ) ( var ) ;
        if ((Token::Match(tok->next(), "( %name% ) ;|)|,|]") ||
             (Token::Match(tok->next(), "( %name% ) %cop%") && (tok->tokAt(2)->varId()>0 || !Token::Match(tok->tokAt(4), "[*&+-~]")))) &&
            !tok->isName() &&
            tok->str() != ">" &&
            tok->str() != ")" &&
            tok->str() != "]") {
            tok->deleteNext();
            tok = tok->next();
            tok->deleteNext();
            ret = true;
        }

        if (Token::Match(tok->previous(), "(|&&|%oror% %char% %comp% %num% &&|%oror%|)")) {
            tok->str(MathLib::toString(MathLib::toLongNumber(tok->str())));
        }

        if (tok->isNumber()) {
            if (simplifyNumericCalculations(tok)) {
                ret = true;
                Token *prev = tok->tokAt(-2);
                while (prev && simplifyNumericCalculations(prev)) {
                    tok = prev;
                    prev = prev->tokAt(-2);
                }
            }

            // Remove redundant conditions (0&&x) (1||x)
            if (Token::Match(tok->previous(), "[(=,] 0 &&") ||
                Token::Match(tok->previous(), "[(=,] 1 %oror%")) {
                unsigned int par = 0;
                const Token *tok2 = tok;
                const bool andAnd = (tok->next()->str() == "&&");
                for (; tok2; tok2 = tok2->next()) {
                    if (tok2->str() == "(" || tok2->str() == "[")
                        ++par;
                    else if (tok2->str() == ")" || tok2->str() == "]") {
                        if (par == 0)
                            break;
                        --par;
                    } else if (par == 0 && isLowerThanLogicalAnd(tok2) && (andAnd || tok2->str() != "||"))
                        break;
                }
                if (tok2) {
                    eraseTokens(tok, tok2);
                    ret = true;
                }
                continue;
            }

            if (tok->str() == "0") {
                if ((Token::Match(tok->previous(), "[+-] 0 %cop%|;") && isLowerThanMulDiv(tok->next())) ||
                    (Token::Match(tok->previous(), "%or% 0 %cop%|;") && isLowerThanXor(tok->next()))) {
                    tok = tok->previous();
                    if (Token::Match(tok->tokAt(-4), "[;{}] %name% = %name% [+-|] 0 ;") &&
                        tok->strAt(-3) == tok->previous()->str()) {
                        tok = tok->tokAt(-4);
                        tok->deleteNext(5);
                    } else {
                        tok = tok->previous();
                        tok->deleteNext(2);
                    }
                    ret = true;
                } else if (Token::Match(tok->previous(), "[=([,] 0 [+|]") ||
                           Token::Match(tok->previous(), "return|case 0 [+|]")) {
                    tok = tok->previous();
                    tok->deleteNext(2);
                    ret = true;
                } else if (Token::Match(tok->previous(), "[=[(,] 0 * %name%|%num% ,|]|)|;|=|%cop%") ||
                           Token::Match(tok->previous(), "[=[(,] 0 * (") ||
                           Token::Match(tok->previous(), "return|case 0 *|&& %name%|%num% ,|:|;|=|%cop%") ||
                           Token::Match(tok->previous(), "return|case 0 *|&& (")) {
                    tok->deleteNext();
                    if (tok->next()->str() == "(")
                        eraseTokens(tok, tok->next()->link());
                    tok->deleteNext();
                    ret = true;
                } else if (Token::Match(tok->previous(), "[=[(,] 0 && *|& %any% ,|]|)|;|=|%cop%") ||
                           Token::Match(tok->previous(), "return|case 0 && *|& %any% ,|:|;|=|%cop%")) {
                    tok->deleteNext();
                    tok->deleteNext();
                    if (tok->next()->str() == "(")
                        eraseTokens(tok, tok->next()->link());
                    tok->deleteNext();
                    ret = true;
                }
            }

            if (tok->str() == "1") {
                if (Token::Match(tok->previous(), "[=[(,] 1 %oror% %any% ,|]|)|;|=|%cop%") ||
                    Token::Match(tok->previous(), "return|case 1 %oror% %any% ,|:|;|=|%cop%")) {
                    tok->deleteNext();
                    if (tok->next()->str() == "(")
                        eraseTokens(tok, tok->next()->link());
                    tok->deleteNext();
                    ret = true;
                } else if (Token::Match(tok->previous(), "[=[(,] 1 %oror% *|& %any% ,|]|)|;|=|%cop%") ||
                           Token::Match(tok->previous(), "return|case 1 %oror% *|& %any% ,|:|;|=|%cop%")) {
                    tok->deleteNext();
                    tok->deleteNext();
                    if (tok->next()->str() == "(")
                        eraseTokens(tok, tok->next()->link());
                    tok->deleteNext();
                    ret = true;
                }
            }

            if (Token::Match(tok->tokAt(-2), "%any% * 1") || Token::Match(tok->previous(), "%any% 1 *")) {
                tok = tok->previous();
                if (tok->str() == "*")
                    tok = tok->previous();
                tok->deleteNext(2);
                ret = true;
            }

            // Remove parentheses around number..
            if (Token::Match(tok->tokAt(-2), "%op%|< ( %num% )") && tok->strAt(-2) != ">") {
                tok = tok->previous();
                tok->deleteThis();
                tok->deleteNext();
                ret = true;
            }

            if (Token::Match(tok->previous(), "( 0 [|+]") ||
                Token::Match(tok->previous(), "[|+-] 0 )")) {
                tok = tok->previous();
                if (Token::Match(tok, "[|+-]"))
                    tok = tok->previous();
                tok->deleteNext(2);
                ret = true;
            }

            if (Token::Match(tok, "%num% %comp% %num%") &&
                MathLib::isInt(tok->str()) &&
                MathLib::isInt(tok->strAt(2))) {
                if (Token::Match(tok->previous(), "(|&&|%oror%") && Token::Match(tok->tokAt(3), ")|&&|%oror%|?")) {
                    const MathLib::bigint op1(MathLib::toLongNumber(tok->str()));
                    const std::string &cmp(tok->next()->str());
                    const MathLib::bigint op2(MathLib::toLongNumber(tok->strAt(2)));

                    std::string result;

                    if (cmp == "==")
                        result = (op1 == op2) ? "1" : "0";
                    else if (cmp == "!=")
                        result = (op1 != op2) ? "1" : "0";
                    else if (cmp == "<=")
                        result = (op1 <= op2) ? "1" : "0";
                    else if (cmp == ">=")
                        result = (op1 >= op2) ? "1" : "0";
                    else if (cmp == "<")
                        result = (op1 < op2) ? "1" : "0";
                    else
                        result = (op1 > op2) ? "1" : "0";

                    tok->str(result);
                    tok->deleteNext(2);
                    ret = true;
                    tok = tok->previous();
                }
            }
        }
    }
    return ret;
}