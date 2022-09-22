

//---------------------------------------------------------------------------
#ifndef checkreturnvalueoffunctionH
#define checkreturnvalueoffunctionH
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

class CPPCHECKLIB CheckReturnValueOfFunction : public Check {
public:
    /** @brief This constructor is used when registering the CheckClass */
    CheckReturnValueOfFunction() : Check(myName()) {
    }

    /** @brief This constructor is used when running checks. */
    CheckReturnValueOfFunction(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger)
        : Check(myName(), tokenizer, settings, errorLogger) {
    }

    /** @brief Run checks against the normal token list */
    void runChecks(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger) OVERRIDE {
        CheckReturnValueOfFunction checkBool(tokenizer, settings, errorLogger);

        // Checks
        checkBool.returnValueMatch();
    }

    /** @brief Run checks against the simplified token list */
    void runSimplifiedChecks(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger) OVERRIDE {
        (void)tokenizer;
        (void)settings;
        (void)errorLogger;
    }

    /** @brief %Check if a function return values mathces with declaration */
    void returnValueMatch();

private:
    // Error messages..
    void returnValueNotMatchError(const Token *tok, const std::string &retValType, const std::string &declType);
    void voidFunctionReturnValueError(const Token *tok);
    void missingFunctionReturnValueError(const Token *tok, const std::string &declType);
    void voidReturnValueMissing(const Token *tok);

    void getErrorMessages(ErrorLogger *errorLogger, const Settings *settings) const OVERRIDE {
        CheckReturnValueOfFunction c(nullptr, settings, errorLogger);

        c.returnValueNotMatchError(nullptr, "float", "int");
        c.voidFunctionReturnValueError(nullptr);
        c.missingFunctionReturnValueError(nullptr, "int");
        c.voidReturnValueMissing(nullptr);
    }

    static std::string myName() {
        return "FunctionReturnValue";
    }

    std::string classInfo() const OVERRIDE {
        return "Function return type checks\n"
               "- Returning an value type from a function with return value does not match\n"
               "- Void function returning value\n"
               "- Missing return value in non void function\n"
               "- Missing void in void function\n";
    }
};
/// @}
//---------------------------------------------------------------------------
#endif // checkreturnvalueoffunctionH
