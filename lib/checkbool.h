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
#ifndef checkboolH
#define checkboolH
//---------------------------------------------------------------------------

#include "check.h"
#include "config.h"

#include <string>

class ErrorLogger;
class Settings;
class Token;
class Tokenizer;

/// @addtogroup Checks
/// @{


/** @brief checks dealing with suspicious usage of boolean type (not for evaluating conditions) */

class CPPCHECKLIB CheckBool : public Check {
public:
    /** @brief This constructor is used when registering the CheckClass */
    CheckBool() : Check(myName()) {
    }

    /** @brief This constructor is used when running checks. */
    CheckBool(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger)
        : Check(myName(), tokenizer, settings, errorLogger) {
    }

    /** @brief Run checks against the normal token list */
    void runChecks(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger) OVERRIDE {
        CheckBool checkBool(tokenizer, settings, errorLogger);

        // Checks
        checkBool.checkComparisonOfBoolExpressionWithInt();
        checkBool.divideBoolean();
        checkBool.returnValueOfFunctionReturningBool();
        checkBool.checkComparisonOfBoolWithRelationOperator();
        checkBool.checkIncrementBoolean();
        checkBool.checkBitwiseOnBoolean();
    }

    /** @brief Run checks against the simplified token list */
    void runSimplifiedChecks(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger) OVERRIDE {
        (void)tokenizer;
        (void)settings;
        (void)errorLogger;
    }

    /** @brief %Check for comparison of variable of type bool*/
    void checkComparisonOfBoolWithRelationOperator();

    /** @brief %Check for using postfix increment on bool */
    void checkIncrementBoolean();

    /** @brief %Check for multiplictation or dividation on bool */
    void divideBoolean();

    /** @brief %Check for using bool in bitwise expression */
    void checkBitwiseOnBoolean();

    /** @brief %Check for comparing a bool expression with an integer other than 0 or 1 */
    void checkComparisonOfBoolExpressionWithInt();

    /** @brief %Check if a function returning bool returns an integer other than 0 or 1 */
    void returnValueOfFunctionReturningBool();

private:
    // Error messages..
    void comparisonOfBoolWithRelationOperatorError(const Token *tok, const std::string &expression);
    void incrementBooleanError(const Token *tok, const std::string &expression, const std::string &op);
    void divideBooleanError(const Token *tok, const std::string &expression);
    void bitwiseOnBooleanError(const Token *tok, const std::string &varname, const Token *op);
    void comparisonOfBoolExpressionWithIntError(const Token *tok, bool n0o1);
    void returnValueBoolError(const Token *tok);

    void getErrorMessages(ErrorLogger *errorLogger, const Settings *settings) const OVERRIDE {
        CheckBool c(nullptr, settings, errorLogger);

        c.comparisonOfBoolWithRelationOperatorError(nullptr, "var_name");
        c.incrementBooleanError(nullptr, "var_name", "++");
        c.divideBooleanError(nullptr, "var_name");
        c.bitwiseOnBooleanError(nullptr, "varname", nullptr);
        c.comparisonOfBoolExpressionWithIntError(nullptr, true);
        c.returnValueBoolError(nullptr);
    }

    static std::string myName() {
        return "Boolean";
    }

    std::string classInfo() const OVERRIDE {
        return "Boolean type checks\n"
               "- using increment on boolean\n"
               "- comparison of a boolean expression with an integer other than 0 or 1\n"
               "- comparison of a function returning boolean value using relational operator\n"
               "- comparison of a boolean value with boolean value using relational operator\n"
               "- using bool in bitwise expression\n"
               "- Returning an integer other than 0 or 1 from a function with boolean return value\n";
    }

    bool isBool(const Token *tok, std::string &expression);
};
/// @}
//---------------------------------------------------------------------------
#endif // checkboolH
