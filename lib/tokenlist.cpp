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
#include "tokenlist.h"

#include "errorlogger.h"
#include "mathlib.h"
#include "path.h"
#include "settings.h"
#include "token.h"

#include <simplecpp.h>
#include <cctype>
#include <cstring>
#include <set>
#include <stack>

// How many compileExpression recursions are allowed?
// For practical code this could be endless. But in some special torture test
// there needs to be a limit.
static const unsigned int AST_MAX_DEPTH = 50U;


TokenList::TokenList(const Settings* settings) :
    mTokensFrontBack(),
    mSettings(settings),
    mIsCtrl(false)
{
}

TokenList::~TokenList()
{
    deallocateTokens();
}

//---------------------------------------------------------------------------

const std::string& TokenList::getSourceFilePath() const
{
    if (getFiles().empty()) {
        return emptyString;
    }
    return getFiles()[0];
}

//---------------------------------------------------------------------------

// Deallocate lists..
void TokenList::deallocateTokens()
{
    deleteTokens(mTokensFrontBack.front);
    mTokensFrontBack.front = nullptr;
    mTokensFrontBack.back = nullptr;
    mFiles.clear();
}

unsigned int TokenList::appendFileIfNew(const std::string &fileName)
{
    // Has this file been tokenized already?
    for (std::size_t i = 0; i < mFiles.size(); ++i)
        if (Path::sameFileName(mFiles[i], fileName))
            return (unsigned int)i;

    // The "mFiles" vector remembers what files have been tokenized..
    mFiles.push_back(fileName);

    // Update mIsC and mIsCpp properties
    if (mFiles.size() == 1) { // Update only useful if first file added to _files
        if (!mSettings) {
            mIsCtrl = Path::isCTRL(getSourceFilePath());
        } else {
            mIsCtrl = mSettings->enforcedLang == Settings::CTRL || (mSettings->enforcedLang == Settings::None && Path::isCTRL(getSourceFilePath()));
        }
    }
    return mFiles.size() - 1U;
}

void TokenList::deleteTokens(Token *tok)
{
    while (tok) {
        Token *next = tok->next();
        delete tok;
        tok = next;
    }
}


//---------------------------------------------------------------------------
// InsertTokens - Copy and insert tokens
//---------------------------------------------------------------------------

void TokenList::insertTokens(Token *dest, const Token *src, unsigned int n)
{
    // std::cout << "insert tokens " << dest->str() << " "  << src->str() << std::endl;
    std::stack<Token *> link;

    while (n > 0) {
        dest->insertToken(src->str(), src->originalName());
        dest = dest->next();

        // Set links
        if (Token::Match(dest, "<|(|[|{"))
            link.push(dest);
        else if (!link.empty() && Token::Match(dest, ">|)|]|}")) {
            Token::createMutualLinks(dest, link.top());
            link.pop();
        }

        dest->fileIndex(src->fileIndex());
        dest->linenr(src->linenr());
        dest->setVarId(src->varId());
        dest->tokType(src->tokType());
        dest->flags(src->flags());
        src  = src->next();
        --n;
    }
}

//---------------------------------------------------------------------------
// Tokenize - tokenizes a given file.
//---------------------------------------------------------------------------

bool TokenList::createTokens(std::istream &code, const std::string& file0)
{
    appendFileIfNew(file0);

    simplecpp::OutputList outputList;
    simplecpp::TokenList tokens(code, mFiles, file0, &outputList);

    createTokens(&tokens);

    return outputList.empty();
}

//---------------------------------------------------------------------------

void TokenList::createTokens(const simplecpp::TokenList *tokenList)
{
    if (tokenList->cfront())
        mOrigFiles = mFiles = tokenList->cfront()->location.files;
    else
        mFiles.clear();

    mIsCtrl = false;
    if (!mFiles.empty()) {
        mIsCtrl = Path::isCTRL(getSourceFilePath());
    }
    if (mSettings && mSettings->enforcedLang != Settings::None) {
        mIsCtrl = (mSettings->enforcedLang == Settings::CTRL);
    }

    for (const simplecpp::Token *tok = tokenList->cfront(); tok; tok = tok->next) {

        std::string str = tok->str();

        // Replace hexadecimal value with decimal
        // TODO: Remove this
        const bool isHex = MathLib::isIntHex(str) ;
        if (isHex || MathLib::isOct(str) || MathLib::isBin(str)) {
            // TODO: It would be better if TokenList didn't simplify hexadecimal numbers
            std::string suffix;
            if (isHex &&
                mSettings &&
                str.size() == (2 + mSettings->int_bit / 4) &&
                (str[2] >= '8') &&  // includes A-F and a-f
                MathLib::getSuffix(str).empty()
               )
                suffix = "U";
            str = MathLib::value(str).str() + suffix;
        }

        // Float literal
        if (str.size() > 1 && str[0] == '.' && std::isdigit(str[1]))
            str = '0' + str;

        if (mTokensFrontBack.back) {
            mTokensFrontBack.back->insertToken(str);
        } else {
            mTokensFrontBack.front = new Token(&mTokensFrontBack);
            mTokensFrontBack.back = mTokensFrontBack.front;
            mTokensFrontBack.back->str(str);
        }

        if (mTokensFrontBack.back->str() == "delete")
            mTokensFrontBack.back->isKeyword(true);
        mTokensFrontBack.back->fileIndex(tok->location.fileIndex);
        mTokensFrontBack.back->linenr(tok->location.line);
        mTokensFrontBack.back->col(tok->location.col);
    }

    if (mSettings && mSettings->relativePaths) {
        for (std::size_t i = 0; i < mFiles.size(); i++)
            mFiles[i] = Path::getRelativePath(mFiles[i], mSettings->basePaths);
    }

    Token::assignProgressValues(mTokensFrontBack.front);
}

//---------------------------------------------------------------------------

unsigned long long TokenList::calculateChecksum() const
{
    unsigned long long checksum = 0;
    for (const Token* tok = front(); tok; tok = tok->next()) {
        const unsigned int subchecksum1 = tok->flags() + tok->varId() + tok->tokType();
        unsigned int subchecksum2 = 0;
        for (std::size_t i = 0; i < tok->str().size(); i++)
            subchecksum2 += (unsigned int)tok->str()[i];
        if (!tok->originalName().empty()) {
            for (std::size_t i = 0; i < tok->originalName().size(); i++)
                subchecksum2 += (unsigned int) tok->originalName()[i];
        }

        checksum ^= ((static_cast<unsigned long long>(subchecksum1) << 32) | subchecksum2);

        const bool bit1 = (checksum & 1) != 0;
        checksum >>= 1;
        if (bit1)
            checksum |= (1ULL << 63);
    }
    return checksum;
}


//---------------------------------------------------------------------------

struct AST_state {
    std::stack<Token*> op;
    unsigned int depth;
    unsigned int inArrayAssignment;
    unsigned int assign;
    bool inCase; // true from case to :
    explicit AST_state() : depth(0), inArrayAssignment(0), assign(0U), inCase(false) {}
};

static Token * skipDecl(Token *tok)
{
    if (!Token::Match(tok->previous(), "( %name%"))
        return tok;

    Token *vartok = tok;
    while (Token::Match(vartok, "%name%|*|&|::|<")) {
        if (vartok->str() == "<") {
            if (vartok->link())
                vartok = vartok->link();
            else
                return tok;
        } else if (Token::Match(vartok, "%name% [:=]")) {
            return vartok;
        }
        vartok = vartok->next();
    }
    return tok;
}

static bool iscast(const Token *tok)
{
    if (!Token::Match(tok, "( ::| %name%"))
        return false;

    if (Token::simpleMatch(tok->link(), ") ( )"))
        return false;

    if (tok->previous() && tok->previous()->isName() && tok->previous()->str() != "return")
        return false;

    if (Token::simpleMatch(tok->previous(), ">") && tok->previous()->link())
        return false;

    if (Token::Match(tok->link(), ") }|)|]"))
        return false;

    if (Token::Match(tok->link(), ") %cop%") && !Token::Match(tok->link(), ") [&*+-~]"))
        return false;

    if (Token::Match(tok->previous(), "= ( %name% ) {") && tok->next()->varId() == 0)
        return true;

    bool type = false;
    for (const Token *tok2 = tok->next(); tok2; tok2 = tok2->next()) {
        if (tok2->varId() != 0)
            return false;

        while (tok2->link() && Token::Match(tok2, "(|[|<"))
            tok2 = tok2->link()->next();

        if (tok2->str() == ")") {
            if (Token::simpleMatch(tok2, ") (") && Token::simpleMatch(tok2->linkAt(1), ") ."))
                return true;
            return type || tok2->strAt(-1) == "*" || Token::simpleMatch(tok2, ") ~") ||
                   (Token::Match(tok2, ") %any%") &&
                    !tok2->next()->isOp() &&
                    !Token::Match(tok2->next(), "[[]);,?:.]"));
        }
        if (!Token::Match(tok2, "%name%|*|&|::"))
            return false;

        if (tok2->isStandardType() && (tok2->next()->str() != "(" || Token::Match(tok2->next(), "( * *| )")))
            type = true;
    }

    return false;
}

// X{} X<Y>{} etc
static bool iscpp11init(const Token * const tok)
{
    const Token *nameToken = tok;
    while (nameToken && nameToken->str() == "{") {
        nameToken = nameToken->previous();
        if (nameToken && nameToken->str() == "," && Token::simpleMatch(nameToken->previous(), "} ,"))
            nameToken = nameToken->linkAt(-1);
    }
    if (!nameToken)
        return false;
    if (nameToken->str() == ">" && nameToken->link())
        nameToken = nameToken->link()->previous();

    const Token *endtok = nullptr;
    if (Token::Match(nameToken, "%name% { !!["))
        endtok = nameToken->linkAt(1);
    else if (Token::Match(nameToken,"%name% <") && Token::simpleMatch(nameToken->linkAt(1),"> {"))
        endtok = nameToken->linkAt(1)->linkAt(1);
    else
        return false;
    // There is no initialisation for example here: 'class Fred {};'
    if (!Token::simpleMatch(endtok, "} ;"))
        return true;
    const Token *prev = nameToken;
    while (Token::Match(prev, "%name%|::|:|<|>")) {
        if (Token::Match(prev, "class|struct"))
            return false;

        prev = prev->previous();
    }
    return true;
}

static void compileUnaryOp(Token *&tok, AST_state& state, void(*f)(Token *&tok, AST_state& state))
{
    Token *unaryop = tok;
    if (f) {
        tok = tok->next();
        state.depth++;
        if (tok && state.depth <= AST_MAX_DEPTH)
            f(tok, state);
        state.depth--;
    }

    if (!state.op.empty()) {
        unaryop->astOperand1(state.op.top());
        state.op.pop();
    }
    state.op.push(unaryop);
}

static void compileBinOp(Token *&tok, AST_state& state, void(*f)(Token *&tok, AST_state& state))
{
    Token *binop = tok;
    if (f) {
        tok = tok->next();
        state.depth++;
        if (tok && state.depth <= AST_MAX_DEPTH)
            f(tok, state);
        state.depth--;
    }

    // TODO: Should we check if op is empty.
    // * Is it better to add assertion that it isn't?
    // * Write debug warning if it's empty?
    if (!state.op.empty()) {
        binop->astOperand2(state.op.top());
        state.op.pop();
    }
    if (!state.op.empty()) {
        binop->astOperand1(state.op.top());
        state.op.pop();
    }
    state.op.push(binop);
}

static void compileExpression(Token *&tok, AST_state& state);

static void compileTerm(Token *&tok, AST_state& state)
{
    if (!tok)
        return;
    if (Token::Match(tok, "L %str%|%char%"))
        tok = tok->next();
    if (state.inArrayAssignment && Token::Match(tok->previous(), "[{,] . %name%")) { // Jump over . in C style struct initialization
        state.op.push(tok);
        tok->astOperand1(tok->next());
        tok = tok->tokAt(2);
    }
    if (state.inArrayAssignment && Token::Match(tok->previous(), "[{,] [ %num%|%name% ]")) {
        state.op.push(tok);
        tok->astOperand1(tok->next());
        tok = tok->tokAt(3);
    }
    if (tok->isLiteral()) {
        state.op.push(tok);
        do {
            tok = tok->next();
        } while (Token::Match(tok, "%name%|%str%"));
    } else if (tok->isName()) {
        if (Token::Match(tok, "return|case") || tok->str() == "throw") {
            if (tok->str() == "case")
                state.inCase = true;
            compileUnaryOp(tok, state, compileExpression);
            state.op.pop();
            if (state.inCase && Token::simpleMatch(tok, ": ;")) {
                state.inCase = false;
                tok = tok->next();
            }
        }  else if (iscpp11init(tok)) { // X{} X<Y>{} etc
            state.op.push(tok);
            tok = tok->next();
            if (tok->str() == "<")
                tok = tok->link()->next();
        } else if (!Token::Match(tok, "new|delete %name%|*|&|::|(|[")) {
            tok = skipDecl(tok);
            while (tok->next() && tok->next()->isName())
                tok = tok->next();
            state.op.push(tok);
            if (Token::Match(tok, "%name% <") && tok->linkAt(1))
                tok = tok->linkAt(1);
            else if (Token::Match(tok, "%name% . . ."))
                tok = tok->tokAt(3);
            tok = tok->next();
            if (Token::Match(tok, "%str%")) {
                while (Token::Match(tok, "%name%|%str%"))
                    tok = tok->next();
            }
        }
    }
}

static void compileScope(Token *&tok, AST_state& state)
{
    compileTerm(tok, state);
    while (tok) {
        if (tok->str() == "::") {
            Token *binop = tok;
            tok = tok->next();
            if (tok && tok->str() == "~") // Jump over ~ of destructor definition
                tok = tok->next();
            if (tok)
                compileTerm(tok, state);

            if (binop->previous() && (binop->previous()->isName() || (binop->previous()->link() && binop->strAt(-1) == ">")))
                compileBinOp(binop, state, nullptr);
            else
                compileUnaryOp(binop, state, nullptr);
        } else break;
    }
}

static bool isPrefixUnary(const Token* tok)
{
    if (!tok->previous()
        || ((Token::Match(tok->previous(), "(|[|{|%op%|;|}|?|:|,|.|return|::") || (tok->strAt(-1) == "throw"))
            && (tok->previous()->tokType() != Token::eIncDecOp || tok->tokType() == Token::eIncDecOp)))
        return true;

    if (tok->str() == "*" && tok->previous()->tokType() == Token::eIncDecOp && isPrefixUnary(tok->previous()))
        return true;

    return tok->strAt(-1) == ")" && iscast(tok->linkAt(-1));
}

static void compilePrecedence2(Token *&tok, AST_state& state)
{
    compileScope(tok, state);
    while (tok) {
        if (tok->tokType() == Token::eIncDecOp && !isPrefixUnary(tok)) {
            compileUnaryOp(tok, state, compileScope);
        } else if (tok->str() == "." && tok->strAt(1) != "*") {
            if (tok->strAt(1) == ".") {
                state.op.push(tok);
                tok = tok->tokAt(3);
                break;
            } else
                compileBinOp(tok, state, compileScope);
        } else if (tok->str() == "[") {
            const Token* const tok2 = tok;
            if (tok->strAt(1) != "]")
                compileBinOp(tok, state, compileExpression);
            else
                compileUnaryOp(tok, state, compileExpression);
            tok = tok2->link()->next();
        } else if (tok->str() == "(" && (!iscast(tok) || Token::Match(tok->previous(), "if|while|for|switch"))) {
            Token* tok2 = tok;
            tok = tok->next();
            const bool opPrevTopSquare = !state.op.empty() && state.op.top() && state.op.top()->str() == "[";
            const std::size_t oldOpSize = state.op.size();
            compileExpression(tok, state);
            tok = tok2;
            if ((tok->previous() && tok->previous()->isName() && (!Token::Match(tok->previous(), "return|case") && (!Token::Match(tok->previous(), "throw|delete"))))
                || (tok->strAt(-1) == "]" && (!Token::Match(tok->linkAt(-1)->previous(), "new|delete")))
                || (tok->strAt(-1) == ">" && tok->linkAt(-1))
                || (tok->strAt(-1) == ")" && !iscast(tok->linkAt(-1))) // Don't treat brackets to clarify precedence as function calls
                || (tok->strAt(-1) == "}" && opPrevTopSquare)) {
                const bool operandInside = oldOpSize < state.op.size();
                if (operandInside)
                    compileBinOp(tok, state, nullptr);
                else
                    compileUnaryOp(tok, state, nullptr);
            }
            tok = tok->link()->next();
        } else if (tok->str() == "{" && iscpp11init(tok)) {
            if (Token::simpleMatch(tok, "{ }"))
                compileUnaryOp(tok, state, compileExpression);
            else
                compileBinOp(tok, state, compileExpression);
            if (Token::simpleMatch(tok, "}"))
                tok = tok->next();
        } else break;
    }
}

static void compilePrecedence3(Token *&tok, AST_state& state)
{
    compilePrecedence2(tok, state);
    while (tok) {
        if ((Token::Match(tok, "[+-!~*&]") || tok->tokType() == Token::eIncDecOp) &&
            isPrefixUnary(tok)) {
            compileUnaryOp(tok, state, compilePrecedence3);
        } else if (tok->str() == "(" && iscast(tok)) {
            Token* castTok = tok;
            castTok->isCast(true);
            tok = tok->link()->next();
            compilePrecedence3(tok, state);
            compileUnaryOp(castTok, state, nullptr);
        }
        else break;
    }
}

static void compileMulDiv(Token *&tok, AST_state& state)
{
    compilePrecedence3(tok, state);
    while (tok) {
        if (Token::Match(tok, "[/%]") || (tok->str() == "*" && !tok->astOperand1())) {
            compileBinOp(tok, state, compilePrecedence3);
        } else break;
    }
}

static void compileAddSub(Token *&tok, AST_state& state)
{
    compileMulDiv(tok, state);
    while (tok) {
        if (Token::Match(tok, "+|-") && !tok->astOperand1()) {
            compileBinOp(tok, state, compileMulDiv);
        } else break;
    }
}

static void compileShift(Token *&tok, AST_state& state)
{
    compileAddSub(tok, state);
    while (tok) {
        if (Token::Match(tok, "<<|>>")) {
            compileBinOp(tok, state, compileAddSub);
        } else break;
    }
}

static void compileRelComp(Token *&tok, AST_state& state)
{
    compileShift(tok, state);
    while (tok) {
        if (Token::Match(tok, "<|<=|>=|>") && !tok->link()) {
            compileBinOp(tok, state, compileShift);
        } else break;
    }
}

static void compileEqComp(Token *&tok, AST_state& state)
{
    compileRelComp(tok, state);
    while (tok) {
        if (Token::Match(tok, "==|!=")) {
            compileBinOp(tok, state, compileRelComp);
        } else break;
    }
}

static void compileAnd(Token *&tok, AST_state& state)
{
    compileEqComp(tok, state);
    while (tok) {
        if (tok->str() == "&" && !tok->astOperand1()) {
            Token* tok2 = tok->next();
            if (!tok2)
                break;
            if (tok2->str() == "&")
                tok2 = tok2->next();
            if (Token::Match(tok2, ",|)")) {
                tok = tok2;
                break; // rValue reference
            }
            compileBinOp(tok, state, compileEqComp);
        } else break;
    }
}

static void compileXor(Token *&tok, AST_state& state)
{
    compileAnd(tok, state);
    while (tok) {
        if (tok->str() == "^") {
            compileBinOp(tok, state, compileAnd);
        } else break;
    }
}

static void compileOr(Token *&tok, AST_state& state)
{
    compileXor(tok, state);
    while (tok) {
        if (tok->str() == "|") {
            compileBinOp(tok, state, compileXor);
        } else break;
    }
}

static void compileLogicAnd(Token *&tok, AST_state& state)
{
    compileOr(tok, state);
    while (tok) {
        if (tok->str() == "&&") {
            compileBinOp(tok, state, compileOr);
        } else break;
    }
}

static void compileLogicOr(Token *&tok, AST_state& state)
{
    compileLogicAnd(tok, state);
    while (tok) {
        if (tok->str() == "||") {
            compileBinOp(tok, state, compileLogicAnd);
        } else break;
    }
}

static void compileAssignTernary(Token *&tok, AST_state& state)
{
    compileLogicOr(tok, state);
    while (tok) {
        if (tok->isAssignmentOp()) {
            state.assign++;
            compileBinOp(tok, state, compileAssignTernary);
            if (state.assign > 0U)
                state.assign--;
        } else if (tok->str() == "?") {
            // http://en.cppreference.com/w/cpp/language/operator_precedence says about ternary operator:
            //       "The expression in the middle of the conditional operator (between ? and :) is parsed as if parenthesized: its precedence relative to ?: is ignored."
            // Hence, we rely on Tokenizer::prepareTernaryOpForAST() to add such parentheses where necessary.
            if (tok->strAt(1) == ":") {
                state.op.push(nullptr);
            }
            const unsigned int assign = state.assign;
            state.assign = 0U;
            compileBinOp(tok, state, compileAssignTernary);
            state.assign = assign;
        } else if (tok->str() == ":") {
            if (state.depth == 1U && state.inCase) {
                state.inCase = false;
                tok = tok->next();
                break;
            }
            if (state.assign > 0U)
                break;
            compileBinOp(tok, state, compileAssignTernary);
        } else break;
    }
}

static void compileComma(Token *&tok, AST_state& state)
{
    compileAssignTernary(tok, state);
    while (tok) {
        if (tok->str() == ",") {
            if (Token::simpleMatch(tok, ", }"))
                tok = tok->next();
            else
                compileBinOp(tok, state, compileAssignTernary);
        } else break;
    }
}

static void compileExpression(Token *&tok, AST_state& state)
{
    if (state.depth > AST_MAX_DEPTH)
        return; // ticket #5592
    if (tok)
        compileComma(tok, state);
}

static Token * findAstTop(Token *tok1, Token *tok2)
{
    for (Token *tok = tok1; tok && (tok != tok2); tok = tok->next()) {
        if (tok->astParent() || tok->astOperand1() || tok->astOperand2())
            return const_cast<Token *>(tok->astTop());
        if (Token::simpleMatch(tok, "( {"))
            tok = tok->link();
    }
    for (Token *tok = tok1; tok && (tok != tok2); tok = tok->next()) {
        if (tok->isName() || tok->isNumber())
            return tok;
        if (Token::simpleMatch(tok, "( {"))
            tok = tok->link();
    }
    return nullptr;
}

static Token * createAstAtToken(Token *tok)
{
    if (Token::simpleMatch(tok, "for (")) {
        Token *tok2 = skipDecl(tok->tokAt(2));
        Token *init1 = nullptr;
        Token * const endPar = tok->next()->link();
        while (tok2 && tok2 != endPar && tok2->str() != ";") {
            if (Token::Match(tok2, "%name% %op%|(|[|.|:|::") || Token::Match(tok2->previous(), "[(;{}] %cop%|(")) {
                init1 = tok2;
                AST_state state1;
                compileExpression(tok2, state1);
                if (Token::Match(tok2, ";|)"))
                    break;
                init1 = nullptr;
            }
            if (!tok2) // #7109 invalid code
                return nullptr;
            tok2 = tok2->next();
        }
        if (!tok2 || tok2->str() != ";") {
            if (tok2 == endPar && init1) {
                tok->next()->astOperand2(init1);
                tok->next()->astOperand1(tok);
            }
            return tok2;
        }

        Token * const init = init1 ? init1 : tok2;

        Token * const semicolon1 = tok2;
        tok2 = tok2->next();
        AST_state state2;
        compileExpression(tok2, state2);

        Token * const semicolon2 = tok2;
        if (!semicolon2)
            return nullptr; // invalid code #7235
        tok2 = tok2->next();
        AST_state state3;
        if (Token::simpleMatch(tok2, "( {")) {
            state3.op.push(tok2->next());
            tok2 = tok2->link()->next();
        }
        compileExpression(tok2, state3);

        if (init != semicolon1)
            semicolon1->astOperand1(const_cast<Token*>(init->astTop()));
        tok2 = findAstTop(semicolon1->next(), semicolon2);
        if (tok2)
            semicolon2->astOperand1(tok2);
        tok2 = findAstTop(semicolon2->next(), endPar);
        if (tok2)
            semicolon2->astOperand2(tok2);
        else if (!state3.op.empty())
            semicolon2->astOperand2(const_cast<Token*>(state3.op.top()));

        semicolon1->astOperand2(semicolon2);
        tok->next()->astOperand1(tok);
        tok->next()->astOperand2(semicolon1);

        return endPar;
    }

    if (Token::simpleMatch(tok, "( {"))
        return tok;

    if (Token::Match(tok, "%type% <") && !Token::Match(tok->linkAt(1), "> [({]"))
        return tok->linkAt(1);

    if (Token::Match(tok, "%type% %name%|*|&|::") && tok->str() != "return") {
        bool decl = false;
        Token *typetok = tok;
        while (Token::Match(typetok, "%type%|::|*|&")) {
            if (typetok->isStandardType() || Token::Match(typetok, "struct|const|static"))
                decl = true;
            typetok = typetok->next();
        }
        if (decl && Token::Match(typetok->previous(), "[*&] %var% ="))
            tok = typetok;
    }

    if (Token::Match(tok, "return|case") || tok->str() == "throw" || !tok->previous() || Token::Match(tok, "%name% %op%|(|[|.|::|<|?|;") || Token::Match(tok->previous(), "[;{}] %cop%|++|--|( !!{")) {
        if ((Token::Match(tok->tokAt(-2), "[;{}] new|delete %name%") || Token::Match(tok->tokAt(-3), "[;{}] :: new|delete %name%")))
            tok = tok->previous();

        Token * const tok1 = tok;
        AST_state state;
        compileExpression(tok, state);
        const Token * const endToken = tok;
        if (endToken == tok1 || !endToken)
            return tok1;

        return endToken->previous();
    }

    return tok;
}

void TokenList::createAst()
{
    for (Token *tok = mTokensFrontBack.front; tok; tok = tok ? tok->next() : nullptr) {
        tok = createAstAtToken(tok);
    }
}

void TokenList::validateAst() const
{
    // Check for some known issues in AST to avoid crash/hang later on
    std::set < const Token* > safeAstTokens; // list of "safe" AST tokens without endless recursion
    for (const Token *tok = mTokensFrontBack.front; tok; tok = tok->next()) {
        // Syntax error if binary operator only has 1 operand
        if ((tok->isAssignmentOp() || tok->isComparisonOp() || Token::Match(tok,"[|^/%]")) && tok->astOperand1() && !tok->astOperand2())
            throw InternalError(tok, "Syntax Error: AST broken, binary operator has only one operand.", InternalError::AST);

        // Syntax error if we encounter "?" with operand2 that is not ":"
        if (tok->astOperand2() && tok->str() == "?" && tok->astOperand2()->str() != ":")
            throw InternalError(tok, "Syntax Error: AST broken, ternary operator lacks ':'.", InternalError::AST);

        // Check for endless recursion
        const Token* parent = tok->astParent();
        if (parent) {
            std::set < const Token* > astTokens; // list of anchestors
            astTokens.insert(tok);
            do {
                if (safeAstTokens.find(parent) != safeAstTokens.end())
                    break;
                if (astTokens.find(parent) != astTokens.end())
                    throw InternalError(tok, "AST broken: endless recursion from '" + tok->str() + "'", InternalError::AST);
                astTokens.insert(parent);
            } while ((parent = parent->astParent()) != nullptr);
            safeAstTokens.insert(astTokens.begin(), astTokens.end());
        } else if (tok->str() == ";") {
            safeAstTokens.clear();
        } else {
            safeAstTokens.insert(tok);
        }

        // Check binary operators
        if (Token::Match(tok, "%or%|%oror%|%assign%|%comp%")) {
            // Skip lambda captures
            if (Token::Match(tok, "= ,|]"))
                continue;
            // Don't check templates
            if (tok->link())
                continue;
            // Skip operator definitions
            if (Token::simpleMatch(tok->previous(), "operator"))
                continue;
            // Skip incomplete code
            if (!tok->astOperand1() && !tok->astOperand2() && !tok->astParent())
                continue;
            // Skip lambda assignment and/or initializer
            if (Token::Match(tok, "= {|^|["))
                continue;
            // FIXME: Workaround broken AST assignment in type aliases
            if (Token::Match(tok->previous(), "%name% = %name%"))
                continue;
            if (!tok->astOperand1() || !tok->astOperand2())
                throw InternalError(tok, "Syntax Error: AST broken, binary operator '" + tok->str() + "' doesn't have two operands.", InternalError::AST);
        }
    }
}

std::string TokenList::getOrigFile(const Token *tok) const
{
    return mOrigFiles.at(tok->fileIndex());
}

const std::string& TokenList::file(const Token *tok) const
{
    return mFiles.at(tok->fileIndex());
}

std::string TokenList::fileLine(const Token *tok) const
{
    return ErrorLogger::ErrorMessage::FileLocation(tok, this).stringify();
}

void TokenList::simplifyPlatformTypes()
{
    enum { isLongLong, isLong, isInt } type;

    //This assumes a flat address space.

    if (mSettings->sizeof_size_t == mSettings->sizeof_long)
        type = isLong;
    else if (mSettings->sizeof_size_t == mSettings->sizeof_long_long)
        type = isLongLong;
    else if (mSettings->sizeof_size_t == mSettings->sizeof_int)
        type = isInt;
    else
        return;

    const std::string platform_type(mSettings->platformString());

    for (Token *tok = front(); tok; tok = tok->next()) {
        if (tok->tokType() != Token::eType && tok->tokType() != Token::eName)
            continue;

        const Library::PlatformType * const platformtype = mSettings->library.platform_type(tok->str(), platform_type);

        if (platformtype) {
            // check for namespace
            if (tok->strAt(-1) == "::") {
                const Token * tok1 = tok->tokAt(-2);
                // skip when non-global namespace defined
                if (tok1 && tok1->tokType() == Token::eName)
                    continue;
                tok = tok->previous();
                tok->deleteThis();
            }
            Token *typeToken;
            if (platformtype->_const_ptr) {
                tok->str("const");
                tok->insertToken("*");
                tok->insertToken(platformtype->mType);
                typeToken = tok;
            } else if (platformtype->_pointer) {
                tok->str(platformtype->mType);
                typeToken = tok;
                tok->insertToken("*");
            } else if (platformtype->_ptr_ptr) {
                tok->str(platformtype->mType);
                typeToken = tok;
                tok->insertToken("*");
                tok->insertToken("*");
            } else {
                tok->originalName(tok->str());
                tok->str(platformtype->mType);
                typeToken = tok;
            }
            if (platformtype->_signed)
                typeToken->isSigned(true);
            if (platformtype->_unsigned)
                typeToken->isUnsigned(true);
            if (platformtype->_long)
                typeToken->isLong(true);
        }
    }
}

void TokenList::simplifyStdType()
{
    for (Token *tok = front(); tok; tok = tok->next()) {
        if (Token::Match(tok, "char|short|int|long|unsigned|signed|double|float") ) {
            bool isFloat= false;
            bool isSigned = false;
            bool isUnsigned = false;
            unsigned int countLong = 0;
            Token* typeSpec = nullptr;

            Token* tok2 = tok;
            for (; tok2->next(); tok2 = tok2->next()) {
                if (tok2->str() == "long") {
                    countLong++;
                    if (!isFloat)
                        typeSpec = tok2;
                } else if (tok2->str() == "short") {
                    typeSpec = tok2;
                } else if (tok2->str() == "unsigned")
                    isUnsigned = true;
                else if (tok2->str() == "signed")
                    isSigned = true;
                else if (Token::Match(tok2, "float|double")) {
                    isFloat = true;
                    typeSpec = tok2;
                }
                else if (Token::Match(tok2, "char|int")) {
                    if (!typeSpec)
                        typeSpec = tok2;
                } else
                    break;
            }

            if (!typeSpec) { // unsigned i; or similar declaration
                tok->str("int");
                tok->isSigned(isSigned);
                tok->isUnsigned(isUnsigned);
            } else {
                typeSpec->isLong(typeSpec->isLong() || (isFloat && countLong == 1) || countLong > 1);
                typeSpec->isSigned(typeSpec->isSigned() || isSigned);
                typeSpec->isUnsigned(typeSpec->isUnsigned() || isUnsigned);

                // Remove specifiers
                const Token* tok3 = tok->previous();
                tok2 = tok2->previous();
                while (tok3 != tok2) {
                    if (tok2 != typeSpec )
                        tok2->deleteThis();
                    tok2 = tok2->previous();
                }
            }
        }
    }
}

