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
#include "tokenize.h"

#include "check.h"
#include "library.h"
#include "mathlib.h"
#include "path.h"
#include "platform.h"
#include "settings.h"
#include "symbols/symbols.h"
#include "calculationssimplifier.h"
#include "timer.h"
#include "token.h"
#include "utils.h"
#include "valueflow.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstring>
#include <ctime>
#include <iostream>
#include <stack>
#include <unordered_map>
#include <utility>
#include <vector>
#include <symbols/symboldatabase.h>
//---------------------------------------------------------------------------

namespace {
    // local struct used in setVarId
    // in order to store information about the scope
    struct VarIdScopeInfo {
        VarIdScopeInfo()
            :isExecutable(false), isStructInit(false), isEnum(false), startVarid(0) {
        }
        VarIdScopeInfo(bool _isExecutable, bool _isStructInit, bool _isEnum, unsigned int _startVarid)
            :isExecutable(_isExecutable), isStructInit(_isStructInit), isEnum(_isEnum), startVarid(_startVarid) {
        }

        const bool isExecutable;
        const bool isStructInit;
        const bool isEnum;
        const unsigned int startVarid;
    };
}

/** Return whether tok is the "{" that starts an enumerator list */
static bool isEnumStart(const Token* tok)
{
    if (!tok || tok->str() != "{")
        return false;
    return (tok->strAt(-1) == "enum") || (tok->strAt(-2) == "enum");
}

template<typename T>
static void skipEnumBody(T **tok)
{
    T *defStart = *tok;
    while (Token::Match(defStart, "%name%|::|:"))
        defStart = defStart->next();
    if (defStart && defStart->str() == "{")
        *tok = defStart->link()->next();
}

const Token * Tokenizer::isFunctionHead(const Token *tok, const std::string &endsWith) const
{
    return Tokenizer::isFunctionHead_static(tok, endsWith);
}

const Token * Tokenizer::isFunctionHead_static(const Token *tok, const std::string &endsWith)
{
    if (!tok)
        return nullptr;
    if (tok->str() == "(")
        tok = tok->link();
    if (Token::Match(tok, ") ;|{|[")) {
        tok = tok->next();
        while (tok && tok->str() == "[" && tok->link())
            tok = tok->link()->next();
        return (tok && endsWith.find(tok->str()) != std::string::npos) ? tok : nullptr;
    }
    if (tok->str() == ")") {
        tok = tok->next();
        while (Token::Match(tok, "const|noexcept|override|final|&|&& !!(") ||
               (Token::Match(tok, "%name% !!(") && tok->isUpperCaseName()))
            tok = tok->next();
        if (tok && tok->str() == ")")
            tok = tok->next();
        while (tok && tok->str() == "[")
            tok = tok->link()->next();
        if (Token::Match(tok, "throw|noexcept ("))
            tok = tok->linkAt(1)->next();
        if (Token::Match(tok, "%name% (") && tok->isUpperCaseName())
            tok = tok->linkAt(1)->next();
        if (tok && tok->str() == ".") { // trailing return type
            for (tok = tok->next(); tok && !Token::Match(tok, ";|{|override|final"); tok = tok->next())
                if (tok->link() && Token::Match(tok, "<|[|("))
                    tok = tok->link();
        }
        while (Token::Match(tok, "override|final !!(") ||
               (Token::Match(tok, "%name% !!(") && tok->isUpperCaseName()))
            tok = tok->next();
        if (Token::Match(tok, "= 0|default|delete ;"))
            tok = tok->tokAt(2);

        return (tok && endsWith.find(tok->str()) != std::string::npos) ? tok : nullptr;
    }
    return nullptr;
}

/**
 * is tok the start brace { of a class, struct, or enum
 */
static bool isClassStructEnumStart(const Token * tok)
{
    if (!Token::Match(tok->previous(), "class|struct|enum|%name%|>|>> {"))
        return false;
    const Token * tok2 = tok->previous();
    while (tok2 && !Token::Match(tok2, "class|struct|enum|{|}|;"))
        tok2 = tok2->previous();
    return Token::Match(tok2, "class|struct|enum");
}


//---------------------------------------------------------------------------
void Tokenizer::syntaxError(const Token *tok) const
{
  printDebugOutput(0);
  throw InternalError(tok, "Syntax error", InternalError::SYNTAX);
}

//---------------------------------------------------------------------------
void Tokenizer::syntaxError(const Token *tok, const std::string &detail) const
{
  printDebugOutput(0);
  throw InternalError(tok, "Syntax error: " + detail, InternalError::SYNTAX);
}

// define thie var to get more intern output in stack trace
//#define __cppcheckLocations__ = true
//---------------------------------------------------------------------------
static void __syntaxErrorDetail__(const Token *tok, const std::string &file, int line, const std::string &function)
{
    //printDebugOutput(0);
    #if ( defined __cppcheckLocations__ )
    std::cout << "<debug syntaxError>" << std::endl;
    std::cout << Path::stripDirectoryPart(file) << ":" << MathLib::toString(line) << ":" << function  << std::endl;

    Token::toStdOut(tok);
    if (tok != nullptr)
    {
        std::cout << "previous" << std::endl;
        Token::toStdOut(tok->previous());
        std::cout << "next" << std::endl;
        Token::toStdOut(tok->next());
    }
    #endif
  throw InternalError(tok, "Syntax error", InternalError::SYNTAX);
}

//---------------------------------------------------------------------------
void Tokenizer::syntaxErrorDetailMsg(const Token *tok, std::string detail)
{
  //printDebugOutput(0);
  if ( detail.length() > 0 )
    detail[0] = toupper(detail[0]);

  throw InternalError(tok, "Syntax error: " + detail, InternalError::SYNTAX);
}



#if (defined __cplusplus) && __cplusplus >= 201103L
#define syntaxErrorDetail(tok) __syntaxErrorDetail__(tok, __FILE__, __LINE__, __func__)
#elif (defined __GNUC__) || (defined __clang__) || (defined _MSC_VER)
#define syntaxErrorDetail(tok) __syntaxErrorDetail__(tok, __FILE__, __LINE__, __FUNCTION__)
#else
#define syntaxErrorDetail(tok) __syntaxErrorDetail__(tok, __FILE__, __LINE__, "(valueFlow)")
#endif


//---------------------------------------------------------------------------


Tokenizer::Tokenizer(const Settings *settings, ErrorLogger *errorLogger) :
    list(settings),
    mSettings(settings),
    mErrorLogger(errorLogger),
    mSymbolDatabase(nullptr),
    mCalculationsSimplifier(nullptr),
    mVarId(0),
    mUnnamedCount(0),
    mCodeWithTemplates(false), //is there any templates?
    mTimerResults(nullptr)
#ifdef MAXTIME
    ,mMaxTime(std::time(0) + MAXTIME)
#endif
{
    // make sure settings are specified
    assert(mSettings);

    mCalculationsSimplifier = new CalculationsSimplifier(this);
}

Tokenizer::~Tokenizer()
{
    delete mSymbolDatabase;
    delete mCalculationsSimplifier;
}


//---------------------------------------------------------------------------
// SizeOfType - gives the size of a type
//---------------------------------------------------------------------------

unsigned int Tokenizer::sizeOfType(const Token *type) const
{
    if (!type || type->str().empty())
        return 0;

    if (type->tokType() == Token::eString)
        return Token::getStrLength(type) + 1U;

    const std::map<std::string, unsigned int>::const_iterator it = mTypeSize.find(type->str());
    if (it == mTypeSize.end()) {
        const Library::PodType* podtype = mSettings->library.podtype(type->str());
        if (!podtype)
            return 0;

        return podtype->size;
    } else if (type->isLong()) {
        if (type->str() == "double")
            return mSettings->sizeof_long_double;
        else if (type->str() == "long")
            return mSettings->sizeof_long_long;
    }

    return it->second;
}


namespace {
    struct Space {
        Space() : bodyEnd(nullptr), isNamespace(false) { }
        std::string className;
        const Token * bodyEnd;
        bool isNamespace;
    };
}

bool Tokenizer::createTokens(std::istream &code,
                             const std::string& FileName)
{
    // make sure settings specified
    assert(mSettings);

    return list.createTokens(code, FileName);
}

void Tokenizer::createTokens(const simplecpp::TokenList *tokenList)
{
    // make sure settings specified
    assert(mSettings);
    list.createTokens(tokenList);
}

bool Tokenizer::simplifyTokens1(const std::string &configuration)
{
    // Fill the map mTypeSize..
    fillTypeSizes();

    mConfiguration = configuration;

    if (!simplifyTokenList1(list.getFiles().front().c_str()))
        return false;

    if (mTimerResults) {
        Timer t("Tokenizer::simplifyTokens1::createAst", mSettings->showtime, mTimerResults);
        list.createAst();
        list.validateAst();
    } else {
        list.createAst();
        list.validateAst();
    }

    if (mTimerResults) {
        Timer t("Tokenizer::simplifyTokens1::createSymbolDatabase", mSettings->showtime, mTimerResults);
        createSymbolDatabase();
    } else {
        createSymbolDatabase();
    }

    // Use symbol database to identify rvalue references. Split && to & &. This is safe, since it doesn't delete any tokens (which might be referenced by symbol database)
    for (const Variable* var : mSymbolDatabase->variableList()) {
        if (var && var->isRValueReference()) {
            Token* endTok = const_cast<Token*>(var->typeEndToken());
            endTok->str("&");
            endTok->astOperand1(nullptr);
            endTok->astOperand2(nullptr);
            endTok->insertToken("&");
            endTok->next()->scope(endTok->scope());
        }
    }

    if (mTimerResults) {
        Timer t("Tokenizer::simplifyTokens1::setValueType", mSettings->showtime, mTimerResults);
        mSymbolDatabase->setValueTypeInTokenList();
    } else {
        mSymbolDatabase->setValueTypeInTokenList();
    }

    if (mTimerResults) {
        Timer t("Tokenizer::simplifyTokens1::ValueFlow", mSettings->showtime, mTimerResults);
        ValueFlow::setValues(&list, mSymbolDatabase, mErrorLogger, mSettings);
    } else {
        ValueFlow::setValues(&list, mSymbolDatabase, mErrorLogger, mSettings);
    }

    printDebugOutput(1);

    return true;
}

bool Tokenizer::tokenize(std::istream &code,
                         const char FileName[],
                         const std::string &configuration)
{
    if (!createTokens(code, FileName))
        return false;

    return simplifyTokens1(configuration);
}
//---------------------------------------------------------------------------

void Tokenizer::checkForEnumsWithTypedef()
{
    for (const Token *tok = list.front(); tok; tok = tok->next()) {
        if (Token::Match(tok, "enum %name% {")) {
            tok = tok->tokAt(2);
            const Token *tok2 = Token::findsimplematch(tok, "typedef", tok->link());
            if (tok2)
                syntaxErrorDetailMsg(tok2, "Enum with typedef");
            tok = tok->link();
        }
    }
}


//---------------------------------------------------------------------------
void Tokenizer::fillTypeSizes()
{
    mTypeSize.clear();
    mTypeSize["char"] = 1;
    mTypeSize["bool"] = mSettings->sizeof_bool;
    mTypeSize["short"] = mSettings->sizeof_short;
    mTypeSize["int"] = mSettings->sizeof_int;
    mTypeSize["long"] = mSettings->sizeof_long;
    mTypeSize["float"] = mSettings->sizeof_float;
    mTypeSize["double"] = mSettings->sizeof_double;
    mTypeSize["size_t"] = mSettings->sizeof_size_t;
    mTypeSize["*"] = mSettings->sizeof_pointer;
}

void Tokenizer::combineOperators()
{
    // Combine tokens..
    for (Token *tok = list.front(); tok && tok->next(); tok = tok->next()) {
        const char c1 = tok->str()[0];

        if (tok->str().length() == 1 && tok->next()->str().length() == 1) {
            const char c2 = tok->next()->str()[0];

            // combine +-*/ and =
            if (c2 == '=' && (std::strchr("+-*/%|^=!<>", c1))) {
                tok->str(tok->str() + c2);
                tok->deleteNext();
                continue;
            }
        } else if (tok->next()->str() == "=") {
            if (tok->str() == ">>") {
                tok->str(">>=");
                tok->deleteNext();
            } else if (tok->str() == "<<") {
                tok->str("<<=");
                tok->deleteNext();
            }
        } else if ((c1 == 'p' || c1 == '_') &&
                   Token::Match(tok, "private|protected|public : !!:")) {
            bool simplify = false;
            unsigned int par = 0U;
            for (const Token *prev = tok->previous(); prev; prev = prev->previous()) {
                if (prev->str() == ")") {
                    ++par;
                } else if (prev->str() == "(") {
                    if (par == 0U)
                        break;
                    --par;
                }
                if (par != 0U || prev->str() == "(")
                    continue;
                if (Token::Match(prev, "[;{}]")) {
                    simplify = true;
                    break;
                }
                if (prev->isName() && prev->isUpperCaseName())
                    continue;
                if (prev->isName() && endsWith(prev->str(), ':'))
                    simplify = true;
                break;
            }
            if (simplify) {
                tok->str(tok->str() + ":");
                tok->deleteNext();
            }
        }
    }
}

void Tokenizer::combineStringAndCharLiterals()
{
    for (Token *tok = list.front(); tok; tok = tok->next()) {
        const std::string prefix[4] = {"u8", "L", "U", "u"};
        for (const std::string & p : prefix) {
            if (((tok->tokType() == Token::eString) && (tok->str().find(p + "\"") == 0)) ||
                ((tok->tokType() == Token::eChar) && (tok->str().find(p + "\'") == 0))) {
                tok->str(tok->str().substr(p.size()));
                tok->isLong(p != "u8");
                break;
            }
        }
    }

    // Combine strings
    for (Token *tok = list.front();
         tok;
         tok = tok->next()) {
        if (tok->str()[0] != '"')
            continue;

        tok->str(simplifyString(tok->str()));
        while (tok->next() && tok->next()->tokType() == Token::eString) {
            // Two strings after each other, combine them
            tok->concatStr(simplifyString(tok->next()->str()));
            tok->deleteNext();
        }
    }
}

void Tokenizer::concatenateNegativeNumberAndAnyPositive()
{
    for (Token *tok = list.front(); tok; tok = tok->next()) {
        if (!Token::Match(tok, "?|:|,|(|[|{|return|case|%op% +|-") || tok->tokType() == Token::eIncDecOp)
            continue;

        while (tok->str() != ">" && tok->next() && tok->next()->str() == "+")
            tok->deleteNext();

        if (Token::Match(tok->next(), "- %num%")) {
            tok->deleteNext();
            tok->next()->str("-" + tok->next()->str());
        }
    }
}


void Tokenizer::simplifyRedundantConsecutiveBraces()
{
    // Remove redundant consecutive braces, i.e. '.. { { .. } } ..' -> '.. { .. } ..'.
    for (Token *tok = list.front(); tok;) {
        if (Token::simpleMatch(tok, "= {")) {
            tok = tok->linkAt(1);
        } else if (Token::simpleMatch(tok, "{ {") && Token::simpleMatch(tok->next()->link(), "} }")) {
            //remove internal parentheses
            tok->next()->link()->deleteThis();
            tok->deleteNext();
        } else
            tok = tok->next();
    }
}

void Tokenizer::simplifyDoublePlusAndDoubleMinus()
{
    // Convert - - into + and + - into -
    for (Token *tok = list.front(); tok; tok = tok->next()) {
        while (tok->next()) {
            if (tok->str() == "+") {
                if (tok->next()->str()[0] == '-') {
                    tok = tok->next();
                    if (tok->str().size() == 1) {
                        tok = tok->previous();
                        tok->str("-");
                        tok->deleteNext();
                    } else if (tok->isNumber()) {
                        tok->str(tok->str().substr(1));
                        tok = tok->previous();
                        tok->str("-");
                    }
                    continue;
                }
            } else if (tok->str() == "-") {
                if (tok->next()->str()[0] == '-') {
                    tok = tok->next();
                    if (tok->str().size() == 1) {
                        tok = tok->previous();
                        tok->str("+");
                        tok->deleteNext();
                    } else if (tok->isNumber()) {
                        tok->str(tok->str().substr(1));
                        tok = tok->previous();
                        tok->str("+");
                    }
                    continue;
                }
            }

            break;
        }
    }
}

/** Specify array size if it hasn't been given */

void Tokenizer::arraySize()
{
    for (Token *tok = list.front(); tok; tok = tok->next()) {
        if (!tok->isName() || !Token::Match(tok, "%var% [ ] ="))
            continue;
        bool addlength = false;
        if (Token::Match(tok, "%var% [ ] = { %str% } ;")) {
            Token *t = tok->tokAt(3);
            t->deleteNext();
            t->next()->deleteNext();
            addlength = true;
        }

        if (addlength || Token::Match(tok, "%var% [ ] = %str% ;")) {
            tok = tok->next();
            const std::size_t sz = Token::getStrSize(tok->tokAt(3));
            tok->insertToken(MathLib::toString(sz));
            tok = tok->tokAt(5);
        }

        else if (Token::Match(tok, "%var% [ ] = {")) {
            MathLib::biguint sz = 1;
            tok = tok->next();
            Token *end = tok->linkAt(3);
            for (Token *tok2 = tok->tokAt(4); tok2 && tok2 != end; tok2 = tok2->next()) {
                if (tok2->link() && Token::Match(tok2, "{|(|[|<")) {
                    if (tok2->str() == "[" && tok2->link()->strAt(1) == "=") { // designated initializer
                        if (Token::Match(tok2, "[ %num% ]"))
                            sz = std::max(sz, MathLib::toULongNumber(tok2->strAt(1)) + 1U);
                        else {
                            sz = 0;
                            break;
                        }
                    }
                    tok2 = tok2->link();
                } else if (tok2->str() == ",") {
                    if (!Token::Match(tok2->next(), "[},]"))
                        ++sz;
                    else {
                        tok2 = tok2->previous();
                        tok2->deleteNext();
                    }
                }
            }

            if (sz != 0)
                tok->insertToken(MathLib::toString(sz));

            tok = end->next() ? end->next() : end;
        }
    }
}

static Token *skipTernaryOp(Token *tok)
{
    unsigned int colonLevel = 1;
    while (nullptr != (tok = tok->next())) {
        if (tok->str() == "?") {
            ++colonLevel;
        } else if (tok->str() == ":") {
            --colonLevel;
            if (colonLevel == 0) {
                tok = tok->next();
                break;
            }
        }
        if (tok->link() && Token::Match(tok, "[(<]"))
            tok = tok->link();
        else if (Token::Match(tok->next(), "[{};)]"))
            break;
    }
    if (colonLevel > 0) // Ticket #5214: Make sure the ':' matches the proper '?'
        return nullptr;
    return tok;
}

const Token * Tokenizer::startOfExecutableScope(const Token * tok)
{
    if (tok->str() != ")")
        return nullptr;

    tok = isFunctionHead_static(tok, ":{");

    if (Token::Match(tok, ": %name% [({]")) {
        while (Token::Match(tok, "[:,] %name% [({]"))
            tok = tok->linkAt(2)->next();
    }

    return (tok && tok->str() == "{") ? tok : nullptr;
}


/** simplify labels and case|default in the code: add a ";" if not already in.*/

void Tokenizer::simplifyLabelsCaseDefault()
{
    bool executablescope = false;
    unsigned int indentLevel = 0;
    for (Token *tok = list.front(); tok; tok = tok->next()) {
        // Simplify labels in the executable scope..
        Token *start = const_cast<Token *>(startOfExecutableScope(tok));
        if (start) {
            tok = start;
            executablescope = true;
        }

        if (!executablescope)
            continue;

        if (tok->str() == "{") {
            if (tok->previous()->str() == "=")
                tok = tok->link();
            else
                ++indentLevel;
        } else if (tok->str() == "}") {
            --indentLevel;
            if (indentLevel == 0) {
                executablescope = false;
                continue;
            }
        } else if (Token::Match(tok, "(|["))
            tok = tok->link();

        if (Token::Match(tok, "[;{}:] case")) {
            while (nullptr != (tok = tok->next())) {
                if (Token::Match(tok, "(|[")) {
                    tok = tok->link();
                } else if (tok->str() == "?") {
                    Token *tok1 = skipTernaryOp(tok);
                    if (!tok1) {
                        syntaxErrorDetailMsg(tok, "Make sure the ':' matches the proper '?'");
                    }
                    tok = tok1;
                }
                if (Token::Match(tok->next(),"[:{};]"))
                    break;
            }
            if (!tok)
                break;
            if (tok->str() != "case" && tok->next() && tok->next()->str() == ":") {
                tok = tok->next();
                if (!tok->next())
                    syntaxErrorDetail(tok);
                if (tok->next()->str() != ";" && tok->next()->str() != "case")
                    tok->insertToken(";");
                else
                    tok = tok->previous();
            } else {
                syntaxErrorDetail(tok);
            }
        } else if (Token::Match(tok, "[;{}] %name% : !!;")) {
            if (!Token::Match(tok->next(), "class|struct|enum")) {
                tok = tok->tokAt(2);
                tok->insertToken(";");
            }
        }
    }
}

//---------------------------------------------------------------------------

static bool setVarIdParseDeclaration(const Token **tok, const std::map<std::string,unsigned int> &variableId, bool executableScope, std::string &err)
{
    const Token *tok2 = *tok;
    if (!tok2->isName())
        return false;

    unsigned int typeCount = 0;
    unsigned int singleNameCount = 0;
    bool hasstruct = false;   // Is there a "struct" or "class"?
    bool ref = false;
    while (tok2) {
        if (tok2->isName()) {

            if (Token::Match(tok2, "const|public|private|protected"))
                ;  // just skip
            if (Token::Match(tok2, "struct|class|typename|enum") ) {
                hasstruct = true;
                typeCount = 0;
                singleNameCount = 0;
            }  else if (!hasstruct && variableId.find(tok2->str()) != variableId.end() && tok2->previous()->str() != "::") {
                ++typeCount;
                tok2 = tok2->next();
                if (!tok2 || tok2->str() != "::")
                    break;
            } else {
                if (tok2->str() != "void" ) // just "void" cannot be a variable type
                    ++typeCount;
                ++singleNameCount;
            }
        }
        // search for vectors or shared_ptr
        // vector<int> object1;
        // shared_ptr<int> object1;
        else if (Token::Match(tok2, "< %name% >")) {
            const Token *start = *tok;
            if ( (start->str() != "shared_ptr") &&
                 (start->str() != "vector") &&
                 !SymbolDatabase::isReservedName(start->str())
                 )

            {
                std::cout << __FUNCTION__ << " " << start->str() << " " << SymbolDatabase::isReservedName(start->str()) << std::endl;
                // syntax error ?
                err = "Search for vector or shared_ptr, but " + start->str() + " found";
                throw start;
            }
            const Token * const closingBracket = tok2->tokAt(2);
            if (closingBracket == nullptr) {
                // this is just defensive check, becuase missing brackets are checked in linkBrackets()
                err = "closing brackets are missing " + tok2->str() + ".at(2)";
                throw tok2;
            }
            tok2 = closingBracket;
            if (tok2->str() != ">"){
                // this is just defensive check, becuase missing brackets are checked in linkBrackets()
                err = "closing brackets are missing " + tok2->str() + ".at(2)";
                throw tok2;
            }
            singleNameCount = 1;
            
            if (!Token::Match(tok2, "> %name%") && !Token::Match(tok2, "> & %name%"))
            {
                err = "Variable name is missing after '" + tok2->strAt(1) + "'";
                throw tok2->tokAt(1);
            }
            if ( Token::Match(tok2, "> %name%") && !Token::Match(tok2->tokAt(2), "(|)|,|;|=") )
            {
                err = "Invalid character '" + tok2->strAt(2) + "' found after variable declaration found!";
                throw tok2->tokAt(2);
            }
            if ( Token::Match(tok2, "> & %name%") && !Token::Match(tok2->tokAt(3), "(|)|,|;|=") )
            {
                err = "Invalid character '" + tok2->strAt(3) + "' found after variable declaration found!";
                throw tok2->tokAt(3);
            }

        }else if (Token::Match(tok2, "&")) {
            ref = true;
        }  else if (tok2->str() == "::") {
            singleNameCount = 0;
        } else if (tok2->str() != "*" && tok2->str() != "::") {
            break;
        }
        tok2 = tok2->next();
    }

    if (tok2)
    {
        *tok = tok2;

        // In executable scopes, references must be assigned
        // Catching by reference is an exception
        if (executableScope && ref) {
            if (Token::Match(tok2, "(|=|{|:"))
                ;   // reference is assigned => ok
            else if (tok2->str() != ")" || tok2->link()->strAt(-1) != "catch")
                return false;   // not catching by reference => not declaration
        }
    }

    // Check if array declaration is valid (#2638)
    // invalid declaration: AAA a[4] = 0;
    if (typeCount >= 2 && executableScope && tok2 && tok2->str() == "[") {
        const Token *tok3 = tok2->link()->next();
        while (tok3 && tok3->str() == "[") {
            tok3 = tok3->link()->next();
        }
        if (Token::Match(tok3, "= %num%"))
            return false;
    }

    return (typeCount >= 2 && tok2 && Token::Match(tok2->tokAt(-2), "!!:: %type%"));
}


static void setVarIdStructMembers(Token **tok1,
                                  std::map<unsigned int, std::map<std::string, unsigned int> >& structMembers,
                                  unsigned int *varId)
{
    Token *tok = *tok1;

    while (Token::Match(tok->next(), ")| . %name% !!(")) {
        const unsigned int struct_varid = tok->varId();
        tok = tok->tokAt(2);
        if (struct_varid == 0)
            continue;

        if (tok->str() == ".")
            tok = tok->next();


        std::map<std::string, unsigned int>& members = structMembers[struct_varid];
        const std::map<std::string, unsigned int>::iterator it = members.find(tok->str());
        if (it == members.end()) {
            members[tok->str()] = ++(*varId);
            tok->setVarId(*varId);
        } else {
            tok->setVarId(it->second);
        }
    }
    // tok can't be null
    *tok1 = tok;
}


void Tokenizer::setVarIdClassDeclaration(const Token * const startToken,
        const VariableMap &variableMap,
        const unsigned int scopeStartVarId,
        std::map<unsigned int, std::map<std::string,unsigned int> >& structMembers)
{
    // end of scope
    const Token * const endToken = startToken->link();

    // determine class name
    std::string className;
    for (const Token *tok = startToken->previous(); tok; tok = tok->previous()) {
        if (!tok->isName() && tok->str() != ":")
            break;
        if (Token::Match(tok, "class|struct|enum %type% [:{]")) {
            className = tok->next()->str();
            break;
        }
    }

    // replace varids..
    unsigned int indentlevel = 0;
    bool initList = false;
    bool inEnum = false;
    const Token *initListArgLastToken = nullptr;
    for (Token *tok = startToken->next(); tok != endToken; tok = tok->next()) {
        if (!tok)
            syntaxErrorDetail(nullptr);
        if (initList) {
            if (tok == initListArgLastToken)
                initListArgLastToken = nullptr;
            else if (!initListArgLastToken &&
                     Token::Match(tok->previous(), "%name%|>|>> {|(") &&
                     Token::Match(tok->link(), "}|) ,|{"))
                initListArgLastToken = tok->link();
        }
        if (tok->str() == "{") {
            inEnum = isEnumStart(tok);
            if (initList && !initListArgLastToken)
                initList = false;
            ++indentlevel;
        } else if (tok->str() == "}") {
            --indentlevel;
            inEnum = false;
        } else if (initList && indentlevel == 0 && Token::Match(tok->previous(), "[,:] %name% [({]")) {
            const std::map<std::string, unsigned int>::const_iterator it = variableMap.find(tok->str());
            if (it != variableMap.end()) {
                tok->setVarId(it->second);
            }
        } else if (tok->isName() && tok->varId() <= scopeStartVarId) {
            if (indentlevel > 0 || initList) {
                if (Token::Match(tok->previous(), "::|.") && tok->strAt(-2) != "this" && !Token::simpleMatch(tok->tokAt(-5), "( * this ) ."))
                    continue;
                if (!tok->next())
                    syntaxErrorDetail(nullptr);
                if (tok->next()->str() == "::") {
                    if (tok->str() == className)
                        tok = tok->tokAt(2);
                    else
                        continue;
                }

                if (!inEnum) {
                    const std::map<std::string, unsigned int>::const_iterator it = variableMap.find(tok->str());
                    if (it != variableMap.end()) {
                        tok->setVarId(it->second);
                        setVarIdStructMembers(&tok, structMembers, variableMap.getVarId());
                    }
                }
            }
        } else if (indentlevel == 0 && tok->str() == ":" && !initListArgLastToken)
            initList = true;
    }
}



// Update the variable ids..
// Parse each function..
static void setVarIdClassFunction(const std::string &classname,
                                  Token * const startToken,
                                  const Token * const endToken,
                                  const std::map<std::string, unsigned int> &varlist,
                                  std::map<unsigned int, std::map<std::string, unsigned int> >& structMembers,
                                  unsigned int *varId_)
{
//    std::cout << __FUNCTION__ << std::endl;
    for (Token *tok2 = startToken; tok2 && tok2 != endToken; tok2 = tok2->next()) {
        if (tok2->varId() != 0 || !tok2->isName())
            continue;
        if (Token::Match(tok2->tokAt(-2), ("!!" + classname + " ::").c_str()))
            continue;
        if (Token::Match(tok2->tokAt(-4), "%name% :: %name% ::")) // Currently unsupported
            continue;
        if (Token::Match(tok2->tokAt(-2), "!!this .") && !Token::simpleMatch(tok2->tokAt(-5), "( * this ) ."))
            continue;

        const std::map<std::string,unsigned int>::const_iterator it = varlist.find(tok2->str());
        if (it != varlist.end()) {
            tok2->setVarId(it->second);
            setVarIdStructMembers(&tok2, structMembers, varId_);
        }
    }
}



void Tokenizer::setVarId()
{
    // Clear all variable ids
    for (Token *tok = list.front(); tok; tok = tok->next()) {
        if (tok->isName())
            tok->setVarId(0);
    }

    setPodTypes();

    setVarIdPass1();

    setVarIdPass2();
}


// Variable declarations can't start with "return" etc.
static const std::set<std::string> notstart = {
    "case", "default", "goto", "return", "typedef",
    "delete", "new", "throw", "using"
    };

void Tokenizer::setVarIdPass1()
{
    // Variable declarations can't start with "return" etc.

    VariableMap variableMap;
    std::map<unsigned int, std::map<std::string, unsigned int> > structMembers;

    std::stack<VarIdScopeInfo> scopeStack;

    scopeStack.push(VarIdScopeInfo());
    std::stack<const Token *> functionDeclEndStack;
    const Token *functionDeclEndToken = nullptr;
    bool initlist = false;
    for (Token *tok = list.front(); tok; tok = tok->next()) {
        if (tok->isOp())
            continue;
        if (tok == functionDeclEndToken) {
            functionDeclEndStack.pop();
            functionDeclEndToken = functionDeclEndStack.empty() ? nullptr : functionDeclEndStack.top();
            if (tok->str() == ":")
                initlist = true;
            else if (tok->str() == ";") {
                if (!variableMap.leaveScope())
                    cppcheckError(tok);
            } else if (tok->str() == "{")
                scopeStack.push(VarIdScopeInfo(true, scopeStack.top().isStructInit || tok->strAt(-1) == "=", /*isEnum=*/false, *variableMap.getVarId()));
        } else if (!initlist && tok->str()=="(") {
            const Token * newFunctionDeclEnd = nullptr;
            if (!scopeStack.top().isExecutable)
                newFunctionDeclEnd = isFunctionHead(tok, "{:;");
            else {
                Token const * const tokenLinkNext = tok->link()->next();
                if (tokenLinkNext && tokenLinkNext->str() == "{") // might be for- or while-loop or if-statement
                    newFunctionDeclEnd = tokenLinkNext;
            }
            if (newFunctionDeclEnd && newFunctionDeclEnd != functionDeclEndToken) {
                functionDeclEndStack.push(newFunctionDeclEnd);
                functionDeclEndToken = newFunctionDeclEnd;
                variableMap.enterScope();
            }
        } else if (Token::Match(tok, "{|}")) {

            const Token * const startToken = (tok->str() == "{") ? tok : tok->link();

            // parse anonymous structs/enums as part of the current scope
            if (!Token::Match(startToken->previous(), "struct|enum {") &&
                !(initlist && Token::Match(startToken->previous(), "%name%|>|>>") && Token::Match(startToken->link(), "} ,|{"))) {

                if (tok->str() == "{") {
                    bool isExecutable;
                    const Token *prev = tok->previous();
                    while (Token::Match(prev, "%name%|."))
                        prev = prev->previous();
                    const bool isLambda = prev && prev->str() == ")" && Token::simpleMatch(prev->link()->previous(), "] (");
                    if ((!isLambda && (tok->strAt(-1) == ")" || Token::Match(tok->tokAt(-2), ") %type%"))) ||
                        (initlist && tok->strAt(-1) == "}")) {
                        isExecutable = true;
                    } else {
                        isExecutable = ((scopeStack.top().isExecutable || initlist || tok->strAt(-1) == "else") &&
                                        !isClassStructEnumStart(tok));
                        if (!(scopeStack.top().isStructInit || tok->strAt(-1) == "="))
                            variableMap.enterScope();
                    }
                    initlist = false;
                    scopeStack.push(VarIdScopeInfo(isExecutable, scopeStack.top().isStructInit || tok->strAt(-1) == "=", isEnumStart(tok), *variableMap.getVarId()));
                } else { /* if (tok->str() == "}") */

                    // Set variable ids in class declaration..
                    if (!initlist && !scopeStack.top().isExecutable && tok->link()) {
                        setVarIdClassDeclaration(tok->link(),
                                                 variableMap,
                                                 scopeStack.top().startVarid,
                                                 structMembers);
                    }

                    if (!scopeStack.top().isStructInit) {
                        variableMap.leaveScope();
                    }

                    scopeStack.pop();
                    if (scopeStack.empty()) {  // should be impossible
                        scopeStack.push(VarIdScopeInfo());
                    }
                }
            }
        }

        if (!scopeStack.top().isStructInit &&
            (tok == list.front() ||
             Token::Match(tok, "[;{}]") ||
             (tok->str() == "(" && isFunctionHead(tok,"{")) ||
             (tok->str() == "(" && !scopeStack.top().isExecutable && isFunctionHead(tok,";:")) ||
             (tok->str() == "," && (!scopeStack.top().isExecutable)) ||
             (tok->isName() && endsWith(tok->str(), ':')))) {

            if (mSettings->terminated())
                return;

            // locate the variable name..
            const Token *tok2 = (tok->isName()) ? tok : tok->next();

            // private: protected: public: etc
            while (tok2 && endsWith(tok2->str(), ':')) {
                tok2 = tok2->next();
            }
            if (!tok2)
                break;

            // Variable declaration can't start with "return", etc
            if (notstart.find(tok2->str()) != notstart.end())
                continue;

            if (Token::simpleMatch(tok2, "const new"))
                continue;

            bool decl;
            std::string errText;
            try { /* Ticket #8151 */
                decl = setVarIdParseDeclaration(&tok2, variableMap.map(), scopeStack.top().isExecutable, errText);
            } catch (const Token * errTok) {
                syntaxErrorDetailMsg(errTok, errText);
            }
            if (decl) {
                const Token* prev2 = tok2->previous();
                if (Token::Match(prev2, "%type% [;[=,)]") && tok2->previous()->str() != "const")
                    ;
                else if (Token::Match(prev2, "%type% :") && tok->strAt(-1) == "for") {
                    ;
                } else
                    decl = false;

                if (decl) {
                    variableMap.addVariable(prev2->str());

                    tok = tok2->previous();
                }
            }
        }

        if (tok->isName()) {
            // don't set variable id after a struct|enum
            if (Token::Match(tok->previous(), "struct|enum") || tok->strAt(-1) == "class")
                continue;

            // function declaration inside executable scope? Function declaration is of form: type name "(" args ")"
            if (scopeStack.top().isExecutable && Token::Match(tok, "%name% [,)]")) {
                bool par = false;
                const Token *start, *end;

                // search begin of function declaration
                for (start = tok; Token::Match(start, "%name%|*|&|,|("); start = start->previous()) {
                    if (start->str() == "(") {
                        if (par)
                            break;
                        par = true;
                    }
                    if (Token::Match(start, "[(,]")) {
                        if (!Token::Match(start, "[(,] %type% %name%|*|&"))
                            break;
                    }
                    if (start->varId() > 0U)
                        break;
                }

                // search end of function declaration
                for (end = tok->next(); Token::Match(end, "%name%|*|&|,"); end = end->next()) {}

                // there are tokens which can't appear at the begin of a function declaration such as "return"
                const bool isNotstartKeyword = start->next() && notstart.find(start->next()->str()) != notstart.end();

                // now check if it is a function declaration
                if (Token::Match(start, "[;{}] %type% %name%|*") && par && Token::simpleMatch(end, ") ;") && !isNotstartKeyword)
                    // function declaration => don't set varid
                    continue;
            }

            // don't set variable id when it is a inline function call
            if (Token::Match(tok->next(), "(") )
                continue;

            if (!scopeStack.top().isEnum) {
                const std::map<std::string, unsigned int>::const_iterator it = variableMap.find(tok->str());
                if (it != variableMap.end()) {
                    tok->setVarId(it->second);
                    setVarIdStructMembers(&tok, structMembers, variableMap.getVarId());
                }
            }
        } else if (Token::Match(tok, "::|. %name%")) {
            // Don't set varid after a :: or . token
            tok = tok->next();
        } else if (tok->str() == ":" && Token::Match(tok->tokAt(-2), "class %type%")) {
            do {
                tok = tok->next();
            } while (tok && (tok->isName() || tok->str() == ","));
            if (!tok)
                break;
            tok = tok->previous();
        }
    }

    mVarId = *variableMap.getVarId();
}

namespace {
    struct Member {
        Member(const std::list<std::string> &s, const std::list<const Token *> &ns, Token *t) : usingnamespaces(ns), scope(s), tok(t) {}
        std::list<const Token *> usingnamespaces;
        std::list<std::string> scope;
        Token *tok;
    };

    struct ScopeInfo2 {
        ScopeInfo2(const std::string &name_, const Token *bodyEnd_) : name(name_), bodyEnd(bodyEnd_) {}
        const std::string name;
        const Token * const bodyEnd;
    };
}

static std::string getScopeName(const std::list<ScopeInfo2> &scopeInfo)
{
    std::string ret;
    for (const ScopeInfo2 &si : scopeInfo)
        ret += (ret.empty() ? "" : " :: ") + (si.name);
    return ret;
}

static Token * matchMemberName(const std::list<std::string> &scope, const Token *nsToken, Token *memberToken, const std::list<ScopeInfo2> &scopeInfo)
{
    std::list<ScopeInfo2>::const_iterator scopeIt = scopeInfo.begin();

    // Current scope..
    for (std::list<std::string>::const_iterator it = scope.begin(); it != scope.end(); ++it) {
        if (scopeIt == scopeInfo.end() || scopeIt->name != *it)
            return nullptr;
        ++scopeIt;
    }

    // using namespace..
    if (nsToken) {
        while (Token::Match(nsToken, "%name% ::")) {
            if (scopeIt != scopeInfo.end() && nsToken->str() == scopeIt->name) {
                nsToken = nsToken->tokAt(2);
                ++scopeIt;
            } else {
                return nullptr;
            }
        }
        if (!Token::Match(nsToken, "%name% ;"))
            return nullptr;
        if (scopeIt == scopeInfo.end() || nsToken->str() != scopeIt->name)
            return nullptr;
        ++scopeIt;
    }

    // Parse member tokens..
    while (scopeIt != scopeInfo.end()) {
        if (!Token::Match(memberToken, "%name% ::"))
            return nullptr;
        if (memberToken->str() != scopeIt->name)
            return nullptr;

        memberToken = memberToken->tokAt(2);
        ++scopeIt;
    }

    return Token::Match(memberToken, "~| %name%") ? memberToken : nullptr;
}

static Token * matchMemberName(const Member &member, const std::list<ScopeInfo2> &scopeInfo)
{
    if (scopeInfo.empty())
        return nullptr;

    // Does this member match without "using namespace"..
    Token *ret = matchMemberName(member.scope, nullptr, member.tok, scopeInfo);
    if (ret)
        return ret;

    // Try to match member using the "using namespace ..." namespaces..
    for (const Token *ns : member.usingnamespaces) {
        ret = matchMemberName(member.scope, ns, member.tok, scopeInfo);
        if (ret)
            return ret;
    }

    return nullptr;
}

static Token * matchMemberVarName(const Member &var, const std::list<ScopeInfo2> &scopeInfo)
{
    Token *tok = matchMemberName(var, scopeInfo);
    return Token::Match(tok, "%name% !!(") ? tok : nullptr;
}

static Token * matchMemberFunctionName(const Member &func, const std::list<ScopeInfo2> &scopeInfo)
{
    Token *tok = matchMemberName(func, scopeInfo);
    return Token::Match(tok, "~| %name% (") ? tok : nullptr;
}

void Tokenizer::setVarIdPass2()
{
    std::map<unsigned int, std::map<std::string, unsigned int> > structMembers;

    // Member functions and variables in this source
    std::list<Member> allMemberFunctions;
    std::list<Member> allMemberVars;

    std::map<const Token *, std::string> endOfScope;
    std::list<std::string> scope;
    std::list<const Token *> usingnamespaces;
    for (Token *tok = list.front(); tok; tok = tok->next()) {

        if (tok->str() == "}") {
            const std::map<const Token *, std::string>::iterator it = endOfScope.find(tok);
            if (it != endOfScope.end())
                scope.remove(it->second);
        }

        Token* const tok1 = tok;
        if (Token::Match(tok->previous(), "!!:: %name% :: ~| %name%"))
            tok = tok->next();
        else
            continue;

        while (Token::Match(tok, ":: ~| %name%")) {
            tok = tok->next();
            if (tok->str() == "~")
                tok = tok->next();
            else if (Token::Match(tok, "%name% ::"))
                tok = tok->next();
            else
                break;
        }
        if (!tok->next())
            syntaxErrorDetail(tok);
        if (Token::Match(tok, "%name% ("))
            allMemberFunctions.emplace_back(scope, usingnamespaces, tok1);
        else
            allMemberVars.emplace_back(scope, usingnamespaces, tok1);
    }


    std::list<ScopeInfo2> scopeInfo;

    // class members..
    std::map<std::string, std::map<std::string, unsigned int> > varsByClass;
    for (Token *tok = list.front(); tok; tok = tok->next()) {
        while (tok->str() == "}" && !scopeInfo.empty() && tok == scopeInfo.back().bodyEnd)
            scopeInfo.pop_back();

        if (!Token::Match(tok, "class|struct %name% {|:|::"))
            continue;

        const std::string &scopeName(getScopeName(scopeInfo));
        const std::string scopeName2(scopeName.empty() ? std::string() : (scopeName + " :: "));

        std::list<const Token *> classnameTokens;
        classnameTokens.push_back(tok->next());
        const Token* tokStart = tok->tokAt(2);
        while (Token::Match(tokStart, ":: %name%")) {
            classnameTokens.push_back(tokStart->next());
            tokStart = tokStart->tokAt(2);
        }

        std::string classname;
        for (const Token *it : classnameTokens)
            classname += (classname.empty() ? "" : " :: ") + it->str();

        std::map<std::string, unsigned int> &thisClassVars = varsByClass[scopeName2 + classname];
        while (Token::Match(tokStart, ":|::|,|%name%")) {
            if (Token::Match(tokStart, "%name% ,|{")) {
                const std::map<std::string, unsigned int>& baseClassVars = varsByClass[tokStart->str()];
                thisClassVars.insert(baseClassVars.begin(), baseClassVars.end());
            }
            tokStart = tokStart->next();
        }
        if (!Token::simpleMatch(tokStart, "{"))
            continue;

        // What member variables are there in this class?
        for (const Token *it : classnameTokens)
            scopeInfo.emplace_back(it->str(), tokStart->link());

        for (Token *tok2 = tokStart->next(); tok2 && tok2 != tokStart->link(); tok2 = tok2->next()) {
            // skip parentheses..
            if (tok2->link()) {
                if (tok2->str() == "{") {
                    if (tok2->strAt(-1) == ")" || tok2->strAt(-2) == ")")
                        setVarIdClassFunction(scopeName2 + classname, tok2, tok2->link(), thisClassVars, structMembers, &mVarId);
                    tok2 = tok2->link();
                } else if (tok2->str() == "(" && tok2->link()->strAt(1) != "(") {
                    tok2 = tok2->link();

                    // Skip initialization list
                    while (Token::Match(tok2, ") [:,] %name% ("))
                        tok2 = tok2->linkAt(3);
                }
            }

            // Found a member variable..
            else if (tok2->varId() > 0)
                thisClassVars[tok2->str()] = tok2->varId();
        }

        // Are there any member variables in this class?
        if (thisClassVars.empty())
            continue;

        // Member variables
        for (const Member &var : allMemberVars) {
            Token *tok2 = matchMemberVarName(var, scopeInfo);
            if (!tok2)
                continue;
            tok2->setVarId(thisClassVars[tok2->str()]);
        }

        // Set variable ids in member functions for this class..
        for (const Member &func : allMemberFunctions) {
            Token *tok2 = matchMemberFunctionName(func, scopeInfo);
            if (!tok2)
                continue;

            if (tok2->str() == "~")
                tok2 = tok2->linkAt(2);
            else
                tok2 = tok2->linkAt(1);

            // If this is a function implementation.. add it to funclist
            Token * start = const_cast<Token *>(isFunctionHead(tok2, "{"));
            if (start) {
                setVarIdClassFunction(classname, start, start->link(), thisClassVars, structMembers, &mVarId);
            }

            if (Token::Match(tok2, ") %name% ("))
                tok2 = tok2->linkAt(2);

            // constructor with initializer list
            if (!Token::Match(tok2, ") : ::| %name%"))
                continue;

            Token *tok3 = tok2;
            while (Token::Match(tok3, "[)}] [,:]")) {
                tok3 = tok3->tokAt(2);
                if (Token::Match(tok3, ":: %name%"))
                    tok3 = tok3->next();
                while (Token::Match(tok3, "%name% :: %name%"))
                    tok3 = tok3->tokAt(2);
                if (!Token::Match(tok3, "%name% (|{|<"))
                    break;

                // set varid
                const std::map<std::string, unsigned int>::const_iterator varpos = thisClassVars.find(tok3->str());
                if (varpos != thisClassVars.end())
                    tok3->setVarId(varpos->second);

                // goto end of var
                tok3 = tok3->linkAt(1);
            }
            if (Token::Match(tok3, ")|} {")) {
                setVarIdClassFunction(classname, tok2, tok3->next()->link(), thisClassVars, structMembers, &mVarId);
            }
        }
    }
}

static void linkBrackets(const Tokenizer * const tokenizer, std::stack<const Token*>& type, std::stack<Token*>& links, Token * const token, const char open, const char close)
{
  if ( token->str().length() != 1 )
    return; // brackets has only 1 character

  if ( Token::Match(token, "<|>") )
  {
    // try to find vector | shared_ptr . Otherwise it is simple operator < >
    std::string typeTok = token->str() == "<" ? token->strAt(-1) : token->strAt(-3);
    if ( (typeTok != "vector") && (typeTok != "shared_ptr"))
      return;
  }

    if (token->str()[0] == open) {
        links.push(token);
        type.push(token);
    } else if (token->str()[0] == close) {
        if (links.empty()) {
            // Error, { and } don't match.
            tokenizer->unmatchedToken(token);
        }
        if (type.top()->str()[0] != open) {
            tokenizer->unmatchedToken(type.top());
        }
        type.pop();

        Token::createMutualLinks(links.top(), token);
        links.pop();
    }
}

void Tokenizer::createLinks()
{
    std::stack<const Token*> type;
    std::stack<Token*> links1;
    std::stack<Token*> links2;
    std::stack<Token*> links3;
    std::stack<Token*> links4;
    for (Token *token = list.front(); token; token = token->next()) {
        if (token->link()) {
            token->setLink(nullptr);
        }

        linkBrackets(this, type, links1, token, '{', '}');

        linkBrackets(this, type, links2, token, '(', ')');

        linkBrackets(this, type, links3, token, '[', ']');

        linkBrackets(this, type, links4, token, '<', '>');
    }

    if (!links1.empty()) {
        // Error, { and } don't match.
        unmatchedToken(links1.top());
    }

    if (!links2.empty()) {
        // Error, ( and ) don't match.
        unmatchedToken(links2.top());
    }

    if (!links3.empty()) {
        // Error, [ and ] don't match.
        unmatchedToken(links3.top());
    }

    if (!links4.empty()) {
        // Error, < and > don't match.
        unmatchedToken(links4.top());
    }
}

bool Tokenizer::simplifyTokenList1(const char FileName[])
{
    if (mSettings->terminated())
        return false;


    // Combine strings and character literals, e.g. L"string", L'c', "string1" "string2"
    combineStringAndCharLiterals();

    createLinks();

    // Bail out if code is garbage
    if (mTimerResults) {
        Timer t("Tokenizer::tokenize::findGarbageCode", mSettings->showtime, mTimerResults);
        findGarbageCode();
    } else {
        findGarbageCode();
    }

    if (mSettings->terminated())
        return false;

    // remove __attribute__((?))
    simplifyAttribute();

    // Combine tokens..
    combineOperators();

    // replace 'sin(0)' to '0' and other similar math expressions
    simplifyMathExpressions();

    // combine "- %num%"
    concatenateNegativeNumberAndAnyPositive();

    // check for simple syntax errors..
    for (const Token *tok = list.front(); tok; tok = tok->next()) {
        if (Token::simpleMatch(tok, "> struct {") &&
            Token::simpleMatch(tok->linkAt(2), "} ;")) {
            syntaxErrorDetail(tok);
        }
    }

    if (!simplifyAddBraces())
        return false;

    if (mSettings->terminated())
        return false;

    // simplify simple calculations inside <..>
    Token *lt = nullptr;
    for (Token *tok = list.front(); tok; tok = tok->next()) {
        if (Token::Match(tok, "[;{}]"))
            lt = nullptr;
        else if (Token::Match(tok, "%type% <"))
            lt = tok->next();
        else if (lt && Token::Match(tok, ">|>> %name%|::|(")) {
            const Token * const end = tok;
            for (tok = lt; tok != end; tok = tok->next()) {
                if (tok->isNumber())
                    mCalculationsSimplifier->simplifyNumericCalculations(tok);
            }
            lt = tok->next();
        }
    }


    // Convert K&R function declarations to modern C
    simplifyVarDecl(true);
    // simplify labels and 'case|default'-like syntaxes
    simplifyLabelsCaseDefault();

    if (mSettings->terminated())
        return false;

    // That call here fixes #7190
    validate();

    // remove unnecessary member qualification..
    removeUnnecessaryQualification();

    if (mSettings->terminated())
        return false;

    // syntax error: enum with typedef in it
    checkForEnumsWithTypedef();

    // Add parentheses to ternary operator where necessary
    prepareTernaryOpForAST();


    // Split up variable declarations.
    simplifyVarDecl(false);

    // Add parentheses to ternary operator where necessary
    // TODO: this is only necessary if one typedef simplification had a comma and was used within ?:
    // If typedef handling is refactored and moved to symboldatabase someday we can remove this
    prepareTernaryOpForAST();

    for (Token* tok = list.front(); tok;) {
        if (Token::Match(tok, "struct|class struct|class"))
            tok->deleteNext();
        else
            tok = tok->next();
    }


    // catch bad typedef canonicalization
    //
    // to reproduce bad typedef, download upx-ucl from:
    // http://packages.debian.org/sid/upx-ucl
    // analyse the file src/stub/src/i386-linux.elf.interp-main.c
    validate();

    // The simplify enum have inner loops
    if (mSettings->terminated())
        return false;

//@todo I think we can remove this for ctrl code.
    // When the assembly code has been cleaned up, no @ is allowed
    for (const Token *tok = list.front(); tok; tok = tok->next()) {
        if (tok->str() == "(") {
            const Token *tok1 = tok;
            tok = tok->link();
            if (!tok)
                syntaxErrorDetail(tok1);
        } else if (tok->str() == "@") {
            syntaxErrorDetail(tok);
        }
    }

    // Order keywords "static" and "const"
    simplifyStaticConst();

    // convert platform dependent types to standard types
    // 32 bits: size_t -> unsigned long
    // 64 bits: size_t -> unsigned long long
    list.simplifyPlatformTypes();

    // collapse compound standard types into a single token
    // unsigned long long int => long (with _isUnsigned=true,_isLong=true)
    list.simplifyStdType();

    if (mSettings->terminated())
        return false;

    // struct simplification "struct S {} s; => struct S { } ; S s ;
    simplifyStructDecl();

    if (mSettings->terminated())
        return false;

    simplifyVariableMultipleAssign();

    // Remove redundant parentheses
    simplifyRedundantParentheses();

    // The simplifyTemplates have inner loops
    if (mSettings->terminated())
        return false;

    // sometimes the "simplifyTemplates" fail and then unsimplified
    // function calls etc remain. These have the "wrong" syntax. So
    // this function will just fix so that the syntax is corrected.
    validate(); // #6847 - invalid code


    // Split up variable declarations.
    simplifyVarDecl(false);

    validate(); // #6772 "segmentation fault (invalid code) in Tokenizer::setVarId"

    if (mTimerResults) {
        Timer t("Tokenizer::tokenize::setVarId", mSettings->showtime, mTimerResults);
        setVarId();
    } else {
        setVarId();
    }

    // specify array size
    arraySize();

    // The simplify enum might have inner loops
    if (mSettings->terminated())
        return false;


    // Convert e.g. atol("0") into 0
    simplifyMathFunctions();

    simplifyDoublePlusAndDoubleMinus();

    Token::assignProgressValues(list.front());

    removeRedundantSemicolons();

    simplifyRedundantConsecutiveBraces();

    elseif();


    validate();
    return true;
}

bool Tokenizer::simplifyTokenList2()
{
    // clear the _functionList so it can't contain dead pointers
    deleteSymbolDatabase();

    // Clear AST,ValueFlow. These will be created again at the end of this function.
    for (Token *tok = list.front(); tok; tok = tok->next()) {
        tok->clearAst();
        tok->clearValueFlow();
    }

    // f(x=g())   =>   x=g(); f(x)
    simplifyAssignmentInFunctionCall();

    // ";a+=b;" => ";a=a+b;"
    simplifyCompoundAssignment();

    // simplify references
    simplifyReference();

    if (mSettings->terminated())
        return false;

    simplifyUndefinedSizeArray();

    simplifyCasts();

    // Simplify simple calculations before replace constants, this allows the replacement of constants that are calculated
    // e.g. const static int value = sizeof(X)/sizeof(Y);
    simplifyCalculations();

    if (mSettings->terminated())
        return false;

    removeRedundantAssignment();


    // Simplify variable declarations
    simplifyVarDecl(false);

    simplifyIfAndWhileAssign();
    simplifyRedundantParentheses();
    simplifyFuncInWhile();

    simplifyIfAndWhileAssign();

    // replace strlen(str)
    for (Token *tok = list.front(); tok; tok = tok->next()) {
        if (Token::Match(tok, "strlen ( %str% )")) {
            tok->str(MathLib::toString(Token::getStrLength(tok->tokAt(2))));
            tok->deleteNext(3);
        }
    }

    bool modified = true;
    while (modified) {
        if (mSettings->terminated())
            return false;

        modified = false;
        modified |= simplifyConditions();
        modified |= simplifyFunctionReturn();
        modified |= simplifyKnownVariables();
        modified |= simplifyStrlen();

        modified |= removeRedundantConditions();
        modified |= simplifyRedundantParentheses();
        modified |= simplifyConstTernaryOp();
        modified |= simplifyCalculations();
        validate();
    }

    // simplify redundant loops
    simplifyWhile0();
    removeRedundantFor();

    // Remove redundant parentheses in return..
    for (Token *tok = list.front(); tok; tok = tok->next()) {
        while (Token::simpleMatch(tok, "return (")) {
            Token *tok2 = tok->next()->link();
            if (Token::simpleMatch(tok2, ") ;")) {
                tok->deleteNext();
                tok2->deleteThis();
            } else {
                break;
            }
        }
    }

    removeRedundantAssignment();

    simplifyComma();

    removeRedundantSemicolons();

    simplifyFlowControl();

    simplifyRedundantConsecutiveBraces();

    simplifyMathFunctions();

    validate();

    Token::assignProgressValues(list.front());

    list.createAst();
    // needed for #7208 (garbage code) and #7724 (ast max depth limit)
    list.validateAst();

    // Create symbol database and then remove const keywords
    createSymbolDatabase();
    mSymbolDatabase->setValueTypeInTokenList();

    ValueFlow::setValues(&list, mSymbolDatabase, mErrorLogger, mSettings);

    if (mSettings->terminated())
        return false;

    printDebugOutput(2);

    return true;
}
//---------------------------------------------------------------------------

void Tokenizer::printDebugOutput(unsigned int simplification) const
{
    const bool debug = (simplification != 1U && mSettings->debugSimplified) ||
                       (simplification != 2U && mSettings->debugnormal);

    if (debug && list.front()) {
        list.front()->printOut(nullptr, list.getFiles());

        if (mSettings->xml)
            std::cout << "<debug>" << std::endl;

        if (mSymbolDatabase) {
            if (mSettings->xml)
                mSymbolDatabase->printXml(std::cout);
            else if (mSettings->verbose) {
                mSymbolDatabase->printOut("Symbol database");
            }
        }

        if (mSettings->verbose)
            list.front()->printAst(mSettings->verbose, mSettings->xml, std::cout);

        list.front()->printValueFlow(mSettings->xml, std::cout);

        if (mSettings->xml)
            std::cout << "</debug>" << std::endl;
    }

    if (mSymbolDatabase && simplification == 2U && mSettings->debugwarnings) {
        printUnknownTypes();

        // the typeStartToken() should come before typeEndToken()
        for (const Variable *var : mSymbolDatabase->variableList()) {
            if (!var)
                continue;

            const Token * typetok = var->typeStartToken();
            while (typetok && typetok != var->typeEndToken())
                typetok = typetok->next();

            if (typetok != var->typeEndToken()) {
                reportError(var->typeStartToken(),
                            Severity::debug,
                            "debug",
                            "Variable::typeStartToken() of variable '" + var->name() + "' is not located before Variable::typeEndToken(). The location of the typeStartToken() is '" + var->typeStartToken()->str() + "' at line " + MathLib::toString(var->typeStartToken()->linenr()));
            }
        }
    }
}

void Tokenizer::dump(std::ostream &out) const
{
    // Create a xml data dump.
    // The idea is not that this will be readable for humans. It's a
    // data dump that 3rd party tools could load and get useful info from.

    // tokens..
    out << "  <tokenlist>" << std::endl;
    for (const Token *tok = list.front(); tok; tok = tok->next()) {
        out << "    <token id=\"" << tok << "\" file=\"" << ErrorLogger::toxml(list.file(tok)) << "\" linenr=\"" << tok->linenr() << '\"';
        out << " str=\"" << ErrorLogger::toxml(tok->str()) << '\"';
        out << " scope=\"" << tok->scope() << '\"';
        if ( tok->tokType() == Token::eBracket )
          out << " bracket=\"True\"";
        if (tok->isName()) {
            out << " type=\"name\"";
            if (tok->isUnsigned())
                out << " isUnsigned=\"true\"";
            else if (tok->isSigned())
                out << " isSigned=\"true\"";
        } else if (tok->isNumber()) {
            out << " type=\"number\"";
            if (MathLib::isInt(tok->str()))
                out << " isInt=\"True\"";
            if (MathLib::isFloat(tok->str()))
                out << " isFloat=\"True\"";
        } else if (tok->tokType() == Token::eString)
            out << " type=\"string\" strlen=\"" << Token::getStrLength(tok) << '\"';
        else if (tok->tokType() == Token::eChar)
            out << " type=\"char\"";
        else if (tok->isBoolean())
            out << " type=\"boolean\"";
        else if (tok->isOp()) {
            out << " type=\"op\"";
            if (tok->isArithmeticalOp())
                out << " isArithmeticalOp=\"True\"";
            else if (tok->isAssignmentOp())
                out << " isAssignmentOp=\"True\"";
            else if (tok->isComparisonOp())
                out << " isComparisonOp=\"True\"";
            else if (tok->tokType() == Token::eLogicalOp)
                out << " isLogicalOp=\"True\"";
        }
        if (tok->link())
            out << " link=\"" << tok->link() << '\"';
        if (tok->varId() > 0U)
            out << " varId=\"" << MathLib::toString(tok->varId()) << '\"';
        if (tok->variable())
            out << " variable=\"" << tok->variable() << '\"';
        if (tok->function())
            out << " function=\"" << tok->function() << '\"';
        if (!tok->values().empty())
            out << " values=\"" << &tok->values() << '\"';
        if (tok->type())
            out << " type-scope=\"" << tok->type()->classScope << '\"';
        if (tok->astParent())
            out << " astParent=\"" << tok->astParent() << '\"';
        if (tok->astOperand1())
            out << " astOperand1=\"" << tok->astOperand1() << '\"';
        if (tok->astOperand2())
            out << " astOperand2=\"" << tok->astOperand2() << '\"';
        if (!tok->originalName().empty())
            out << " originalName=\"" << tok->originalName() << '\"';
        if (tok->valueType()) {
            const std::string vt = tok->valueType()->dump();
            if (!vt.empty())
                out << ' ' << vt;
        }
        out << "/>" << std::endl;
    }
    out << "  </tokenlist>" << std::endl;

    mSymbolDatabase->printXml(out);
    if (list.front())
        list.front()->printValueFlow(true, out);
}

//---------------------------------------------------------------------------

void Tokenizer::removeRedundantAssignment()
{
    for (Token *tok = list.front(); tok; tok = tok->next()) {
        if (tok->str() == "{")
            tok = tok->link();

        const Token * const start = const_cast<Token *>(startOfExecutableScope(tok));
        if (start) {
            tok = start->previous();
            // parse in this function..
            std::set<unsigned int> localvars;
            const Token * const end = tok->next()->link();
            for (Token * tok2 = tok->next(); tok2 && tok2 != end; tok2 = tok2->next()) {
                // skip local class or struct
                if (Token::Match(tok2, "class|struct %type% {|:")) {
                    // skip to '{'
                    tok2 = tok2->tokAt(2);
                    while (tok2 && tok2->str() != "{")
                        tok2 = tok2->next();

                    if (tok2)
                        tok2 = tok2->link(); // skip local class or struct
                    else
                        return;
                } else if (Token::Match(tok2, "[;{}] %type% * %name% ;") && tok2->next()->str() != "return") {
                    tok2 = tok2->tokAt(3);
                    localvars.insert(tok2->varId());
                } else if (Token::Match(tok2, "[;{}] %type% %name% ;") && tok2->next()->isStandardType()) {
                    tok2 = tok2->tokAt(2);
                    localvars.insert(tok2->varId());
                } else if (tok2->varId() &&
                           !Token::Match(tok2->previous(), "[;{}] %name% = %char%|%num%|%name% ;")) {
                    localvars.erase(tok2->varId());
                }
            }
            localvars.erase(0);
            if (!localvars.empty()) {
                for (Token *tok2 = tok->next(); tok2 && tok2 != end;) {
                    if (Token::Match(tok2, "[;{}] %type% %name% ;") && localvars.find(tok2->tokAt(2)->varId()) != localvars.end()) {
                        tok2->deleteNext(3);
                    } else if ((Token::Match(tok2, "[;{}] %type% * %name% ;") &&
                                localvars.find(tok2->tokAt(3)->varId()) != localvars.end()) ||
                               (Token::Match(tok2, "[;{}] %name% = %any% ;") &&
                                localvars.find(tok2->next()->varId()) != localvars.end())) {
                        tok2->deleteNext(4);
                    } else
                        tok2 = tok2->next();
                }
            }
        }
    }
}

void Tokenizer::simplifyFlowControl()
{
    for (Token *begin = list.front(); begin; begin = begin->next()) {

        if (Token::Match(begin, "(|[") ||
            (begin->str() == "{" && begin->previous() && begin->strAt(-1) == "="))
            begin = begin->link();

        //function scope
        if (!Token::simpleMatch(begin, ") {") && !Token::Match(begin, ") %name% {"))
            continue;

        Token* end = begin->linkAt(1+(begin->next()->str() == "{" ? 0 : 1));
        unsigned int indentLevel = 0;
        bool stilldead = false;

        for (Token *tok = begin; tok && tok != end; tok = tok->next()) {
            if (Token::Match(tok, "(|[")) {
                tok = tok->link();
                continue;
            }

            if (tok->str() == "{") {
                if (tok->previous() && tok->previous()->str() == "=") {
                    tok = tok->link();
                    continue;
                }
                ++indentLevel;
            } else if (tok->str() == "}") {
                if (indentLevel == 0)
                    break;
                --indentLevel;
                if (stilldead) {
                    eraseDeadCode(tok, nullptr);
                    if (indentLevel == 1 || tok->next()->str() != "}" || !Token::Match(tok->next()->link()->previous(), ";|{|}|do {"))
                        stilldead = false;
                    continue;
                }
            }

            if (indentLevel == 0)
                continue;

            if (Token::Match(tok,"continue|break ;")) {
                tok = tok->next();
                eraseDeadCode(tok, nullptr);

            } else if (Token::Match(tok,"return|goto") ||
                       (Token::Match(tok->previous(), "[;{}] %name% (") &&
                        mSettings->library.isnoreturn(tok)) ||
                       tok->str() == "throw") {
                if (tok->next()->str() == "}")
                    syntaxErrorDetail(tok->next()); // invalid code like in #6731
                //TODO: ensure that we exclude user-defined 'exit|abort|throw', except for 'noreturn'
                //catch the first ';'
                for (Token *tok2 = tok->next(); tok2; tok2 = tok2->next()) {
                    if (Token::Match(tok2, "(|[")) {
                        tok2 = tok2->link();
                    } else if (tok2->str() == ";") {
                        tok = tok2;
                        eraseDeadCode(tok, nullptr);
                        break;
                    } else if (Token::Match(tok2, "[{}]"))
                        break;
                }
                //if everything is removed, then remove also the code after an inferior scope
                //only if the actual scope is not special
                if (indentLevel > 1 && tok->next()->str() == "}" && Token::Match(tok->next()->link()->previous(), ";|{|}|do {"))
                    stilldead = true;
            }
        }
        begin = end;
    }
}


bool Tokenizer::removeRedundantConditions()
{
    // Return value for function. Set to true if there are any simplifications
    bool ret = false;

    for (Token *tok = list.front(); tok; tok = tok->next()) {
        if (!Token::Match(tok, "if ( %bool% ) {"))
            continue;

        // Find matching else
        Token *elseTag = tok->linkAt(4)->next();

        const bool boolValue = (tok->strAt(2) == "true");

        // Handle if with else
        if (Token::simpleMatch(elseTag, "else {")) {
            // Handle else
            if (!boolValue) {
                // Convert "if( false ) {aaa;} else {bbb;}" => "{bbb;}"

                //remove '(false)'
                tok->deleteNext(3);
                //delete dead code inside scope
                eraseDeadCode(tok, elseTag);
                //remove 'else'
                elseTag->deleteThis();
                //remove 'if'
                tok->deleteThis();
            } else {
                // Convert "if( true ) {aaa;} else {bbb;}" => "{aaa;}"
                const Token *end = elseTag->next()->link()->next();

                // Remove "else { bbb; }"
                elseTag = elseTag->previous();
                eraseDeadCode(elseTag, end);

                // Remove "if( true )"
                tok->deleteNext(3);
                tok->deleteThis();
            }

            ret = true;
        }

        // Handle if without else
        else {
            if (!boolValue) {
                //remove '(false)'
                tok->deleteNext(3);
                //delete dead code inside scope
                eraseDeadCode(tok, elseTag);
                //remove 'if'
                tok->deleteThis();
            } else {
                // convert "if( true ) {aaa;}" => "{aaa;}"
                tok->deleteNext(3);
                tok->deleteThis();
            }

            ret = true;
        }
    }

    return ret;
}

void Tokenizer::removeRedundantFor()
{
    for (Token *tok = list.front(); tok; tok = tok->next()) {
        if (Token::Match(tok, "[;{}] for ( %name% = %num% ; %name% < %num% ; ++| %name% ++| ) {") ||
            Token::Match(tok, "[;{}] for ( %type% %name% = %num% ; %name% < %num% ; ++| %name% ++| ) {")) {
            // Same variable name..
            const Token* varTok = tok->tokAt(3);
            const bool type = varTok->next()->isName();
            if (type)
                varTok = varTok->next();
            const std::string varname(varTok->str());
            const unsigned int varid(varTok->varId());
            if (varname != varTok->strAt(4))
                continue;
            const Token *vartok2 = tok->linkAt(2)->previous();
            if (vartok2->str() == "++")
                vartok2 = vartok2->previous();
            else if (vartok2->strAt(-1) != "++")
                continue;
            if (varname != vartok2->str())
                continue;

            // Check that the difference of the numeric values is 1
            const MathLib::bigint num1(MathLib::toLongNumber(varTok->strAt(2)));
            const MathLib::bigint num2(MathLib::toLongNumber(varTok->strAt(6)));
            if (num1 + 1 != num2)
                continue;

            // check how loop variable is used in loop..
            bool read = false;
            bool write = false;
            const Token* end = tok->linkAt(2)->next()->link();
            for (const Token *tok2 = tok->linkAt(2); tok2 != end; tok2 = tok2->next()) {
                if (tok2->str() == varname) {
                    if (tok2->previous()->isArithmeticalOp() &&
                        tok2->next() &&
                        (tok2->next()->isArithmeticalOp() || tok2->next()->str() == ";")) {
                        read = true;
                    } else {
                        read = write = true;
                        break;
                    }
                }
            }

            // Simplify loop if loop variable isn't written
            if (!write) {
                Token* bodyBegin = tok->linkAt(2)->next();
                // remove "for ("
                tok->deleteNext(2);

                // If loop variable is read then keep assignment before
                // loop body..
                if (type) {
                    tok->insertToken("{");
                    Token::createMutualLinks(tok->next(), bodyBegin->link());
                    bodyBegin->deleteThis();
                    tok = tok->tokAt(6);
                } else if (read) {
                    // goto ";"
                    tok = tok->tokAt(4);
                } else {
                    // remove "x = 0 ;"
                    tok->deleteNext(4);
                }

                // remove "x < 1 ; x ++ )"
                tok->deleteNext(7);

                if (!type) {
                    // Add assignment after the loop body so the loop variable
                    // get the correct end value
                    Token *tok2 = tok->next()->link();
                    tok2->insertToken(";");
                    tok2->insertToken(MathLib::toString(num2));
                    tok2->insertToken("=");
                    tok2->insertToken(varname);
                    tok2->next()->setVarId(varid);
                }
            }
        }
    }
}


void Tokenizer::removeRedundantSemicolons()
{
    for (Token *tok = list.front(); tok; tok = tok->next()) {
        if (tok->link() && tok->str() == "(") {
            tok = tok->link();
            continue;
        }
        for (;;) {
            if (Token::simpleMatch(tok, "; ;")) {
                tok->deleteNext();
            } else if (Token::simpleMatch(tok, "; { ; }")) {
                tok->deleteNext(3);
            } else {
                break;
            }
        }
    }
}


bool Tokenizer::simplifyAddBraces()
{
    for (Token *tok = list.front(); tok; tok = tok->next()) {
        Token const * tokRet=simplifyAddBracesToCommand(tok);
        if (!tokRet)
            return false;
    }
    return true;
}

Token *Tokenizer::simplifyAddBracesToCommand(Token *tok)
{
    Token * tokEnd=tok;
    if (Token::Match(tok,"for|switch|BOOST_FOREACH")) {
        tokEnd=simplifyAddBracesPair(tok,true);
    } else if (tok->str()=="while") {
        Token *tokPossibleDo=tok->previous();
        if (Token::simpleMatch(tok->previous(), "{"))
            tokPossibleDo = nullptr;
        else if (Token::simpleMatch(tokPossibleDo,"}"))
            tokPossibleDo = tokPossibleDo->link();
        if (!tokPossibleDo || tokPossibleDo->strAt(-1) != "do")
            tokEnd=simplifyAddBracesPair(tok,true);
    } else if (tok->str()=="do") {
        tokEnd=simplifyAddBracesPair(tok,false);
        if (tokEnd!=tok) {
            // walk on to next token, i.e. "while"
            // such that simplifyAddBracesPair does not close other braces
            // before the "while"
            if (tokEnd) {
                tokEnd=tokEnd->next();
                if (!tokEnd || tokEnd->str()!="while") // no while
                    syntaxErrorDetailMsg(tok, "does not close other braces before the 'while'");
            }
        }
    } else if (tok->str()=="if") {
        tokEnd=simplifyAddBracesPair(tok,true);
        if (!tokEnd)
            return nullptr;
        if (tokEnd->strAt(1) == "else") {
            Token * tokEndNextNext= tokEnd->tokAt(2);
            if (!tokEndNextNext || tokEndNextNext->str() == "}")
                syntaxErrorDetail(tokEndNextNext);
            if (tokEndNextNext->str() == "if")
                // do not change "else if ..." to "else { if ... }"
                tokEnd=simplifyAddBracesToCommand(tokEndNextNext);
            else
                tokEnd=simplifyAddBracesPair(tokEnd->next(),false);
        }
    }

    return tokEnd;
}

Token *Tokenizer::simplifyAddBracesPair(Token *tok, bool commandWithCondition)
{
    Token * tokCondition=tok->next();
    if (!tokCondition) // Missing condition
        return tok;

    Token *tokAfterCondition=tokCondition;
    if (commandWithCondition) {
        if (tokCondition->str()=="(")
            tokAfterCondition=tokCondition->link();
        else
            syntaxErrorDetailMsg(tok, "bad condition"); // Bad condition

        if (!tokAfterCondition || tokAfterCondition->strAt(1) == "]")
            syntaxErrorDetailMsg(tok, "bad condition"); // Bad condition

        tokAfterCondition=tokAfterCondition->next();
        if (!tokAfterCondition || Token::Match(tokAfterCondition, ")|}|,")) {
            // No tokens left where to add braces around
            return tok;
        }
    }
    Token * tokBracesEnd=nullptr;
    if (tokAfterCondition->str()=="{") {
        // already surrounded by braces
        tokBracesEnd=tokAfterCondition->link();
    } else if (Token::simpleMatch(tokAfterCondition, "try {") &&
               Token::simpleMatch(tokAfterCondition->linkAt(1), "} catch")) {
        tokAfterCondition->previous()->insertToken("{");
        Token * tokOpenBrace = tokAfterCondition->previous();
        Token * tokEnd = tokAfterCondition->linkAt(1)->linkAt(2)->linkAt(1);
        if (!tokEnd) {
            syntaxErrorDetail(tokAfterCondition);
        }
        tokEnd->insertToken("}");
        Token * tokCloseBrace = tokEnd->next();

        Token::createMutualLinks(tokOpenBrace, tokCloseBrace);
        tokBracesEnd = tokCloseBrace;
    } else if (Token::Match(tokAfterCondition, "%name% : {")) {
        tokAfterCondition->previous()->insertToken("{");
        tokAfterCondition->linkAt(2)->insertToken("}");
        tokBracesEnd = tokAfterCondition->linkAt(2)->next();
        Token::createMutualLinks(tokAfterCondition->previous(), tokBracesEnd);
    } else {
        Token * tokEnd = simplifyAddBracesToCommand(tokAfterCondition);
        if (!tokEnd) // Ticket #4887
            return tok;
        if (tokEnd->str()!="}") {
            // Token does not end with brace
            // Look for ; to add own closing brace after it
            while (tokEnd && !Token::Match(tokEnd, ";|)|}")) {
                if (tokEnd->tokType()==Token::eBracket || tokEnd->str() == "(") {
                    tokEnd = tokEnd->link();
                    if (!tokEnd) {
                        // Inner bracket does not close
                        return tok;
                    }
                }
                tokEnd=tokEnd->next();
            }
            if (!tokEnd || tokEnd->str() != ";") {
                // No trailing ;
                return tok;
            }
        }

        tokAfterCondition->previous()->insertToken("{");
        Token * tokOpenBrace=tokAfterCondition->previous();

        tokEnd->insertToken("}");
        Token * TokCloseBrace=tokEnd->next();

        Token::createMutualLinks(tokOpenBrace,TokCloseBrace);
        tokBracesEnd=TokCloseBrace;
    }

    return tokBracesEnd;
}

void Tokenizer::simplifyCompoundAssignment()
{
    // Simplify compound assignments:
    // "a+=b" => "a = a + b"
    for (Token *tok = list.front(); tok; tok = tok->next()) {
        if (!Token::Match(tok, "[;{}] (| *| (| %name%"))
            continue;
        if (tok->next()->str() == "return")
            continue;
        // backup current token..
        Token * const tok1 = tok;

        if (tok->next()->str() == "*")
            tok = tok->next();

        if (tok->next() && tok->next()->str() == "(") {
            tok = tok->next()->link()->next();
        } else {
            // variable..
            tok = tok->tokAt(2);
            while (Token::Match(tok, ". %name%") ||
                   Token::Match(tok, "[|(")) {
                if (tok->str() == ".")
                    tok = tok->tokAt(2);
                else {
                    // goto "]" or ")"
                    tok = tok->link();

                    // goto next token..
                    tok = tok ? tok->next() : nullptr;
                }
            }
        }
        if (!tok)
            break;

        // Is current token at a compound assignment: +=|-=|.. ?
        const std::string &str = tok->str();
        std::string op;  // operator used in assignment
        if (tok->isAssignmentOp() && str.size() == 2)
            op = str.substr(0, 1);
        else if (tok->isAssignmentOp() && str.size() == 3)
            op = str.substr(0, 2);
        else {
            tok = tok1;
            continue;
        }

        // Remove the whole statement if it says: "+=0;", "-=0;", "*=1;" or "/=1;"
        if (Token::Match(tok, "+=|-= 0 ;") ||
            Token::simpleMatch(tok, "|= 0 ;") ||
            Token::Match(tok, "*=|/= 1 ;")) {
            tok = tok1;
            while (tok->next()->str() != ";")
                tok->deleteNext();
        } else {
            // Enclose the rhs in parentheses..
            if (!Token::Match(tok->tokAt(2), "[;)]")) {
                // Only enclose rhs in parentheses if there is some operator
                bool someOperator = false;
                for (Token *tok2 = tok->next(); tok2; tok2 = tok2->next()) {
                    if (tok2->link() && Token::Match(tok2, "{|[|("))
                        tok2 = tok2->link();

                    if (Token::Match(tok2->next(), "[;)]")) {
                        if (someOperator) {
                            tok->insertToken("(");
                            tok2->insertToken(")");
                            Token::createMutualLinks(tok->next(), tok2->next());
                        }
                        break;
                    }

                    someOperator |= (tok2->isOp() || tok2->str() == "?");
                }
            }

            // simplify the compound assignment..
            tok->str("=");
            tok->insertToken(op);

            std::stack<Token *> tokend;
            for (Token *tok2 = tok->previous(); tok2 && tok2 != tok1; tok2 = tok2->previous()) {
                // Don't duplicate ++ and --. Put preincrement in lhs. Put
                // postincrement in rhs.
                if (tok2->tokType() == Token::eIncDecOp) {
                    // pre increment/decrement => don't copy
                    if (tok2->next()->isName()) {
                        continue;
                    }

                    // post increment/decrement => move from lhs to rhs
                    tok->insertToken(tok2->str());
                    tok2->deleteThis();
                    continue;
                }

                // Copy token from lhs to rhs
                tok->insertToken(tok2->str());
                tok->next()->setVarId(tok2->varId());
                if (Token::Match(tok->next(), "]|)|}|>"))
                    tokend.push(tok->next());
                else if (Token::Match(tok->next(), "(|[|{|<")) {
                    Token::createMutualLinks(tok->next(), tokend.top());
                    tokend.pop();
                }
            }
        }
    }
}

/// @todo add also TRUE,FALSE constants equivaalent to true,false
bool Tokenizer::simplifyConditions()
{
    bool ret = false;

    for (Token *tok = list.front(); tok; tok = tok->next()) {
        if (Token::Match(tok, "! %bool%|%num%")) {
            tok->deleteThis();
            if (Token::Match(tok, "0|false"))
                tok->str("true");
            else
                tok->str("false");

            ret = true;
        }

        if (Token::simpleMatch(tok, "&& true &&")) {
            tok->deleteNext(2);
            ret = true;
        }

        else if (Token::simpleMatch(tok, "|| false ||")) {
            tok->deleteNext(2);
            ret = true;
        }

        else if (Token::Match(tok, "(|&& true && true &&|)")) {
            tok->deleteNext(2);
            ret = true;
        }

        else if (Token::Match(tok, "%oror%|( false %oror% false %oror%|)")) {
            tok->deleteNext(2);
            ret = true;
        }

        else if (Token::simpleMatch(tok, "( true ||") ||
                 Token::simpleMatch(tok, "( false &&")) {
            Token::eraseTokens(tok->next(), tok->link());
            ret = true;
        }

        else if (Token::simpleMatch(tok, "|| true )") ||
                 Token::simpleMatch(tok, "&& false )")) {
            tok = tok->next();
            Token::eraseTokens(tok->next()->link(), tok);
            ret = true;
        }

        else if (Token::simpleMatch(tok, "&& false &&") ||
                 Token::simpleMatch(tok, "|| true ||")) {
            //goto '('
            Token *tok2 = tok;
            while (tok2->previous()) {
                if (tok2->previous()->str() == ")")
                    tok2 = tok2->previous()->link();
                else {
                    tok2 = tok2->previous();
                    if (tok2->str() == "(")
                        break;
                }
            }
            if (!tok2)
                continue;
            //move tok to 'true|false' position
            tok = tok->next();
            //remove everything before 'true|false'
            Token::eraseTokens(tok2, tok);
            //remove everything after 'true|false'
            Token::eraseTokens(tok, tok2->link());
            ret = true;
        }

        // Change numeric constant in condition to "true" or "false"
        if (Token::Match(tok, "if|while ( %num% )|%oror%|&&")) {
            tok->tokAt(2)->str((tok->strAt(2) != "0") ? "true" : "false");
            ret = true;
        }
        if (Token::Match(tok, "&&|%oror% %num% )|%oror%|&&")) {
            tok->next()->str((tok->next()->str() != "0") ? "true" : "false");
            ret = true;
        }

        // Reduce "(%num% == %num%)" => "(true)"/"(false)"
        if (Token::Match(tok, "&&|%oror%|(") &&
            (Token::Match(tok->next(), "%num% %any% %num%") ||
             Token::Match(tok->next(), "%bool% %any% %bool%")) &&
            Token::Match(tok->tokAt(4), "&&|%oror%|)|?")) {
            std::string cmp = tok->strAt(2);
            bool result = false;
            if (tok->next()->isNumber()) {
                // Compare numbers

                if (cmp == "==" || cmp == "!=") {
                    const std::string& op1(tok->next()->str());
                    const std::string& op2(tok->strAt(3));

                    bool eq = false;
                    if (MathLib::isInt(op1) && MathLib::isInt(op2))
                        eq = (MathLib::toLongNumber(op1) == MathLib::toLongNumber(op2));
                    else {
                        eq = (op1 == op2);

                        // It is inconclusive whether two unequal float representations are numerically equal
                        if (!eq && MathLib::isFloat(op1))
                            cmp.clear();
                    }

                    if (cmp == "==")
                        result = eq;
                    else
                        result = !eq;
                } else {
                    const double op1 = MathLib::toDoubleNumber(tok->next()->str());
                    const double op2 = MathLib::toDoubleNumber(tok->strAt(3));
                    if (cmp == ">=")
                        result = (op1 >= op2);
                    else if (cmp == ">")
                        result = (op1 > op2);
                    else if (cmp == "<=")
                        result = (op1 <= op2);
                    else if (cmp == "<")
                        result = (op1 < op2);
                    else
                        cmp.clear();
                }
            } else {
                // Compare boolean
                const bool op1 = (tok->next()->str() == std::string("true"));
                const bool op2 = (tok->strAt(3) == std::string("true"));

                if (cmp == "==")
                    result = (op1 == op2);
                else if (cmp == "!=")
                    result = (op1 != op2);
                else if (cmp == ">=")
                    result = (op1 >= op2);
                else if (cmp == ">")
                    result = (op1 > op2);
                else if (cmp == "<=")
                    result = (op1 <= op2);
                else if (cmp == "<")
                    result = (op1 < op2);
                else
                    cmp.clear();
            }

            if (! cmp.empty()) {
                tok = tok->next();
                tok->deleteNext(2);

                tok->str(result ? "true" : "false");
                ret = true;
            }
        }
    }

    return ret;
}

bool Tokenizer::simplifyConstTernaryOp()
{
    bool ret = false;
    const Token *templateParameterEnd = nullptr; // The end of the current template parameter list, if any
    for (Token *tok = list.front(); tok; tok = tok->next()) {
        if (tok == templateParameterEnd)
            templateParameterEnd = nullptr; // End of the current template parameter list
        if (tok->str() != "?")
            continue;

        if (!Token::Match(tok->tokAt(-2), "<|=|,|(|[|{|}|;|case|return %bool%|%num%") &&
            !Token::Match(tok->tokAt(-4), "<|=|,|(|[|{|}|;|case|return ( %bool%|%num% )"))
            continue;

        const int offset = (tok->previous()->str() == ")") ? 2 : 1;

        // Find the token ":" then go to the next token
        Token *colon = skipTernaryOp(tok);
        if (!colon || colon->previous()->str() != ":" || !colon->next())
            continue;

        //handle the GNU extension: "x ? : y" <-> "x ? x : y"
        if (colon->previous() == tok->next())
            tok->insertToken(tok->strAt(-offset));

        // go back before the condition, if possible
        tok = tok->tokAt(-2);
        if (offset == 2) {
            // go further back before the "("
            tok = tok->tokAt(-2);
            //simplify the parentheses
            tok->deleteNext();
            tok->next()->deleteNext();
        }

        if (Token::Match(tok->next(), "false|0")) {
            // Use code after colon, remove code before it.
            Token::eraseTokens(tok, colon);

            tok = tok->next();
            ret = true;
        }

        // The condition is true. Delete the operator after the ":"..
        else {
            // delete the condition token and the "?"
            tok->deleteNext(2);

            unsigned int ternaryOplevel = 0;
            for (const Token *endTok = colon; endTok; endTok = endTok->next()) {
                if (Token::Match(endTok, "(|[|{")) {
                    endTok = endTok->link();
                }

                else if (endTok->str() == "?")
                    ++ternaryOplevel;
                else if (Token::Match(endTok, ")|}|]|;|,|:|>")) {
                    if (endTok->str() == ":" && ternaryOplevel)
                        --ternaryOplevel;
                    else if (endTok->str() == ">" && !templateParameterEnd)
                        ;
                    else {
                        Token::eraseTokens(colon->tokAt(-2), endTok);
                        ret = true;
                        break;
                    }
                }
            }
        }
    }

    return ret;
}

void Tokenizer::simplifyUndefinedSizeArray()
{
    for (Token *tok = list.front(); tok; tok = tok->next()) {
        if (Token::Match(tok, "%type%")) {
            Token *tok2 = tok->next();
            while (tok2 && tok2->str() == "*")
                tok2 = tok2->next();
            if (!Token::Match(tok2, "%name% [ ] ;|["))
                continue;

            tok = tok2->previous();
            Token *end = tok2->next();
            unsigned int count = 0;
            do {
                end = end->tokAt(2);
                ++count;
            } while (Token::Match(end, "[ ] [;=[]"));
            if (Token::Match(end, "[;=]")) {
                do {
                    tok2->deleteNext(2);
                    tok->insertToken("*");
                } while (--count);
                tok = end;
            } else
                tok = tok->tokAt(3);
        }
    }
}

void Tokenizer::simplifyCasts()
{
    for (Token *tok = list.front(); tok; tok = tok->next()) {
        // Don't remove cast in such cases:
        // *((char *)a + 1) = 0;
        // Remove cast when casting a function pointer:
        // (*(void (*)(char *))fp)(x);
        if (!tok->isName() &&
            Token::simpleMatch(tok->next(), "* (") &&
            !Token::Match(tok->linkAt(2), ") %name%|&")) {
            tok = tok->linkAt(2);
            continue;
        }
        // #3935 : don't remove cast in such cases:
        // ((char *)a)[1] = 0;
        if (tok->str() == "(" && Token::simpleMatch(tok->link(), ") [")) {
            tok = tok->link();
            continue;
        }
        // #4164 : ((unsigned char)1) => (1)
        if (Token::Match(tok->next(), "( %type% ) %num%") && tok->next()->link()->previous()->isStandardType()) {
            const MathLib::bigint value = MathLib::toLongNumber(tok->next()->link()->next()->str());
            unsigned int bits = mSettings->char_bit * mTypeSize[tok->next()->link()->previous()->str()];
            if (!tok->tokAt(2)->isUnsigned() && bits > 0)
                bits--;
            if (bits < 31 && value >= 0 && value < (1LL << bits)) {
                tok->linkAt(1)->next()->isCast(true);
                Token::eraseTokens(tok, tok->next()->link()->next());
            }
            continue;
        }

        while ((Token::Match(tok->next(), "( %type% *| *| *|&| ) *|&| %name%") && (tok->str() != ")" || tok->tokAt(2)->isStandardType())) ||
               Token::Match(tok->next(), "( const| %type% * *| *|&| ) *|&| %name%") ||
               Token::Match(tok->next(), "( const| %type% %type% *| *| *|&| ) *|&| %name%") ||
               (!tok->isName() && (Token::Match(tok->next(), "( %type% * *| *|&| ) (") ||
                                   Token::Match(tok->next(), "( const| %type% %type% * *| *|&| ) (")))) {
            if (tok->isName() && tok->str() != "return")
                break;

            if (tok->strAt(-1) == "operator")
                break;

            // Remove cast..
            Token::eraseTokens(tok, tok->next()->link()->next());

            // Set isCasted flag.
            Token *tok2 = tok->next();
            if (!Token::Match(tok2, "%name% [|."))
                tok2->isCast(true);
            else {
                // TODO: handle more complex expressions
                tok2->next()->isCast(true);
            }

            if (tok->str() == ")" && tok->link()->previous()) {
                // If there was another cast before this, go back
                // there to check it also. e.g. "(int)(char)x"
                tok = tok->link()->previous();
            }
        }
    }
}

bool Tokenizer::simplifyFunctionReturn()
{
    std::map<std::string, const Token*> functions;

    for (const Token *tok = tokens(); tok; tok = tok->next()) {
        if (tok->str() == "{")
            tok = tok->link();

        else if (Token::Match(tok, "%name% ( ) { return %bool%|%char%|%num%|%str% ; }") && tok->strAt(-1) != "::") {
            const Token* const any = tok->tokAt(5);
            functions[tok->str()] = any;
            tok = any;
        }
    }

    if (functions.empty())
        return false;

    bool ret = false;
    for (Token *tok = list.front(); tok; tok = tok->next()) {
        if (Token::Match(tok, "(|[|=|return|%op% %name% ( ) ;|]|)|%cop%")) {
            tok = tok->next();
            auto it = functions.find(tok->str());
            if (it != functions.cend()) {
                tok->str(it->second->str());
                tok->deleteNext(2);
                ret = true;
            }
        }
    }

    return ret;
}

void Tokenizer::simplifyVarDecl(const bool only_k_r_fpar)
{
    simplifyVarDecl(list.front(), nullptr, only_k_r_fpar);
}

void Tokenizer::simplifyVarDecl(Token * tokBegin, const Token * const tokEnd, const bool only_k_r_fpar)
{
    // Split up variable declarations..
    // "int a=4;" => "int a; a=4;"
    bool finishedwithkr = true;
    for (Token *tok = tokBegin; tok != tokEnd; tok = tok->next()) {
        if (Token::simpleMatch(tok, "= {")) {
            tok = tok->next()->link();
        }
        if (!tok) {
            syntaxErrorDetail(tokBegin);
        }
        if (only_k_r_fpar && finishedwithkr) {
            if (Token::Match(tok, "(|[|{")) {
                tok = tok->link();
                if (tok->next() && Token::Match(tok, ") !!{"))
                    tok = tok->next();
                else
                    continue;
            } else
                continue;
        } else if (tok->str() == "(") {

            tok = tok->link();
        }

        if (!tok)
            syntaxErrorDetail(nullptr); // #7043 invalid code
        if (tok->previous() && !Token::Match(tok->previous(), "{|}|;|)|public:|protected:|private:"))
            continue;


        Token *type0 = tok;
        if (!Token::Match(type0, "::| %type%"))
            continue;
        if (Token::Match(type0, "else|return|public:|protected:|private:"))
            continue;
        if (type0->str() == "using")
            continue;

        bool isconst = false;
        bool isstatic = false;
        Token *tok2 = type0;
        unsigned int typelen = 1;

        if (Token::Match(tok2, "::")) {
            tok2 = tok2->next();
            typelen++;
        }

        //check if variable is declared 'const' or 'static' or both
        while (tok2) {
            if (!Token::Match(tok2, "const|static") && Token::Match(tok2, "%type% const|static")) {
                tok2 = tok2->next();
                ++typelen;
            }

            if (tok2->str() == "const")
                isconst = true;

            else if (tok2->str() == "static")
                isstatic = true;

            else if (Token::Match(tok2, "%type% :: %type%")) {
                tok2 = tok2->next();
                ++typelen;
            }

            else
                break;

            if (Token::Match(tok2->next(), "& %name% ,"))
                break;

            tok2 = tok2->next();
            ++typelen;
        }

        // strange looking variable declaration => don't split up.
        if (Token::Match(tok2, "%type% *|&| %name% , %type% *|&| %name%"))
            continue;

        if (Token::Match(tok2, "struct|class %type%")) {
            tok2 = tok2->next();
            ++typelen;
        }

        // check for qualification..
        if (Token::Match(tok2,  ":: %type%")) {
            ++typelen;
            tok2 = tok2->next();
        }

        //skip combinations of templates and namespaces
        while ( (Token::Match(tok2, "%type% <") || Token::Match(tok2, "%type% ::")) ) {
            typelen += 2;
            tok2 = tok2->tokAt(2);
            if (tok2 && tok2->previous()->str() == "::")
                continue;
            unsigned int indentlevel = 0;
            unsigned int parens = 0;

            for (Token *tok3 = tok2; tok3; tok3 = tok3->next()) {
                ++typelen;

                if (!parens && tok3->str() == "<") {
                    ++indentlevel;
                } else if (!parens && tok3->str() == ">") {
                    if (indentlevel == 0) {
                        tok2 = tok3->next();
                        break;
                    }
                    --indentlevel;
                } else if (!parens && tok3->str() == ">>") {
                    if (indentlevel <= 1U) {
                        tok2 = tok3->next();
                        break;
                    }
                    indentlevel -= 2;
                } else if (tok3->str() == "(") {
                    ++parens;
                } else if (tok3->str() == ")") {
                    if (!parens) {
                        tok2 = nullptr;
                        break;
                    }
                    --parens;
                } else if (tok3->str() == ";") {
                    break;
                }
            }

            if (Token::Match(tok2,  ":: %type%")) {
                ++typelen;
                tok2 = tok2->next();
            }
        }

        //pattern: "%type% *| ... *| const| %name% ,|="
        if (Token::Match(tok2, "%type%") ||
            (tok2 && tok2->previous() && tok2->previous()->str() == ">")) {
            Token *varName = tok2;
            if (!tok2->previous() || tok2->previous()->str() != ">")
                varName = varName->next();
            else
                --typelen;
            //skip all the pointer part
            bool isReference = false;
            while ( Token::Match(varName, "& %name% ,")) {
                isReference = true;
                varName = varName->next();
            }

            while (Token::Match(varName, "%type% %type%")) {
                if (varName->str() != "const") {
                    ++typelen;
                }
                varName = varName->next();
            }
            //non-VLA case
            if (Token::Match(varName, "%name% ,|=")) {
                if (varName->str() != "operator") {
                    tok2 = varName->next(); // The ',' or '=' token

                    if (tok2->str() == "=" && (isstatic || (isconst && !isReference))) {
                        //do not split const non-pointer variables..
                        while (tok2 && tok2->str() != "," && tok2->str() != ";") {
                            if (Token::Match(tok2, "{|(|["))
                                tok2 = tok2->link();

                            if (!tok2)
                                syntaxErrorDetail(nullptr); // #6881 invalid code
                            tok2 = tok2->next();
                        }
                        if (tok2 && tok2->str() == ";")
                            tok2 = nullptr;
                    }
                } else
                    tok2 = nullptr;
            }

            // brace initialization
            else if (Token::Match(varName, "%name% {")) {
                tok2 = varName->next();
                tok2 = tok2->link();
                if (tok2)
                    tok2 = tok2->next();
                if (tok2 && tok2->str() != ",")
                    tok2 = nullptr;
            }

            /// @todo bad idea for ctrl lang
            // parenthesis, functions can't be declared like:
            // int f1(a,b), f2(c,d);
            // so if there is a comma assume this is a variable declaration
            else if (Token::Match(varName, "%name% (") && Token::simpleMatch(varName->linkAt(1), ") ,")) {
           // activae this error     syntaxErrorDetailMsg(tok, "parenthesis, functions can't be declared");
                tok2 = varName->linkAt(1)->next();
            }

            else
                tok2 = nullptr;
        } else {
            tok2 = nullptr;
        }

        if (!tok2) {
            if (only_k_r_fpar)
                finishedwithkr = false;
            continue;
        }

        if (tok2->str() == ",") {
            tok2->str(";");
            list.insertTokens(tok2, type0, typelen);
        }

        else {
            Token *eq = tok2;

            while (tok2) {
                if (Token::Match(tok2, "{|("))
                    tok2 = tok2->link();

                else if (std::strchr(";,", tok2->str()[0])) {
                    // "type var ="   =>   "type var; var ="
                    const Token *varTok = type0->tokAt((int)typelen);
                    while (Token::Match(varTok, "*|&|const"))
                        varTok = varTok->next();
                    if (!varTok)
                        syntaxErrorDetail(tok2); // invalid code
                    list.insertTokens(eq, varTok, 2);
                    eq->str(";");

                    // "= x, "   =>   "= x; type "
                    if (tok2->str() == ",") {
                        tok2->str(";");
                        list.insertTokens(tok2, type0, typelen);
                    }
                    break;
                }
                if (tok2)
                    tok2 = tok2->next();
            }
        }
        finishedwithkr = (only_k_r_fpar && tok2 && tok2->strAt(1) == "{");
    }
}

void Tokenizer::simplifyStaticConst()
{
    // This function will simplify the token list so that the qualifiers static"
    // and "const" appear in the same order as in the array below.
    const std::string qualifiers[] = {"static", "const"};

    // Move 'const' before all other qualifiers and types and then
    // move 'static' before all other qualifiers and types, ...
    for (Token *tok = list.front(); tok; tok = tok->next()) {
        bool continue2 = false;
        for (size_t i = 0; i < sizeof(qualifiers)/sizeof(qualifiers[0]); i++) {

            // Keep searching for a qualifier
            if (!tok->next() || tok->next()->str() != qualifiers[i])
                continue;

            // Look backwards to find the beginning of the declaration
            Token* leftTok = tok;
            bool behindOther = false;
            for (; leftTok; leftTok = leftTok->previous()) {
                for (size_t j = 0; j <= i; j++) {
                    if (leftTok->str() == qualifiers[j]) {
                        behindOther = true;
                        break;
                    }
                }
                if (behindOther)
                    break;
                if (!Token::Match(leftTok, "%type%|struct|::") ||
                    (Token::Match(leftTok, "private|protected|public|operator"))) {
                    break;
                }
            }

            // The token preceding the declaration should indicate the start of a declaration
            if (leftTok == tok)
                continue;

            if (leftTok && !behindOther && !Token::Match(leftTok, ";|{|}|(|,|private|protected|public")) {
                continue2 = true;
                break;
            }

            // Move the qualifier to the left-most position in the declaration
            tok->deleteNext();
            if (!leftTok) {
                list.front()->insertToken(qualifiers[i], emptyString, false);
                list.front()->swapWithNext();
                tok = list.front();
            } else if (leftTok->next()) {
                leftTok->next()->insertToken(qualifiers[i], emptyString, true);
                tok = leftTok->next();
            } else {
                leftTok->insertToken(qualifiers[i]);
                tok = leftTok;
            }
        }
        if (continue2)
            continue;
    }
}

void Tokenizer::simplifyIfAndWhileAssign()
{
    for (Token *tok = list.front(); tok; tok = tok->next()) {
        if (!Token::Match(tok->next(), "if|while ("))
            continue;

        const Token* tokAt3 = tok->tokAt(3);
        if (!Token::Match(tokAt3, "!| (| %name% =") &&
            !Token::Match(tokAt3, "!| (| %name% . %name% =") &&
            !Token::Match(tokAt3, "0 == (| %name% =") &&
            !Token::Match(tokAt3, "0 == (| %name% . %name% ="))
            continue;

        // simplifying a "while(cond) { }" condition ?
        const bool iswhile(tok->next()->str() == "while");

        // simplifying a "do { } while(cond);" condition ?
        const bool isDoWhile = iswhile && Token::simpleMatch(tok, "}") && Token::simpleMatch(tok->link()->previous(), "do");
        Token* openBraceTok = tok->link();

        // delete the "if|while"
        tok->deleteNext();

        // Remember if there is a "!" or not. And delete it if there are.
        const bool isNot(Token::Match(tok->tokAt(2), "!|0"));
        if (isNot)
            tok->next()->deleteNext((tok->strAt(2) == "0") ? 2 : 1);

        // Delete parentheses.. and remember how many there are with
        // their links.
        std::stack<Token *> braces;
        while (tok->next()->str() == "(") {
            braces.push(tok->next()->link());
            tok->deleteNext();
        }

        // Skip the "%name% = ..."
        Token *tok2;
        for (tok2 = tok->next(); tok2; tok2 = tok2->next()) {
            if (tok2->str() == "(")
                tok2 = tok2->link();
            else if (tok2->str() == ")")
                break;
        }

        // Insert "; if|while ( .."
        tok2 = tok2->previous();
        if (tok->strAt(2) == ".") {
            tok2->insertToken(tok->strAt(3));
            tok2->next()->setVarId(tok->tokAt(3)->varId());
            tok2->insertToken(".");
        }
        tok2->insertToken(tok->next()->str());
        tok2->next()->setVarId(tok->next()->varId());

        while (! braces.empty()) {
            tok2->insertToken("(");
            Token::createMutualLinks(tok2->next(), braces.top());
            braces.pop();
        }

        if (isNot)
            tok2->next()->insertToken("!");
        tok2->insertToken(iswhile ? "while" : "if");
        if (isDoWhile) {
            tok2->insertToken("}");
            Token::createMutualLinks(openBraceTok, tok2->next());
        }

        tok2->insertToken(";");

        // delete the extra "}"
        if (isDoWhile)
            tok->deleteThis();

        // If it's a while loop, insert the assignment in the loop
        if (iswhile && !isDoWhile) {
            unsigned int indentlevel = 0;
            Token *tok3 = tok2;

            for (; tok3; tok3 = tok3->next()) {
                if (tok3->str() == "{")
                    ++indentlevel;
                else if (tok3->str() == "}") {
                    if (indentlevel <= 1)
                        break;
                    --indentlevel;
                }
            }

            if (tok3 && indentlevel == 1) {
                tok3 = tok3->previous();
                std::stack<Token *> braces2;

                for (tok2 = tok2->next(); tok2 && tok2 != tok; tok2 = tok2->previous()) {
                    tok3->insertToken(tok2->str());
                    Token *newTok = tok3->next();

                    newTok->setVarId(tok2->varId());
                    newTok->fileIndex(tok2->fileIndex());
                    newTok->linenr(tok2->linenr());

                    // link() new tokens manually
                    if (tok2->link()) {
                        if (Token::Match(newTok, "}|)|]|>")) {
                            braces2.push(newTok);
                        } else {
                            Token::createMutualLinks(newTok, braces2.top());
                            braces2.pop();
                        }
                    }
                }
            }
        }
    }
}

void Tokenizer::simplifyVariableMultipleAssign()
{
    for (Token *tok = list.front(); tok; tok = tok->next()) {
        if (Token::Match(tok, "%name% = %name% = %num%|%name% ;")) {
            // skip intermediate assignments
            Token *tok2 = tok->previous();
            while (tok2 &&
                   tok2->str() == "=" &&
                   Token::Match(tok2->previous(), "%name%")) {
                tok2 = tok2->tokAt(-2);
            }

            if (!tok2 || tok2->str() != ";") {
                continue;
            }

            Token *stopAt = tok->tokAt(2);
            const Token *valueTok = stopAt->tokAt(2);
            const std::string& value(valueTok->str());
            tok2 = tok2->next();

            while (tok2 != stopAt) {
                tok2->next()->insertToken(";");
                tok2->next()->insertToken(value);
                tok2 = tok2->tokAt(4);
            }
        }
    }
}



bool Tokenizer::simplifyKnownVariables()
{
    // return value for function. Set to true if any simplifications are made
    bool ret = false;

    // constants..
    {
        std::unordered_map<unsigned int, std::string> constantValues;
        std::map<unsigned int, Token*> constantVars;
        std::unordered_map<unsigned int, std::list<Token*>> constantValueUsages;
        bool goback = false;
        for (Token *tok = list.front(); tok; tok = tok->next()) {
            if (goback) {
                tok = tok->previous();
                goback = false;
            }
            // Reference to variable
            if (Token::Match(tok, "%type%|* & %name% = %name% ;")) {
                Token *start = tok->previous();
                while (Token::Match(start,"%type%|*|&"))
                    start = start->previous();
                if (!Token::Match(start,"[;{}]"))
                    continue;
                const Token *reftok = tok->tokAt(2);
                const Token *vartok = reftok->tokAt(2);
                int level = 0;
                for (Token *tok2 = tok->tokAt(6); tok2; tok2 = tok2->next()) {
                    if (tok2->str() == "{") {
                        ++level;
                    } else if (tok2->str() == "}") {
                        if (level <= 0)
                            break;
                        --level;
                    } else if (tok2->varId() == reftok->varId()) {
                        tok2->str(vartok->str());
                        tok2->setVarId(vartok->varId());
                    }
                }
                Token::eraseTokens(start, tok->tokAt(6));
                tok = start;
            }

            if (tok->isName() && (Token::Match(tok, "static| const| static| %type% const| %name% = %any% ;") ||
                                  Token::Match(tok, "static| const| static| %type% const| %name% ( %any% ) ;"))) {
                bool isconst = false;
                for (const Token *tok2 = tok; (tok2->str() != "=") && (tok2->str() != "("); tok2 = tok2->next()) {
                    if (tok2->str() == "const") {
                        isconst = true;
                        break;
                    }
                }
                if (!isconst)
                    continue;

                Token *tok1 = tok;

                // start of statement
                if (tok != list.front() && !Token::Match(tok->previous(),";|{|}|private|protected|public"))
                    continue;
                // skip "const" and "static"
                while (Token::Match(tok, "const|static"))
                    tok = tok->next();
                // pod type
                if (!tok->isStandardType())
                    continue;

                Token * const vartok = (tok->next() && tok->next()->str() == "const") ? tok->tokAt(2) : tok->next();
                const Token * const valuetok = vartok->tokAt(2);
                if (Token::Match(valuetok, "%bool%|%char%|%num%|%str% )| ;")) {
                    // record a constant value for this variable
                    constantValues[vartok->varId()] = valuetok->str();
                    constantVars[vartok->varId()] = tok1;
                }
            } else if (tok->varId()) {
                // find the entry for the known variable, if any.  Exclude the location where the variable is assigned with next == "="
                if (constantValues.find(tok->varId()) != constantValues.end() && tok->next()->str() != "=") {
                    constantValueUsages[tok->varId()].push_back(tok);
                }
            }
        }

        for (auto constantVar = constantVars.rbegin(); constantVar != constantVars.rend(); constantVar++) {
            bool referenceFound = false;
            std::list<Token*> usageList = constantValueUsages[constantVar->first];
            for (Token* usage : usageList) {
                // check if any usages of each known variable are a reference
                if (Token::Match(usage->tokAt(-2), "(|[|,|{|return|%op% & %varid%", constantVar->first)) {
                    referenceFound = true;
                    break;
                }
            }

            if (!referenceFound) {
                // replace all usages of non-referenced known variables with their value
                for (Token* usage : usageList) {
                    usage->str(constantValues[constantVar->first]);
                }

                Token* startTok = constantVar->second;
                // remove variable assignment statement
                while (startTok->next()->str() != ";")
                    startTok->deleteNext();
                startTok->deleteNext();

                // #8579 if we can we want another token to delete startTok. if we can't it doesn't matter
                if (startTok->previous()) {
                    startTok->previous()->deleteNext();
                } else if (startTok->next()) {
                    startTok->next()->deletePrevious();
                } else {
                    startTok->deleteThis();
                }
                startTok = nullptr;

                constantVar->second = nullptr;
                ret = true;
            }
        }
    }

    // variable id for local, float/double, array variables
    std::set<unsigned int> localvars;
    std::set<unsigned int> floatvars;
    std::set<unsigned int> arrays;

    // auto variables..
    for (Token *tok = list.front(); tok; tok = tok->next()) {
        // Search for a block of code
        Token * const start = const_cast<Token *>(startOfExecutableScope(tok));
        if (!start)
            continue;

        for (const Token *tok2 = start->previous(); tok2 && !Token::Match(tok2, "[;{}]"); tok2 = tok2->previous()) {
            if (tok2->varId() != 0)
                localvars.insert(tok2->varId());
        }

        tok = start;
        // parse the block of code..
        int indentlevel = 0;
        Token *tok2 = tok;
        for (; tok2; tok2 = tok2->next()) {
            if (Token::Match(tok2, "[;{}] %type% %name%|*")) {
                bool isfloat = false;
                const Token *vartok = tok2->next();
                while (Token::Match(vartok, "%name%|* %name%|*")) {
                    if (Token::Match(vartok, "float|double"))
                        isfloat = true;
                    vartok = vartok->next();
                }
                if (Token::Match(vartok, "%var% ;|["))
                    localvars.insert(vartok->varId());
                if (isfloat && Token::Match(vartok, "%var% ;"))
                    floatvars.insert(vartok->varId());
                if (Token::Match(vartok, "%var% ["))
                    arrays.insert(vartok->varId());
            }

            if (tok2->str() == "{")
                ++indentlevel;

            else if (tok2->str() == "}") {
                --indentlevel;
                if (indentlevel <= 0)
                    break;
            }

            else if (Token::simpleMatch(tok2, "for ("))
                tok2 = tok2->next()->link();

            else if (tok2->previous()->str() != "*" && !Token::Match(tok2->tokAt(-2), "* --|++") &&
                     (Token::Match(tok2, "%name% = %bool%|%char%|%num%|%str%|%name% ;") ||
                      Token::Match(tok2, "%name% [ %num%| ] = %str% ;") ||
                      Token::Match(tok2, "%name% = & %name% ;") ||
                      (Token::Match(tok2, "%name% = & %name% [ 0 ] ;") && arrays.find(tok2->tokAt(3)->varId()) != arrays.end()))) {
                const unsigned int varid = tok2->varId();
                if (varid == 0)
                    continue;

                if (Token::Match(tok2->previous(), "[;{}]") && localvars.find(varid) == localvars.end())
                    continue;

                // initialization of static variable => the value is not *known*
                {
                    bool isstatic = false;
                    const Token *decl = tok2->previous();
                    while (decl && decl->isName()) {
                        if (decl->str() == "static") {
                            isstatic = true;
                            break;
                        }
                        decl = decl->previous();
                    }
                    if (isstatic)
                        continue;
                }

                // skip loop variable
                if (Token::Match(tok2->tokAt(-2), "(|:: %type%")) {
                    const Token *tok3 = tok2->previous();
                    do {
                        tok3 = tok3->tokAt(-2);
                    } while (Token::Match(tok3->previous(), ":: %type%"));
                    if (Token::Match(tok3->tokAt(-2), "for ( %type%"))
                        continue;
                }

                // struct name..
                if (Token::Match(tok2, "%varid% = &| %varid%", tok2->varId()))
                    continue;

                const std::string structname = Token::Match(tok2->tokAt(-3), "[;{}] %name% .") ?
                                               std::string(tok2->strAt(-2) + " .") :
                                               std::string();

                const Token * const valueToken = tok2->tokAt(2);

                std::string value;
                unsigned int valueVarId = 0;

                Token *tok3 = nullptr;

                // there could be a hang here if tok2 is moved back by the function calls below for some reason
                if (mSettings->terminated())
                    return false;

                if (!simplifyKnownVariablesGetData(varid, &tok2, &tok3, value, valueVarId, floatvars.find(tok2->varId()) != floatvars.end()))
                    continue;

                if (valueVarId > 0 && arrays.find(valueVarId) != arrays.end())
                    continue;

                ret |= simplifyKnownVariablesSimplify(&tok2, tok3, varid, structname, value, valueVarId, valueToken, indentlevel);
            }

            else if (Token::Match(tok2, "strcpy|sprintf ( %name% , %str% ) ;")) {
                const unsigned int varid(tok2->tokAt(2)->varId());
                if (varid == 0)
                    continue;

                const Token * const valueToken = tok2->tokAt(4);
                std::string value(valueToken->str());
                if (tok2->str() == "sprintf") {
                    std::string::size_type n = 0;
                    while ((n = value.find("%%", n)) != std::string::npos) {
                        // Replace "%%" with "%" - erase the first '%' and continue past the second '%'
                        value.erase(n, 1);
                        ++n;
                    }
                }
                const unsigned int valueVarId(0);
                Token *tok3 = tok2->tokAt(6);
                ret |= simplifyKnownVariablesSimplify(&tok2, tok3, varid, emptyString, value, valueVarId, valueToken, indentlevel);

                // there could be a hang here if tok2 was moved back by the function call above for some reason
                if (mSettings->terminated())
                    return false;
            }
        }

        if (tok2)
            tok = tok2->previous();
    }

    return ret;
}

bool Tokenizer::simplifyKnownVariablesGetData(unsigned int varid, Token **_tok2, Token **_tok3, std::string &value, unsigned int &valueVarId, bool floatvar)
{
    Token *tok2 = *_tok2;
    Token *tok3 = nullptr;

    if (Token::simpleMatch(tok2->tokAt(-2), "for (")) {
        // only specific for loops is handled
        if (!Token::Match(tok2, "%varid% = %num% ; %varid% <|<= %num% ; ++| %varid% ++| ) {", varid))
            return false;

        // is there a "break" in the for loop?
        bool hasbreak = false;
        const Token* end4 = tok2->linkAt(-1)->linkAt(1);
        for (const Token *tok4 = tok2->previous()->link(); tok4 != end4; tok4 = tok4->next()) {
            if (tok4->str() == "break") {
                hasbreak = true;
                break;
            }
        }
        if (hasbreak)
            return false;

        // no break => the value of the counter value is known after the for loop..
        const Token* compareTok = tok2->tokAt(5);
        if (compareTok->str() == "<") {
            value = compareTok->next()->str();
            valueVarId = compareTok->next()->varId();
        } else
            value = MathLib::toString(MathLib::toLongNumber(compareTok->next()->str()) + 1);

        // Skip for-body..
        tok3 = tok2->previous()->link()->next()->link()->next();
    } else {
        value = tok2->strAt(2);
        valueVarId = tok2->tokAt(2)->varId();
        if (tok2->strAt(1) == "[") {
            value = tok2->next()->link()->strAt(2);
            valueVarId = 0;
        } else if (value == "&") {
            /// @todo makes sense for ctrl code?
            value = tok2->strAt(3);
            valueVarId = tok2->tokAt(3)->varId();
        }

        // Add a '.0' to a decimal value and therefore convert it to an floating point number.
        else if (MathLib::isDec(tok2->strAt(2)) && floatvar) {
            value += ".0";
        }

        // float variable: convert true/false to 1.0 / 0.0
        else if (tok2->tokAt(2)->isBoolean() && floatvar) {
            value = (value == "true") ? "1.0" : "0.0";
        }

        if (Token::simpleMatch(tok2->next(), "= &"))
            tok2 = tok2->tokAt(3);

        tok3 = tok2->next();
    }
    *_tok2 = tok2;
    *_tok3 = tok3;
    return true;
}

bool Tokenizer::simplifyKnownVariablesSimplify(Token **tok2, Token *tok3, unsigned int varid, const std::string &structname, std::string &value, unsigned int valueVarId, const Token * const valueToken, int indentlevel) const
{
 //   std::cout << __FUNCTION__ << std::endl;
    const bool pointeralias(valueToken->isName() || Token::Match(valueToken, "& %name% ["));
    const bool varIsGlobal = (indentlevel == 0);
    const bool printDebug = mSettings->debugwarnings;

    if (mErrorLogger && !list.getFiles().empty())
        mErrorLogger->reportProgress(list.getFiles()[0], "Tokenize (simplifyKnownVariables)", tok3->progressValue());

    if (isMaxTime())
        return false;

    bool ret = false;

    Token* bailOutFromLoop = nullptr;
    int indentlevel3 = indentlevel;
    bool ret3 = false;
    for (; tok3; tok3 = tok3->next()) {
        if (tok3->str() == "{") {
            ++indentlevel3;
        } else if (tok3->str() == "}") {
            --indentlevel3;
            if (indentlevel3 < indentlevel) {
                if (Token::Match((*tok2)->tokAt(-7), "%type% * %name% ; %name% = & %name% ;") &&
                    (*tok2)->strAt(-5) == (*tok2)->strAt(-3)) {
                    (*tok2) = (*tok2)->tokAt(-4);
                    Token::eraseTokens((*tok2), (*tok2)->tokAt(6));
                }
                break;
            }
        }

        // Stop if label is found
        if (Token::Match(tok3, "; %type% : ;"))
            break;

        // Stop if break/continue is found ..
        if (Token::Match(tok3, "break|continue"))
            break;
        if ((indentlevel3 > 1 || !Token::simpleMatch(Token::findsimplematch(tok3,";"), "; }")) && tok3->str() == "return")
            ret3 = true;
        if (ret3 && tok3->str() == ";")
            break;

        if (pointeralias && Token::Match(tok3, ("!!= " + value).c_str()))
            break;

        // Stop if a loop is found
        if (pointeralias && Token::Match(tok3, "do|for|while"))
            break;

        // Stop if unknown function call is seen and the variable is global: it might be
        // changed by the function call
        if (varIsGlobal && tok3->str() == ")" && tok3->link() &&
            Token::Match(tok3->link()->tokAt(-2), "[;{}] %name% (") &&
            !Token::Match(tok3->link()->previous(), "if|for|while|switch"))
            break;

        // Stop if something like 'while (--var)' is found
        if (Token::Match(tok3, "for|while|do")) {
            const Token *endpar = tok3->next()->link();
            if (Token::simpleMatch(endpar, ") {"))
                endpar = endpar->next()->link();
            bool bailout = false;
            for (const Token *tok4 = tok3; tok4 && tok4 != endpar; tok4 = tok4->next()) {
                if (Token::Match(tok4, "++|-- %varid%", varid) ||
                    Token::Match(tok4, "%varid% ++|--|=", varid)) {
                    bailout = true;
                    break;
                }
            }
            if (bailout)
                break;
        }

        if (bailOutFromLoop) {
            // This could be a loop, skip it, but only if it doesn't contain
            // the variable we are checking for. If it contains the variable
            // we will bail out.
            if (tok3->varId() == varid) {
                // Continue
                //tok2 = bailOutFromLoop;
                break;
            } else if (tok3 == bailOutFromLoop) {
                // We have skipped the loop
                bailOutFromLoop = nullptr;
                continue;
            }

            continue;
        } else if (tok3->str() == "{" && tok3->previous()->str() == ")") {
            // There is a possible loop after the assignment. Try to skip it.
            if (tok3->previous()->link() &&
                tok3->previous()->link()->strAt(-1) != "if")
                bailOutFromLoop = tok3->link();
            continue;
        }

        // Variable used in realloc (see Ticket #1649)
        if (Token::Match(tok3, "%name% = realloc ( %name% ,") &&
            tok3->varId() == varid &&
            tok3->tokAt(4)->varId() == varid) {
            tok3->tokAt(4)->str(value);
            ret = true;
        }

        // condition "(|&&|%OROR% %varid% )|&&|%OROR%|;
        if (!Token::Match(tok3->previous(), "( %name% )") &&
            Token::Match(tok3->previous(), "&&|(|%oror% %varid% &&|%oror%|)|;", varid)) {
            tok3->str(value);
            tok3->setVarId(valueVarId);
            ret = true;
        }

        // parameter in function call..
        if (tok3->varId() == varid && Token::Match(tok3->previous(), "[(,] %name% [,)]")) {
            // If the parameter is passed by value then simplify it
            if (isFunctionParameterPassedByValue(tok3)) {
                tok3->str(value);
                tok3->setVarId(valueVarId);
                ret = true;
            }
        }

        // Variable is used somehow in a non-defined pattern => bail out
        if (tok3->varId() == varid) {
            // This is a really generic bailout so let's try to avoid this.
            // There might be lots of false negatives.
            if (printDebug) {
                // FIXME: Fix all the debug warnings for values and then
                // remove this bailout
                if (pointeralias)
                    break;

                // suppress debug-warning when calling member function
                if (Token::Match(tok3->next(), ". %name% ("))
                    break;

                // suppress debug-warning when assignment
                if (tok3->strAt(1) == "=")
                    break;

                // taking address of variable..
                if (Token::Match(tok3->tokAt(-2), "return|= & %name% ;"))
                    break;

                // parameter in function call..
                if (Token::Match(tok3->tokAt(-2), "%name% ( %name% ,|)") ||
                    Token::Match(tok3->previous(), ", %name% ,|)"))
                    break;

                // conditional increment
                if (Token::Match(tok3->tokAt(-3), ") { ++|--") ||
                    Token::Match(tok3->tokAt(-2), ") { %name% ++|--"))
                    break;

                reportError(tok3, Severity::debug, "debug",
                            "simplifyKnownVariables: bailing out (variable="+tok3->str()+", value="+value+")");
            }

            break;
        }

        // Using the variable in condition..
        if (Token::Match(tok3->previous(), ("if ( " + structname + " %varid% %cop%|)").c_str(), varid) ||
            Token::Match(tok3, ("( " + structname + " %varid% %comp%").c_str(), varid) ||
            Token::Match(tok3, ("%comp%|!|= " + structname + " %varid% %cop%|)|;").c_str(), varid) ||
            /// @todo free makes no sense for ctrl
            /// @todo what abouts unistrllen ?
            Token::Match(tok3->previous(), "strlen|free ( %varid% )", varid)) {
            if (value[0] == '\"' && tok3->previous()->str() != "strlen") {
                // bail out if value is a string unless if it's just given
                // as parameter to strlen
                break;
            }
            if (!structname.empty()) {
                tok3->deleteNext(2);
            }
            if (Token::Match(valueToken, "& %name% ;")) {
                tok3->insertToken("&");
                tok3 = tok3->next();
            }
            tok3 = tok3->next();
            tok3->str(value);
            tok3->setVarId(valueVarId);
            ret = true;
        }


        // Delete pointer alias
        if (pointeralias && (tok3->str() == "delete") && tok3->next() &&
            (Token::Match(tok3->next(), "%varid% ;", varid) ||
             Token::Match(tok3->next(), "[ ] %varid%", varid))) {
            tok3 = (tok3->next()->str() == "[") ? tok3->tokAt(3) : tok3->next();
            tok3->str(value);
            tok3->setVarId(valueVarId);
            ret = true;
        }

        // array usage
        if (value[0] != '\"' && Token::Match(tok3, ("[(,] " + structname + " %varid% [|%cop%").c_str(), varid)) {
            if (!structname.empty()) {
                tok3->deleteNext(2);
            }
            tok3 = tok3->next();
            tok3->str(value);
            tok3->setVarId(valueVarId);
            ret = true;
        }


        // Variable is used in calculation..
        if (((tok3->previous()->varId() > 0) && Token::Match(tok3, ("& " + structname + " %varid%").c_str(), varid)) ||
            (Token::Match(tok3, ("[=+-*/%^|[] " + structname + " %varid% [=?+-*/%^|;])]").c_str(), varid) && !Token::Match(tok3, ("= " + structname + " %name% =").c_str())) ||
            Token::Match(tok3, ("[(=+-*/%^|[] " + structname + " %varid% <<|>>").c_str(), varid) ||
            Token::Match(tok3, ("<<|>> " + structname + " %varid% %cop%|;|]|)").c_str(), varid) ||
            Token::Match(tok3->previous(), ("[=+-*/%^|[] ( " + structname + " %varid% !!=").c_str(), varid)) {
            if (value[0] == '\"')
                break;
            if (!structname.empty()) {
                tok3->deleteNext(2);
                ret = true;
            }
            tok3 = tok3->next();
            if (tok3->str() != value)
                ret = true;
            tok3->str(value);
            tok3->setVarId(valueVarId);

            if (Token::Match(valueToken, "& %name% ;"))
            {
                tok3->insertToken("&", emptyString, true);
            }
        }

        if (Token::simpleMatch(tok3, "= {")) {
            const Token* const end4 = tok3->linkAt(1);
            for (const Token *tok4 = tok3; tok4 != end4; tok4 = tok4->next()) {
                if (Token::Match(tok4, "{|, %varid% ,|}", varid)) {
                    tok4->next()->str(value);
                    tok4->next()->setVarId(valueVarId);
                    ret = true;
                }
            }
        }

        // Using the variable in for-condition..
        if (Token::simpleMatch(tok3, "for (")) {
            for (Token *tok4 = tok3->tokAt(2); tok4; tok4 = tok4->next()) {
                if (Token::Match(tok4, "(|)"))
                    break;

                // Replace variable used in condition..
                if (Token::Match(tok4, "; %name% <|<=|!= %name% ; ++| %name% ++| )")) {
                    const Token *inctok = tok4->tokAt(5);
                    if (inctok->str() == "++")
                        inctok = inctok->next();
                    if (inctok->varId() == varid)
                        break;

                    if (tok4->next()->varId() == varid) {
                        tok4->next()->str(value);
                        tok4->next()->setVarId(valueVarId);
                        ret = true;
                    }
                    if (tok4->tokAt(3)->varId() == varid) {
                        tok4->tokAt(3)->str(value);
                        tok4->tokAt(3)->setVarId(valueVarId);
                        ret = true;
                    }
                }
            }
        }

        if (indentlevel == indentlevel3 && Token::Match(tok3->next(), "%varid% ++|--", varid) && MathLib::isInt(value)) {
            const std::string op(tok3->strAt(2));
            if (Token::Match(tok3, "[{};] %any% %any% ;")) {
                tok3->deleteNext(3);
            } else {
                tok3 = tok3->next();
                tok3->str(value);
                tok3->setVarId(valueVarId);
                tok3->deleteNext();
            }
            value = MathLib::incdec(value, op);
            if (!Token::simpleMatch((*tok2)->tokAt(-2), "for (")) {
                (*tok2)->tokAt(2)->str(value);
                (*tok2)->tokAt(2)->setVarId(valueVarId);
            }
            ret = true;
        }

        if (indentlevel == indentlevel3 && Token::Match(tok3->next(), "++|-- %varid%", varid) && MathLib::isInt(value) &&
            !Token::Match(tok3->tokAt(3), "[.[]")) {
            value = MathLib::incdec(value, tok3->next()->str());
            (*tok2)->tokAt(2)->str(value);
            (*tok2)->tokAt(2)->setVarId(valueVarId);
            if (Token::Match(tok3, "[;{}] %any% %any% ;")) {
                tok3->deleteNext(3);
            } else {
                tok3->deleteNext();
                tok3->next()->str(value);
                tok3->next()->setVarId(valueVarId);
            }
            tok3 = tok3->next();
            ret = true;
        }

        // return variable..
        if (Token::Match(tok3, "return %varid% %any%", varid) &&
            valueToken->str() != "&" &&
            (tok3->tokAt(2)->isExtendedOp() || tok3->strAt(2) == ";") &&
            value[0] != '\"') {
            tok3->next()->str(value);
            tok3->next()->setVarId(valueVarId);
        }

        else if (pointeralias && Token::Match(tok3, "return * %varid% ;", varid) && value[0] != '\"') {
            tok3->deleteNext();
            tok3->next()->str(value);
            tok3->next()->setVarId(valueVarId);
        }
    }
    return ret;
}


void Tokenizer::elseif()
{
    for (Token *tok = list.front(); tok; tok = tok->next()) {
        if (!Token::simpleMatch(tok, "else if"))
            continue;

        for (Token *tok2 = tok; tok2; tok2 = tok2->next()) {
            if (Token::Match(tok2, "(|{|["))
                tok2 = tok2->link();

            if (Token::Match(tok2, "}|;")) {
                if (tok2->next() && tok2->next()->str() != "else") {
                    tok->insertToken("{");
                    tok2->insertToken("}");
                    Token::createMutualLinks(tok->next(), tok2->next());
                    break;
                }
            }
        }
    }
}


bool Tokenizer::simplifyRedundantParentheses()
{
    bool ret = false;
    for (Token *tok = list.front(); tok; tok = tok->next()) {
        if (tok->str() != "(")
            continue;

        if (Token::simpleMatch(tok, "( {"))
            continue;

        if (Token::Match(tok->link(), ") %num%")) {
            tok = tok->link();
            continue;
        }

        // !!operator = ( x ) ;
        if (tok->strAt(-2) != "operator" &&
            tok->previous() && tok->previous()->str() == "=" &&
            tok->next() && tok->next()->str() != "{" &&
            Token::simpleMatch(tok->link(), ") ;")) {
            tok->link()->deleteThis();
            tok->deleteThis();
            continue;
        }

        while (Token::simpleMatch(tok, "( (") &&
               tok->link() && tok->link()->previous() == tok->next()->link()) {
            // We have "(( *something* ))", remove the inner
            // parentheses
            tok->deleteNext();
            tok->link()->tokAt(-2)->deleteNext();
            ret = true;
        }

        if (Token::Match(tok->previous(), "! ( %name% )")) {
            // Remove the parentheses
            tok->deleteThis();
            tok->deleteNext();
            ret = true;
        }

        if (Token::Match(tok->previous(), "[(,;{}] ( %name% ) .")) {
            // Remove the parentheses
            tok->deleteThis();
            tok->deleteNext();
            ret = true;
        }

        if (Token::Match(tok->previous(), "[(,;{}] ( %name% (") &&
            tok->link()->previous() == tok->linkAt(2)) {
            // We have "( func ( *something* ))", remove the outer
            // parentheses
            tok->link()->deleteThis();
            tok->deleteThis();
            ret = true;
        }

        if (!Token::simpleMatch(tok->tokAt(-2), "operator delete") &&
            Token::Match(tok->previous(), "delete|; (") &&
            (tok->previous()->str() != "delete" || tok->next()->varId() > 0) &&
            Token::Match(tok->link(), ") ;|,")) {
            tok->link()->deleteThis();
            tok->deleteThis();
            ret = true;
        }

        if (Token::Match(tok->previous(), "[(!*;{}] ( %name% )") &&
            (tok->next()->varId() != 0 || Token::Match(tok->tokAt(3), "[+-/=]")) && !tok->next()->isStandardType()) {
            // We have "( var )", remove the parentheses
            tok->deleteThis();
            tok->deleteNext();
            ret = true;
        }

        while (Token::Match(tok->previous(), "[;{}[(,!*] ( %name% .")) {
            Token *tok2 = tok->tokAt(2);
            while (Token::Match(tok2, ". %name%")) {
                tok2 = tok2->tokAt(2);
            }
            if (tok2 != tok->link())
                break;
            // We have "( var . var . ... . var )", remove the parentheses
            tok = tok->previous();
            tok->deleteNext();
            tok2->deleteThis();
            ret = true;
            continue;
        }

        if (Token::simpleMatch(tok->previous(), "? (") && Token::simpleMatch(tok->link(), ") :")) {
            const Token *tok2 = tok->next();
            while (tok2 && (Token::Match(tok2,"%bool%|%num%|%name%") || tok2->isArithmeticalOp()))
                tok2 = tok2->next();
            if (tok2 && tok2->str() == ")") {
                tok->link()->deleteThis();
                tok->deleteThis();
                ret = true;
                continue;
            }
        }

        while (Token::Match(tok->previous(), "[{([,] ( !!{") &&
               Token::Match(tok->link(), ") [;,])]") &&
               !Token::simpleMatch(tok->tokAt(-2), "operator ,") && // Ticket #5709
               !Token::findsimplematch(tok, ",", tok->link())) {
            // We have "( ... )", remove the parentheses
            tok->link()->deleteThis();
            tok->deleteThis();
            ret = true;
        }

        if (Token::simpleMatch(tok->previous(), ", (") &&
            Token::simpleMatch(tok->link(), ") =")) {
            tok->link()->deleteThis();
            tok->deleteThis();
            ret = true;
        }

        // Simplify "!!operator !!%name%|)|>|>> ( %num%|%bool% ) %op%|;|,|)"
        if (Token::Match(tok, "( %bool%|%num% ) %cop%|;|,|)") &&
            tok->strAt(-2) != "operator" &&
            tok->previous() &&
            !Token::Match(tok->previous(), "%name%|)") &&
            (!(Token::Match(tok->previous(),">|>>")))) {
            tok->link()->deleteThis();
            tok->deleteThis();
            ret = true;
        }

        if (Token::Match(tok->previous(), "*|& ( %name% )")) {
            // We may have a variable declaration looking like "type_name *(var_name)"
            Token *tok2 = tok->tokAt(-2);
            while (Token::Match(tok2, "%type%|static|const|extern") && tok2->str() != "operator") {
                tok2 = tok2->previous();
            }
            if (tok2 && !Token::Match(tok2, "[;,{]")) {
                // Not a variable declaration
            } else {
                tok->deleteThis();
                tok->deleteNext();
            }
        }
    }
    return ret;
}


void Tokenizer::simplifyReference()
{
  //  std::cout << __FUNCTION__ << std::endl;
    for (Token *tok = list.front(); tok; tok = tok->next()) {
        // starting executable scope..
        Token *start = const_cast<Token *>(startOfExecutableScope(tok));
        if (start) {
            tok = start;
            // replace references in this scope..
            Token * const end = tok->link();
            for (Token *tok2 = tok; tok2 && tok2 != end; tok2 = tok2->next()) {
                // found a reference..
                if (Token::Match(tok2, "[;{}] %type% & %name% (|= %name% )| ;")) {
                    const unsigned int refId = tok2->tokAt(3)->varId();
                    if (!refId)
                        continue;

                    // replace reference in the code..
                    for (Token *tok3 = tok2->tokAt(7); tok3 && tok3 != end; tok3 = tok3->next()) {
                        if (tok3->varId() == refId) {
                            tok3->str(tok2->strAt(5));
                            tok3->setVarId(tok2->tokAt(5)->varId());
                        }
                    }

                    tok2->deleteNext(6+(tok2->strAt(6)==")" ? 1 : 0));
                }
            }
            tok = end;
        }
    }
}

bool Tokenizer::simplifyCalculations()
{
    return mCalculationsSimplifier->simplifyCalculations();
}


//---------------------------------------------------------------------------
// Helper functions for handling the tokens list
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------

bool Tokenizer::IsScopeNoReturn(const Token *endScopeToken, bool *unknown) const
{
    std::string unknownFunc;
    const bool ret = mSettings->library.isScopeNoReturn(endScopeToken,&unknownFunc);
    if (unknown)
        *unknown = !unknownFunc.empty();


    /// @todo check sence of this for ctrl lang (checkLibraryNoReturn)
    /// https://trello.com/c/YRk4MgZ5
    /// so i disable it now
    const bool enableThis = false;

    if (enableThis && !unknownFunc.empty() && mSettings->checkLibrary && mSettings->isEnabled(Settings::INFORMATION)) {
        // Is function global?
        bool globalFunction = true;
        if (Token::simpleMatch(endScopeToken->tokAt(-2), ") ; }")) {
            const Token * const ftok = endScopeToken->linkAt(-2)->previous();
            if (ftok &&
                ftok->isName() &&
                ftok->function() &&
                ftok->function()->nestedIn &&
                ftok->function()->nestedIn->type != Scope::eGlobal) {
                globalFunction = false;
            }
        }

        // don't warn for nonglobal functions (class methods, functions hidden in namespaces) since they can't be configured yet
        // FIXME: when methods and namespaces can be configured properly, remove the "globalFunction" check
        if (globalFunction) {
            reportError(endScopeToken->previous(),
                        Severity::information,
                        "checkLibraryNoReturn",
                        "--check-library: Function " + unknownFunc + "() should have <noreturn> configuration");
        }
    }
    return ret;
}

//---------------------------------------------------------------------------

bool Tokenizer::isFunctionParameterPassedByValue(const Token *fpar) const
{
    // TODO: If symbol database is available, use it.
    const Token *ftok;

    // Look at function call, what parameter number is it?
    unsigned int parameter = 1;
    for (ftok = fpar->previous(); ftok; ftok = ftok->previous()) {
        if (ftok->str() == "(")
            break;
        else if (ftok->str() == ")")
            ftok = ftok->link();
        else if (ftok->str() == ",")
            ++parameter;
        else if (Token::Match(ftok, "[;{}]"))
            break;
    }

    // Is this a function call?
    if (ftok && Token::Match(ftok->tokAt(-2), "[;{}=] %name% (")) {
        const std::string& functionName(ftok->previous()->str());

        if (functionName == "return")
            return true;

        // Locate function declaration..
        for (const Token *tok = tokens(); tok; tok = tok->next()) {
            if (tok->str() == "{")
                tok = tok->link();
            else if (Token::Match(tok, "%type% (") && tok->str() == functionName) {
                // Goto parameter
                tok = tok->tokAt(2);
                unsigned int par = 1;
                while (tok && par < parameter) {
                    if (tok->str() == ")")
                        break;
                    if (tok->str() == ",")
                        ++par;
                    tok = tok->next();
                }
                if (!tok)
                    return false;

                // If parameter was found, determine if it's passed by value
                if (par == parameter) {
                    bool knowntype = false;
                    while (tok && tok->isName()) {
                        knowntype |= tok->isStandardType();
                        knowntype |= (tok->str() == "struct");
                        tok = tok->next();
                    }
                    if (!tok || !knowntype)
                        return false;
                    if (tok->str() != "," && tok->str() != ")")
                        return false;
                    return true;
                }
            }
        }
    }
    return false;
}

//---------------------------------------------------------------------------

void Tokenizer::eraseDeadCode(Token *begin, const Token *end)
{
    if (!begin)
        return;
    const bool isgoto = Token::Match(begin->tokAt(-2), "goto %name% ;");
    unsigned int indentlevel = 1,
                 indentcase = 0,
                 indentswitch = 0,
                 indentlabel = 0,
                 roundbraces = 0,
                 indentcheck = 0;
    std::vector<unsigned int> switchindents;
    bool checklabel = false;
    Token *tok = begin;
    Token *tokcheck = nullptr;
    while (tok->next() && tok->next() != end) {
        if (tok->next()->str() == "(") {
            ++roundbraces;
            tok->deleteNext();
            continue;
        } else if (tok->next()->str() == ")") {
            if (!roundbraces)
                break;  //too many ending round parentheses
            --roundbraces;
            tok->deleteNext();
            continue;
        }

        if (roundbraces) {
            tok->deleteNext();
            continue;
        }

        if (Token::Match(tok, "[{};] switch (")) {
            if (!checklabel) {
                if (!indentlabel) {
                    //remove 'switch ( ... )'
                    Token::eraseTokens(tok, tok->linkAt(2)->next());
                } else {
                    tok = tok->linkAt(2);
                }
                if (tok->next()->str() == "{") {
                    ++indentswitch;
                    indentcase = indentlevel + 1;
                    switchindents.push_back(indentcase);
                }
            } else {
                tok = tok->linkAt(2);
                if (Token::simpleMatch(tok, ") {")) {
                    ++indentswitch;
                    indentcase = indentlevel + 1;
                    switchindents.push_back(indentcase);
                }
            }
        } else if (tok->next()->str() == "{") {
            ++indentlevel;
            if (!checklabel) {
                checklabel = true;
                tokcheck = tok;
                indentcheck = indentlevel;
                indentlabel = 0;
            }
            tok = tok->next();
        } else if (tok->next()->str() == "}") {
            --indentlevel;
            if (!indentlevel)
                break;

            if (!checklabel) {
                tok->deleteNext();
            } else {
                if (indentswitch && indentlevel == indentcase)
                    --indentlevel;
                if (indentlevel < indentcheck) {
                    const Token *end2 = tok->next();
                    tok = end2->link()->previous();  //return to initial '{'
                    if (indentswitch && Token::simpleMatch(tok, ") {") && Token::Match(tok->link()->tokAt(-2), "[{};] switch ("))
                        tok = tok->link()->tokAt(-2);       //remove also 'switch ( ... )'
                    Token::eraseTokens(tok, end2->next());
                    checklabel = false;
                    tokcheck = nullptr;
                    indentcheck = 0;
                } else {
                    tok = tok->next();
                }
            }
            if (indentswitch && indentlevel <= indentcase) {
                --indentswitch;
                switchindents.pop_back();
                if (!indentswitch)
                    indentcase = 0;
                else
                    indentcase = switchindents[indentswitch-1];
            }
        } else if (Token::Match(tok, "[{};:] case")) {
            const Token *tok2 = Token::findsimplematch(tok->next(), ": ;", end);
            if (!tok2) {
                tok->deleteNext();
                continue;
            }
            if (indentlevel == 1)
                break;      //it seems like the function was called inside a case-default block.
            if (indentlevel == indentcase)
                ++indentlevel;
            tok2 = tok2->next();
            if (!checklabel || !indentswitch) {
                Token::eraseTokens(tok, tok2->next());
            } else {
                tok = const_cast<Token *>(tok2);
            }
        }  else if (Token::Match(tok, "[{};] default : ;")) {
            if (indentlevel == 1)
                break;      //it seems like the function was called inside a case-default block.
            if (indentlevel == indentcase)
                ++indentlevel;
            if (!checklabel || !indentswitch) {
                tok->deleteNext(3);
            } else {
                tok = tok->tokAt(3);
            }
        } else if (Token::Match(tok, "[{};] %name% : ;") && tok->next()->str() != "default") {
            if (checklabel) {
                indentlabel = indentlevel;
                tok = tokcheck->next();
                checklabel = false;
                indentlevel = indentcheck;
            } else {
                if (indentswitch) {
                    //Before stopping the function, since the 'switch()'
                    //instruction is removed, there's no sense to keep the
                    //case instructions. Remove them, if there are any.
                    Token *tok2 = tok->tokAt(3);
                    unsigned int indentlevel2 = indentlevel;
                    while (tok2->next() && tok2->next() != end) {
                        if (Token::Match(tok2->next(), "{|[|(")) {
                            tok2 = tok2->next()->link();
                        } else if (Token::Match(tok2, "[{};:] case")) {
                            const Token *tok3 = Token::findsimplematch(tok2->next(), ": ;", end);
                            if (!tok3) {
                                tok2 = tok2->next();
                                continue;
                            }
                            Token::eraseTokens(tok2, tok3->next());
                        } else if (Token::Match(tok2, "[{};] default : ;")) {
                            tok2->deleteNext(3);
                        } else if (tok2->next()->str() == "}") {
                            --indentlevel2;
                            if (indentlevel2 <= indentcase)
                                break;
                            tok2 = tok2->next();
                        } else {
                            tok2 = tok2->next();
                        }
                    }
                }
                break;  //stop removing tokens, we arrived to the label.
            }
        } else if (isgoto && Token::Match(tok, "[{};] do|while|for")) {
            //it's possible that code inside loop is not dead,
            //because of the possible presence of the label pointed by 'goto'
            const Token *start = tok->tokAt(2);
            if (start && start->str() == "(")
                start = start->link()->next();
            if (start && start->str() == "{") {
                std::string labelpattern = "[{};] " + begin->previous()->str() + " : ;";
                bool simplify = true;
                for (Token *tok2 = start->next(); tok2 != start->link(); tok2 = tok2->next()) {
                    if (Token::Match(tok2, labelpattern.c_str())) {
                        simplify = false;
                        break;
                    }
                }
                //bailout for now
                if (!simplify)
                    break;
            }
            tok->deleteNext();
        } else {
            // no need to keep the other strings, remove them.
            if (tok->strAt(1) == "while") {
                if (tok->str() == "}" && tok->link()->strAt(-1) == "do")
                    tok->link()->previous()->deleteThis();
            }
            tok->deleteNext();
        }
    }
}


//---------------------------------------------------------------------------
void Tokenizer::unmatchedToken(const Token *tok) const
{
    bool openBracket = Token::Match(tok, "(|{|[|<");

    printDebugOutput(0);
    std::string lineNumber = std::to_string(tok->linenr());
    syntaxErrorDetailMsg(tok,
                        "Unmatched bracket '" + tok->str() + "'\n" +
                        "The bracket '" + tok->str() + "' which was " + (openBracket ? "opened" : "closed") +
                        " at line number '" + lineNumber +  "' was not " + (openBracket ? "closed" : "open") + "."
                        );
}


void Tokenizer::cppcheckError(const Token *tok) const
{
    printDebugOutput(0);
    throw InternalError(tok, "Analysis failed. If the code is valid then please report this failure.", InternalError::INTERNAL);
}
/**
 * Helper function to check whether number is equal to integer constant X
 * or floating point pattern X.0
 * @param s the string to check
 * @param intConstant the integer constant to check against
 * @param floatConstant the string with stringified float constant to check against
 * @return true in case s is equal to X or X.0 and false otherwise.
 */
static bool isNumberOneOf(const std::string &s, const MathLib::bigint& intConstant, const char* floatConstant)
{
    if (MathLib::isInt(s)) {
        if (MathLib::toLongNumber(s) == intConstant)
            return true;
    } else if (MathLib::isFloat(s)) {
        if (MathLib::toString(MathLib::toDoubleNumber(s)) == floatConstant)
            return true;
    }
    return false;
}

// ------------------------------------------------------------------------
// Helper function to check whether number is zero (0 or 0.0 or 0E+0) or not?
// @param s the string to check
// @return true in case s is zero and false otherwise.
// ------------------------------------------------------------------------
bool Tokenizer::isZeroNumber(const std::string &s)
{
    return isNumberOneOf(s, 0L, "0.0");
}

// ------------------------------------------------------------------------
// Helper function to check whether number is one (1 or 0.1E+1 or 1E+0) or not?
// @param s the string to check
// @return true in case s is one and false otherwise.
// ------------------------------------------------------------------------
bool Tokenizer::isOneNumber(const std::string &s)
{
    if (!MathLib::isPositive(s))
        return false;
    return isNumberOneOf(s, 1L, "1.0");
}

// ------------------------------------------------------------------------
// Helper function to check whether number is two (2 or 0.2E+1 or 2E+0) or not?
// @param s the string to check
// @return true in case s is two and false otherwise.
// ------------------------------------------------------------------------
bool Tokenizer::isTwoNumber(const std::string &s)
{
    if (!MathLib::isPositive(s))
        return false;
    return isNumberOneOf(s, 2L, "2.0");
}

// ------------------------------------------------------
// Simplify math functions.
// It simplifies the following functions: pow(),
// sqrt(), exp(),  log2(),  log10(),  log(),
// acos(), cos(), asin(), tan(), tanh(), atan(), sin(),
// sinh()
// in the tokenlist.
// ------------------------------------------------------
/// @todo be carefull and addapt it for ctrl
void Tokenizer::simplifyMathFunctions()
{
    for (Token *tok = list.front(); tok; tok = tok->next()) {
        if (tok->isName() && !tok->varId() && tok->strAt(1) == "(") { // precondition for function
            bool simplifcationMade = false;
            if (Token::Match(tok, "sqrt ( %num% )")) {
                // Simplify: sqrt(0) = 0 and cbrt(0) == 0
                //           sqrt(1) = 1 and cbrt(1) == 1
                // get number string
                const std::string& parameter(tok->strAt(2));
                // is parameter 0 ?
                if (isZeroNumber(parameter)) {
                    tok->deleteNext(3);  // delete tokens
                    tok->str("0"); // insert result into token list
                    simplifcationMade = true;
                } else if (isOneNumber(parameter)) {
                    tok->deleteNext(3);  // delete tokens
                    tok->str("1"); // insert result into token list
                    simplifcationMade = true;
                }
            } else if (Token::Match(tok, "exp|cos|cosh ( %num% )")) {
                // Simplify: exp[f|l](0)  = 1 and exp2[f|l](0) = 1
                //           cosh[f|l](0) = 1 and cos[f|l](0)  = 1
                //           erfc[f|l](0) = 1
                // get number string
                const std::string& parameter(tok->strAt(2));
                // is parameter 0 ?
                if (isZeroNumber(parameter)) {
                    tok->deleteNext(3);  // delete tokens
                    tok->str("1"); // insert result into token list
                    simplifcationMade = true;
                }
            } else if (Token::Match(tok, "sin|sinh|asin|tan|tanh|atan ( %num% )")) {
                // Simplify: log1p[f|l](0) = 0 and sin[f|l](0)  = 0
                //           sinh[f|l](0)  = 0 and erf[f|l](0)  = 0
                //           asin[f|l](0)  = 0 and sinh[f|l](0) = 0
                //           tan[f|l](0)   = 0 and tanh[f|l](0) = 0
                //           atan[f|l](0)  = 0 and atanh[f|l](0)= 0
                //           expm1[f|l](0) = 0
                // get number string
                const std::string& parameter(tok->strAt(2));
                // is parameter 0 ?
                if (isZeroNumber(parameter)) {
                    tok->deleteNext(3);  // delete tokens
                    tok->str("0"); // insert result into token list
                    simplifcationMade = true;
                }
            } else if (Token::Match(tok, "log|log10|acos ( %num% )")) {
                // Simplify: log2[f|l](1)  = 0 , log10[f|l](1)  = 0
                //           log[f|l](1)   = 0 , logb10[f|l](1) = 0
                //           acosh[f|l](1) = 0 , acos[f|l](1)   = 0
                //           ilogb[f|l](1) = 0
                // get number string
                const std::string& parameter(tok->strAt(2));
                // is parameter 1 ?
                if (isOneNumber(parameter)) {
                    tok->deleteNext(3);  // delete tokens
                    tok->str("0"); // insert result into token list
                    simplifcationMade = true;
                }
            } else if (Token::Match(tok, "pow (")) {
                if (Token::Match(tok->tokAt(2), "%num% , %num% )")) {
                    // In case of pow ( 0 , anyNumber > 0): It can be simplified to 0
                    // In case of pow ( 0 , 0 ): It simplified to 1
                    // In case of pow ( 1 , anyNumber ): It simplified to 1
                    const std::string& leftNumber(tok->strAt(2)); // get the left parameter
                    const std::string& rightNumber(tok->strAt(4)); // get the right parameter
                    const bool isLeftNumberZero = isZeroNumber(leftNumber);
                    const bool isLeftNumberOne = isOneNumber(leftNumber);
                    const bool isRightNumberZero = isZeroNumber(rightNumber);
                    if (isLeftNumberZero && !isRightNumberZero && MathLib::isPositive(rightNumber)) { // case: 0^(y) = 0 and y > 0
                        tok->deleteNext(5); // delete tokens
                        tok->str("0");  // insert simplified result
                        simplifcationMade = true;
                    } else if (isLeftNumberZero && isRightNumberZero) { // case: 0^0 = 1
                        tok->deleteNext(5); // delete tokens
                        tok->str("1");  // insert simplified result
                        simplifcationMade = true;
                    } else if (isLeftNumberOne) { // case 1^(y) = 1
                        tok->deleteNext(5); // delete tokens
                        tok->str("1");  // insert simplified result
                        simplifcationMade = true;
                    }
                }
                if (Token::Match(tok->tokAt(2), "%any% , %num% )")) {
                    // In case of pow( x , 1 ): It can be simplified to x.
                    const std::string& leftParameter(tok->strAt(2)); // get the left parameter
                    const std::string& rightNumber(tok->strAt(4)); // get right number
                    if (isOneNumber(rightNumber)) { // case: x^(1) = x
                        tok->str(leftParameter);  // insert simplified result
                        tok->deleteNext(5); // delete tokens
                        simplifcationMade = true;
                    } else if (isZeroNumber(rightNumber)) { // case: x^(0) = 1
                        tok->deleteNext(5); // delete tokens
                        tok->str("1");  // insert simplified result
                        simplifcationMade = true;
                    }
                }
            }
            // Jump back to begin of statement if a simplification was performed
            if (simplifcationMade) {
                while (tok->previous() && tok->str() != ";") {
                    tok = tok->previous();
                }
            }
        }
    }
}

void Tokenizer::simplifyComma()
{
    bool inReturn = false;

    for (Token *tok = list.front(); tok; tok = tok->next()) {

        // skip enums
        if (Token::Match(tok, "enum class|struct| %name%| :|{")) {
            skipEnumBody(&tok);
        }
        if (!tok)
            syntaxErrorDetail(nullptr); // invalid code like in #4195

        if (Token::Match(tok, "(|[") || Token::Match(tok->previous(), "%name%|= {")) {
            tok = tok->link();
            continue;
        }

        if (tok->str() == "return" && Token::Match(tok->previous(), "[;{}]"))
            inReturn = true;

        if (inReturn && Token::Match(tok, "[;{}?:]"))
            inReturn = false;

        if (!tok->next() || tok->str() != ",")
            continue;

        // We must not accept just any keyword, e.g. accepting int
        // would cause function parameters to corrupt.
        if (tok->strAt(1) == "delete") {
            // Handle "delete a, delete b;"
            tok->str(";");
        }

        if (!inReturn && tok->tokAt(-2)) {
            bool replace = false;
            for (Token *tok2 = tok->previous(); tok2; tok2 = tok2->previous()) {
                if (tok2->str() == "=") {
                    // Handle "a = 0, b = 0;"
                    replace = true;
                } else if (Token::Match(tok2, "delete %name%") ||
                                       Token::Match(tok2, "delete [ ] %name%")) {
                    // Handle "delete a, a = 0;"
                    replace = true;
                } else if (Token::Match(tok2, "[?:;,{}()]")) {
                    if (replace && Token::Match(tok2, "[;{}]"))
                        tok->str(";");
                    break;
                }
            }
        }

        // find token where return ends and also count commas
        if (inReturn) {
            Token *startFrom = nullptr;    // "[;{}]" token before "return"
            Token *endAt = nullptr;        // first ";" token after "[;{}] return"

            // find "; return" pattern before comma
            for (Token *tok2 = tok->previous(); tok2; tok2 = tok2->previous()) {
                if (tok2->str() == "return") {
                    startFrom = tok2->previous();
                    break;
                }
            }
            if (!startFrom)
                // to be very sure...
                return;
            std::size_t commaCounter = 0;
            for (Token *tok2 = startFrom->next(); tok2; tok2 = tok2->next()) {
                if (tok2->str() == ";") {
                    endAt = tok2;
                    break;

                } else if (Token::Match(tok2, "(|[") ||
                           (tok2->str() == "{" && tok2->previous() && tok2->previous()->str() == "=")) {
                    tok2 = tok2->link();

                } else if (tok2->str() == ",") {
                    ++commaCounter;
                }
            }

            if (!endAt)
                //probably a syntax error
                return;

            if (commaCounter) {
                // change tokens:
                // "; return a ( ) , b ( ) , c ;"
                // to
                // "; a ( ) ; b ( ) ; return c ;"

                // remove "return"
                startFrom->deleteNext();
                for (Token *tok2 = startFrom->next(); tok2 != endAt; tok2 = tok2->next()) {
                    if (Token::Match(tok2, "(|[") ||
                        (tok2->str() == "{" && tok2->previous() && tok2->previous()->str() == "=")) {
                        tok2 = tok2->link();

                    } else if (tok2->str() == ",") {
                        tok2->str(";");
                        --commaCounter;
                        if (commaCounter == 0) {
                            tok2->insertToken("return");
                        }
                    }
                }
                tok = endAt;
            }
        }
    }
}

void Tokenizer::validate() const
{
    std::stack<const Token *> linkTokens;
    const Token *lastTok = nullptr;
    for (const Token *tok = tokens(); tok; tok = tok->next()) {
        lastTok = tok;
        if (Token::Match(tok, "[{([]") || (tok->str() == "<" && tok->link())) {
            if (tok->link() == nullptr)
                cppcheckError(tok);

            linkTokens.push(tok);
        }

        else if (Token::Match(tok, "[})]]") || (Token::Match(tok, ">|>>") && tok->link())) {
            if (tok->link() == nullptr)
                cppcheckError(tok);

            if (linkTokens.empty() == true)
                cppcheckError(tok);

            if (tok->link() != linkTokens.top())
                cppcheckError(tok);

            if (tok != tok->link()->link())
                cppcheckError(tok);

            linkTokens.pop();
        }

        else if (tok->link() != nullptr)
            cppcheckError(tok);
    }

    if (!linkTokens.empty())
        cppcheckError(linkTokens.top());

    // Validate that the Tokenizer::list.back() is updated correctly during simplifications
    if (lastTok != list.back())
        cppcheckError(lastTok);
}

static const Token *findUnmatchedTernaryOp(const Token * const begin, const Token * const end, unsigned depth = 0)
{
    std::stack<const Token *> ternaryOp;
    for (const Token *tok = begin; tok != end && tok->str() != ";"; tok = tok->next()) {
        if (tok->str() == "?")
            ternaryOp.push(tok);
        else if (!ternaryOp.empty() && tok->str() == ":")
            ternaryOp.pop();
        else if (depth < 100 && Token::Match(tok,"(|[")) {
            const Token *inner = findUnmatchedTernaryOp(tok->next(), tok->link(), depth+1);
            if (inner)
                return inner;
            tok = tok->link();
        }
    }
    return ternaryOp.empty() ? nullptr : ternaryOp.top();
}

void Tokenizer::findGarbageCode() const
{
    // Inside [] there can't be ; or various keywords
    for (const Token *tok = tokens(); tok; tok = tok->next()) {
        if (tok->str() != "[")
            continue;
        for (const Token *inner = tok->next(); inner != tok->link(); inner = inner->next()) {
            if (Token::Match(inner, "(|["))
                inner = inner->link();
            else if (Token::Match(inner, ";|goto|return|typedef"))
                syntaxErrorDetailMsg(inner, "Inside [] there can't be ; or various keywords");
        }
    }

    for (const Token *tok = tokens(); tok; tok = tok->next()) {
        if (Token::Match(tok, "if|while|for|switch")) { // if|while|for|switch (EXPR) { ... }
            if (tok->previous() && !Token::Match(tok->previous(), "%name%|:|;|{|}|(|)|,"))
                syntaxErrorDetail(tok);
            if (Token::Match(tok->previous(), "[(,]"))
                continue;
            if (!Token::Match(tok->next(), "( !!)"))
                syntaxErrorDetail(tok);
            if (tok->str() != "for") {
                if (isGarbageExpr(tok->next(), tok->linkAt(1)))
                    syntaxErrorDetail(tok);
            }
        }
    }

    // case keyword must be inside switch
    for (const Token *tok = tokens(); tok; tok = tok->next()) {
        if (Token::simpleMatch(tok, "switch (")) {
            if (Token::simpleMatch(tok->linkAt(1), ") {")) {
                tok = tok->linkAt(1)->linkAt(1);
                continue;
            }
            const Token *switchToken = tok;
            tok = tok->linkAt(1);
            if (!tok)
                syntaxErrorDetail(switchToken);
            // Look for the end of the switch statement, i.e. the first semi-colon or '}'
            for (; tok ; tok = tok->next()) {
                if (tok->str() == "{") {
                    tok = tok->link();
                }
                if (Token::Match(tok, ";|}")) {
                    // We're at the end of the switch block
                    if (tok->str() == "}" && tok->strAt(-1) == ":") // Invalid case
                        syntaxErrorDetailMsg(switchToken, "invalid case. We're at the end of the switch block");
                    break;
                }
            }
            if (!tok)
                break;
        } else if (tok->str() == "(") {
            tok = tok->link();
        } else if (tok->str() == "case") {
            syntaxErrorDetail(tok);
        }
    }

    for (const Token *tok = tokens(); tok ; tok = tok->next()) {
        if (!Token::simpleMatch(tok, "for (")) // find for loops
            continue;
        // count number of semicolons
        unsigned int semicolons = 0;
        const Token* const startTok = tok;
        tok = tok->next()->link()->previous(); // find ")" of the for-loop
        // walk backwards until we find the beginning (startTok) of the for() again
        for (; tok != startTok; tok = tok->previous()) {
            if (tok->str() == ";") { // do the counting
                semicolons++;
            } else if (tok->str() == ")") { // skip pairs of ( )
                tok = tok->link();
            }
        }
        // if we have an invalid number of semicolons inside for( ), assume syntax error
        if ((semicolons == 1) || (semicolons > 2)) {
            syntaxErrorDetailMsg(tok, "invalid number of semicolons inside for( )");
        }
    }

    // Operators without operands..
    for (const Token *tok = tokens(); tok; tok = tok->next()) {
        if (Token::Match(tok, "%or%|%oror%|==|!=|+|-|/|!|>=|<=|~|++|--|::|throw {|if|else|try|catch|while|do|for|return|switch|break"))
            syntaxErrorDetailMsg(tok, "operator without operands ...");
        if (Token::Match(tok, "( %any% )") && tok->next()->isKeyword() && !Token::simpleMatch(tok->next(), "void"))
            syntaxErrorDetailMsg(tok, "operator without operands ...");
        if (Token::Match(tok, "%num%|%bool%|%char%|%str% %num%|%bool%|%char%|%str%") && !Token::Match(tok, "%str% %str%"))
            syntaxErrorDetailMsg(tok, "operator without operands ...");
        if (Token::Match(tok, "%assign% typename|class %assign%"))
            syntaxErrorDetailMsg(tok, "operator without operands ...");
        if (Token::Match(tok, "%cop%|=|,|[ %or%|%oror%|/|%"))
            syntaxErrorDetailMsg(tok, "operator without operands ...");
        if (Token::Match(tok, ";|(|[ %comp%"))
            syntaxErrorDetailMsg(tok, "operator without operands ...");
        if (Token::Match(tok, "%cop%|= ]") && !Token::Match(tok->previous(), "[|, &|= ]"))
            syntaxErrorDetailMsg(tok, "operator without operands ...");
        if (Token::Match(tok, "[+-] [;,)]}]"))
            syntaxErrorDetailMsg(tok, "operator without operands ...");
    }

    // ternary operator without :
    if (const Token *ternaryOp = findUnmatchedTernaryOp(tokens(), nullptr))
        syntaxErrorDetailMsg(ternaryOp, "ternary operator without :");

    // Code must not start with an arithmetical operand
    if (Token::Match(list.front(), "%cop%"))
        syntaxErrorDetailMsg(list.front(), "code must not start with an arithmetical operand");

    // Code must end with } ; ) NAME
    if (!Token::Match(list.back(), "%name%|;|}|)"))
        syntaxErrorDetailMsg(list.back(), "code must end with } ; )");
    if (list.back()->str() == ")" && !Token::Match(list.back()->link()->previous(), "%name% ("))
        syntaxErrorDetailMsg(list.back(), "code must end with } ; )");
    if (Token::Match(list.back(), "void|char|short|int|long|float|double|const|static|synchronized|struct|class|enum|case|break|continue|typedef"))
        syntaxErrorDetailMsg(list.back(), "code must end with } ; )");
    if ((list.back()->str()==")" || list.back()->str()=="}") && list.back()->previous() && list.back()->previous()->isControlFlowKeyword())
        syntaxErrorDetailMsg(list.back()->previous(), "code must end with } ; )");


    // Objective C/C++
    for (const Token *tok = tokens(); tok; tok = tok->next()) {
        if (Token::Match(tok, "[;{}] [ %name% %name% ] ;"))
            syntaxErrorDetail(tok->next());
    }
}


bool Tokenizer::isGarbageExpr(const Token *start, const Token *end)
{
    for (const Token *tok = start; tok != end; tok = tok->next()) {
        if (tok->isControlFlowKeyword())
            return true;
        if (tok->str() == ";")
            return true;
        if (tok->str() == "{")
            tok = tok->link();
    }
    return false;
}

std::string Tokenizer::simplifyString(const std::string &source)
{
    std::string str = source;

    for (std::string::size_type i = 0; i + 1U < str.size(); ++i) {
        if (str[i] != '\\')
            continue;

        int c = 'a';   // char
        unsigned int sz = 0;    // size of stringdata
        if (str[i+1] == 'x') {
            sz = 2;
            while (sz < 4 && std::isxdigit((unsigned char)str[i+sz]))
                sz++;
            if (sz > 2) {
                std::istringstream istr(str.substr(i+2, sz-2));
                istr >> std::hex >> c;
            }
        } else if (MathLib::isOctalDigit(str[i+1])) {
            sz = 2;
            while (sz < 4 && MathLib::isOctalDigit(str[i+sz]))
                sz++;
            std::istringstream istr(str.substr(i+1, sz-1));
            istr >> std::oct >> c;
            str = str.substr(0,i) + (char)c + str.substr(i+sz);
            continue;
        }

        if (sz <= 2)
            i++;
        else if (i+sz < str.size())
            str.replace(i, sz, std::string(1U, (char)c));
        else
            str.replace(i, str.size() - i - 1U, "a");
    }

    return str;
}

void Tokenizer::simplifyWhile0()
{
    for (Token *tok = list.front(); tok; tok = tok->next()) {
        // while (0)
        const bool while0(Token::Match(tok->previous(), "[{};] while ( 0|false )"));

        // for (0) - not banal, ticket #3140
        const bool for0((Token::Match(tok->previous(), "[{};] for ( %name% = %num% ; %name% < %num% ;") &&
                         tok->strAt(2) == tok->strAt(6) && tok->strAt(4) == tok->strAt(8)) ||
                        (Token::Match(tok->previous(), "[{};] for ( %type% %name% = %num% ; %name% < %num% ;") &&
                         tok->strAt(3) == tok->strAt(7) && tok->strAt(5) == tok->strAt(9)));

        if (!while0 && !for0)
            continue;

        if (while0 && tok->previous()->str() == "}") {
            // find "do"
            Token *tok2 = tok->previous()->link();
            tok2 = tok2->previous();
            if (tok2 && tok2->str() == "do") {
                const bool flowmatch = Token::findmatch(tok2, "continue|break", tok) != nullptr;
                // delete "do ({)"
                tok2->deleteThis();
                if (!flowmatch)
                    tok2->deleteThis();

                // delete "(}) while ( 0 ) (;)"
                tok = tok->previous();
                tok->deleteNext(4);  // while ( 0 )
                if (tok->next() && tok->next()->str() == ";")
                    tok->deleteNext(); // ;
                if (!flowmatch)
                    tok->deleteThis(); // }

                continue;
            }
        }

        // remove "while (0) { .. }"
        if (Token::simpleMatch(tok->next()->link(), ") {")) {
            Token *end = tok->next()->link(), *old_prev = tok->previous();
            end = end->next()->link();
            if (Token::Match(tok, "for ( %name% ="))
                old_prev = end->link();
            eraseDeadCode(old_prev, end->next());
            if (old_prev && old_prev->next())
                tok = old_prev->next();
            else
                break;
        }
    }
}

void Tokenizer::simplifyFuncInWhile()
{
//    std::cout << __FUNCTION__ << std::endl;
    unsigned int count = 0;
    for (Token *tok = list.front(); tok; tok = tok->next()) {
        if (!Token::Match(tok, "while ( %name% ( %name% ) ) {"))
            continue;

        Token *func = tok->tokAt(2);
        const Token * const var = tok->tokAt(4);
        Token * const end = tok->next()->link()->next()->link();

        const unsigned int varid = ++mVarId; // Create new variable
        const std::string varname("cppcheck:r" + MathLib::toString(++count));
        tok->str("int");
        tok->next()->insertToken(varname);
        tok->tokAt(2)->setVarId(varid);
        tok->insertToken("while");
        tok->insertToken(";");
        tok->insertToken(")");
        tok->insertToken(var->str());
        tok->next()->setVarId(var->varId());
        tok->insertToken("(");
        tok->insertToken(func->str());
        tok->insertToken("=");
        tok->insertToken(varname);
        tok->next()->setVarId(varid);
        Token::createMutualLinks(tok->tokAt(4), tok->tokAt(6));
        end->previous()->insertToken(varname);
        end->previous()->setVarId(varid);
        end->previous()->insertToken("=");
        Token::move(func, func->tokAt(3), end->previous());
        end->previous()->insertToken(";");

        tok = end;
    }
}

void Tokenizer::simplifyStructDecl()
{
    // A counter that is used when giving unique names for anonymous structs.
    unsigned int count = 0;

    std::stack<bool> skip; // true = in function, false = not in function
    skip.push(false);

    // Add names for anonymous structs
    for (Token *tok = list.front(); tok; tok = tok->next()) {
        if (!tok->isName())
            continue;
        // check for anonymous struct
        if (Token::Match(tok, "struct {")) {
            if (Token::Match(tok->next()->link(), "} const| *|&| const| %type% ,|;|[|(|{|=")) {
                tok->insertToken("Anonymous" + MathLib::toString(count++));
            }
        }
        // check for derived anonymous class/struct
        else if (Token::Match(tok, "class|struct :")) {
            const Token *tok1 = Token::findsimplematch(tok, "{");
            if (tok1 && Token::Match(tok1->link(), "} const| *|&| const| %type% ,|;|[|(|{")) {
                tok->insertToken("Anonymous" + MathLib::toString(count++));
            }
        }
        // check for anonymous enum
        else if ((Token::simpleMatch(tok, "enum {") &&
                  !Token::Match(tok->tokAt(-3), "using %name% =") &&
                  Token::Match(tok->next()->link(), "} %type%| ,|;|[|(|{")) ||
                 (Token::Match(tok, "enum : %type% {") && Token::Match(tok->linkAt(3), "} %type%| ,|;|[|(|{"))) {
            tok->insertToken("Anonymous" + MathLib::toString(count++));
        }
    }

    for (Token *tok = list.front(); tok; tok = tok->next()) {

        // check for start of scope and determine if it is in a function
        if (tok->str() == "{")
            skip.push(Token::Match(tok->previous(), "const|)"));

        // end of scope
        else if (tok->str() == "}" && !skip.empty())
            skip.pop();

        // check for named struct
        else if (Token::Match(tok, "class|struct|enum %type% :|{")) {
            Token *start = tok;
            while (Token::Match(start->previous(), "%type%"))
                start = start->previous();
            const Token * const type = tok->next();
            Token *next = tok->tokAt(2);

            while (next && next->str() != "{")
                next = next->next();
            if (!next)
                continue;
            skip.push(false);
            tok = next->link();
            if (!tok)
                break; // see #4869 segmentation fault in Tokenizer::simplifyStructDecl (invalid code)
            Token *restart = next;

            tok = restart;
        }
    }
}

void Tokenizer::simplifyAttribute()
{
    for (Token *tok = list.front(); tok; tok = tok->next()) {
        if (Token::Match(tok, "%type% (") && !mSettings->library.isNotLibraryFunction(tok)) {
            if (mSettings->library.isFunctionConst(tok->str(), true))
                tok->isAttributePure(true);
            if (mSettings->library.isFunctionConst(tok->str(), false))
                tok->isAttributeConst(true);
        }
    }
}

void Tokenizer::simplifyAssignmentInFunctionCall()
{
    // std::cout << __FUNCTION__ << std::endl;
    for (Token *tok = list.front(); tok; tok = tok->next()) {
        if (tok->str() == "(")
            tok = tok->link();

        // Find 'foo(var='. Exclude 'assert(var=' to allow tests to check that assert(...) does not contain side-effects
        else if (Token::Match(tok, "[;{}] %name% ( %name% =") &&
                 Token::simpleMatch(tok->linkAt(2), ") ;") &&
                 !Token::Match(tok->next(), "assert|while")) {
            const std::string& funcname(tok->next()->str());
            Token* const vartok = tok->tokAt(3);

            // Goto ',' or ')'..
            for (Token *tok2 = vartok->tokAt(2); tok2; tok2 = tok2->next()) {
                if (tok2->link() && Token::Match(tok2, "(|[|{"))
                    tok2 = tok2->link();
                else if (tok2->str() == ";")
                    break;
                else if (Token::Match(tok2, ")|,")) {
                    tok2 = tok2->previous();

                    tok2->insertToken(vartok->str());
                    tok2->next()->setVarId(vartok->varId());

                    tok2->insertToken("(");
                    Token::createMutualLinks(tok2->next(), tok->linkAt(2));

                    tok2->insertToken(funcname);
                    tok2->insertToken(";");

                    Token::eraseTokens(tok, vartok);
                    break;
                }
            }
        }
    }
}

void Tokenizer::createSymbolDatabase()
{
    if (!mSymbolDatabase)
        mSymbolDatabase = new SymbolDatabase(this, mSettings, mErrorLogger);
    mSymbolDatabase->validate();
}

void Tokenizer::deleteSymbolDatabase()
{
    delete mSymbolDatabase;
    mSymbolDatabase = nullptr;
}

// remove unnecessary member qualification..
void Tokenizer::removeUnnecessaryQualification()
{
    std::vector<Space> classInfo;
    for (Token *tok = list.front(); tok; tok = tok->next()) {
        if (Token::Match(tok, "class|struct %type% :|{") &&
            (!tok->previous() || tok->previous()->str() != "enum")) {
            Space info;
            tok = tok->next();
            info.className = tok->str();
            tok = tok->next();
            while (tok && tok->str() != "{")
                tok = tok->next();
            if (!tok)
                return;
            info.bodyEnd = tok->link();
            classInfo.push_back(info);
        } else if (!classInfo.empty()) {
            if (tok == classInfo.back().bodyEnd)
                classInfo.pop_back();
        }
    }
}

void Tokenizer::printUnknownTypes() const
{
    if (!mSymbolDatabase)
        return;

    std::multimap<std::string, const Token *> unknowns;

    for (unsigned int i = 1; i <= mVarId; ++i) {
        const Variable *var = mSymbolDatabase->getVariableFromVarId(i);
        if (!var)
            continue;
        // is unknown type?
        if (var->type() || var->typeStartToken()->isStandardType())
            continue;

        std::string name;
        const Token * nameTok;

        // single token type?
        if (var->typeStartToken() == var->typeEndToken()) {
            nameTok = var->typeStartToken();
            name = nameTok->str();
        }

        // complicated type
        else {
            const Token *tok = var->typeStartToken();
            int level = 0;

            nameTok =  tok;

            while (tok) {
                // skip pointer and reference part of type
                if (level == 0 && Token::Match(tok, "*|&"))
                    break;

                name += tok->str();

                if (Token::Match(tok, "struct|enum"))
                    name += " ";

                // pointers and references are OK in template
                else if (tok->str() == "<")
                    ++level;
                else if (tok->str() == ">")
                    --level;

                if (tok == var->typeEndToken())
                    break;

                tok = tok->next();
            }
        }

        unknowns.insert(std::pair<std::string, const Token *>(name, nameTok));
    }

    if (!unknowns.empty()) {
        std::string last;
        size_t count = 0;

        for (std::multimap<std::string, const Token *>::const_iterator it = unknowns.begin(); it != unknowns.end(); ++it) {
            // skip types is std namespace because they are not interesting
            if (it->first.find("std::") != 0) {
                if (it->first != last) {
                    last = it->first;
                    count = 1;
                    reportError(it->second, Severity::debug, "debug", "Unknown type \'" + it->first + "\'.");
                } else {
                    if (count < 3) // limit same type to 3
                        reportError(it->second, Severity::debug, "debug", "Unknown type \'" + it->first + "\'.");
                    count++;
                }
            }
        }
    }
}

void Tokenizer::simplifyMathExpressions()
{
    for (Token *tok = list.front(); tok; tok = tok->next()) {

        //simplify Pythagorean trigonometric identity: pow(sin(x),2)+pow(cos(x),2) = 1
        //                                             pow(cos(x),2)+pow(sin(x),2) = 1
        // @todo: sin(x) * sin(x) + cos(x) * cos(x) = 1
        //        cos(x) * cos(x) + sin(x) * sin(x) = 1
        //simplify Hyperbolic identity: pow(sinh(x),2)-pow(cosh(x),2) = -1
        //                              pow(cosh(x),2)-pow(sinh(x),2) = -1
        // @todo: sinh(x) * sinh(x) - cosh(x) * cosh(x) = -1
        //        cosh(x) * cosh(x) - sinh(x) * sinh(x) = -1
        if (Token::Match(tok, "pow (")) {
            if (Token::Match(tok->tokAt(2), "sin (")) {
                Token * const tok2 = tok->linkAt(3);
                if (!Token::Match(tok2, ") , %num% ) + pow ( cos ("))
                    continue;
                const std::string& leftExponent = tok2->strAt(2);
                if (!isTwoNumber(leftExponent))
                    continue; // left exponent is not 2
                const Token * const tok3 = tok2->tokAt(8);
                Token * const tok4 = tok3->link();
                if (!Token::Match(tok4, ") , %num% )"))
                    continue;
                const std::string& rightExponent = tok4->strAt(2);
                if (!isTwoNumber(rightExponent))
                    continue; // right exponent is not 2
                if (tok->tokAt(3)->stringifyList(tok2->next()) == tok3->stringifyList(tok4->next())) {
                    Token::eraseTokens(tok, tok4->tokAt(4));
                    tok->str("1");
                }
            } else if (Token::Match(tok->tokAt(2), "cos (")) {
                Token * const tok2 = tok->linkAt(3);
                if (!Token::Match(tok2, ") , %num% ) + pow ( sin ("))
                    continue;
                const std::string& leftExponent = tok2->strAt(2);
                if (!isTwoNumber(leftExponent))
                    continue; // left exponent is not 2
                const Token * const tok3 = tok2->tokAt(8);
                Token * const tok4 = tok3->link();
                if (!Token::Match(tok4, ") , %num% )"))
                    continue;
                const std::string& rightExponent = tok4->strAt(2);
                if (!isTwoNumber(rightExponent))
                    continue; // right exponent is not 2
                if (tok->tokAt(3)->stringifyList(tok2->next()) == tok3->stringifyList(tok4->next())) {
                    Token::eraseTokens(tok, tok4->tokAt(4));
                    tok->str("1");
                }
            } else if (Token::Match(tok->tokAt(2), "sinh (")) {
                Token * const tok2 = tok->linkAt(3);
                if (!Token::Match(tok2, ") , %num% ) - pow ( cosh ("))
                    continue;
                const std::string& leftExponent = tok2->strAt(2);
                if (!isTwoNumber(leftExponent))
                    continue; // left exponent is not 2
                const Token * const tok3 = tok2->tokAt(8);
                Token * const tok4 = tok3->link();
                if (!Token::Match(tok4, ") , %num% )"))
                    continue;
                const std::string& rightExponent = tok4->strAt(2);
                if (!isTwoNumber(rightExponent))
                    continue; // right exponent is not 2
                if (tok->tokAt(3)->stringifyList(tok2->next()) == tok3->stringifyList(tok4->next())) {
                    Token::eraseTokens(tok, tok4->tokAt(4));
                    tok->str("-1");
                }
            } else if (Token::Match(tok->tokAt(2), "cosh (")) {
                Token * const tok2 = tok->linkAt(3);
                if (!Token::Match(tok2, ") , %num% ) - pow ( sinh ("))
                    continue;
                const std::string& leftExponent = tok2->strAt(2);
                if (!isTwoNumber(leftExponent))
                    continue; // left exponent is not 2
                const Token * const tok3 = tok2->tokAt(8);
                Token * const tok4 = tok3->link();
                if (!Token::Match(tok4, ") , %num% )"))
                    continue;
                const std::string& rightExponent = tok4->strAt(2);
                if (!isTwoNumber(rightExponent))
                    continue; // right exponent is not 2
                if (tok->tokAt(3)->stringifyList(tok2->next()) == tok3->stringifyList(tok4->next())) {
                    Token::eraseTokens(tok, tok4->tokAt(4));
                    tok->str("-1");
                }
            }
        }
    }
}

bool Tokenizer::simplifyStrlen()
{
    // replace strlen(str)
    bool modified=false;
    for (Token *tok = list.front(); tok; tok = tok->next()) {
        if (Token::Match(tok, "strlen ( %str% )")) {
            tok->str(MathLib::toString(Token::getStrLength(tok->tokAt(2))));
            tok->deleteNext(3);
            modified=true;
        }
    }
    return modified;
}

void Tokenizer::prepareTernaryOpForAST()
{
    // http://en.cppreference.com/w/cpp/language/operator_precedence says about ternary operator:
    //       "The expression in the middle of the conditional operator (between ? and :) is parsed as if parenthesized: its precedence relative to ?: is ignored."
    // The AST parser relies on this function to add such parentheses where necessary.
    for (Token* tok = list.front(); tok; tok = tok->next()) {
        if (tok->str() == "?") {
            bool parenthesesNeeded = false;
            unsigned int depth = 0;
            Token* tok2 = tok->next();
            for (; tok2; tok2 = tok2->next()) {
                if (tok2->link() && Token::Match(tok2, "[|(|<"))
                    tok2 = tok2->link();
                else if (tok2->str() == ":") {
                    if (depth == 0)
                        break;
                    depth--;
                } else if (tok2->str() == ";" || (tok2->link() && tok2->str() != "{" && tok2->str() != "}"))
                    break;
                else if (tok2->str() == ",")
                    parenthesesNeeded = true;
                else if (tok2->str() == "<")
                    parenthesesNeeded = true;
                else if (tok2->str() == "?") {
                    depth++;
                    parenthesesNeeded = true;
                }
            }
            if (parenthesesNeeded && tok2 && tok2->str() == ":") {
                tok->insertToken("(");
                tok2->insertToken(")", emptyString, true);
                Token::createMutualLinks(tok->next(), tok2->previous());
            }
        }
    }
}

void Tokenizer::reportError(const Token* tok, const Severity::SeverityType severity, const std::string& id, const std::string& msg, bool inconclusive) const
{
    const std::list<const Token*> callstack(1, tok);
    reportError(callstack, severity, id, msg, inconclusive);
}

void Tokenizer::reportError(const std::list<const Token*>& callstack, Severity::SeverityType severity, const std::string& id, const std::string& msg, bool inconclusive) const
{
    const ErrorLogger::ErrorMessage errmsg(callstack, &list, severity, id, msg, inconclusive);
    if (mErrorLogger)
        mErrorLogger->reportErr(errmsg);
    else
        Check::reportError(errmsg);
}

void Tokenizer::setPodTypes()
{
    if (!mSettings)
        return;
    for (Token *tok = list.front(); tok; tok = tok->next()) {
        if (!tok->isName())
            continue;

        // pod type
        const struct Library::PodType *podType = mSettings->library.podtype(tok->str());
        if (podType) {
            const Token *prev = tok->previous();
            while (prev && prev->isName())
                prev = prev->previous();
            if (prev && !Token::Match(prev, ";|{|}|,|("))
                continue;
            tok->isStandardType(true);
        }
    }
}


Tokenizer::VariableMap::VariableMap() : mVarId(0) {}

void Tokenizer::VariableMap::enterScope()
{
    mScopeInfo.push(std::list<std::pair<std::string, unsigned int>>());
}

bool Tokenizer::VariableMap::leaveScope()
{
    if (mScopeInfo.empty())
        return false;

    for (const std::pair<std::string, unsigned int> &outerVariable : mScopeInfo.top()) {
        if (outerVariable.second != 0)
            mVariableId[outerVariable.first] = outerVariable.second;
        else
            mVariableId.erase(outerVariable.first);
    }
    mScopeInfo.pop();
    return true;
}

void Tokenizer::VariableMap::addVariable(const std::string &varname)
{
    if (mScopeInfo.empty()) {
        mVariableId[varname] = ++mVarId;
        return;
    }
    std::map<std::string, unsigned int>::iterator it = mVariableId.find(varname);
    if (it == mVariableId.end()) {
        mScopeInfo.top().push_back(std::pair<std::string, unsigned int>(varname, 0));
        mVariableId[varname] = ++mVarId;
        return;
    }
    mScopeInfo.top().push_back(std::pair<std::string, unsigned int>(varname, it->second));
    it->second = ++mVarId;
}
