
//-----------------------------------------------------------------------------
#ifndef checknamingH
#define checknamingH
//-----------------------------------------------------------------------------

#include "check.h"
#include "settings.h"
#include <regex>

class ErrorLogger;
class Settings;
class Token;
class Tokenizer;

/// @addtogroup Checks
/// @{

/** @brief Various small checks */

class CPPCHECKLIB CheckNaming : public Check
{
public:
    /** @brief This constructor is used when registering the CheckClass */
    CheckNaming() : Check(myName())
    {
    }

    /** @brief This constructor is used when running checks. */
    CheckNaming(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger)
        : Check(myName(), tokenizer, settings, errorLogger)
    {
    }

    /** @brief Run checks against the raw token list */
    /*void runRawChecks(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger) OVERRIDE {
        CheckNaming checkNaming(tokenizer, settings, errorLogger);
    }*/

    /** @brief Run checks against the normal token list */
    void runChecks(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger) OVERRIDE
    {
        CheckNaming checkNaming(tokenizer, settings, errorLogger);

        // Checks
        checkNaming.loadNamingRules();
        checkNaming.checkVariableNaming();
        //checkNaming.checkFunctionNaming();
        //checkNaming.checkClassNaming();
    }

    /** @brief Run checks against the simplified token list */
    void runSimplifiedChecks(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger) OVERRIDE
    {
        (void)tokenizer;
        (void)settings;
        (void)errorLogger;
    }

    /** @brief %Check for variable naming */
    void checkVariableNaming();
    /** @brief %Check for function naming */
    void checkFunctionNaming();
    /** @brief %Check for class naming */
    void checkClassNaming();

private:
    std::list<Settings::Rule> mRules;
    std::string mFileName;

    void loadNamingRules();
    bool doesFileExist(const std::string &filePath);
    void xmlGetText(tinyxml2::XMLElement *element, std::string &value);
    const Settings::Rule *getRule(const std::string &ruleId);
    std::regex preProcessRegex(const std::string &origRegexString);
    void replaceAll(std::string &str, const std::string &from, const std::string &to);
    std::list<std::string> getVariableFlags(const Variable *var);
    std::list<std::string> getFunctionFlags(const Function *var);
    std::string getRuleId(const std::list<std::string> &variableFlags);

    // Error messages..
    void namingError(const Token *tok, const Settings::Rule *rule);
    void namingErrors(const Token *tok, const std::list<const Settings::Rule *> &rules);

    void getErrorMessages(ErrorLogger *errorLogger, const Settings *settings) const OVERRIDE
    {
        // CheckNaming c(nullptr, settings, errorLogger);

        // c.namingError(nullptr, &Settings::Rule());
    }

    static std::string myName()
    {
        return "Naming";
    }

    std::string classInfo() const OVERRIDE
    {
        return "Naming checks\n"
               "- if variables are named correct\n";
    }
};
/// @}
//-----------------------------------------------------------------------------
#endif // checknamingH
