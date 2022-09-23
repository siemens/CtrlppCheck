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
// Check functions
//---------------------------------------------------------------------------

#include "checkfunctions.h"

#include "astutils.h"
#include "mathlib.h"
#include "symbols/symbols.h"
#include "token.h"
#include "tokenize.h"
#include "valueflow.h"

#include <cmath>
#include <cstddef>
#include <iomanip>
#include <ostream>
#include <vector>

//---------------------------------------------------------------------------


// Register this check class (by creating a static instance of it)
namespace {
    CheckFunctions instance;
}

static const CWE CWE252(252U);  // Unchecked Return Value

/// Indicator of Poor Code Quality
static const CWE CWE398(398U);

static const CWE CWE477(477U);  // Use of Obsolete Functions
static const CWE CWE758(758U);  // Reliance on Undefined, Unspecified, or Implementation-Defined Behavior
static const CWE CWE628(628U);  // Function Call with Incorrectly Specified Arguments
static const CWE CWE686(686U);  // Function Call With Incorrect Argument Type
static const CWE CWE687(687U);  // Function Call With Incorrectly Specified Argument Value
static const CWE CWE688(688U);  // Function Call With Incorrect Variable or Reference as Argument


void CheckFunctions::checkProhibitedFunctions()
{
    const bool checkAlloca = mSettings->isEnabled(Settings::WARNING);

    const SymbolDatabase *symbolDatabase = mTokenizer->getSymbolDatabase();
    for (const Scope *scope : symbolDatabase->functionScopes) {
        for (const Token* tok = scope->bodyStart; tok != scope->bodyEnd; tok = tok->next()) {
            if (!Token::Match(tok, "%name% (") && tok->varId() == 0)
                continue;

                if (tok->function() && tok->function()->hasBody())
                    continue;

                //@todo adapt getWarnInfo and all associated funtions to work with ctrl functions
                const Library::WarnInfo* wi = mSettings->library.getWarnInfo(tok);
                if (wi) {
                    if (mSettings->isEnabled(wi->severity)) {
                        reportError(tok, wi->severity, tok->str() + "Called", wi->message, CWE477, false);
                    }
                }
            
        }
    }
}

//---------------------------------------------------------------------------
// Check <valid> and <not-bool>
//---------------------------------------------------------------------------
void CheckFunctions::invalidFunctionUsage()
{
    const SymbolDatabase* symbolDatabase = mTokenizer->getSymbolDatabase();
    for (const Scope *scope : symbolDatabase->functionScopes) {
        for (const Token* tok = scope->bodyStart->next(); tok != scope->bodyEnd; tok = tok->next()) {
            if (!Token::Match(tok, "%name% ( !!)"))
                continue;
            const Token * const functionToken = tok;
            const std::vector<const Token *> arguments = getArguments(tok);
            for (unsigned int argnr = 1; argnr <= arguments.size(); ++argnr) {
                const Token * const argtok = arguments[argnr-1];

                // check <valid>...</valid>
                const ValueFlow::Value *invalidValue = argtok->getInvalidValue(functionToken,argnr,mSettings);
                if (invalidValue) {
                    invalidFunctionArgError(argtok, functionToken->next()->astOperand1()->expressionString(), argnr, invalidValue, mSettings->library.validarg(functionToken, argnr));
                }

                if (astIsBool(argtok)) {
                    // check <not-bool>
                    if (mSettings->library.isboolargbad(functionToken, argnr))
                        invalidFunctionArgBoolError(argtok, functionToken->str(), argnr);

                    // Are the values 0 and 1 valid?
                    else if (!mSettings->library.isIntArgValid(functionToken, argnr, 0))
                        invalidFunctionArgError(argtok, functionToken->str(), argnr, nullptr, mSettings->library.validarg(functionToken, argnr));
                    else if (!mSettings->library.isIntArgValid(functionToken, argnr, 1))
                        invalidFunctionArgError(argtok, functionToken->str(), argnr, nullptr, mSettings->library.validarg(functionToken, argnr));
                }

                if (mSettings->library.isargstrz(functionToken, argnr)) {
                    if (Token::Match(argtok, "& %var% !![") && argtok->next() && argtok->next()->valueType()) {
                        const ValueType * valueType = argtok->next()->valueType();
                        const Variable * variable = argtok->next()->variable();
                        if (valueType->type == ValueType::Type::CHAR && !variable->isArray() && !variable->isGlobal() &&
                            (!argtok->next()->hasKnownValue() || argtok->next()->getValue(0) == nullptr)) {
                            invalidFunctionArgStrError(argtok, functionToken->str(), argnr);
                        }
                    }
                }
            }
        }
    }
}

void CheckFunctions::invalidFunctionArgError(const Token *tok, const std::string &functionName, int argnr, const ValueFlow::Value *invalidValue, const std::string &validstr)
{
    std::ostringstream errmsg;
    errmsg << "$symbol:" << functionName << '\n';
    if (invalidValue && invalidValue->condition)
        errmsg << ValueFlow::eitherTheConditionIsRedundant(invalidValue->condition)
               << " or $symbol() argument nr " << argnr << " can have invalid value.";
    else
        errmsg << "Invalid $symbol() argument nr " << argnr << '.';
    if (invalidValue)
        errmsg << " The value is " << std::setprecision(10) << (invalidValue->isIntValue() ? invalidValue->intvalue : invalidValue->floatValue) << " but the valid values are '" << validstr << "'.";
    else
        errmsg << " The value is 0 or 1 (boolean) but the valid values are '" << validstr << "'.";
    if (invalidValue)
        reportError(getErrorPath(tok, invalidValue, "Invalid argument"),
                    invalidValue->errorSeverity() ? Severity::error : Severity::warning,
                    "invalidFunctionArg",
                    errmsg.str(),
                    CWE628,
                    invalidValue->isInconclusive());
    else
        reportError(tok,
                    Severity::error,
                    "invalidFunctionArg",
                    errmsg.str(),
                    CWE628,
                    false);
}

void CheckFunctions::invalidFunctionArgBoolError(const Token *tok, const std::string &functionName, int argnr)
{
    std::ostringstream errmsg;
    errmsg << "$symbol:" << functionName << '\n';
    errmsg << "Invalid $symbol() argument nr " << argnr << ". A non-boolean value is required.";
    reportError(tok, Severity::error, "invalidFunctionArgBool", errmsg.str(), CWE628, false);
}

void CheckFunctions::invalidFunctionArgStrError(const Token *tok, const std::string &functionName, unsigned int argnr)
{
    std::ostringstream errmsg;
    errmsg << "$symbol:" << functionName << '\n';
    errmsg << "Invalid $symbol() argument nr " << argnr << ". A nul-terminated string is required.";
    reportError(tok, Severity::error, "invalidFunctionArgStr", errmsg.str(), CWE628, false);
}

//---------------------------------------------------------------------------
// Check for ignored return values.
//---------------------------------------------------------------------------
void CheckFunctions::checkIgnoredReturnValue()
{
    if (!mSettings->isEnabled(Settings::WARNING))
        return;

    const SymbolDatabase *symbolDatabase = mTokenizer->getSymbolDatabase();
    for (const Scope *scope : symbolDatabase->functionScopes) {
        for (const Token* tok = scope->bodyStart->next(); tok != scope->bodyEnd; tok = tok->next()) {
            // skip c++11 initialization, ({...})
            if (Token::Match(tok, "%var%|(|, {"))
                tok = tok->linkAt(1);
            else if (Token::Match(tok, "[(<]") && tok->link())
                tok = tok->link();

            if (tok->varId() || !Token::Match(tok, "%name% ("))
                continue;

            if (tok->next()->astParent())
                continue;

            if (!tok->scope()->isExecutable()) {
                tok = tok->scope()->bodyEnd;
                continue;
            }

            if ((!tok->function() || !Token::Match(tok->function()->retDef, "void %name%")) &&
                (mSettings->library.isUseRetVal(tok) || (tok->function() && tok->function()->isAttributeNodiscard())) &&
                !WRONG_DATA(!tok->next()->astOperand1(), tok)) {
                ignoredReturnValueError(tok, tok->next()->astOperand1()->expressionString());
            }
        }
    }
}

void CheckFunctions::ignoredReturnValueError(const Token* tok, const std::string& function)
{
    reportError(tok, Severity::warning, "ignoredReturnValue",
                "$symbol:" + function + "\nReturn value of function is not used: $symbol()", CWE252, false);
}


//---------------------------------------------------------------------------
// Detect passing wrong values to <cmath> functions like atan(0, x);
//---------------------------------------------------------------------------
void CheckFunctions::checkMathFunctions()
{
    const bool printWarnings = mSettings->isEnabled(Settings::WARNING);

    const SymbolDatabase *symbolDatabase = mTokenizer->getSymbolDatabase();
    for (const Scope *scope : symbolDatabase->functionScopes) {
        for (const Token* tok = scope->bodyStart->next(); tok != scope->bodyEnd; tok = tok->next()) {
            if (tok->varId())
                continue;
            if (printWarnings && Token::Match(tok, "%name% ( !!)")) {
                if (tok->strAt(-1) != "."
                    && Token::Match(tok, "log|log10 ( %num% )")) {
                    const std::string& number = tok->strAt(2);
                    if ((MathLib::isInt(number) && MathLib::toLongNumber(number) <= 0) ||
                        (MathLib::isFloat(number) && MathLib::toDoubleNumber(number) <= 0.))
                        mathfunctionCallWarning(tok);
                }
                // atan2 ( x , y): x and y can not be zero, because this is mathematically not defined
                else if (Token::Match(tok, "atan2 ( %num% , %num% )")) {
                    if (MathLib::isNullValue(tok->strAt(2)) && MathLib::isNullValue(tok->strAt(4)))
                        mathfunctionCallWarning(tok, 2);
                }
                // fmod ( x , y) If y is zero, then either a range error will occur or the function will return zero (implementation-defined).
                else if (Token::Match(tok, "fmod (")) {
                    const Token* nextArg = tok->tokAt(2)->nextArgument();
                    if (nextArg && nextArg->isNumber() && MathLib::isNullValue(nextArg->str()))
                        mathfunctionCallWarning(tok, 2);
                }
                // pow ( x , y) If x is zero, and y is negative --> division by zero
                else if (Token::Match(tok, "pow ( %num% , %num% )")) {
                    if (MathLib::isNullValue(tok->strAt(2)) && MathLib::isNegative(tok->strAt(4)))
                        mathfunctionCallWarning(tok, 2);
                }
            }     
        }
    }
}

void CheckFunctions::mathfunctionCallWarning(const Token *tok, const unsigned int numParam)
{
    if (tok) {
        if (numParam == 1)
            reportError(tok, Severity::warning, "wrongmathcall", "$symbol:" + tok->str() + "\nPassing value " + tok->strAt(2) + " to $symbol() leads to implementation-defined result.", CWE758, false);
        else if (numParam == 2)
            reportError(tok, Severity::warning, "wrongmathcall", "$symbol:" + tok->str() + "\nPassing values " + tok->strAt(2) + " and " + tok->strAt(4) + " to $symbol() leads to implementation-defined result.", CWE758, false);
    } else
        reportError(tok, Severity::warning, "wrongmathcall", "Passing value '#' to #() leads to implementation-defined result.", CWE758, false);
}

//---------------------------------------------------------------------------
// --check-library => warn for unconfigured functions
//---------------------------------------------------------------------------

void CheckFunctions::checkLibraryMatchFunctions()
{
    const bool checkLib = mSettings->checkLibrary;
    
    bool New = false;
    for (const Token *tok = mTokenizer->tokens(); tok; tok = tok->next()) {
        if (!tok->scope() || !tok->scope()->isExecutable())
            continue;

        if (tok->str() == "new")
            New = true;
        else if (tok->str() == ";")
            New = false;
        else if (New)
            continue;

        if (!Token::Match(tok, "%name% (") || Token::Match(tok, "asm|catch"))
            continue;

        if (tok->varId() != 0 || tok->type() || tok->isStandardType() || tok->isControlFlowKeyword())
            continue;

        if (tok->linkAt(1)->strAt(1) == "(")
            continue;

        /*if (!mSettings->library.isNotLibraryFunction(tok))
            continue;*/

        const std::string &functionName = mSettings->library.getFunctionName(tok);

        if (functionName == "")
          continue;

        if (mSettings->library.functions.find(functionName) == mSettings->library.functions.cend())
        {
          if (checkLib)
            checkLibraryMatchFunctionsError(tok, functionName);

          continue;
        }

        if (!mSettings->library.matchArguments(tok, functionName))
        {
          checkLibraryMatchFunctionsArgCountError(tok, functionName);
            continue;
        }

    }
}


//---------------------------------------------------------------------------------------------------------------------------------------
void CheckFunctions::checkLibraryMatchFunctionsError(const Token *tok, std::string functionName)
{
  reportError(tok,
              Severity::information,
              "checkLibraryFunction",
              "--check-library: There is no matching configuration for function " + functionName + "()",
              CWE628,
              false
  );
}

//---------------------------------------------------------------------------------------------------------------------------------------
void CheckFunctions::checkLibraryMatchFunctionsArgCountError(const Token *tok, std::string functionName)
{
  reportError(tok,
              Severity::warning,
              "checkLibraryFunctionArgCount",
              "The function has invalid count of arguments: " + functionName + "()",
              CWE628,
              false
  );
}



//---------------------------------------------------------------------------
static bool isNonReferenceArg(const Token *tok)
{
    const Variable *var = tok->variable();
    return (var && var->isArgument() && !var->isReference() && (var->valueType()->type >= ValueType::Type::VOID || var->type()));
}


//---------------------------------------------------------------------------
static bool variableIsUsedInScope(const Token *start, unsigned int varId, const Scope *scope)
{
    if (!start) // Ticket #5024
        return false;

    for (const Token *tok = start; tok && tok != scope->bodyEnd; tok = tok->next())
    {
        // std::cout << tok->str() << " " << tok->varId() << " " << varId << std::endl;
        if (tok->varId() == varId)
            return true;
        const Scope::ScopeType scopeType = tok->scope()->type;
        // @todo In case of loops, better checking would be necessary
        if (scopeType == Scope::eFor || scopeType == Scope::eDo || scopeType == Scope::eWhile)
            return true;
    }
    return false;
}

//---------------------------------------------------------------------------
void CheckFunctions::assignFunctionArg()
{
    const bool printStyle = mSettings->isEnabled(Settings::WARNING);
    if (false == printStyle)
        return;

    const SymbolDatabase *symbolDatabase = mTokenizer->getSymbolDatabase();
    for (const Scope *scope : symbolDatabase->functionScopes)
    {
        for (const Token *tok = scope->bodyStart; tok && tok != scope->bodyEnd; tok = tok->next())
        {
            // TODO: What happens if this is removed?
            if (tok->astParent())
                continue;
            if (!(tok->isAssignmentOp() || Token::Match(tok, "++|--")) || !Token::Match(tok->astOperand1(), "%var%"))
                continue;
            const Token *const vartok = tok->astOperand1();
            if (isNonReferenceArg(vartok) &&
                !Token::Match(vartok->next(), "= %varid% ;", vartok->varId()) &&
                !variableIsUsedInScope(Token::findsimplematch(vartok->next(), ";"), vartok->varId(), scope) &&
                !Token::findsimplematch(vartok, "goto", scope->bodyEnd))
            {
                errorUselessAssignmentArg(vartok);
            }
        }
    }
}

//---------------------------------------------------------------------------
void CheckFunctions::errorUselessAssignmentArg(const Token *tok)
{
    std::string expr = tok ? tok->str() : "param";
    reportError(tok,
                Severity::warning,
                "uselessAssignmentArg",
                "$symbol:" + expr + "\n" +
                "Assignment of function parameter '$symbol' has no effect outside the function.",
                CWE398, false);
}
