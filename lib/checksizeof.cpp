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
#include "checksizeof.h"

#include "errorlogger.h"
#include "settings.h"
#include "symbols/symbols.h"
#include "token.h"
#include "tokenize.h"

#include <cstddef>
//---------------------------------------------------------------------------

// Register this check class (by creating a static instance of it)
namespace {
    CheckSizeof instance;
}

// CWE IDs used:
static const struct CWE CWE398(398U);   // Indicator of Poor Code Quality
static const struct CWE CWE467(467U);   // Use of sizeof() on a Pointer Type
static const struct CWE CWE682(682U);   // Incorrect Calculation
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void CheckSizeof::checkSizeofForNumericParameter()
{
    if (!mSettings->isEnabled(Settings::WARNING))
        return;

    const SymbolDatabase *symbolDatabase = mTokenizer->getSymbolDatabase();
    for (const Scope * scope : symbolDatabase->functionScopes) {
        for (const Token* tok = scope->bodyStart->next(); tok != scope->bodyEnd; tok = tok->next()) {
            if (Token::Match(tok, "sizeof ( %num% )")) {
                sizeofForNumericParameterError(tok);
            }
        }
    }
}

void CheckSizeof::sizeofForNumericParameterError(const Token *tok)
{
    reportError(tok, Severity::warning,
                "sizeofwithnumericparameter", "Suspicious usage of 'sizeof' with a numeric constant as parameter.\n"
                "It is not allowed to use a constant value with sizeof. For example, 'sizeof(10)'"
                , CWE682, false);
}

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Check for code like sizeof()*sizeof() or sizeof(ptr)/value
//-----------------------------------------------------------------------------
void CheckSizeof::suspiciousSizeofCalculation()
{
    if (!mSettings->isEnabled(Settings::WARNING) || !mSettings->inconclusive)
        return;

    // TODO: Use AST here. This should be possible as soon as sizeof without brackets is correctly parsed
    for (const Token *tok = mTokenizer->tokens(); tok; tok = tok->next()) {
        if (Token::simpleMatch(tok, "sizeof (")) {
            const Token* const end = tok->linkAt(1);
            const Variable* var = end->previous()->variable();
            if (Token::simpleMatch(end, ") * sizeof") && end->next()->astOperand1() == tok->next())
                multiplySizeofError(tok);
        }
    }
}

void CheckSizeof::multiplySizeofError(const Token *tok)
{
    reportError(tok, Severity::warning,
                "multiplySizeof", "Multiplying sizeof() with sizeof() indicates a logic error.", CWE682, true);
}

void CheckSizeof::sizeofVoid()
{
    if (!mSettings->isEnabled(Settings::PORTABILITY))
        return;

    for (const Token *tok = mTokenizer->tokens(); tok; tok = tok->next()) {
        if (Token::simpleMatch(tok, "sizeof ( )")) { // "sizeof(void)" gets simplified to sizeof ( )
            sizeofVoidError(tok);
        }
    }
}

void CheckSizeof::sizeofVoidError(const Token *tok)
{
    const std::string message = "Behaviour of 'sizeof(void)' is not covered by the ISO C standard.";
    const std::string verbose = message + " A value for 'sizeof(void)' is defined only as part of a GNU C extension, which defines 'sizeof(void)' to be 1.";
    reportError(tok, Severity::portability, "sizeofVoid", message + "\n" + verbose, CWE682, false);
}
