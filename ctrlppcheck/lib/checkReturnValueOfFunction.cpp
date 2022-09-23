

//---------------------------------------------------------------------------
#include "checkReturnValueOfFunction.h"

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
namespace
{
CheckReturnValueOfFunction instance;
}

//---------------------------------------------------------------------------
// CWEs

/// Incorrect Type Conversion or Cast
static const CWE CWE704(704U);

//---------------------------------------------------------------------------
/*
 * check return value matching.
 * + void function return value
 *   void f() { return 0; }
 * + non-void function does not return value
 *   int f() { return; }
 * + return value does not match
 *   bool f() { return makeDynString(); }
 */
void CheckReturnValueOfFunction::returnValueMatch(void)
{
    const bool prioWarning = mSettings->isEnabled(Settings::WARNING);

    const SymbolDatabase *const symbolDatabase = mTokenizer->getSymbolDatabase();

    for (const Scope *scope : symbolDatabase->functionScopes)
    {
        if (!scope->function)
        {
            // defensive, the function has no scope
            continue;
        }

        const Token *retDef = scope->function->retDef;

        if (!retDef)
        {
            // defensive, can not find declaration type
            continue;
        }

        const std::string declType = retDef->str();
        // there is a bug #165
        // function without exact return value (or void), returns the function name as type,
        // means main(int i = 0;)  -> return vale type == 'main'
        // keep this work arround till the bug is not fixed
        const bool isVoid = declType == "void";
        const bool hasNoReturnType = declType == scope->function->name();
        bool hasReturn = false;

        for (const Token *tok = scope->bodyStart->next(); tok && (tok != scope->bodyEnd); tok = tok->next())
        {
            if (!Token::simpleMatch(tok, "return"))
            {
                continue;
            }

            // void functon returning value
            // void f() { return 0; }
            if ((isVoid || hasNoReturnType) && tok->astOperand1())
            {
                hasReturn = true;
                voidFunctionReturnValueError(tok);
                continue;
            }

            if (!isVoid && !hasNoReturnType)
            {
                // non void function does not return value
                // int f() { return; }
                if (!tok->astOperand1())
                {
                    missingFunctionReturnValueError(tok, declType);
                    continue;
                }

                // check return value match
                // bool f() { return makeDynString(); }
                const std::string retType = (tok->astOperand1()->valueType()) ? tok->astOperand1()->valueType()->str() : "";
                const ValueType *retValueType = tok->astOperand1()->valueType();
                if (retType == "")
                {
                    // Sometimes are the value types empty.
                    // it means we can not recognize the return value as well.
                    // just skip over and dont warn user about our mistake

                    /// @todo throw some debug here.
                    continue;
                }
                if (prioWarning &&
                    (declType != retType) &&
                    !retValueType->canCastTo(declType) &&
                    mSettings->inconclusive)
                {
                    returnValueNotMatchError(tok, retType, declType);
                }
            }
        }

        if (!hasReturn && hasNoReturnType)
        {
            voidReturnValueMissing(retDef);
        }
    }
}

//---------------------------------------------------------------------------
/// @todo set severity to error in case of not valid casting
void CheckReturnValueOfFunction::returnValueNotMatchError(const Token *tok, const std::string &retValType, const std::string &declType)
{
    reportError(tok,
                Severity::warning,
                "returnValueNotMatchError",
                "Return value '" + retValType + "' does not match with declaration '" + declType + "'.",
                CWE704,
                true);
}

//---------------------------------------------------------------------------
void CheckReturnValueOfFunction::voidFunctionReturnValueError(const Token *tok)
{
    reportError(tok,
                Severity::error,
                "voidFunctionReturnValueError",
                "The function declared as void returns value.\nFunction declared as void can not returns value.",
                CWE704,
                false);
}

//---------------------------------------------------------------------------
void CheckReturnValueOfFunction::missingFunctionReturnValueError(const Token *tok, const std::string &declType)
{
    reportError(tok,
                Severity::error,
                "missingFunctionReturnValueError",
                "Missing return value in non void function.\nThe function must returns value of type '" + declType + "'.",
                CWE704,
                false);
}

//---------------------------------------------------------------------------
void CheckReturnValueOfFunction::voidReturnValueMissing(const Token *tok)
{
    reportError(tok,
                Severity::information,
                "voidReturnValueMissingInformation",
                "Void function without void return type.",
                CWE704,
                false);
}
