/*
 * Cppcheck - A tool for static C/C++ code analysis
 * Copyright (C) 2007-2019 Cppcheck team.
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

#include "checkvaarg.h"

#include "astutils.h"
#include "errorlogger.h"
#include "settings.h"
#include "symbols/symbols.h"
#include "token.h"
#include "tokenize.h"

#include <cstddef>
#include <list>

//---------------------------------------------------------------------------

// Register this check class (by creating a static instance of it)
namespace {
    CheckVaarg instance;
}


//---------------------------------------------------------------------------
// Ensure that correct parameter is passed to va_start()
//---------------------------------------------------------------------------

// CWE ids used:
static const struct CWE CWE664(664U);   // Improper Control of a Resource Through its Lifetime
static const struct CWE CWE688(688U);   // Function Call With Incorrect Variable or Reference as Argument
static const struct CWE CWE758(758U);   // Reliance on Undefined, Unspecified, or Implementation-Defined Behavior

void CheckVaarg::va_start_argument()
{
    const SymbolDatabase* const symbolDatabase = mTokenizer->getSymbolDatabase();
    const std::size_t functions = symbolDatabase->functionScopes.size();

    for (std::size_t i = 0; i < functions; ++i) {
        const Scope* scope = symbolDatabase->functionScopes[i];
        const Function* function = scope->function;
        if (!function)
        {
            // defensive;
            continue;
        }

        for (const Token* tok = scope->bodyStart->next(); tok != scope->bodyEnd; tok = tok->next()) {
            if (!tok->scope()->isExecutable())
            {   
                // va_start can be used only in executable scope
                // this shall be checked by other check
                // see also #174
                tok = tok->scope()->bodyEnd;
            }

            else if (Token::simpleMatch(tok, "va_start (")) {
                const Token *param = tok->tokAt(2); // get first param
                if (!param)
                {
                    // there are no param given.
                    // it is bug, but it is checked by 'checkLibraryFunctionArgCount'
                    // so just skip it here and did not throw it twice.
                    continue;
                }
                const Variable *var = param->variable();
                if (var && var->isReference())
                {
                    referenceAs_va_start_error(param, var->name());
                }
                tok = tok->linkAt(1);
            }
        }
    }
}


void CheckVaarg::referenceAs_va_start_error(const Token *tok, const std::string& paramName)
{
    reportError(tok, Severity::error,
                "va_start_referencePassed", "$symbol:" + paramName + "\nUsing reference '$symbol' as parameter for va_start() results in undefined behaviour.", CWE758, false);
}

//---------------------------------------------------------------------------
// Detect missing va_end() if va_start() was used
// Detect va_list usage after va_end()
//---------------------------------------------------------------------------

void CheckVaarg::va_list_usage()
{
    const SymbolDatabase* const symbolDatabase = mTokenizer->getSymbolDatabase();
    for (const Variable* var : symbolDatabase->variableList()) {
        if (!var || var->isReference() || var->isArray() || !var->scope() || var->typeStartToken()->str() != "va_list")
            continue;
        if (!var->isLocal() && !var->isArgument()) // Check only local variables and arguments
            continue;

        bool open = var->isArgument(); // va_list passed as argument are opened
        bool exitOnEndOfStatement = false;

        const Token* tok = var->nameToken()->next();
        for (;  tok && tok != var->scope()->bodyEnd; tok = tok->next()) {
            if (Token::Match(tok, "va_start ( %varid%", var->declarationId())) {
                if (open)
                    va_start_subsequentCallsError(tok, var->name());
                open = true;
                tok = tok->linkAt(1);
            } else if (Token::Match(tok, "va_end ( %varid%", var->declarationId())) {
                if (!open)
                    va_list_usedBeforeStartedError(tok, var->name());
                open = false;
                tok = tok->linkAt(1);
            } else if (Token::Match(tok, "return"))
                exitOnEndOfStatement = true;
            else if (tok->str() == "break") {
                const Scope* scope = tok->scope();
                while (scope->nestedIn && scope->type != Scope::eFor && scope->type != Scope::eWhile && scope->type != Scope::eDo && scope->type != Scope::eSwitch)
                    scope = scope->nestedIn;
                tok = scope->bodyEnd;
                if (!tok)
                    return;
            } else if (tok->str() == "try") {
                open = false;
                break;
            } else if (!open && tok->varId() == var->declarationId())
                va_list_usedBeforeStartedError(tok, var->name());
            else if (exitOnEndOfStatement && tok->str() == ";")
                break;
        }
        if (open && !var->isArgument())
            va_end_missingError(tok, var->name());
    }
}

void CheckVaarg::va_end_missingError(const Token *tok, const std::string& varname)
{
    reportError(tok, Severity::error,
                "va_end_missing", "$symbol:" + varname + "\nva_list '$symbol' was opened but not closed by va_end().", CWE664, false);
}

void CheckVaarg::va_list_usedBeforeStartedError(const Token *tok, const std::string& varname)
{
    reportError(tok,  Severity::error,
                "va_list_usedBeforeStarted", "$symbol:" + varname + "\nva_list '$symbol' used before va_start() was called.", CWE664, false);
}

void CheckVaarg::va_start_subsequentCallsError(const Token *tok, const std::string& varname)
{
    reportError(tok, Severity::error,
                "va_start_subsequentCalls", "$symbol:" + varname + "\nva_start() called subsequently on '$symbol' without va_end() in between.", CWE664, false);
}
