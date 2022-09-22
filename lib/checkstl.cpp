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

#include "checkstl.h"

#include "errorlogger.h"
#include "settings.h"
#include "symbols/symbols.h"
#include "token.h"
#include "utils.h"
#include "astutils.h"

#include <cstddef>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <utility>

// Register this check class (by creating a static instance of it)
namespace {
    CheckStl instance;
}

// CWE IDs used:
static const struct CWE CWE786(786U);   // Access of Memory Location Before Start of Buffer

void CheckStl::negativeIndex()
{
    // Negative index is out of bounds..
    const SymbolDatabase* const symbolDatabase = mTokenizer->getSymbolDatabase();
    for (const Scope * scope : symbolDatabase->functionScopes) {
        for (const Token* tok = scope->bodyStart->next(); tok != scope->bodyEnd; tok = tok->next()) {
            if (!Token::Match(tok, "%var% [") || WRONG_DATA(!tok->next()->astOperand2(), tok))
                continue;
            const Variable * const var = tok->variable();
            if (!var || tok == var->nameToken())
                continue;
            const ValueFlow::Value *index = tok->next()->astOperand2()->getValueLE(-1, mSettings);
            if (!index)
                continue;
            negativeIndexError(tok, *index);
        }
    }
}

void CheckStl::negativeIndexError(const Token *tok, const ValueFlow::Value &index)
{
    const ErrorPath errorPath = getErrorPath(tok, &index, "Negative array index");
    std::ostringstream errmsg;
    if (index.condition)
        errmsg << ValueFlow::eitherTheConditionIsRedundant(index.condition)
               << ", otherwise there is negative array index " << index.intvalue << ".";
    else
        errmsg << "Array index " << index.intvalue << " is out of bounds.";
    reportError(errorPath, index.errorSeverity() ? Severity::error : Severity::warning, "negativeContainerIndex", errmsg.str(), CWE786, index.isInconclusive());
}