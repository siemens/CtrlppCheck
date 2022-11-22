

//---------------------------------------------------------------------------
#ifndef checky2038H
#define checky2038H
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


/** @brief checks dealing with suspicious usage time casting to eliminate year 2038 issues*/

class CPPCHECKLIB CheckY2038 : public Check {
public:
    /** @brief This constructor is used when registering the CheckClass */
    CheckY2038() : Check(myName()) {
    }

    /** @brief This constructor is used when running checks. */
    CheckY2038(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger)
        : Check(myName(), tokenizer, settings, errorLogger) {
    }

    /** @brief Run checks against the normal token list */
    void runChecks(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger) OVERRIDE {
        CheckY2038 checkBool(tokenizer, settings, errorLogger);

        // Checks
        checkBool.timeVarCast();
    }

    /** @brief Run checks against the simplified token list */
    void runSimplifiedChecks(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger) OVERRIDE {
        (void)tokenizer;
        (void)settings;
        (void)errorLogger;
    }

    /** @brief %Check if a function return values mathces with declaration */
    void timeVarCast();

private:
    void checkConversion(const Token *left, const Token *right);
    void timeVarCastFunction(const Token *tok);
    void timeVarCastOperands(const Token *tok); 
    void timeVarCastExplCast(const Token *tok); 
    const ValueType* getValType(const Token *right);
    std::string getVarName(const Token *tok);

    // Error messages..
    void y2038unknownTypeError(const Token *tok);
    void y2038canNotCastError(const Token *left, const Token *right, Severity::SeverityType prio, bool inconclusive);
    void y2038overflow(const Token *left, const Token *right, Severity::SeverityType prio, bool inconclusive);
    void y2038valueLost(const Token *left, const Token *right, Severity::SeverityType prio, bool inconclusive);

    void getErrorMessages(ErrorLogger *errorLogger, const Settings *settings) const OVERRIDE {
        CheckY2038 c(nullptr, settings, errorLogger);

        c.y2038unknownTypeError(nullptr);
        c.y2038canNotCastError(nullptr, nullptr, Severity::SeverityType::error, false);
        c.y2038overflow(nullptr, nullptr, Severity::SeverityType::error, false);
        c.y2038valueLost(nullptr, nullptr, Severity::SeverityType::error, false);
    }

    static std::string myName() {
        return "Y2038";
    }

    std::string classInfo() const OVERRIDE {
        return "Year 2038 checks\n"
               "- Wrong variable casting of type time in to int ...\n"
               "- Wrong variable casting into type time from int ...\n"
               "- Wrong usage of time functions like getCurrentTime(), formatTime() ...\n";
    }
};
/// @}
//---------------------------------------------------------------------------
#endif // checky2038H
