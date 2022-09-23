/*
 * simplecpp - A simple and high-fidelity C/C++ preprocessor library
 * Copyright (C) 2016 Daniel Marjamäki.
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#if defined(_WIN32) || defined(__CYGWIN__) || defined(__MINGW32__)
#define SIMPLECPP_WINDOWS
#define NOMINMAX
#endif
#include "simplecpp.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <utility>

#ifdef SIMPLECPP_WINDOWS
#include <windows.h>
#undef ERROR
#undef TRUE
#endif

static bool isHex(const std::string &s)
{
    return s.size()>2 && (s.compare(0,2,"0x")==0 || s.compare(0,2,"0X")==0);
}


static const simplecpp::TokenString DEFINE("define");
static const simplecpp::TokenString UNDEF("undef");

static const simplecpp::TokenString INCLUDE("include");
static const simplecpp::TokenString USES("uses");

static const simplecpp::TokenString ERROR("error");
static const simplecpp::TokenString WARNING("warning");

static const simplecpp::TokenString IF("if");
static const simplecpp::TokenString IFDEF("ifdef");
static const simplecpp::TokenString IFNDEF("ifndef");
static const simplecpp::TokenString DEFINED("defined");
static const simplecpp::TokenString ELSE("else");
static const simplecpp::TokenString ELIF("elif");
static const simplecpp::TokenString ENDIF("endif");

static const simplecpp::TokenString PRAGMA("pragma");
static const simplecpp::TokenString ONCE("once");

template<class T> static std::string toString(T t)
{
    std::ostringstream ostr;
    ostr << t;
    return ostr.str();
}

static long long stringToLL(const std::string &s)
{
    long long ret;
    const bool hex = isHex(s);
    std::istringstream istr(hex ? s.substr(2) : s);
    if (hex)
        istr >> std::hex;
    istr >> ret;
    return ret;
}

static unsigned long long stringToULL(const std::string &s)
{
    unsigned long long ret;
    const bool hex = isHex(s);
    std::istringstream istr(hex ? s.substr(2) : s);
    if (hex)
        istr >> std::hex;
    istr >> ret;
    return ret;
}

static bool startsWith(const std::string &str, const std::string &s)
{
    return (str.size() >= s.size() && str.compare(0, s.size(), s) == 0);
}

static bool endsWith(const std::string &s, const std::string &e)
{
    return (s.size() >= e.size() && s.compare(s.size() - e.size(), e.size(), e) == 0);
}

static bool sameline(const simplecpp::Token *tok1, const simplecpp::Token *tok2)
{
    return tok1 && tok2 && tok1->location.sameline(tok2->location);
}

static bool isAlternativeBinaryOp(const simplecpp::Token *tok, const std::string &alt)
{
    return (tok->name &&
            tok->str() == alt &&
            tok->previous &&
            tok->next &&
            (tok->previous->number || tok->previous->name || tok->previous->op == ')') &&
            (tok->next->number || tok->next->name || tok->next->op == '('));
}

static bool isAlternativeUnaryOp(const simplecpp::Token *tok, const std::string &alt)
{
    return ((tok->name && tok->str() == alt) &&
            (!tok->previous || tok->previous->op == '(') &&
            (tok->next && (tok->next->name || tok->next->number)));
}


const std::string simplecpp::Location::emptyFileName;

void simplecpp::Location::adjust(const std::string &str)
{
    if (str.find_first_of("\r\n") == std::string::npos) {
        col += str.size();
        return;
    }

    for (std::size_t i = 0U; i < str.size(); ++i) {
        col++;
        if (str[i] == '\n' || str[i] == '\r') {
            col = 1;
            line++;
            if (str[i] == '\r' && (i+1)<str.size() && str[i+1]=='\n')
                ++i;
        }
    }
}

bool simplecpp::Token::isOneOf(const char ops[]) const
{
    return (op != '\0') && (std::strchr(ops, op) != 0);
}

bool simplecpp::Token::startsWithOneOf(const char c[]) const
{
    return std::strchr(c, string[0]) != 0;
}

bool simplecpp::Token::endsWithOneOf(const char c[]) const
{
    return std::strchr(c, string[string.size() - 1U]) != 0;
}

void simplecpp::Token::printAll() const
{
    const Token *tok = this;
    while (tok->previous)
        tok = tok->previous;
    for (; tok; tok = tok->next) {
        if (tok->previous) {
            std::cout << (sameline(tok, tok->previous) ? ' ' : '\n');
        }
        std::cout << tok->str();
    }
    std::cout << std::endl;
}

void simplecpp::Token::printOut() const
{
    for (const Token *tok = this; tok; tok = tok->next) {
        if (tok != this) {
            std::cout << (sameline(tok, tok->previous) ? ' ' : '\n');
        }
        std::cout << tok->str();
    }
    std::cout << std::endl;
}

simplecpp::TokenList::TokenList(std::vector<std::string> &filenames) : frontToken(NULL), backToken(NULL), files(filenames) {}

simplecpp::TokenList::TokenList(std::istream &istr, std::vector<std::string> &filenames, const std::string &filename, OutputList *outputList)
    : frontToken(NULL), backToken(NULL), files(filenames)
{
    readfile(istr,filename,outputList);
}

simplecpp::TokenList::TokenList(const TokenList &other) : frontToken(NULL), backToken(NULL), files(other.files)
{
    *this = other;
}

simplecpp::TokenList::~TokenList()
{
    clear();
}

simplecpp::TokenList &simplecpp::TokenList::operator=(const TokenList &other)
{
    if (this != &other) {
        clear();
        for (const Token *tok = other.cfront(); tok; tok = tok->next)
            push_back(new Token(*tok));
        sizeOfType = other.sizeOfType;
    }
    return *this;
}

void simplecpp::TokenList::clear()
{
    backToken = NULL;
    while (frontToken) {
        Token *next = frontToken->next;
        delete frontToken;
        frontToken = next;
    }
    sizeOfType.clear();
}

void simplecpp::TokenList::push_back(Token *tok)
{
    if (!frontToken)
        frontToken = tok;
    else
        backToken->next = tok;
    tok->previous = backToken;
    backToken = tok;
}

void simplecpp::TokenList::dump() const
{
    std::cout << stringify() << std::endl;
}

std::string simplecpp::TokenList::stringify() const
{
    std::ostringstream ret;
    Location loc(files);
    for (const Token *tok = cfront(); tok; tok = tok->next) {
        if (tok->location.line < loc.line || tok->location.fileIndex != loc.fileIndex) {
            ret << "\n#line " << tok->location.line << " \"" << tok->location.file() << "\"\n";
            loc = tok->location;
        }

        while (tok->location.line > loc.line) {
            ret << '\n';
            loc.line++;
        }

        if (sameline(tok->previous, tok))
            ret << ' ';

        ret << tok->str();

        loc.adjust(tok->str());
    }

    return ret.str();
}

static unsigned char readChar(std::istream &istr, unsigned int bom)
{
    unsigned char ch = (unsigned char)istr.get();

    // For UTF-16 encoded files the BOM is 0xfeff/0xfffe. If the
    // character is non-ASCII character then replace it with 0xff
    if (bom == 0xfeff || bom == 0xfffe) {
        const unsigned char ch2 = (unsigned char)istr.get();
        const int ch16 = (bom == 0xfeff) ? (ch<<8 | ch2) : (ch2<<8 | ch);
        ch = (unsigned char)((ch16 >= 0x80) ? 0xff : ch16);
    }

    // Handling of newlines..
    if (ch == '\r') {
        ch = '\n';
        if (bom == 0 && (char)istr.peek() == '\n')
            (void)istr.get();
        else if (bom == 0xfeff || bom == 0xfffe) {
            int c1 = istr.get();
            int c2 = istr.get();
            int ch16 = (bom == 0xfeff) ? (c1<<8 | c2) : (c2<<8 | c1);
            if (ch16 != '\n') {
                istr.unget();
                istr.unget();
            }
        }
    }

    return ch;
}

static unsigned char peekChar(std::istream &istr, unsigned int bom)
{
    unsigned char ch = (unsigned char)istr.peek();

    // For UTF-16 encoded files the BOM is 0xfeff/0xfffe. If the
    // character is non-ASCII character then replace it with 0xff
    if (bom == 0xfeff || bom == 0xfffe) {
        (void)istr.get();
        const unsigned char ch2 = (unsigned char)istr.peek();
        istr.unget();
        const int ch16 = (bom == 0xfeff) ? (ch<<8 | ch2) : (ch2<<8 | ch);
        ch = (unsigned char)((ch16 >= 0x80) ? 0xff : ch16);
    }

    // Handling of newlines..
    if (ch == '\r')
        ch = '\n';

    return ch;
}

static void ungetChar(std::istream &istr, unsigned int bom)
{
    istr.unget();
    if (bom == 0xfeff || bom == 0xfffe)
        istr.unget();
}

static unsigned char prevChar(std::istream &istr, unsigned int bom)
{
    ungetChar(istr, bom);
    ungetChar(istr, bom);
    unsigned char c = readChar(istr, bom);
    readChar(istr, bom);
    return c;
}

static unsigned short getAndSkipBOM(std::istream &istr)
{
    const int ch1 = istr.peek();

    // The UTF-16 BOM is 0xfffe or 0xfeff.
    if (ch1 >= 0xfe) {
        unsigned short bom = ((unsigned char)istr.get() << 8);
        if (istr.peek() >= 0xfe)
            return bom | (unsigned char)istr.get();
        istr.unget();
        return 0;
    }

    // Skip UTF-8 BOM 0xefbbbf
    if (ch1 == 0xef) {
        (void)istr.get();
        if (istr.get() == 0xbb && istr.peek() == 0xbf) {
            (void)istr.get();
        } else {
            istr.unget();
            istr.unget();
        }
    }

    return 0;
}

static bool isNameChar(unsigned char ch)
{
    return std::isalnum(ch) || ch == '_' || ch == '$';
}

static std::string escapeString(const std::string &str)
{
    std::ostringstream ostr;
    ostr << '\"';
    for (std::size_t i = 1U; i < str.size() - 1; ++i) {
        char c = str[i];
        if (c == '\\' || c == '\"' || c == '\'')
            ostr << '\\';
        ostr << c;
    }
    ostr << '\"';
    return ostr.str();
}

static void portabilityBackslash(simplecpp::OutputList *outputList, const std::vector<std::string> &files, const simplecpp::Location &location)
{
    if (!outputList)
        return;
    simplecpp::Output err(files);
    err.type = simplecpp::Output::PORTABILITY_BACKSLASH;
    err.location = location;
    err.msg = "Combination 'backslash space newline' is not portable.";
    outputList->push_back(err);
}

static bool isStringLiteralPrefix(const std::string &str)
{
    return str == "u" || str == "U" || str == "L" || str == "u8" ||
           str == "R" || str == "uR" || str == "UR" || str == "LR" || str == "u8R";
}

void simplecpp::TokenList::readfile(std::istream &istr, const std::string &filename, OutputList *outputList)
{
    std::stack<simplecpp::Location> loc;

    unsigned int multiline = 0U;

    const Token *oldLastToken = NULL;

    const unsigned short bom = getAndSkipBOM(istr);

    Location location(files);
    location.fileIndex = fileIndex(filename);
    location.line = 1U;
    location.col  = 1U;
    while (istr.good()) {
        unsigned char ch = readChar(istr,bom);
        if (!istr.good())
            break;
        if (ch < ' ' && ch != '\t' && ch != '\n' && ch != '\r')
            ch = ' ';

        if (ch >= 0x80) {
            if (outputList) {
                simplecpp::Output err(files);
                err.type = simplecpp::Output::UNHANDLED_CHAR_ERROR;
                err.location = location;
                std::ostringstream s;
                s << (int)ch;
                err.msg = "The code contains unhandled character(s) (character code=" + s.str() + "). Neither unicode nor extended ascii is supported.";
                outputList->push_back(err);
            }
            clear();
            return;
        }

        if (ch == '\n') {
            if (cback() && cback()->op == '\\') {
                if (location.col > cback()->location.col + 1U)
                    portabilityBackslash(outputList, files, cback()->location);
                ++multiline;
                deleteToken(back());
            } else {
                location.line += multiline + 1;
                multiline = 0U;
            }
            if (!multiline)
                location.col = 1;

            if (oldLastToken != cback()) {
                oldLastToken = cback();
                const std::string lastline(lastLine());
                if (lastline == "# file %str%") {
                    loc.push(location);
                    location.fileIndex = fileIndex(cback()->str().substr(1U, cback()->str().size() - 2U));
                    location.line = 1U;
                } else if (lastline == "# line %num%") {
                    loc.push(location);
                    location.line = std::atol(cback()->str().c_str());
                } else if (lastline == "# line %num% %str%") {
                    loc.push(location);
                    location.fileIndex = fileIndex(cback()->str().substr(1U, cback()->str().size() - 2U));
                    location.line = std::atol(cback()->previous->str().c_str());
                } else if (lastline == "# %num% %str%") {
                    loc.push(location);
                    location.fileIndex = fileIndex(cback()->str().substr(1U, cback()->str().size() - 2U));
                    location.line = std::atol(cback()->previous->str().c_str());
                }
                // #endfile
                else if (lastline == "# endfile" && !loc.empty()) {
                    location = loc.top();
                    loc.pop();
                }
            }

            continue;
        }

        if (std::isspace(ch)) {
            location.col++;
            continue;
        }

        TokenString currentToken;

        if (cback() && cback()->location.line == location.line && cback()->previous && cback()->previous->op == '#' && (lastLine() == "# error" || lastLine() == "# warning")) {
            char prev = ' ';
            while (istr.good() && (prev == '\\' || (ch != '\r' && ch != '\n'))) {
                currentToken += ch;
                prev = ch;
                ch = readChar(istr, bom);
            }
            ungetChar(istr, bom);
            push_back(new Token(currentToken, location));
            location.adjust(currentToken);
            continue;
        }

        // number or name
        if (isNameChar(ch)) {
            const bool num = (std::isdigit(ch) == 0);
            while (istr.good() && isNameChar(ch)) {
                currentToken += ch;
                ch = readChar(istr,bom);
                if (num && ch=='\'' && isNameChar(peekChar(istr,bom)))
                    ch = readChar(istr,bom);
            }

            ungetChar(istr,bom);
        }

        // comment
        else if (ch == '/' && peekChar(istr,bom) == '/') {
            while (istr.good() && ch != '\r' && ch != '\n') {
                currentToken += ch;
                ch = readChar(istr, bom);
            }
            const std::string::size_type pos = currentToken.find_last_not_of(" \t");
            if (pos < currentToken.size() - 1U && currentToken[pos] == '\\')
                portabilityBackslash(outputList, files, location);
            if (currentToken[currentToken.size() - 1U] == '\\') {
                ++multiline;
                currentToken.erase(currentToken.size() - 1U);
            } else {
                ungetChar(istr, bom);
            }
        }

        // comment
        else if (ch == '/' && peekChar(istr,bom) == '*') {
            currentToken = "/*";
            (void)readChar(istr,bom);
            ch = readChar(istr,bom);
            while (istr.good()) {
                currentToken += ch;
                if (currentToken.size() >= 4U && endsWith(currentToken, "*/"))
                    break;
                ch = readChar(istr,bom);
            }
            // multiline..

            std::string::size_type pos = 0;
            while ((pos = currentToken.find("\\\n",pos)) != std::string::npos) {
                currentToken.erase(pos,2);
                ++multiline;
            }
            if (multiline || startsWith(lastLine(10),"# ")) {
                pos = 0;
                while ((pos = currentToken.find('\n',pos)) != std::string::npos) {
                    currentToken.erase(pos,1);
                    ++multiline;
                }
            }
        }

        // string / char literal
        else if (ch == '\"' || ch == '\'') {
            std::string prefix;
            if (cback() && cback()->name && !std::isspace(prevChar(istr, bom)) && (isStringLiteralPrefix(cback()->str()))) {
                prefix = cback()->str();
            }
            // C++11 raw string literal
            if (ch == '\"' && !prefix.empty() && *cback()->str().rbegin() == 'R') {
                std::string delim;
                currentToken = ch;
                prefix.resize(prefix.size() - 1);
                ch = readChar(istr,bom);
                while (istr.good() && ch != '(' && ch != '\n') {
                    delim += ch;
                    ch = readChar(istr,bom);
                }
                if (!istr.good() || ch == '\n') {
                    if (outputList) {
                        Output err(files);
                        err.type = Output::SYNTAX_ERROR;
                        err.location = location;
                        err.msg = "Invalid newline in raw string delimiter.";
                        outputList->push_back(err);
                    }
                    return;
                }
                const std::string endOfRawString(')' + delim + currentToken);
                while (istr.good() && !(endsWith(currentToken, endOfRawString) && currentToken.size() > 1))
                    currentToken += readChar(istr,bom);
                if (!endsWith(currentToken, endOfRawString)) {
                    if (outputList) {
                        Output err(files);
                        err.type = Output::SYNTAX_ERROR;
                        err.location = location;
                        err.msg = "Raw string missing terminating delimiter.";
                        outputList->push_back(err);
                    }
                    return;
                }
                currentToken.erase(currentToken.size() - endOfRawString.size(), endOfRawString.size() - 1U);
                currentToken = escapeString(currentToken);
                currentToken.insert(0, prefix);
                back()->setstr(currentToken);
                location.adjust(currentToken);
                if (currentToken.find_first_of("\r\n") == std::string::npos)
                    location.col += 2 + 2 * delim.size();
                else
                    location.col += 1 + delim.size();

                continue;
            }

            currentToken = readUntil(istr,location,ch,ch,outputList,bom);
            if (currentToken.size() < 2U)
                // Error is reported by readUntil()
                return;

            std::string s = currentToken;
            std::string::size_type pos;
            int newlines = 0;
            while ((pos = s.find_first_of("\r\n")) != std::string::npos) {
                s.erase(pos,1);
                newlines++;
            }

            if (prefix.empty())
                push_back(new Token(s, location)); // push string without newlines
            else
                back()->setstr(prefix + s);

            if (newlines > 0 && lastLine().compare(0,9,"# define ") == 0) {
                multiline += newlines;
                location.adjust(s);
            } else {
                location.adjust(currentToken);
            }
            continue;
        }

        else {
            currentToken += ch;
        }

        if ( (currentToken == "<") && ( (lastLine() == "# include")  || (lastLine() == "# uses") ) )
        {
            currentToken = readUntil(istr, location, '<', '>', outputList, bom);
            if (currentToken.size() < 2U)
                return;
        }

        push_back(new Token(currentToken, location));

        if (multiline)
            location.col += currentToken.size();
        else
            location.adjust(currentToken);
    }

    combineOperators();
}

void simplecpp::TokenList::constFold()
{
    while (cfront()) {
        // goto last '('
        Token *tok = back();
        while (tok && tok->op != '(')
            tok = tok->previous;

        // no '(', goto first token
        if (!tok)
            tok = front();

        // Constant fold expression
        constFoldUnaryNotPosNeg(tok);
        constFoldMulDivRem(tok);
        constFoldAddSub(tok);
        constFoldShift(tok);
        constFoldComparison(tok);
        constFoldBitwise(tok);
        constFoldLogicalOp(tok);
        constFoldQuestionOp(&tok);

        // If there is no '(' we are done with the constant folding
        if (tok->op != '(')
            break;

        if (!tok->next || !tok->next->next || tok->next->next->op != ')')
            break;

        tok = tok->next;
        deleteToken(tok->previous);
        deleteToken(tok->next);
    }
}

static bool isFloatSuffix(const simplecpp::Token *tok)
{
    if (!tok || tok->str().size() != 1U)
        return false;
    const char c = std::tolower(tok->str()[0]);
    return c == 'f' || c == 'l';
}

void simplecpp::TokenList::combineOperators()
{
    std::stack<bool> executableScope;
    executableScope.push(false);
    for (Token *tok = front(); tok; tok = tok->next) {
        if (tok->op == '{') {
            if (executableScope.top()) {
                executableScope.push(true);
                continue;
            }
            const Token *prev = tok->previous;
            while (prev && prev->isOneOf(";{}()"))
                prev = prev->previous;
            executableScope.push(prev && prev->op == ')');
            continue;
        }
        if (tok->op == '}') {
            if (executableScope.size() > 1)
                executableScope.pop();
            continue;
        }

        if (tok->op == '.') {
            if (tok->previous && tok->previous->op == '.')
                continue;
            if (tok->next && tok->next->op == '.')
                continue;
            // float literals..
            if (tok->previous && tok->previous->number) {
                tok->setstr(tok->previous->str() + '.');
                deleteToken(tok->previous);
                if (isFloatSuffix(tok->next) || (tok->next && tok->next->startsWithOneOf("Ee"))) {
                    tok->setstr(tok->str() + tok->next->str());
                    deleteToken(tok->next);
                }
            }
            if (tok->next && tok->next->number) {
                tok->setstr(tok->str() + tok->next->str());
                deleteToken(tok->next);
            }
        }
        // match: [0-9.]+E [+-] [0-9]+
        const char lastChar = tok->str()[tok->str().size() - 1];
        if (tok->number && !isHex(tok->str()) && (lastChar == 'E' || lastChar == 'e') && tok->next && tok->next->isOneOf("+-") && tok->next->next && tok->next->next->number) {
            tok->setstr(tok->str() + tok->next->op + tok->next->next->str());
            deleteToken(tok->next);
            deleteToken(tok->next);
        }

        if (tok->op == '\0' || !tok->next || tok->next->op == '\0')
            continue;
        if (!sameline(tok,tok->next))
            continue;
        if (tok->location.col + 1U != tok->next->location.col)
            continue;

        if (tok->next->op == '=' && tok->isOneOf("=!<>+-*/%&|^")) {
            if (tok->op == '&' && !executableScope.top()) {
                // don't combine &= if it is a anonymous reference parameter with default value:
                // void f(x&=2)
                int indentlevel = 0;
                const Token *start = tok;
                while (indentlevel >= 0 && start) {
                    if (start->op == ')')
                        ++indentlevel;
                    else if (start->op == '(')
                        --indentlevel;
                    else if (start->isOneOf(";{}"))
                        break;
                    start = start->previous;
                }
                if (indentlevel == -1 && start) {
                    const Token *ftok = start;
                    bool isFuncDecl = ftok->name;
                    while (isFuncDecl) {
                        if (!start->name && start->str() != "::" && start->op != '*' && start->op != '&')
                            isFuncDecl = false;
                        if (!start->previous)
                            break;
                        if (start->previous->isOneOf(";{}:"))
                            break;
                        start = start->previous;
                    }
                    isFuncDecl &= start != ftok && start->name;
                    if (isFuncDecl) {
                        // TODO: we could loop through the parameters here and check if they are correct.
                        continue;
                    }
                }
            }
            tok->setstr(tok->str() + "=");
            deleteToken(tok->next);
        } else if ((tok->op == '|' || tok->op == '&') && tok->op == tok->next->op) {
            tok->setstr(tok->str() + tok->next->str());
            deleteToken(tok->next);
        } else if (tok->op == ':' && tok->next->op == ':') {
            tok->setstr(tok->str() + tok->next->str());
            deleteToken(tok->next);
        } else if (tok->op == '-' && tok->next->op == '>') {
            tok->setstr(tok->str() + tok->next->str());
            deleteToken(tok->next);
        } else if ((tok->op == '<' || tok->op == '>') && tok->op == tok->next->op) {
            tok->setstr(tok->str() + tok->next->str());
            deleteToken(tok->next);
            if (tok->next && tok->next->op == '=') {
                tok->setstr(tok->str() + tok->next->str());
                deleteToken(tok->next);
            }
        } else if ((tok->op == '+' || tok->op == '-') && tok->op == tok->next->op) {
            if (tok->location.col + 1U != tok->next->location.col)
                continue;
            if (tok->previous && tok->previous->number)
                continue;
            if (tok->next->next && tok->next->next->number)
                continue;
            tok->setstr(tok->str() + tok->next->str());
            deleteToken(tok->next);
        }
    }
}

static const std::string COMPL("compl");
static const std::string NOT("not");
void simplecpp::TokenList::constFoldUnaryNotPosNeg(simplecpp::Token *tok)
{
    for (; tok && tok->op != ')'; tok = tok->next) {
        // "not" might be !
        if (isAlternativeUnaryOp(tok, NOT))
            tok->op = '!';
        // "compl" might be ~
        else if (isAlternativeUnaryOp(tok, COMPL))
            tok->op = '~';

        if (tok->op == '!' && tok->next && tok->next->number) {
            tok->setstr(tok->next->str() == "0" ? "1" : "0");
            deleteToken(tok->next);
        } else if (tok->op == '~' && tok->next && tok->next->number) {
            tok->setstr(toString(~stringToLL(tok->next->str())));
            deleteToken(tok->next);
        } else {
            if (tok->previous && (tok->previous->number || tok->previous->name))
                continue;
            if (!tok->next || !tok->next->number)
                continue;
            switch (tok->op) {
            case '+':
                tok->setstr(tok->next->str());
                deleteToken(tok->next);
                break;
            case '-':
                tok->setstr(tok->op + tok->next->str());
                deleteToken(tok->next);
                break;
            }
        }
    }
}

void simplecpp::TokenList::constFoldMulDivRem(Token *tok)
{
    for (; tok && tok->op != ')'; tok = tok->next) {
        if (!tok->previous || !tok->previous->number)
            continue;
        if (!tok->next || !tok->next->number)
            continue;

        long long result;
        if (tok->op == '*')
            result = (stringToLL(tok->previous->str()) * stringToLL(tok->next->str()));
        else if (tok->op == '/' || tok->op == '%') {
            long long rhs = stringToLL(tok->next->str());
            if (rhs == 0)
                throw std::overflow_error("division/modulo by zero");
            long long lhs = stringToLL(tok->previous->str());
            if (rhs == -1 && lhs == std::numeric_limits<long long>::min())
                throw std::overflow_error("division overflow");
            if (tok->op == '/')
                result = (lhs / rhs);
            else
                result = (lhs % rhs);
        } else
            continue;

        tok = tok->previous;
        tok->setstr(toString(result));
        deleteToken(tok->next);
        deleteToken(tok->next);
    }
}

void simplecpp::TokenList::constFoldAddSub(Token *tok)
{
    for (; tok && tok->op != ')'; tok = tok->next) {
        if (!tok->previous || !tok->previous->number)
            continue;
        if (!tok->next || !tok->next->number)
            continue;

        long long result;
        if (tok->op == '+')
            result = stringToLL(tok->previous->str()) + stringToLL(tok->next->str());
        else if (tok->op == '-')
            result = stringToLL(tok->previous->str()) - stringToLL(tok->next->str());
        else
            continue;

        tok = tok->previous;
        tok->setstr(toString(result));
        deleteToken(tok->next);
        deleteToken(tok->next);
    }
}

void simplecpp::TokenList::constFoldShift(Token *tok)
{
    for (; tok && tok->op != ')'; tok = tok->next) {
        if (!tok->previous || !tok->previous->number)
            continue;
        if (!tok->next || !tok->next->number)
            continue;

        long long result;
        if (tok->str() == "<<")
            result = stringToLL(tok->previous->str()) << stringToLL(tok->next->str());
        else if (tok->str() == ">>")
            result = stringToLL(tok->previous->str()) >> stringToLL(tok->next->str());
        else
            continue;

        tok = tok->previous;
        tok->setstr(toString(result));
        deleteToken(tok->next);
        deleteToken(tok->next);
    }
}

static const std::string NOTEQ("not_eq");
void simplecpp::TokenList::constFoldComparison(Token *tok)
{
    for (; tok && tok->op != ')'; tok = tok->next) {
        if (isAlternativeBinaryOp(tok,NOTEQ))
            tok->setstr("!=");

        if (!tok->startsWithOneOf("<>=!"))
            continue;
        if (!tok->previous || !tok->previous->number)
            continue;
        if (!tok->next || !tok->next->number)
            continue;

        int result;
        if (tok->str() == "==")
            result = (stringToLL(tok->previous->str()) == stringToLL(tok->next->str()));
        else if (tok->str() == "!=")
            result = (stringToLL(tok->previous->str()) != stringToLL(tok->next->str()));
        else if (tok->str() == ">")
            result = (stringToLL(tok->previous->str()) > stringToLL(tok->next->str()));
        else if (tok->str() == ">=")
            result = (stringToLL(tok->previous->str()) >= stringToLL(tok->next->str()));
        else if (tok->str() == "<")
            result = (stringToLL(tok->previous->str()) < stringToLL(tok->next->str()));
        else if (tok->str() == "<=")
            result = (stringToLL(tok->previous->str()) <= stringToLL(tok->next->str()));
        else
            continue;

        tok = tok->previous;
        tok->setstr(toString(result));
        deleteToken(tok->next);
        deleteToken(tok->next);
    }
}

static const std::string BITAND("bitand");
static const std::string BITOR("bitor");
static const std::string XOR("xor");
void simplecpp::TokenList::constFoldBitwise(Token *tok)
{
    Token * const tok1 = tok;
    for (const char *op = "&^|"; *op; op++) {
        const std::string* altop;
        if (*op == '&')
            altop = &BITAND;
        else if (*op == '|')
            altop = &BITOR;
        else
            altop = &XOR;
        for (tok = tok1; tok && tok->op != ')'; tok = tok->next) {
            if (tok->op != *op && !isAlternativeBinaryOp(tok, *altop))
                continue;
            if (!tok->previous || !tok->previous->number)
                continue;
            if (!tok->next || !tok->next->number)
                continue;
            long long result;
            if (*op == '&')
                result = (stringToLL(tok->previous->str()) & stringToLL(tok->next->str()));
            else if (*op == '^')
                result = (stringToLL(tok->previous->str()) ^ stringToLL(tok->next->str()));
            else /*if (*op == '|')*/
                result = (stringToLL(tok->previous->str()) | stringToLL(tok->next->str()));
            tok = tok->previous;
            tok->setstr(toString(result));
            deleteToken(tok->next);
            deleteToken(tok->next);
        }
    }
}

static const std::string AND("and");
static const std::string OR("or");
void simplecpp::TokenList::constFoldLogicalOp(Token *tok)
{
    for (; tok && tok->op != ')'; tok = tok->next) {
        if (tok->name) {
            if (isAlternativeBinaryOp(tok,AND))
                tok->setstr("&&");
            else if (isAlternativeBinaryOp(tok,OR))
                tok->setstr("||");
        }
        if (tok->str() != "&&" && tok->str() != "||")
            continue;
        if (!tok->previous || !tok->previous->number)
            continue;
        if (!tok->next || !tok->next->number)
            continue;

        int result;
        if (tok->str() == "||")
            result = (stringToLL(tok->previous->str()) || stringToLL(tok->next->str()));
        else /*if (tok->str() == "&&")*/
            result = (stringToLL(tok->previous->str()) && stringToLL(tok->next->str()));

        tok = tok->previous;
        tok->setstr(toString(result));
        deleteToken(tok->next);
        deleteToken(tok->next);
    }
}

void simplecpp::TokenList::constFoldQuestionOp(Token **tok1)
{
    bool gotoTok1 = false;
    for (Token *tok = *tok1; tok && tok->op != ')'; tok =  gotoTok1 ? *tok1 : tok->next) {
        gotoTok1 = false;
        if (tok->str() != "?")
            continue;
        if (!tok->previous || !tok->next || !tok->next->next)
            throw std::runtime_error("invalid expression");
        if (!tok->previous->number)
            continue;
        if (tok->next->next->op != ':')
            continue;
        Token * const condTok = tok->previous;
        Token * const trueTok = tok->next;
        Token * const falseTok = trueTok->next->next;
        if (!falseTok)
            throw std::runtime_error("invalid expression");
        if (condTok == *tok1)
            *tok1 = (condTok->str() != "0" ? trueTok : falseTok);
        deleteToken(condTok->next); // ?
        deleteToken(trueTok->next); // :
        deleteToken(condTok->str() == "0" ? trueTok : falseTok);
        deleteToken(condTok);
        gotoTok1 = true;
    }
}

void simplecpp::TokenList::removeComments()
{
    Token *tok = frontToken;
    while (tok) {
        Token *tok1 = tok;
        tok = tok->next;
        if (tok1->comment)
            deleteToken(tok1);
    }
}

std::string simplecpp::TokenList::readUntil(std::istream &istr, const Location &location, const char start, const char end, OutputList *outputList, unsigned int bom)
{
    std::string ret;
    ret += start;

    bool backslash = false;
    char ch = 0;
    while (ch != end && ch != '\r' && ch != '\n' && istr.good()) {
        ch = readChar(istr, bom);
        if (backslash && ch == '\n') {
            ch = 0;
            backslash = false;
            continue;
        }
        backslash = false;
        ret += ch;
        if (ch == '\\') {
            const char next = readChar(istr, bom);
            if (next == '\r' || next == '\n') {
                ret.erase(ret.size()-1U);
                backslash = (next == '\r');
            }
            ret += next;
        }
    }

    if (!istr.good() || ch != end) {
        clear();
        if (outputList) {
            Output err(files);
            err.type = Output::SYNTAX_ERROR;
            err.location = location;
            err.msg = std::string("No pair for character (") + start + "). Can't process file. File is either invalid or unicode, which is currently not supported.";
            outputList->push_back(err);
        }
        return "";
    }

    return ret;
}

std::string simplecpp::TokenList::lastLine(int maxsize) const
{
    std::string ret;
    int count = 0;
    for (const Token *tok = cback(); sameline(tok,cback()); tok = tok->previous) {
        if (tok->comment)
            continue;
        if (!ret.empty())
            ret = ' ' + ret;
        ret = (tok->str()[0] == '\"' ? std::string("%str%")
               : tok->number ? std::string("%num%") : tok->str()) + ret;
        if (++count > maxsize)
            return "";
    }
    return ret;
}

unsigned int simplecpp::TokenList::fileIndex(const std::string &filename)
{
    for (unsigned int i = 0; i < files.size(); ++i) {
        if (files[i] == filename)
            return i;
    }
    files.push_back(filename);
    return files.size() - 1U;
}


namespace simplecpp {

    std::string convertCygwinToWindowsPath(const std::string &cygwinPath)
    {
        std::string windowsPath;

        std::string::size_type pos = 0;
        if (cygwinPath.size() >= 11 && startsWith(cygwinPath, "/cygdrive/")) {
            unsigned char driveLetter = cygwinPath[10];
            if (std::isalpha(driveLetter)) {
                if (cygwinPath.size() == 11) {
                    windowsPath = toupper(driveLetter) + ":\\";   // volume root directory
                    pos = 11;
                } else if (cygwinPath[11] == '/') {
                    windowsPath = toupper(driveLetter) + ":";
                    pos = 11;
                }
            }
        }

        for (; pos < cygwinPath.size(); ++pos) {
            unsigned char c = cygwinPath[pos];
            if (c == '/')
                c = '\\';
            windowsPath += c;
        }

        return windowsPath;
    }
}

#ifdef SIMPLECPP_WINDOWS

class ScopedLock {
public:
    explicit ScopedLock(CRITICAL_SECTION& criticalSection)
        : m_criticalSection(criticalSection) {
        EnterCriticalSection(&m_criticalSection);
    }

    ~ScopedLock() {
        LeaveCriticalSection(&m_criticalSection);
    }

private:
    ScopedLock& operator=(const ScopedLock&);
    ScopedLock(const ScopedLock&);

    CRITICAL_SECTION& m_criticalSection;
};

class RealFileNameMap {
public:
    RealFileNameMap() {
        InitializeCriticalSection(&m_criticalSection);
    }

    ~RealFileNameMap() {
        DeleteCriticalSection(&m_criticalSection);
    }

    bool getCacheEntry(const std::string& path, std::string* returnPath) {
        ScopedLock lock(m_criticalSection);

        std::map<std::string, std::string>::iterator it = m_fileMap.find(path);
        if (it != m_fileMap.end()) {
            *returnPath = it->second;
            return true;
        }
        return false;
    }

    void addToCache(const std::string& path, const std::string& actualPath) {
        ScopedLock lock(m_criticalSection);
        m_fileMap[path] = actualPath;
    }

private:
    std::map<std::string, std::string> m_fileMap;
    CRITICAL_SECTION m_criticalSection;
};

static RealFileNameMap realFileNameMap;

static bool realFileName(const std::string &f, std::string *result)
{
    // are there alpha characters in last subpath?
    bool alpha = false;
    for (std::string::size_type pos = 1; pos <= f.size(); ++pos) {
        unsigned char c = f[f.size() - pos];
        if (c == '/' || c == '\\')
            break;
        if (std::isalpha(c)) {
            alpha = true;
            break;
        }
    }

    // do not convert this path if there are no alpha characters (either pointless or cause wrong results for . and ..)
    if (!alpha)
        return false;

    // Lookup filename or foldername on file system
    if (!realFileNameMap.getCacheEntry(f, result)) {

        WIN32_FIND_DATAA FindFileData;

#ifdef __CYGWIN__
        std::string fConverted = simplecpp::convertCygwinToWindowsPath(f);
        HANDLE hFind = FindFirstFileExA(fConverted.c_str(), FindExInfoBasic, &FindFileData, FindExSearchNameMatch, NULL, 0);
#else
        HANDLE hFind = FindFirstFileExA(f.c_str(), FindExInfoBasic, &FindFileData, FindExSearchNameMatch, NULL, 0);
#endif

        if (INVALID_HANDLE_VALUE == hFind)
            return false;
        *result = FindFileData.cFileName;
        realFileNameMap.addToCache(f, *result);
        FindClose(hFind);
    }
    return true;
}

static RealFileNameMap realFilePathMap;

/** Change case in given path to match filesystem */
static std::string realFilename(const std::string &f)
{
    std::string ret;
    ret.reserve(f.size()); // this will be the final size
    if (realFilePathMap.getCacheEntry(f, &ret))
        return ret;

    // Current subpath
    std::string subpath;

    for (std::string::size_type pos = 0; pos < f.size(); ++pos) {
        unsigned char c = f[pos];

        // Separator.. add subpath and separator
        if (c == '/' || c == '\\') {
            // if subpath is empty just add separator
            if (subpath.empty()) {
                ret += c;
                continue;
            }

            bool isDriveSpecification = 
                (pos == 2 && subpath.size() == 2 && std::isalpha(subpath[0]) && subpath[1] == ':');

            // Append real filename (proper case)
            std::string f2;
            if (!isDriveSpecification && realFileName(f.substr(0, pos), &f2))
                ret += f2;
            else
                ret += subpath;

            subpath.clear();

            // Append separator
            ret += c;
        } else {
            subpath += c;
        }
    }

    if (!subpath.empty()) {
        std::string f2;
        if (realFileName(f,&f2))
            ret += f2;
        else
            ret += subpath;
    }

    realFilePathMap.addToCache(f, ret);
    return ret;
}

static bool isAbsolutePath(const std::string &path)
{
    if (path.length() >= 3 && path[0] > 0 && std::isalpha(path[0]) && path[1] == ':' && (path[2] == '\\' || path[2] == '/'))
        return true;
    return path.length() > 1U && (path[0] == '/' || path[0] == '\\');
}
#else
#define realFilename(f)  f

static bool isAbsolutePath(const std::string &path)
{
    return path.length() > 1U && path[0] == '/';
}
#endif

namespace simplecpp {
    /**
     * perform path simplifications for . and ..
     */
    std::string simplifyPath(std::string path)
    {
        if (path.empty())
            return path;

        std::string::size_type pos;

        // replace backslash separators
        std::replace(path.begin(), path.end(), '\\', '/');

        const bool unc(path.compare(0,2,"//") == 0);

        // replace "//" with "/"
        pos = 0;
        while ((pos = path.find("//",pos)) != std::string::npos) {
            path.erase(pos,1);
        }

        // remove "./"
        pos = 0;
        while ((pos = path.find("./",pos)) != std::string::npos) {
            if (pos == 0 || path[pos - 1U] == '/')
                path.erase(pos,2);
            else
                pos += 2;
        }

        // remove trailing dot if path ends with "/."
        if (endsWith(path,"/."))
            path.erase(path.size()-1);

        // simplify ".."
        pos = 1; // don't simplify ".." if path starts with that
        while ((pos = path.find("/..", pos)) != std::string::npos) {
            // not end of path, then string must be "/../"
            if (pos + 3 < path.size() && path[pos + 3] != '/') {
                ++pos;
                continue;
            }
            // get previous subpath
            const std::string::size_type pos1 = path.rfind('/', pos - 1U) + 1U;
            const std::string previousSubPath = path.substr(pos1, pos-pos1);
            if (previousSubPath == "..") {
                // don't simplify
                ++pos;
            } else {
                // remove previous subpath and ".."
                path.erase(pos1,pos-pos1+4);
                if (path.empty())
                    path = ".";
                // update pos
                pos = (pos1 == 0) ? 1 : (pos1 - 1);
            }
        }

        // Remove trailing '/'?
        //if (path.size() > 1 && endsWith(path, "/"))
        //    path.erase(path.size()-1);

        if (unc)
            path = '/' + path;

        return path.find_first_of("*?") == std::string::npos ? realFilename(path) : path;
    }
}

static const char * const altopData[] = {"and","or","bitand","bitor","compl","not","not_eq","xor"};
static const std::set<std::string> altop(&altopData[0], &altopData[8]);
static void simplifyName(simplecpp::TokenList &expr)
{
    for (simplecpp::Token *tok = expr.front(); tok; tok = tok->next) {
        if (tok->name) {
            if (altop.find(tok->str()) != altop.end()) {
                bool alt;
                if (tok->str() == "not" || tok->str() == "compl") {
                    alt = isAlternativeUnaryOp(tok,tok->str());
                } else {
                    alt = isAlternativeBinaryOp(tok,tok->str());
                }
                if (alt)
                    continue;
            }
            tok->setstr("0");
        }
    }
}

static void simplifyNumbers(simplecpp::TokenList &expr)
{
    for (simplecpp::Token *tok = expr.front(); tok; tok = tok->next) {
        if (tok->str().size() == 1U)
            continue;
        if (tok->str().compare(0,2,"0x") == 0)
            tok->setstr(toString(stringToULL(tok->str())));
        else if (tok->str()[0] == '\'')
            tok->setstr(toString(tok->str()[1] & 0xffU));
    }
}

static long long evaluate(simplecpp::TokenList &expr, const std::map<std::string, std::size_t> &sizeOfType)
{
    simplifyName(expr);
    simplifyNumbers(expr);
    expr.constFold();
    // TODO: handle invalid expressions
    return expr.cfront() && expr.cfront() == expr.cback() && expr.cfront()->number ? stringToLL(expr.cfront()->str()) : 0LL;
}

static const simplecpp::Token *gotoNextLine(const simplecpp::Token *tok)
{
    const unsigned int line = tok->location.line;
    const unsigned int file = tok->location.fileIndex;
    while (tok && tok->location.line == line && tok->location.fileIndex == file)
        tok = tok->next;
    return tok;
}

#ifdef SIMPLECPP_WINDOWS

class NonExistingFilesCache {
public:
    NonExistingFilesCache() {
        InitializeCriticalSection(&m_criticalSection);
    }

    ~NonExistingFilesCache() {
        DeleteCriticalSection(&m_criticalSection);
    }

    bool contains(const std::string& path) {
        ScopedLock lock(m_criticalSection);
        return (m_pathSet.find(path) != m_pathSet.end());
    }

    void add(const std::string& path) {
        ScopedLock lock(m_criticalSection);
        m_pathSet.insert(path);
    }

private:
    std::set<std::string> m_pathSet;
    CRITICAL_SECTION m_criticalSection;
};

static NonExistingFilesCache nonExistingFilesCache;

#endif

static std::string _openHeader(std::ifstream &f, const std::string &path)
{
#ifdef SIMPLECPP_WINDOWS
    std::string simplePath = simplecpp::simplifyPath(path);
    
    if (nonExistingFilesCache.contains(simplePath))
    {
        return "";  // file is known not to exist, skip expensive file open call
    }

    
    f.open(simplePath.c_str());

    if (f.is_open())
    {
        return simplePath;
    }
    else {
        nonExistingFilesCache.add(simplePath);
        return "";
    }
#else
    f.open(path.c_str());
    return f.is_open() ? simplecpp::simplifyPath(path) : "";
#endif
}

static bool isCtrlFile(const std::string &path)
{
    if (path == "")
        return false;
    const std::string::size_type dotLocation = path.find_last_of('.');
    std::string extension = (dotLocation == std::string::npos) ? "" : path.substr(dotLocation);

    return extension == ".ctl";
}


/// @warning we dont check ctrl share libraries (.dll)
/// @warning we dont crypted ctrl libs (.ctc)
static std::string openHeader(std::ifstream &f, const simplecpp::DUI &dui, const std::string &sourcefile, const std::string &header, bool systemheader)
{
    if (isAbsolutePath(header)) {
        // is absolute path given.
        /// @todo we can add this as a check, becouse using of absolute pathes is really bad idea.
        return _openHeader(f, header);
    }

    if ( isCtrlFile(sourcefile) )
    {
        // is ctrl code
        // try to find the ctrl lib relative to project directory.
        const size_t pos = sourcefile.find("/scripts/");

        if ( pos == std::string::npos ) {
            return "";
        }

        const std::string s = sourcefile.substr(0, pos) + "/scripts/libs/" + header + ".ctl";
        std::string simplePath = _openHeader(f, s);
        if ( !simplePath.empty() )
            return simplePath;
    }
    else if (!systemheader) {
        // cpp and c code
        if (sourcefile.find_first_of("\\/") != std::string::npos) {
            const std::string s = sourcefile.substr(0, sourcefile.find_last_of("\\/") + 1U) + header;
            std::string simplePath = _openHeader(f, s);
            if (!simplePath.empty())
                return simplePath;
        } else {
            std::string simplePath = _openHeader(f, header);
            if (!simplePath.empty())
                return simplePath;
        }
    }

    // include sub dirs given by option --include, -I or --includes-file
    // we use this option to find the ctrl libs defined in sub-projects or in WinCC OA version dir
    for (std::list<std::string>::const_iterator it = dui.includePaths.begin(); it != dui.includePaths.end(); ++it) {
        std::string s = *it;
        if (!s.empty() && s[s.size()-1U]!='/' && s[s.size()-1U]!='\\')
            s += '/';

        if ( isCtrlFile(sourcefile) )
            s += "scripts/libs/" + header + ".ctl"; // realtive to sub-project directory
        else
          s += header; // c, cpp support

        std::string simplePath = _openHeader(f, s);
        if (!simplePath.empty())
            return simplePath;
    }

    return "";
}

static std::string getFileName(const std::map<std::string, simplecpp::TokenList *> &filedata, const std::string &sourcefile, const std::string &header, const simplecpp::DUI &dui, bool systemheader)
{
    if (filedata.empty()) {
        return "";
    }
    if (isAbsolutePath(header)) {
        return (filedata.find(header) != filedata.end()) ? simplecpp::simplifyPath(header) : "";
    }
    if (isCtrlFile(sourcefile))
    {
        ///@todo search ctrl files also in included pathes
        const size_t pos = sourcefile.find("/scripts/");
        if (pos == std::string::npos) {
            return "";
        }
        const std::string s = simplecpp::simplifyPath( sourcefile.substr(0, pos) + "/scripts/libs/" + header + ".ctl");
        
        if (filedata.find(s) != filedata.end())
            return s;
        else
            return "";
    }
    if (!systemheader) {
        if (sourcefile.find_first_of("\\/") != std::string::npos) {
            const std::string s(simplecpp::simplifyPath(sourcefile.substr(0, sourcefile.find_last_of("\\/") + 1U) + header));
            if (filedata.find(s) != filedata.end())
                return s;
        } else {
            std::string s = simplecpp::simplifyPath(header);
            if (filedata.find(s) != filedata.end())
                return s;
        }
    }

    for (std::list<std::string>::const_iterator it = dui.includePaths.begin(); it != dui.includePaths.end(); ++it) {
        std::string s = *it;
        if (!s.empty() && s[s.size()-1U]!='/' && s[s.size()-1U]!='\\')
            s += '/';
        s += header;
        s = simplecpp::simplifyPath(s);
        if (filedata.find(s) != filedata.end())
            return s;
    }

    return "";
}

static bool hasFile(const std::map<std::string, simplecpp::TokenList *> &filedata, const std::string &sourcefile, const std::string &header, const simplecpp::DUI &dui, bool systemheader)
{
    return !getFileName(filedata, sourcefile, header, dui, systemheader).empty();
}

std::map<std::string, simplecpp::TokenList*> simplecpp::load(const simplecpp::TokenList &rawtokens, std::vector<std::string> &fileNumbers, const simplecpp::DUI &dui, simplecpp::OutputList *outputList)
{
    std::map<std::string, simplecpp::TokenList*> ret;

    std::list<const Token *> filelist;

    // -include files
    for (std::list<std::string>::const_iterator it = dui.includes.begin(); it != dui.includes.end(); ++it) {
        const std::string &filename = realFilename(*it);

        if (ret.find(filename) != ret.end())
            continue;

        std::ifstream fin(filename.c_str());
        if (!fin.is_open())
            continue;

        TokenList *tokenlist = new TokenList(fin, fileNumbers, filename, outputList);
        if (!tokenlist->front()) {
            delete tokenlist;
            continue;
        }

        ret[filename] = tokenlist;
        filelist.push_back(tokenlist->front());
    }

    for (const Token *rawtok = rawtokens.cfront(); rawtok || !filelist.empty(); rawtok = rawtok ? rawtok->next : NULL) {
        if (rawtok == NULL) {
            rawtok = filelist.back();
            filelist.pop_back();
        }

        if (rawtok->op != '#' || sameline(rawtok->previousSkipComments(), rawtok))
            continue;

        rawtok = rawtok->nextSkipComments();
        if ( !rawtok || (rawtok->str() != INCLUDE  && rawtok->str() != USES) )
            continue;

        const std::string &sourcefile = rawtok->location.file();

        const Token *htok = rawtok->nextSkipComments();
        if (!sameline(rawtok, htok))
            continue;

        bool systemheader = (htok->str()[0] == '<');

        const std::string header(realFilename(htok->str().substr(1U, htok->str().size() - 2U)));
        if (hasFile(ret, sourcefile, header, dui, systemheader))
            continue;

        std::ifstream f;
        const std::string header2 = openHeader(f,dui,sourcefile,header,systemheader);
        if (!f.is_open())
            continue;

        TokenList *tokens = new TokenList(f, fileNumbers, header2, outputList);
        ret[header2] = tokens;
        if (tokens->front())
            filelist.push_back(tokens->front());
    }

    return ret;
}

//-----------------------------------------------------------------------------
/**
 * Function preprocess user defines
 * BOOL_VAR --> BOOL_VAR = 262144;
 */
static bool preprocessDefines(simplecpp::TokenList &output, const simplecpp::Token *tok, const std::map<std::string, simplecpp::UserDefinedValue> &defines)
{
  if ( !tok )
  {  // defensive
      return false;
  }

  auto it = defines.find(tok->str());
  if (it != defines.end())
  {
      output.push_back(new simplecpp::Token(it->second.value, tok->location));
      return true;
  }

  return false;
}
//-----------------------------------------------------------------------------
/**
 * Function preprocess oa specific constants.
 * @todo add some how PROJ, PROJ_PATH ...
 * @todo add __FUNCTION__
 */
static bool preprocessOaConst(simplecpp::TokenList &output, const simplecpp::Token *tok)
{
  if ( !tok )
  {  // defensive
      return false;
  }
  if (tok->str() == "__FILE__")
  {
    output.push_back(new simplecpp::Token('\"'+tok->location.file()+'\"', tok->location));
    return true;
  }

  if (tok->str() == "__LINE__") {
      output.push_back(new simplecpp::Token(toString(tok->location.line), tok->location));
      return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
/**
 * Function return variable type from oa const like:
 * INT_VAR --> int
 * DYN_STRING_VAR --> dyn_string
 * ....
 */
static simplecpp::TokenString getVarType(const simplecpp::TokenString &str)
{
  // just remove _VAR and convert it to lo lover case
  std::string var = str;
  std::size_t found = var.find_last_of("_");
  var = var.substr(0, found);
  std::transform(var.begin(), var.end(), var.begin(), ::tolower);
  return var;
}

//-----------------------------------------------------------------------------
/**
 * Function cut variable name from string
 * "AS_TYPEFILTER" --> AS_TYPEFILTER
 */
static simplecpp::TokenString getVarName(const simplecpp::TokenString &str)
{
  std::string var = str;
  // check if string start and ends with "
  // ex: "abc" -- OK
  //      abc" -- NOK

  
  if ( var.size() <= 2 || !startsWith(var, "\"") || !endsWith(var, "\"") ) 
  {
      return "";
  }

  var = var.substr(1);
  var = var.substr(0, var.size() - 1);
  
  return var;
}

//-----------------------------------------------------------------------------
/**
 * Function preprocess function addGlobal.
 * addGlobal("AS_HIST_RANGE_SEC", INT_VAR) --> push on top of file 'global int AS_HIST_RANGE_SEC'
 */
static bool preprocessAddGlobal(simplecpp::TokenList &output, const simplecpp::Token *tok)
{
    // find function addGlobal(str, type);
  if ( !tok )
  {  // defensive
      return false;
  }
  if ( !tok || tok->str() != "addGlobal" )
  {
    return false;
  }

  simplecpp::Location location = tok->location;

  tok = tok->next;
  if ( !tok || tok->str() != "(" )
  {
    return false;
  }

  tok = tok->next;
  if ( !tok )
  {
    return false;
  }
  const simplecpp::Token *varName;
  varName = tok;

  tok = tok->next;
  if ( !tok || tok->str() != "," )
  {
    return false;
  }

  tok = tok->next;
  if ( !tok )
  {
    return false;
  }
  const simplecpp::Token *varType;
  varType = tok;
  
  tok = tok->next;
  if ( !tok || tok->str() != ")" )
  {
    return false;
  }

  simplecpp::TokenString str = getVarName(varName->str());
  
  if ( str == "" )
  {
    return false;
  }


  output.push_back(new simplecpp::Token("global", location));
  output.push_back(new simplecpp::Token(getVarType(varType->str()), location));
  output.push_back(new simplecpp::Token(getVarName(varName->str()), location));

  return true;
}
//-----------------------------------------------------------------------------
/**
 * Function preprocess shared_ptr.
 * shared_ptr<string> var --- Preprocess ---> string var
 */
static bool preprocessSharedPtr(simplecpp::TokenList &output, const simplecpp::Token *tok)
{

  if ( !tok || tok->str() != "shared_ptr" )
  {
    return false;
  }

  simplecpp::Location location = tok->location;

  tok = tok->next;
  if ( !tok || tok->str() != "<" )
  {
    return false;
  }

  tok = tok->next;
  if ( !tok )
  {
    return false;
  }

  const simplecpp::Token *varType;
  varType = tok;

  tok = tok->next;
  if ( !tok || tok->str() != ">" )
  {
    return false;
  }

  output.push_back(new simplecpp::Token(getVarType(varType->str()), location));

  return true;
}

//-----------------------------------------------------------------------------
/**
 * Function preprocess token
 */
static bool preprocessToken(simplecpp::TokenList &output, const simplecpp::Token **tok1, std::map<std::string, simplecpp::UserDefinedValue> defines,  std::vector<std::string> &files, simplecpp::OutputList *outputList)
{
  const simplecpp::Token *tok = *tok1;

  if ( preprocessOaConst(output, tok) )
  {
    *tok1 = tok->next;
    return true;
  }
  else if ( preprocessDefines(output, tok, defines) )
  {
    *tok1 = tok->next;
    return true;
  }
  else if ( preprocessAddGlobal(output, tok) )
  {
    *tok1 = tok->next->next->next->next->next->next;
    return true;
  }
  else if ( preprocessSharedPtr(output, tok) )
  {
    *tok1 = tok->next->next->next->next;
    return true;
  }
  if (!tok->comment)
  {
    
    output.push_back(new simplecpp::Token(*tok));
  }

  *tok1 = tok->next;
  return true;
}


//-----------------------------------------------------------------------------
std::set<std::string> checkedHeaders;
void simplecpp::preprocess(simplecpp::TokenList &output, const simplecpp::TokenList &rawtokens, std::vector<std::string> &files, std::map<std::string, simplecpp::TokenList *> &filedata, const simplecpp::DUI &dui, simplecpp::OutputList *outputList)
{
    std::map<std::string, std::size_t> sizeOfType(rawtokens.sizeOfType);
    sizeOfType.insert(std::pair<std::string, std::size_t>("char", sizeof(char)));
    sizeOfType.insert(std::pair<std::string, std::size_t>("short", sizeof(short)));
    sizeOfType.insert(std::pair<std::string, std::size_t>("short int", sizeOfType["short"]));
    sizeOfType.insert(std::pair<std::string, std::size_t>("int", sizeof(int)));
    sizeOfType.insert(std::pair<std::string, std::size_t>("long", sizeof(long)));
    sizeOfType.insert(std::pair<std::string, std::size_t>("long int", sizeOfType["long"]));
    sizeOfType.insert(std::pair<std::string, std::size_t>("long long", sizeof(long long)));
    sizeOfType.insert(std::pair<std::string, std::size_t>("float", sizeof(float)));
    sizeOfType.insert(std::pair<std::string, std::size_t>("double", sizeof(double)));
    sizeOfType.insert(std::pair<std::string, std::size_t>("long double", sizeof(long double)));
    sizeOfType.insert(std::pair<std::string, std::size_t>("char *", sizeof(char *)));
    sizeOfType.insert(std::pair<std::string, std::size_t>("short *", sizeof(short *)));
    sizeOfType.insert(std::pair<std::string, std::size_t>("short int *", sizeOfType["short *"]));
    sizeOfType.insert(std::pair<std::string, std::size_t>("int *", sizeof(int *)));
    sizeOfType.insert(std::pair<std::string, std::size_t>("long *", sizeof(long *)));
    sizeOfType.insert(std::pair<std::string, std::size_t>("long int *", sizeOfType["long *"]));
    sizeOfType.insert(std::pair<std::string, std::size_t>("long long *", sizeof(long long *)));
    sizeOfType.insert(std::pair<std::string, std::size_t>("float *", sizeof(float *)));
    sizeOfType.insert(std::pair<std::string, std::size_t>("double *", sizeof(double *)));
    sizeOfType.insert(std::pair<std::string, std::size_t>("long double *", sizeof(long double *)));

    // TRUE => code in current #if block should be kept
    // ELSE_IS_TRUE => code in current #if block should be dropped. the code in the #else should be kept.
    // ALWAYS_FALSE => drop all code in #if and #else
    enum IfState { TRUE, ELSE_IS_TRUE, ALWAYS_FALSE };
    std::stack<int> ifstates;
    ifstates.push(TRUE);

    std::stack<const Token *> includetokenstack;

    std::set<std::string> pragmaOnce;



    includetokenstack.push(rawtokens.cfront());
    for (std::list<std::string>::const_iterator it = dui.includes.begin(); it != dui.includes.end(); ++it) {
        const std::map<std::string, TokenList*>::const_iterator f = filedata.find(*it);
        if (f != filedata.end())
            includetokenstack.push(f->second->cfront());
    }
    

    for (const Token *rawtok = NULL; rawtok || !includetokenstack.empty();) {
        if (rawtok == NULL) {
            rawtok = includetokenstack.top();
            includetokenstack.pop();
            continue;
        }

        if (rawtok->op == '#' && !sameline(rawtok->previous, rawtok)) {
            if (!sameline(rawtok, rawtok->next)) {
                rawtok = rawtok->next;
                continue;
            }
            rawtok = rawtok->next;
            if (!rawtok->name) {
                rawtok = gotoNextLine(rawtok);
                continue;
            }

            if (ifstates.size() <= 1U && (rawtok->str() == ELIF || rawtok->str() == ELSE || rawtok->str() == ENDIF)) {
                if (outputList) {
                    simplecpp::Output err(files);
                    err.type = Output::SYNTAX_ERROR;
                    err.location = rawtok->location;
                    err.msg = "#" + rawtok->str() + " without #if";
                    outputList->push_back(err);
                }
                output.clear();
                return;
            }

            if (ifstates.top() == TRUE && (rawtok->str() == ERROR || rawtok->str() == WARNING)) {
                if (outputList) {
                    simplecpp::Output err(rawtok->location.files);
                    err.type = rawtok->str() == ERROR ? Output::ERROR : Output::WARNING;
                    err.location = rawtok->location;
                    for (const Token *tok = rawtok->next; tok && sameline(rawtok,tok); tok = tok->next) {
                        if (!err.msg.empty() && isNameChar(tok->str()[0]))
                            err.msg += ' ';
                        err.msg += tok->str();
                    }
                    err.msg = '#' + rawtok->str() + ' ' + err.msg;
                    outputList->push_back(err);
                }
                if (rawtok->str() == ERROR) {
                    output.clear();
                    return;
                }
            }

            if ( (ifstates.top() == TRUE) && ( (rawtok->str() == INCLUDE) || (rawtok->str() == USES) ) )
            {
                TokenList inc1(files);
                for (const Token *inctok = rawtok->next; sameline(rawtok,inctok); inctok = inctok->next) {
                    if (!inctok->comment)
                        inc1.push_back(new Token(*inctok));
                }
                TokenList inc2(files);
                if (!inc1.empty() && inc1.cfront()->name) {
                    const Token *inctok = inc1.cfront();
                    if (!preprocessToken(inc2, &inctok, dui.defines, files, outputList)) {
                        output.clear();
                        return;
                    }
                } else {
                    inc2.takeTokens(inc1);
                }

                if (!inc2.empty() && inc2.cfront()->op == '<' && inc2.cback()->op == '>') {
                    TokenString hdr;
                    // TODO: Sometimes spaces must be added in the string
                    // Somehow preprocessToken etc must be told that the location should be source location not destination location
                    for (const Token *tok = inc2.cfront(); tok; tok = tok->next) {
                        hdr += tok->str();
                    }
                    inc2.clear();
                    inc2.push_back(new Token(hdr, inc1.cfront()->location));
                    inc2.front()->op = '<';
                }

                if (inc2.empty() || inc2.cfront()->str().size() <= 2U) {
                    if (outputList) {
                        simplecpp::Output err(files);
                        err.type = Output::SYNTAX_ERROR;
                        err.location = rawtok->location;
                        err.msg = "No header in #include";
                        outputList->push_back(err);
                    }
                    output.clear();
                    return;
                }

                const Token *inctok = inc2.cfront();

                const bool systemheader = (inctok->op == '<');
                const std::string header(realFilename(inctok->str().substr(1U, inctok->str().size() - 2U)));

                // cache it for better performance
                // it is necessarry to check it. Otherwise it will check the included files more times.
                if (checkedHeaders.find(header) != checkedHeaders.end()) {
                    continue;
                }
                checkedHeaders.insert(header);

                std::string header2 = getFileName(filedata, rawtok->location.file(), header, dui, systemheader);
                
                if (header2.empty()) {
                    // try to load file..
                    std::ifstream f;
                    header2 = openHeader(f, dui, rawtok->location.file(), header, systemheader);
                    if (f.is_open()) {
                        TokenList *tokens = new TokenList(f, files, header2, outputList);
                        filedata[header2] = tokens;
                    }
                }

                if (header2.empty()) {
                    if (outputList) {
                        simplecpp::Output out(files);
                        out.type = Output::MISSING_HEADER;
                        out.location = rawtok->location;
                        out.msg = "Header not found: " + inctok->str();
                        outputList->push_back(out);
                    }
                } else if (includetokenstack.size() >= 400) {
                    if (outputList) {
                        simplecpp::Output out(files);
                        out.type = Output::INCLUDE_NESTED_TOO_DEEPLY;
                        out.location = rawtok->location;
                        out.msg = "#include nested too deeply";
                        outputList->push_back(out);
                    }
                } else if (pragmaOnce.find(header2) == pragmaOnce.end()) {
                    includetokenstack.push(gotoNextLine(rawtok));
                    const TokenList *includetokens = filedata.find(header2)->second;
                    rawtok = includetokens ? includetokens->cfront() : 0;
                    continue;
                }
            } else if (rawtok->str() == IF || rawtok->str() == IFDEF || rawtok->str() == IFNDEF || rawtok->str() == ELIF) {
                if (!sameline(rawtok,rawtok->next)) {
                    if (outputList) {
                        simplecpp::Output out(files);
                        out.type = Output::SYNTAX_ERROR;
                        out.location = rawtok->location;
                        out.msg = "Syntax error in #" + rawtok->str();
                        outputList->push_back(out);
                    }
                    output.clear();
                    return;
                }

                bool conditionIsTrue;
                if (ifstates.top() == ALWAYS_FALSE || (ifstates.top() == ELSE_IS_TRUE && rawtok->str() != ELIF))
                    conditionIsTrue = false;
                else { /*if (rawtok->str() == IF || rawtok->str() == ELIF)*/
                    TokenList expr(files);
                    for (const Token *tok = rawtok->next; tok && tok->location.sameline(rawtok->location); tok = tok->next) {
                        if (!tok->name) {
                            expr.push_back(new Token(*tok));
                            continue;
                        }

                        if (tok->str() == DEFINED) {
                            tok = tok->next;
                            const bool par = (tok && tok->op == '(');
                            if (par)
                                tok = tok->next;
                            if (tok) {
                                expr.push_back(new Token("0", tok->location));
                            }
                            if (par)
                                tok = tok ? tok->next : NULL;
                            if (!tok || !sameline(rawtok,tok) || (par && tok->op != ')')) {
                                if (outputList) {
                                    Output out(rawtok->location.files);
                                    out.type = Output::SYNTAX_ERROR;
                                    out.location = rawtok->location;
                                    out.msg = "failed to evaluate " + std::string(rawtok->str() == IF ? "#if" : "#elif") + " condition";
                                    outputList->push_back(out);
                                }
                                output.clear();
                                return;
                            }
                            continue;
                        }

                        const Token *tmp = tok;
                        if (!preprocessToken(expr, &tmp, dui.defines, files, outputList)) {
                            output.clear();
                            return;
                        }
                    }
                    try {
                        conditionIsTrue = (evaluate(expr, sizeOfType) != 0);
                    } catch (const std::exception &e) {
                        if (outputList) {
                            Output out(rawtok->location.files);
                            out.type = Output::SYNTAX_ERROR;
                            out.location = rawtok->location;
                            out.msg = "failed to evaluate " + std::string(rawtok->str() == IF ? "#if" : "#elif") + " condition";
                            if (e.what() && *e.what())
                                out.msg += std::string(", ") + e.what();
                            outputList->push_back(out);
                        }
                        output.clear();
                        return;
                    }
                }

                if (rawtok->str() != ELIF) {
                    // push a new ifstate..
                    if (ifstates.top() != TRUE)
                        ifstates.push(ALWAYS_FALSE);
                    else
                        ifstates.push(conditionIsTrue ? TRUE : ELSE_IS_TRUE);
                } else if (ifstates.top() == TRUE) {
                    ifstates.top() = ALWAYS_FALSE;
                } else if (ifstates.top() == ELSE_IS_TRUE && conditionIsTrue) {
                    ifstates.top() = TRUE;
                }
            } else if (rawtok->str() == ELSE) {
                ifstates.top() = (ifstates.top() == ELSE_IS_TRUE) ? TRUE : ALWAYS_FALSE;
            } else if (rawtok->str() == ENDIF) {
                ifstates.pop();
            } else if (ifstates.top() == TRUE && rawtok->str() == PRAGMA && rawtok->next && rawtok->next->str() == ONCE && sameline(rawtok,rawtok->next)) {
                pragmaOnce.insert(rawtok->location.file());
            }
            rawtok = gotoNextLine(rawtok);
            continue;
        }

        if (ifstates.top() != TRUE) {
            // drop code
            rawtok = gotoNextLine(rawtok);
            continue;
        }

        bool hash=false, hashhash=false;
        if (rawtok->op == '#' && sameline(rawtok,rawtok->next)) {
            if (rawtok->next->op != '#') {
                hash = true;
                rawtok = rawtok->next; // skip '#'
            } else if (sameline(rawtok,rawtok->next->next)) {
                hashhash = true;
                rawtok = rawtok->next->next; // skip '#' '#'
            }
        }

        const Location loc(rawtok->location);
        TokenList tokens(files);

        if (!preprocessToken(tokens, &rawtok, dui.defines, files, outputList)) {
            output.clear();
            return;
        }

        if (hash || hashhash) {
            std::string s;
            for (const Token *hashtok = tokens.cfront(); hashtok; hashtok = hashtok->next)
                s += hashtok->str();
            if (hash)
                output.push_back(new Token('\"' + s + '\"', loc));
            else if (output.back())
                output.back()->setstr(output.cback()->str() + s);
            else
                output.push_back(new Token(s, loc));
        } else {
            output.takeTokens(tokens);
        }
    }
}

void simplecpp::cleanup(std::map<std::string, TokenList*> &filedata)
{
    for (std::map<std::string, TokenList*>::iterator it = filedata.begin(); it != filedata.end(); ++it)
        delete it->second;
    filedata.clear();
}
