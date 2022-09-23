#include "checknaming.h"
#include "errorlogger.h"
#include "platform.h"
#include "symbols/symbols.h"
#include "token.h"
#include "tokenize.h"
#include <tinyxml2.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <limits.h>
#endif

#include <algorithm> // std::find_if
#include <cstddef>
#include <list>
#include <ostream>
#include <stack>
//-----------------------------------------------------------------------------

// Register this check class (by creating a static instance of it)
namespace
{
CheckNaming instance;
}

// CWE ids used:
static const struct CWE CWE1099(1099U); // Integer Overflow or Wraparound

//-----------------------------------------------------------------------------
// @private
//-----------------------------------------------------------------------------

std::string getExePath()
{
    std::string path = "Failed to get path";

    #ifdef _WIN32
    char selfp[MAX_PATH];
    DWORD szPath;
    szPath = GetModuleFileNameA(NULL, selfp, MAX_PATH);
    if (szPath != 0) // successfully got path of current program
    {
        // helper string to make life much, much easier
        std::string helper = selfp;
        //find last backslash in current program path
        size_t pos = helper.find_last_of( "\\" );

        if (pos != std::string::npos) // found last backslash
        {
            // remove everything after last backslash. This should remove
            // the current program's name.
            path = helper.substr( 0, pos+1);
        }
    }
    #else
    char buf[PATH_MAX];
    ssize_t readSize = ::readlink("/proc/self/exe", buf, sizeof(buf)-1);
    if (readSize != -1) {
        buf[readSize] = '\0';

        std::string helper = buf;
        size_t pos = helper.find_last_of( "/" );
        if (pos != std::string::npos)
        {
            path = helper.substr( 0, pos+1);
        }
    }
    #endif

    return path;
}

//-----------------------------------------------------------------------------
// @public
//-----------------------------------------------------------------------------
void CheckNaming::checkVariableNaming()
{
    if (!mSettings->isEnabled(Settings::STYLE))
        return;

    const SymbolDatabase *symbolDatabase = mTokenizer->getSymbolDatabase();
    for (const Variable *var : symbolDatabase->variableList())
    {
        if (!var)
        {
            continue;
        }

        mFileName = mTokenizer->list.file(var->nameToken());

        //get complete rule id
        std::list<std::string> variableFlags = getVariableFlags(var);
        std::string ruleId = getRuleId(variableFlags);

        //search if there is an rule for it (cached)
        const Settings::Rule *rule = getRule(ruleId);

        if (rule && !rule->pattern.empty())
        {
            //if yes: than check name with preprocessed regex
            if (!std::regex_match(var->name(), preProcessRegex(rule->pattern.c_str())))
            {
                namingError(var->nameToken(), rule);
            }
        }
        else
        {
            //if no:
            //get Rule for each single rule id (cached)
            std::list<const Settings::Rule *> rules;

            for (std::string id : variableFlags)
            {
                const Settings::Rule *rule = getRule(id);

                if (rule && !rule->pattern.empty())
                {
                    //check each rule with the name
                    if (!std::regex_match(var->name(), preProcessRegex(rule->pattern.c_str())))
                    {
                        //report each error
                        rules.push_back(rule);
                        //namingError(var->nameToken(), rule);
                    }
                }
            }

            if (!rules.empty())
            {
                namingErrors(var->nameToken(), rules);
            }
        }
    }
}

void CheckNaming::checkFunctionNaming()
{
    /*if (!mSettings->isEnabled(Settings::STYLE))
        return;

    const SymbolDatabase *symbolDatabase = mTokenizer->getSymbolDatabase();
    for (const Scope *scope : symbolDatabase->functionScopes)
    {
        if (!scope)
        {
            continue;
        }

        const Function *function = scope->function;

        if (!function || function->name() == "main")
        {
            continue;
        }

        mFileName = mTokenizer->list.file(function->token);

        //get complete rule id
        std::list<std::string> functionFlags = getFunctionFlags(function);
        std::string ruleId = getRuleId(functionFlags);

        //search if there is an rule for it (cached)
        const Settings::Rule *rule = getRule(ruleId);

        if (rule && !rule->pattern.empty())
        {
            //if yes: than check name with preprocessed regex
            if (!std::regex_match(function->name(), preProcessRegex(rule->pattern.c_str())))
            {
                namingError(function->token, rule);
            }
        }
        else
        {
            //if no:
            //get Rule for each single rule id (cached)
            std::list<const Settings::Rule *> rules;

            for (std::string id : functionFlags)
            {
                const Settings::Rule *rule = getRule(id);

                if (rule && !rule->pattern.empty())
                {
                    //check each rule with the name
                    if (!std::regex_match(function->name(), preProcessRegex(rule->pattern.c_str())))
                    {
                        //report each error
                        rules.push_back(rule);
                        //namingError(var->nameToken(), rule);
                    }
                }
            }

            if (!rules.empty())
            {
                namingErrors(function->token, rules);
            }
        }
    }*/
}

void CheckNaming::checkClassNaming()
{
    /*if (!mSettings->isEnabled(Settings::STYLE))
        return;

    const SymbolDatabase *symbolDatabase = mTokenizer->getSymbolDatabase();
    for (const Scope *scope : symbolDatabase->classAndStructScopes)
    {
        if (!scope)
        {
            continue;
        }

        mFileName = mTokenizer->list.getOrigFile(scope->classDef);

        const Settings::Rule *rule = getRule("class");

        if (rule && !rule->pattern.empty())
        {
            if (!std::regex_match(scope->className, preProcessRegex(rule->pattern.c_str())))
            {
                namingError(scope->classDef, rule);
            }
        }
    }*/
}

//-----------------------------------------------------------------------------
// @private
//-----------------------------------------------------------------------------
const std::vector<std::string> splitString(const std::string &s) {
    std::regex regex{R"([\s]+)"}; // split on space
    std::sregex_token_iterator it{s.begin(), s.end(), regex, -1};
    std::vector<std::string> words{it, {}};

    return words;
}

const Settings::Rule *CheckNaming::getRule(const std::string &ruleId)
{
    std::list<Settings::Rule>::const_iterator it = std::find_if(mRules.begin(), mRules.end(),
                                                                [ruleId](const Settings::Rule &rule) {
                                                                        std::vector<std::string> ruleParts = splitString(rule.id);
                                                                        std::vector<std::string> ruleIdParts = splitString(ruleId);

                                                                        if ( ruleParts.size() != ruleIdParts.size() ) {
                                                                            return false;
                                                                        }

                                                                        for (int i = 0; i < ruleIdParts.size(); i++) // access by reference to avoid copying
                                                                        {
                                                                            if ( ruleIdParts[i] != ruleParts[i] && ruleParts[i] != "*" )
                                                                            {
                                                                                return false;
                                                                            }
                                                                        }

                                                                        return true;
                                                                    });
    if (it == mRules.end())
    {
        return nullptr;
    }

    return &*it;
}

std::regex CheckNaming::preProcessRegex(const std::string &origRegexString)
{
    std::string newRegexString = origRegexString;
    std::string fileName = mFileName.substr(0, mFileName.find_last_of("."));

    //replace %fileName% in regex with actual filename without extension
    replaceAll(newRegexString, "%fileName%", fileName);

    std::transform(fileName.begin(), fileName.end(), fileName.begin(), ::toupper);
    replaceAll(newRegexString, "%fileName_allUpper%", fileName);

    std::transform(fileName.begin(), fileName.end(), fileName.begin(), ::tolower);
    replaceAll(newRegexString, "%fileName_allLower%", fileName);

    return std::regex(newRegexString);
}

void CheckNaming::replaceAll(std::string &str, const std::string &from, const std::string &to)
{
    if (from.empty())
        return;
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos)
    {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
    }
}

std::list<std::string> CheckNaming::getVariableFlags(const Variable *var)
{

    std::list<std::string> variableFlags;

    //const
    if (var->isConst())
    {
        variableFlags.push_back("const");
    }
    else
    {
        variableFlags.push_back("nonconst");
    }

    //int | float | long | bool | char
    if (var->valueType())
    {
        variableFlags.push_back(var->valueType()->typeToString());
    }

    //local | argument | global
    if (var->isLocal())
    {
        variableFlags.push_back("local");
    }
    else if (var->isArgument())
    {
        variableFlags.push_back("argument");
    }
    else if (var->isGlobal()) //isGlobal does not differ between manager global and script global
    {
        variableFlags.push_back("global");
    }

    return variableFlags;
}

std::list<std::string> CheckNaming::getFunctionFlags(const Function *function)
{

    std::list<std::string> functionFlags;

    if (function->isConstructor())
    {
        functionFlags.push_back("ctor");
    }
    else if (function->isDestructor())
    {
        functionFlags.push_back("dtor");
    }
    else
    {
        //const
        if (function->isConst())
        {
            functionFlags.push_back("const");
        }
        else
        {
            functionFlags.push_back("nonconst");
        }

        if (function->isStatic())
        {
            functionFlags.push_back("static");
        }
        else if (function->isStaticLocal())
        {
            functionFlags.push_back("static_local");
        }

        if (function->hasBody())
        {
            functionFlags.push_back("function");
        }
        //class method | normal
    }

    return functionFlags;
}

std::string CheckNaming::getRuleId(const std::list<std::string> &variableFlags)
{
    std::string path;

    for (std::string s : variableFlags)
    {
        path += (path.empty() ? "" : " ") + s;
    }

    return path;
}

void CheckNaming::loadNamingRules()
{
    const std::string rulesPath_dev = getExePath() + "../../../WinCCOA_QualityChecks/data/ctrlPpCheck/rule/variableNaming.xml";
    const std::string rulesPath_live = getExePath() + "../../data/ctrlPpCheck/rule/variableNaming.xml";
    std::string rulesPath = doesFileExist(rulesPath_live) ? rulesPath_live : rulesPath_dev;

    if (!mSettings->namingRuleFile.empty() && doesFileExist(mSettings->namingRuleFile))
    {
        std::cout << "cppcheck: naming check: custom rule file path =  " + mSettings->namingRuleFile << std::endl;
        rulesPath = mSettings->namingRuleFile;
    }
    else
    {
        std::cout << "cppcheck: warning: custom naming rule file is not specified or does not exist" << std::endl;

        if (doesFileExist(rulesPath_live))
        {
            std::cout << "cppcheck: naming check: standard rule file path =  " + rulesPath_live << std::endl;
            rulesPath = rulesPath_live;
        }
        else if (doesFileExist(rulesPath_dev))
        {
            std::cout << "cppcheck: naming check: standard rule file path =  " + rulesPath_dev << std::endl;
            rulesPath = rulesPath_dev;
        }
        else
        {
            std::cout << "cppcheck: warning: standard naming rule file does not exist" << std::endl;
            return;
        }
    }

    tinyxml2::XMLDocument doc;
    std::list<std::string> ids;
    if (doc.LoadFile(rulesPath.c_str()) == tinyxml2::XML_SUCCESS)
    {
        tinyxml2::XMLElement *node = doc.FirstChildElement();
        for (; node && strcmp(node->Value(), "rule") == 0; node = node->NextSiblingElement())
        {
            Settings::Rule rule;

            //tinyxml2::XMLElement *tokenlist = node->FirstChildElement("tokenlist");
            //xmlGetText(tokenlist, rule.tokenlist);

            tinyxml2::XMLElement *pattern = node->FirstChildElement("pattern");
            rule.pattern = "";
            xmlGetText(pattern, rule.pattern);

            tinyxml2::XMLElement *message = node->FirstChildElement("message");
            if (message)
            {
                tinyxml2::XMLElement *id = message->FirstChildElement("id");
                rule.id = "";
                xmlGetText(id, rule.id);

                tinyxml2::XMLElement *summary = message->FirstChildElement("summary");
                xmlGetText(summary, rule.summary);
            }

            if (!rule.pattern.empty())
            {
                if (std::find(ids.begin(), ids.end(), rule.id) == ids.end())
                {
                    ids.push_back(rule.id);
                    mRules.push_back(rule);
                }
                else
                {
                    std::cout << "cppcheck: warning: The following rule ID occurs several times: " +
                                     rule.id +
                                     ". Only the first rule is used for the check"
                              << std::endl;
                }
            }
        }
    }
    else
    {
        std::cout << "cppcheck: error: unable to load rule-file: " + rulesPath << std::endl;
        return;
    }
}

bool CheckNaming::doesFileExist(const std::string &filePath)
{
    struct stat buffer;
    return (stat(filePath.c_str(), &buffer) == 0);
}

void CheckNaming::xmlGetText(tinyxml2::XMLElement *element, std::string &value)
{
    if (element)
    {
        const char *txt = element->GetText();
        if (txt)
            value = txt;
    }
}

//-----------------------------------------------------------------------------
// Error messages
//-----------------------------------------------------------------------------

void CheckNaming::namingError(const Token *tok, const Settings::Rule *rule)
{
    std::string expr = tok ? tok->str() : "name";
    ErrorPath errorPath;
    errorPath.push_back(ErrorPathItem(tok, rule->id + " - " + rule->summary));
    const char *id = "namingError";
    std::string message = "$symbol:" + expr + "\n" +
                          "The name '$symbol' does not match the following rule: " + rule->id;
    reportError(errorPath, Severity::style, id, message, CWE1099, false);
}

void CheckNaming::namingErrors(const Token *tok, const std::list<const Settings::Rule *> &rules)
{
    ErrorPath errorPath;
    std::string message;

    for (const Settings::Rule *rule : rules)
    {
        errorPath.push_back(ErrorPathItem(tok, rule->id + " - " + rule->summary));
        message += (message.empty() ? "" : " | ") + rule->id;
    }

    message = "The name " + tok->str() + " does not match the following rule(s): " + message;

    const char *id = "namingError";
    reportError(errorPath, Severity::style, id, message, CWE1099, false);
}
