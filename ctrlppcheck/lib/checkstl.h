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
#ifndef checkstlH
#define checkstlH
//---------------------------------------------------------------------------

#include "check.h"
#include "config.h"
#include "library.h"
#include "tokenize.h"

#include <map>
#include <string>

class ErrorLogger;
class Scope;
class Settings;
class Token;
class Variable;


/// @addtogroup Checks
/// @{


/** @brief %Check STL usage (invalidation of iterators, mismatching containers, etc) */
class CPPCHECKLIB CheckStl : public Check {
public:
    /** This constructor is used when registering the CheckClass */
    CheckStl() : Check(myName()) {
    }

    /** This constructor is used when running checks. */
    CheckStl(const Tokenizer* tokenizer, const Settings* settings, ErrorLogger* errorLogger)
        : Check(myName(), tokenizer, settings, errorLogger) {
    }

    /** run checks, the token list is not simplified */
    virtual void runChecks(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger) OVERRIDE {

    }

    /** Simplified checks. The token list is simplified. */
    void runSimplifiedChecks(const Tokenizer* tokenizer, const Settings* settings, ErrorLogger* errorLogger) OVERRIDE {

        CheckStl checkStl(tokenizer, settings, errorLogger);
        checkStl.negativeIndex();
    }



    /**
     * negative index for array like containers
     */
    void negativeIndex();

private:

    void negativeIndexError(const Token* tok, const ValueFlow::Value& index);

    void getErrorMessages(ErrorLogger* errorLogger, const Settings* settings) const OVERRIDE {
        CheckStl c(nullptr, settings, errorLogger);
        c.negativeIndexError(nullptr, ValueFlow::Value(-1));
    }

    static std::string myName() {
        return "STL usage";
    }

    std::string classInfo() const OVERRIDE {
        return "Check for invalid usage of STL:\n"
               "- out of bounds errors\n"
               "- negative index errors\n";
    }
};
/// @}
//---------------------------------------------------------------------------
#endif // checkstlH
