/*
 * Cppcheck - A tool for static C/C++ code analysis
 * Copyright (C) 2007-2018 Cppcheck team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


//---------------------------------------------------------------------------
#include "checkbool.h"

#include "astutils.h"
#include "errorlogger.h"
#include "mathlib.h"
#include "settings.h"
#include "symbols/symbols.h"
#include "token.h"
#include "tokenize.h"
#include "valueflow.h"

#include <cstddef>
#include <list>
//---------------------------------------------------------------------------

// Register this check class (by creating a static instance of it)
namespace {
    CheckBool instance;
}

static const CWE CWE398(398U);  // Indicator of Poor Code Quality
static const CWE CWE571(571U);  // Expression is Always True

static const CWE CWE704(704U);  // Incorrect Type Conversion or Cast

//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
static Token *getNextToken(const Token *tok)
{
    if (!tok->next())
    {
        return nullptr;
    }
    const Variable *var = tok->variable();

    if (var && var->valueType())
    {
        // is dyn_variable []
        if (tok->next()->str() == "[" && tok->next()->link())
        {
            return tok->next()->link()->next();
        }
        return tok->next();
    }

    // is function call
    if (Token::Match(tok, "%name% ("))
    {
        if (tok->next()->str() == "(" && tok->next()->link())
        {
            return tok->next()->link()->next();
        }
        // missing () after function call ?!?
        return nullptr;
    }

    // expression in ()
    if (tok->str() == "(" && tok->link())
    {
        return tok->link()->next();
    }

    // standard constant
    if (Token::Match(tok, "TRUE|true|FALSE|false"))
    {
        return tok->next();
    }

    return nullptr;
}

//---------------------------------------------------------------------------
bool CheckBool::isBool(const Token *tok, std::string &expression)
{
    const Variable *var = tok->variable();

    if (var && var->valueType())
    {
        expression = tok->str();
        // is bool variable
        if (var->valueType()->type == ValueType::BOOL)
        {
            return true;
        }
        /// element of dyn bool
        if ((var->valueType()->type == ValueType::DYN_BOOL || var->valueType()->type == ValueType::DYN_DYN_BOOL) && tok->next()->str() == "[")
        {
            return true;
        }

        return false;
    }

    // is function call
    if (Token::Match(tok, "%name% ("))
    {
        expression = tok->str() + "(...)";
        std::string type = "";
        const Function *func = tok->function();
        if (func && func->retDef)
        {
            type = func->retDef->str();
        }
        else
        {
            type = mSettings->library.returnValueType(tok);
        }
        // is bool variable
        if (type == "bool")
        {
            return true;
        }
        /// element of dyn bool
        if ((type == "dyn_bool" || type == "dyn_dyn_bool") && tok->next()->str() == "[")
        {
            return true;
        }

        return false;
    }

    // expression in ()
    if (tok->str() == "(" && tok->link() &&
        !(tok->link()->next() && tok->link()->next()->str() == "?")) //skip if followed by ternary operator
    {
        bool isExprBool = false;
        expression = "";
        for (const Token *tok2 = tok; tok2 != tok->link(); tok2 = tok2->next())
        {
            expression += tok2->str() + " ";

            if (Token::Match(tok2, "?|:"))
            {
                expression = "";
                return false;
            }

            if (astIsBool(tok2))
            {
                isExprBool = true;
            }
        }
        expression += ")";
        return isExprBool;
    }

    // standard constant
    if (Token::Match(tok, "TRUE|true|FALSE|false"))
    {
        expression = tok->str();
        return true;
    }

    return false;
}

//---------------------------------------------------------------------------
static bool isBool(const Variable *var)
{
    return (var && Token::Match(var->typeEndToken(), "bool"));
}

//---------------------------------------------------------------------------
static bool isNonBoolStdType(const Variable *var)
{
    return (var && var->typeEndToken()->isStandardType() && !Token::Match(var->typeEndToken(), "bool"));
}

//---------------------------------------------------------------------------
const std::set<std::string> incDecOperators = {
    "++",
    "+=",
    "=+",
    "--",
    "-=",
    "=-"};

//---------------------------------------------------------------------------
static bool isIncDecOperator(const Token *tok)
{
    if (!tok)
        return false;

    std::string s = tok->str();
    return (incDecOperators.find(s) != incDecOperators.end());
}

//---------------------------------------------------------------------------
// bool++, bool-- bool+=, bool=+, bool-=, bool=-
//---------------------------------------------------------------------------
void CheckBool::checkIncrementBoolean()
{
    if (!mSettings->isEnabled(Settings::WARNING))
        return;

    const SymbolDatabase *symbolDatabase = mTokenizer->getSymbolDatabase();
    for (const Scope *scope : symbolDatabase->functionScopes)
    {
        for (const Token *tok = scope->bodyStart->next(); tok != scope->bodyEnd; tok = tok->next())
        {
            std::string expression;
            if (!isBool(tok, expression))
            {
                continue;
            }
            // ++b
            if (isIncDecOperator(tok->previous()))
            {
                incrementBooleanError(tok, expression, tok->previous()->str() + "operator");
            }
            // b++
            const Token *next = getNextToken(tok);
            if (isIncDecOperator(next))
            {
                incrementBooleanError(tok, expression, "operator" + next->str());
            }
        }
    }
}

//---------------------------------------------------------------------------
void CheckBool::incrementBooleanError(const Token *tok, const std::string &expression, const std::string &op)
{
    reportError(
        tok,
        Severity::warning,
        "incrementboolean",
        "$symbol:" + expression + "\n" +
        "Incrementing/Decrementing a variable/expression '$symbol' of type 'bool' with " + op + " is not allowed." +
        " You should assign it the value 'true' or 'false' instead.\n" +
        "You should assign it the value 'true' or 'false' instead.",
        CWE398, false);
}

//---------------------------------------------------------------------------
// divide or multiple bool
//---------------------------------------------------------------------------
void CheckBool::divideBoolean()
{
    if (!mSettings->isEnabled(Settings::WARNING))
        return;

    const SymbolDatabase *symbolDatabase = mTokenizer->getSymbolDatabase();
    for (const Scope *scope : symbolDatabase->functionScopes)
    {
        for (const Token *tok = scope->bodyStart->next(); tok != scope->bodyEnd; tok = tok->next())
        {
            std::string expression;
            if (!isBool(tok, expression))
            {
                continue;
            }
            // b/
            if (Token::Match(tok->previous(), "/"))
            {
                divideBooleanError(tok, expression);
            }
            // /b
            const Token *next = getNextToken(tok);
            if (next && Token::Match(next, "/"))
            {
                divideBooleanError(next, expression);
            }
        }
    }
}

//---------------------------------------------------------------------------
void CheckBool::divideBooleanError(const Token *tok, const std::string &expression)
{
    reportError(
        tok,
        Severity::warning,
        "divideBool",
        "$symbol:" + expression + "\n" +
            "Dividation variable/expression '$symbol' of type 'bool' is not allowed.",
        CWE398, false);
}

//---------------------------------------------------------------------------
// if (bool & bool) -> if (bool && bool)
// if (bool | bool) -> if (bool || bool)
//---------------------------------------------------------------------------
void CheckBool::checkBitwiseOnBoolean()
{
    if (!mSettings->isEnabled(Settings::WARNING))
        return;

    const SymbolDatabase *symbolDatabase = mTokenizer->getSymbolDatabase();
    for (const Scope *scope : symbolDatabase->functionScopes)
    {
        for (const Token *tok = scope->bodyStart->next(); tok != scope->bodyEnd; tok = tok->next())
        {
            std::string expression;
            if (!isBool(tok, expression))
            {
                continue;
            }
            // | bool
            if (Token::Match(tok->previous(), "%or%|&"))
            {
                bitwiseOnBooleanError(tok, expression, tok);
            }
            // bool |
            const Token *next = getNextToken(tok);
            if (next && Token::Match(next, "%or%|&"))
            {
                bitwiseOnBooleanError(next, expression, next);
            }
        }
    }
}

//---------------------------------------------------------------------------
void CheckBool::bitwiseOnBooleanError(const Token *tok, const std::string &varname, const Token *op)
{
    std::string okOerator = (op && op->str() == "|") ? "||" : "&&";
    reportError(tok,
                Severity::warning,
                "bitwiseOnBoolean",
                "$symbol:" + varname + "\n" +
                    "Boolean variable/expression '$symbol' is used in bitwise operation. Did you mean '" + okOerator + "'?",
                CWE398,
                true);
}

//-------------------------------------------------------------------------------
// Comparison of bool with relation operator <= < >= >
//-------------------------------------------------------------------------------

void CheckBool::checkComparisonOfBoolWithRelationOperator()
{
    if (!mSettings->isEnabled(Settings::WARNING))
        return;

    const SymbolDatabase *const symbolDatabase = mTokenizer->getSymbolDatabase();

    for (const Scope *scope : symbolDatabase->functionScopes)
    {
        for (const Token *tok = scope->bodyStart->next(); tok != scope->bodyEnd; tok = tok->next())
        {

            std::string expression;
            if (!isBool(tok, expression))
            {
                continue;
            }

            // <= bool
            const Token *previous = tok->previous();
            if (previous->isRelationOp())
            {
                comparisonOfBoolWithRelationOperatorError(tok, expression);
            }
            // bool <=
            const Token *next = getNextToken(tok);
            if (next && next->isRelationOp())
            {
                comparisonOfBoolWithRelationOperatorError(next, expression);
            }
        }
    }
}

//---------------------------------------------------------------------------
void CheckBool::comparisonOfBoolWithRelationOperatorError(const Token *tok, const std::string &expression)
{
    reportError(tok,
                Severity::warning,
                "comparisonOfBoolWithBoolError",
                "Comparison of a variable having boolean value using relational (<, >, <= or >=) operator.\n"
                "The variable '" +
                    expression + "' is of type 'bool' "
                                 "and comparing 'bool' value using relational (<, >, <= or >=)"
                                 " operator could cause unexpected results.",
                CWE398,
                false);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CheckBool::checkComparisonOfBoolExpressionWithInt()
{
    if (!mSettings->isEnabled(Settings::WARNING))
        return;

    const SymbolDatabase* symbolDatabase = mTokenizer->getSymbolDatabase();

    for (const Scope * scope : symbolDatabase->functionScopes) {
        for (const Token* tok = scope->bodyStart->next(); tok != scope->bodyEnd; tok = tok->next()) {
            if (!tok->isComparisonOp())
                continue;

            const Token* numTok = nullptr;
            const Token* boolExpr = nullptr;
            bool numInRhs;
            if (astIsBool(tok->astOperand1())) {
                boolExpr = tok->astOperand1();
                numTok = tok->astOperand2();
                numInRhs = true;
            } else if (astIsBool(tok->astOperand2())) {
                boolExpr = tok->astOperand2();
                numTok = tok->astOperand1();
                numInRhs = false;
            } else {
                continue;
            }

            if (!numTok || !boolExpr)
                continue;

            if (boolExpr->isOp() && numTok->isName() && Token::Match(tok, "==|!="))
                // there is weird code such as:  ((a<b)==c)
                // but it is probably written this way by design.
                continue;

            if (numTok->isNumber()) {
                const MathLib::bigint num = MathLib::toLongNumber(numTok->str());
                if (num==0 &&
                    (numInRhs ? Token::Match(tok, ">|==|!=")
                     : Token::Match(tok, "<|==|!=")))
                    continue;
                if (num==1 &&
                    (numInRhs ? Token::Match(tok, "<|==|!=")
                     : Token::Match(tok, ">|==|!=")))
                    continue;
                comparisonOfBoolExpressionWithIntError(tok, true);
            } else if (isNonBoolStdType(numTok->variable()) && mTokenizer->isCTRL())
                comparisonOfBoolExpressionWithIntError(tok, false);
        }
    }
}

void CheckBool::comparisonOfBoolExpressionWithIntError(const Token *tok, bool n0o1)
{
    if (n0o1)
        reportError(tok, Severity::warning, "compareBoolExpressionWithInt",
                    "Comparison of a boolean expression with an integer other than 0 or 1.", CWE398, false);
    else
        reportError(tok, Severity::warning, "compareBoolExpressionWithInt",
                    "Comparison of a boolean expression with an integer.", CWE398, false);
}

void CheckBool::returnValueOfFunctionReturningBool(void)
{
    if (!mSettings->isEnabled(Settings::WARNING))
        return;

    const SymbolDatabase * const symbolDatabase = mTokenizer->getSymbolDatabase();

    for (const Scope * scope : symbolDatabase->functionScopes) {
        if (!(scope->function && Token::Match(scope->function->retDef, "bool")))
            continue;

        for (const Token* tok = scope->bodyStart->next(); tok && (tok != scope->bodyEnd); tok = tok->next()) {
            if (Token::simpleMatch(tok, "return") && tok->astOperand1() &&
                (tok->astOperand1()->getValueGE(2, mSettings) || tok->astOperand1()->getValueLE(-1, mSettings)))
                returnValueBoolError(tok);
        }
    }
}

void CheckBool::returnValueBoolError(const Token *tok)
{
    reportError(tok, Severity::warning, "returnNonBoolInBooleanFunction", "Non-boolean value returned from function returning bool");
}
