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


//---------------------------------------------------------------------------
#ifndef checkotherH
#define checkotherH
//---------------------------------------------------------------------------

#include "check.h"
#include "config.h"
#include "valueflow.h"

#include <cstddef>
#include <string>
#include <vector>

class ErrorLogger;
class Settings;
class Token;
class Tokenizer;
class Variable;

/// @addtogroup Checks
/// @{


/** @brief Various small checks */

class CPPCHECKLIB CheckOther : public Check {
public:
    /** @brief This constructor is used when registering the CheckClass */
    CheckOther() : Check(myName()) {
    }

    /** @brief This constructor is used when running checks. */
    CheckOther(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger)
        : Check(myName(), tokenizer, settings, errorLogger) {
    }

    /** @brief Run checks against the normal token list */
    void runChecks(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger) OVERRIDE {
        CheckOther checkOther(tokenizer, settings, errorLogger);

        // Checks
        checkOther.checkRedundantAssignment();
        checkOther.checkRedundantAssignmentInSwitch();
        checkOther.checkSuspiciousCaseInSwitch();
        checkOther.checkDuplicateBranch();
        checkOther.checkDuplicateExpression();
        checkOther.checkUnreachableCode();
        checkOther.checkSuspiciousSemicolon();
        checkOther.checkPerformanceInLoops();
        checkOther.checkEmptyScope();
        checkOther.checkVariableScope();
        //checkOther.checkSignOfUnsignedVariable();  // don't ignore casts (#3574)
        checkOther.checkVarFuncNullUB();
        checkOther.checkNanInArithmeticExpression();
        checkOther.checkCommaSeparatedReturn();
        checkOther.checkZeroDivision();
        checkOther.checkNegativeBitwiseShift();
        checkOther.checkUnusedLabel();
        checkOther.checkEvaluationOrder();
        checkOther.checkShadowVariables();
        checkOther.checkConstArgument();
        checkOther.checkIncompleteStatement();
        checkOther.clarifyCalculation();
        checkOther.checkPassByReference();
        checkOther.checkUndeclaredVar();
    }

    /** @brief Run checks against the simplified token list */
    void runSimplifiedChecks(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger) OVERRIDE {
        CheckOther checkOther(tokenizer, settings, errorLogger);

        // Checks
        checkOther.clarifyStatement();
        checkOther.checkMisusedScopedObject();
    }

    /** @brief Clarify calculation for ".. a * b ? .." */
    void clarifyCalculation();

    /** @brief Suspicious statement like '*A++;' */
    void clarifyStatement();

    /** @brief %Check scope of variables */
    void checkVariableScope();
    static bool checkInnerScope(const Token *tok, const Variable* var, bool& used);

    /** @brief %Check for comma separated statements in return */
    void checkCommaSeparatedReturn();

    /** @brief %Check for function parameters that should be passed by reference */
    void checkPassByReference();

    /** @brief Incomplete statement. A statement that only contains a constant or variable */
    void checkIncompleteStatement();

    /** @brief %Check zero division*/
    void checkZeroDivision();

    /** @brief Check for NaN (not-a-number) in an arithmetic expression */
    void checkNanInArithmeticExpression();

    /** @brief copying to memory or assigning to a variable twice */
    void checkRedundantAssignment();

    /** @brief %Check for assigning to the same variable twice in a switch statement*/
    void checkRedundantAssignmentInSwitch();

    /** @brief %Check for code like 'case A||B:'*/
    void checkSuspiciousCaseInSwitch();

    /** @brief %Check for objects that are destroyed immediately */
    void checkMisusedScopedObject();

    /** @brief %Check for suspicious code where if and else branch are the same (e.g "if (a) b = true; else b = true;") */
    void checkDuplicateBranch();

    /** @brief %Check for suspicious code with the same expression on both sides of operator (e.g "if (a && a)") */
    void checkDuplicateExpression();

    /** @brief %Check for code that gets never executed, such as duplicate break statements */
    void checkUnreachableCode();

    /** @brief %Check for testing sign of unsigned variable */
    //@todo sign - activate when there is ctrl sign implementaion
    //void checkSignOfUnsignedVariable();

    /** @brief %Check for suspicious use of semicolon */
    void checkSuspiciousSemicolon();

    /** @brief %Check for not performant code in loops */
    void checkPerformanceInLoops();

    /** @brief %Check for empty scope*/
    void checkEmptyScope();

    /** @brief %Check for bitwise shift with negative right operand */
    void checkNegativeBitwiseShift();

    /** @brief %Check that variadic function calls don't use NULL. If NULL is \#defined as 0 and the function expects a pointer, the behaviour is undefined. */
    void checkVarFuncNullUB();

    /** @brief %Check for unused labels */
    void checkUnusedLabel();

    /** @brief %Check for expression that depends on order of evaluation of side effects */
    void checkEvaluationOrder();

    /** @brief %Check for shadow variables. Less noisy than gcc/clang -Wshadow. */
    void checkShadowVariables();

    void checkConstArgument();

    //---------------------------------------------------------------------------------------------------------------------------------------
    /** @brief Check for undeclared variables.
        @details This check find all undeclared variables. It is for ctrl lang relevant. C, and c++ compilers does not allow it.
     */
    void checkUndeclaredVar();
    bool isUndeclaredVar(const Token *tok);
    void checkUndeclaredVarError(const Token *tok);

private:
    // Error messages..
    void clarifyCalculationError(const Token *tok, const std::string &op);
    void clarifyStatementError(const Token* tok);
    void passedByValueError(const Token *tok, const std::string &parname, bool inconclusive);
    void constStatementError(const Token *tok, const std::string &type, bool inconclusive);
    void variableScopeError(const Token *tok, const std::string &varname);
    void zerodivError(const Token *tok, const ValueFlow::Value *value);
    void nanInArithmeticExpressionError(const Token *tok);
    void redundantAssignmentError(const Token *tok1, const Token* tok2, const std::string& var, bool inconclusive);
    void redundantAssignmentInSwitchError(const Token *tok1, const Token *tok2, const std::string &var);
    void redundantBitwiseOperationInSwitchError(const Token *tok, const std::string &varname);
    void suspiciousCaseInSwitchError(const Token* tok, const std::string& operatorString);
    void selfAssignmentError(const Token *tok, const std::string &varname);
    void misusedScopeObjectError(const Token *tok, const std::string &varname);
    void duplicateBranchError(const Token *tok1, const Token *tok2, ErrorPath errors);
    void duplicateAssignExpressionError(const Token *tok1, const Token *tok2, bool inconclusive);
    void oppositeExpressionError(const Token *opTok, ErrorPath errors);
    void duplicateExpressionError(const Token *tok1, const Token *tok2, const Token *opTok, ErrorPath errors);
    void duplicateValueTernaryError(const Token *tok);
    void duplicateExpressionTernaryError(const Token *tok, ErrorPath errors);
    void duplicateBreakError(const Token *tok, bool inconclusive);
    void unreachableCodeError(const Token* tok, bool inconclusive);
    void unsignedLessThanZeroError(const Token *tok, const ValueFlow::Value *v, const std::string &varname);
    void unsignedPositiveError(const Token *tok, const ValueFlow::Value *v, const std::string &varname);
    void SuspiciousSemicolonError(const Token *tok);
    void emptyScopeError(const Token *tok);
    void performanceInLoopsError(const Token *function);
    void negativeBitwiseShiftError(const Token *tok, int op);
    void varFuncNullUBError(const Token *tok);
    void commaSeparatedReturnError(const Token *tok);
    void unusedLabelError(const Token* tok, bool inSwitch);
    void unknownEvaluationOrder(const Token* tok);
    static bool isMovedParameterAllowedForInconclusiveFunction(const Token * tok);
    void shadowError(const Token *var, const Token *shadowed, bool shadowVar);
    void notUniqueArgNameError(const Token *var);
    void constArgumentError(const Token *tok, const Token *ftok, const ValueFlow::Value *value);
    void checkShadowVar(const Scope & scope, const Variable &var);

    void getErrorMessages(ErrorLogger *errorLogger, const Settings *settings) const OVERRIDE {
        CheckOther c(nullptr, settings, errorLogger);

        ErrorPath errorPath;

        // error
        c.zerodivError(nullptr, nullptr);
        c.misusedScopeObjectError(nullptr, "varname");
        c.negativeBitwiseShiftError(nullptr, 1);
        c.negativeBitwiseShiftError(nullptr, 2);

        //performance
        c.redundantAssignmentError(nullptr, nullptr, "var", false);

        // style/warning
        c.passedByValueError(nullptr, "parametername", false);
        c.constStatementError(nullptr, "type", false);
        c.variableScopeError(nullptr,  "varname");
        c.redundantAssignmentInSwitchError(nullptr, nullptr, "var");
        c.suspiciousCaseInSwitchError(nullptr,  "||");
        c.selfAssignmentError(nullptr,  "varname");
        c.clarifyCalculationError(nullptr,  "+");
        c.clarifyStatementError(nullptr);
        c.duplicateBranchError(nullptr, nullptr, errorPath);
        c.duplicateAssignExpressionError(nullptr, nullptr, true);
        c.oppositeExpressionError(nullptr, errorPath);
        c.duplicateExpressionError(nullptr, nullptr, nullptr, errorPath);
        c.duplicateValueTernaryError(nullptr);
        c.duplicateExpressionTernaryError(nullptr, errorPath);
        c.duplicateBreakError(nullptr,  false);
        c.unreachableCodeError(nullptr,  false);
        c.unsignedLessThanZeroError(nullptr, nullptr, "varname");
        c.unsignedPositiveError(nullptr, nullptr, "varname");
        c.SuspiciousSemicolonError(nullptr);
        c.emptyScopeError(nullptr);
        c.performanceInLoopsError(nullptr);
        c.varFuncNullUBError(nullptr);
        c.nanInArithmeticExpressionError(nullptr);
        c.commaSeparatedReturnError(nullptr);
        c.unusedLabelError(nullptr,  true);
        c.unusedLabelError(nullptr,  false);
        c.unknownEvaluationOrder(nullptr);
        c.redundantBitwiseOperationInSwitchError(nullptr, "varname");
        c.shadowError(nullptr, nullptr, false);
        c.shadowError(nullptr, nullptr, true);
        c.notUniqueArgNameError(nullptr);
        c.constArgumentError(nullptr, nullptr, nullptr);
        c.checkUndeclaredVarError(nullptr);

        const std::vector<const Token *> nullvec;
    }

    static std::string myName() {
        return "Other";
    }

    std::string classInfo() const OVERRIDE {
        return "Other checks\n"

               // error
               "- division with zero\n"
               "- scoped object destroyed immediately after construction\n"
               "- assignment in an assert statement\n"
               "- free() or delete of an invalid memory location\n"
               "- bitwise operation with negative right operand\n"
               "- provide wrong dimensioned array to pipe() system command (--std=posix)\n"
               "- cast the return values of getc(),fgetc() and getchar() to character and compare it to EOF\n"
               "- race condition with non-interlocked access after InterlockedDecrement() call\n"
               "- expression 'x = x++;' depends on order of evaluation of side effects\n"

               // warning
               "- either division by zero or useless condition\n"
               "- access of moved or forwarded variable.\n"
               "- undeclared variable.\n"

               // performance
               "- redundant data copying for const variable\n"
               "- subsequent assignment or copying to a variable or buffer\n"
               "- passing parameter by value\n"

               // style
               "- [Incomplete statement](IncompleteStatement)\n"
               "- [check how signed char variables are used](CharVar)\n"
               "- variable scope can be limited\n"
               "- unusual pointer arithmetic. For example: \"abc\" + 'd'\n"
               "- redundant assignment, increment, or bitwise operation in a switch statement\n"
               "- redundant strcpy in a switch statement\n"
               "- Suspicious case labels in switch()\n"
               "- assignment of a variable to itself\n"
               "- Comparison of values leading always to true or false\n"
               "- Clarify calculation with parentheses\n"
               "- suspicious comparison of '\\0' with a char\\* variable\n"
               "- duplicate break statement\n"
               "- unreachable code\n"
               "- testing if unsigned variable is negative/positive\n"
               "- Suspicious use of ; at the end of 'if/for/while' statement.\n"
               "- Array filled incompletely using memset/memcpy/memmove.\n"
               "- NaN (not a number) value used in arithmetic expression.\n"
               "- comma in return statement (the comma can easily be misread as a semicolon).\n"
               "- prefer erfc, expm1 or log1p to avoid loss of precision.\n"
               "- identical code in both branches of if/else or ternary operator.\n"
               "- find unused 'goto' labels.\n"
               "- function declaration and definition argument names different.\n"
               "- function declaration and definition argument order different.\n"
               "- shadow variable.\n"
               "- not unique function parameter.\n";
    }
};
/// @}
//---------------------------------------------------------------------------
#endif // checkotherH
