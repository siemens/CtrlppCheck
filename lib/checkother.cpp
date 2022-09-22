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
#include "checkother.h"
#include "checkuninitvar.h" // CheckUninitVar::isVariableUsage

#include "astutils.h"
#include "errorlogger.h"
#include "library.h"
#include "mathlib.h"
#include "settings.h"
#include "symbols/symbols.h"
#include "token.h"
#include "tokenize.h"
#include "utils.h"

#include <algorithm> // find_if()
#include <list>
#include <map>
#include <ostream>
#include <set>
#include <utility>
//---------------------------------------------------------------------------

// Register this check class (by creating a static instance of it)
namespace {
    CheckOther instance;
}

static const struct CWE CWE197(197U); // Numeric Truncation Error
static const struct CWE CWE369(369U); // Divide By Zero
static const struct CWE CWE398(398U); // Indicator of Poor Code Quality
static const struct CWE CWE475(475U); // Undefined Behavior for Input to API
static const struct CWE CWE561(561U); // Dead Code
static const struct CWE CWE563(563U); // Assignment to Variable without Use ('Unused Variable')
static const struct CWE CWE570(570U); // Expression is Always False
static const struct CWE CWE672(672U); // Operation on a Resource after Expiration or Release
static const struct CWE CWE628(628U); // Function Call with Incorrectly Specified Arguments
static const struct CWE CWE683(683U); // Function Call With Incorrect Order of Arguments
static const struct CWE CWE758(758U); // Reliance on Undefined, Unspecified, or Implementation-Defined Behavior
static const struct CWE CWE768(768U); // Incorrect Short Circuit Evaluation
static const struct CWE CWE783(783U); // Operator Precedence Logic Error

//---------------------------------------------------------------------------
// Clarify calculation precedence for ternary operators.
//---------------------------------------------------------------------------
void CheckOther::clarifyCalculation()
{
    if (!mSettings->isEnabled(Settings::STYLE))
        return;

    const SymbolDatabase *symbolDatabase = mTokenizer->getSymbolDatabase();
    for (const Scope * scope : symbolDatabase->functionScopes) {
        for (const Token* tok = scope->bodyStart->next(); tok != scope->bodyEnd; tok = tok->next()) {
            // ? operator where lhs is arithmetical expression
            if (tok->str() != "?" || !tok->astOperand1() || !tok->astOperand1()->isCalculation())
                continue;
            if (!tok->astOperand1()->isArithmeticalOp() && tok->astOperand1()->tokType() != Token::eBitOp)
                continue;

            // Is code clarified by parentheses already?
            const Token *tok2 = tok->astOperand1();
            for (; tok2; tok2 = tok2->next()) {
                if (tok2->str() == "(")
                    tok2 = tok2->link();
                else if (tok2->str() == ")")
                    break;
                else if (tok2->str() == "?") {
                    clarifyCalculationError(tok, tok->astOperand1()->str());
                    break;
                }
            }
        }
    }
}

void CheckOther::clarifyCalculationError(const Token *tok, const std::string &op)
{
    // suspicious calculation
    const std::string calc("'a" + op + "b?c:d'");

    // recommended calculation #1
    const std::string s1("'(a" + op + "b)?c:d'");

    // recommended calculation #2
    const std::string s2("'a" + op + "(b?c:d)'");

    reportError(tok,
                Severity::style,
                "clarifyCalculation",
                "Clarify calculation precedence for '" + op + "' and '?'.\n"
                "Suspicious calculation. Please use parentheses to clarify the code. "
                "The code '" + calc + "' should be written as either '" + s1 + "' or '" + s2 + "'.", CWE783, false);
}

//---------------------------------------------------------------------------
// Clarify (meaningless) statements like *foo++; with parentheses.
//---------------------------------------------------------------------------
void CheckOther::clarifyStatement()
{
    if (!mSettings->isEnabled(Settings::WARNING))
        return;

    const SymbolDatabase *symbolDatabase = mTokenizer->getSymbolDatabase();
    for (const Scope * scope : symbolDatabase->functionScopes) {
        for (const Token* tok = scope->bodyStart; tok && tok != scope->bodyEnd; tok = tok->next()) {
            if (Token::Match(tok, "* %name%") && tok->astOperand1()) {
                const Token *tok2 = tok->previous();

                while (tok2 && tok2->str() == "*")
                    tok2 = tok2->previous();

                if (tok2 && !tok2->astParent() && Token::Match(tok2, "[{};]")) {
                    tok2 = tok->astOperand1();
                    if (Token::Match(tok2, "++|-- [;,]"))
                        clarifyStatementError(tok2);
                }
            }
        }
    }
}

void CheckOther::clarifyStatementError(const Token *tok)
{
    reportError(tok, Severity::warning, "clarifyStatement", "Ineffective statement similar to '*A++;'. Did you intend to write '(*A)++;'?\n"
                "A statement like '*A++;' might not do what you intended. Postfix 'operator++' is executed before 'operator*'. "
                "Thus, the dereference is meaningless. Did you intend to write '(*A)++;'?", CWE783, false);
}

//---------------------------------------------------------------------------
// Check for suspicious occurrences of 'if(); {}'.
//---------------------------------------------------------------------------
void CheckOther::checkSuspiciousSemicolon()
{
    if (!mSettings->inconclusive || !mSettings->isEnabled(Settings::WARNING))
        return;

    const SymbolDatabase* const symbolDatabase = mTokenizer->getSymbolDatabase();

    // Look for "if(); {}", "for(); {}" or "while(); {}"
    for (const Scope &scope : symbolDatabase->scopeList) {
        if (scope.type == Scope::eIf || scope.type == Scope::eElse || scope.type == Scope::eWhile || scope.type == Scope::eFor) {
            // Ensure the semicolon is at the same line number as the if/for/while statement
            // and the {..} block follows it without an extra empty line.
            if (Token::simpleMatch(scope.bodyStart, "{ ; }"))
            {
                SuspiciousSemicolonError(scope.classDef);
            }
        }
    }
}

void CheckOther::SuspiciousSemicolonError(const Token* tok)
{
    reportError(tok, Severity::warning, "suspiciousSemicolon",
                "Suspicious use of ; at the end of '" + (tok ? tok->str() : std::string()) + "' statement.", CWE398, true);
}


//---------------------------------------------------------------------------
// Check for empty scope 'if() {}'.
//---------------------------------------------------------------------------
void CheckOther::checkEmptyScope()
{
    if (!mSettings->inconclusive || !mSettings->isEnabled(Settings::WARNING))
        return;

    const SymbolDatabase *const symbolDatabase = mTokenizer->getSymbolDatabase();

    // Look for "if() {}", "for() {}" or "while() {}"
    for (const Scope &scope : symbolDatabase->scopeList)
    {
        if (scope.type == Scope::eIf || scope.type == Scope::eElse ||                               // if && else
            scope.type == Scope::eWhile || scope.type == Scope::eFor || scope.type == Scope::eDo || // loops
            scope.type == Scope::eCatch || scope.type == Scope::eTry ||                             // try cacth ...
            scope.type == Scope::eClass || scope.type == Scope::eStruct || scope.type == Scope::eEnum || scope.type == Scope::eFunction ||
            scope.type == Scope::eSwitch // switch case
        )
        {
            if (Token::simpleMatch(scope.bodyStart, "{ }"))
            {
                emptyScopeError(scope.classDef);
            }
        }
    }
}

void CheckOther::emptyScopeError(const Token *tok)
{
    reportError(tok, Severity::warning, "emptyScope",
                "The scope is empty: '" + (tok ? tok->str() : std::string()) + "'", CWE398, true);
}

//---------------------------------------------------------------------------
// Check for non-perfomante usage of functions in loop like dpGet, dpSet ....
// Look for "for() { dpGet(); dpSet(); dpQuery(); delay(); }" or "while() { .... }"
//---------------------------------------------------------------------------
void CheckOther::checkPerformanceInLoops()
{
    if (!mSettings->isEnabled(Settings::WARNING))
        return;

    const SymbolDatabase *const symbolDatabase = mTokenizer->getSymbolDatabase();

    for (const Scope &scope : symbolDatabase->scopeList)
    {

        if (scope.type == Scope::eWhile || scope.type == Scope::eFor)
        {
            // in scope of loop
            for (const Token *tok = scope.bodyStart->next(); tok != scope.bodyEnd; tok = tok->next())
            {
                // get token by token in the loop and check if the token is function and if is in bad-list
                if (!Token::Match(tok, "%name% ("))
                    continue;

                if (mSettings->library.isFunctionNotInLoop(tok, mSettings->inconclusive))
                {
                    performanceInLoopsError(tok);
                }

                /// @todo we can start here some profiling and check if the bad functions are part of called function.
            }
        }
    }
}

void CheckOther::performanceInLoopsError(const Token *function)
{
    reportError(function, Severity::performance, "badPerformanceInLoops",
                "Non-performing usage of a function in a loop: " + (function ? function->str() : std::string()), CWE398, true);
}

//---------------------------------------------------------------------------
// Detect redundant assignments: x = 0; x = 4;
//---------------------------------------------------------------------------

void CheckOther::checkRedundantAssignment()
{
    if (!mSettings->isEnabled(Settings::STYLE))
        return;
    const SymbolDatabase* symbolDatabase = mTokenizer->getSymbolDatabase();
    for (const Scope *scope : symbolDatabase->functionScopes) {
        if (!scope->bodyStart)
            continue;
        for (const Token* tok = scope->bodyStart->next(); tok != scope->bodyEnd; tok = tok->next()) {
            if (Token::simpleMatch(tok, "] ("))
                // todo: handle lambdas
                break;
            if (Token::simpleMatch(tok, "try {"))
                // todo: check try blocks
                tok = tok->linkAt(1);
            if ((tok->isAssignmentOp() || Token::Match(tok, "++|--")) && tok->astOperand1()) {
                if (tok->astParent())
                    continue;

                // Do not warn about redundant initialization when rhs is trivial
                // TODO : do not simplify the variable declarations
                if (Token::Match(tok->tokAt(-3), "%var% ; %var% =") && tok->previous()->variable() && tok->previous()->variable()->nameToken() == tok->tokAt(-3) && tok->tokAt(-3)->linenr() == tok->previous()->linenr()) {
                    bool trivial = true;
                    visitAstNodes(tok->astOperand2(),
                    [&](const Token *rhs) {
                        if (Token::Match(rhs, "%str%|%num%|%name%"))
                            return ChildrenToVisit::op1_and_op2;
                        if (rhs->str() == "(" && !rhs->previous()->isName())
                            return ChildrenToVisit::op1_and_op2;
                        trivial = false;
                        return ChildrenToVisit::done;
                    });
                    if (trivial)
                        continue;
                }

                // Do not warn about assignment with 0 / NULL
                if (Token::simpleMatch(tok->astOperand2(), "0") || FwdAnalysis::isNullOperand(tok->astOperand2()))
                    continue;

                if (tok->astOperand1()->variable() && tok->astOperand1()->variable()->isReference())
                    // todo: check references
                    continue;

                if (tok->astOperand1()->variable() && tok->astOperand1()->variable()->isStatic())
                    // todo: check static variables
                    continue;

                // If there is a custom assignment operator => this is inconclusive
                bool inconclusive = false;
                if (tok->astOperand1()->valueType() && tok->astOperand1()->valueType()->typeScope)
                {
                    const std::string op = "operator" + tok->str();
                    for (const Function &f : tok->astOperand1()->valueType()->typeScope->functionList) {
                        if (f.name() == op) {
                            inconclusive = true;
                            break;
                        }
                    }
                }
                if (inconclusive && !mSettings->inconclusive)
                    continue;

                FwdAnalysis fwdAnalysis(mSettings->library);
                if (fwdAnalysis.hasOperand(tok->astOperand2(), tok->astOperand1()))
                    continue;

                // Is there a redundant assignment?
                const Token *start;
                if (tok->isAssignmentOp())
                    start = tok->next();
                else
                    start = tok->findExpressionStartEndTokens().second->next();

                // Get next assignment..
                const Token *nextAssign = fwdAnalysis.reassign(tok->astOperand1(), start, scope->bodyEnd);

                if (!nextAssign)
                    continue;

                // there is redundant assignment. Is there a case between the assignments?
                bool hasCase = false;
                for (const Token *tok2 = tok; tok2 != nextAssign; tok2 = tok2->next()) {
                    if (tok2->str() == "break" || tok2->str() == "return")
                        break;
                    if (tok2->str() == "case") {
                        hasCase = true;
                        break;
                    }
                }

                // warn
                if (hasCase)
                    redundantAssignmentInSwitchError(tok, nextAssign, tok->astOperand1()->expressionString());
                else
                    redundantAssignmentError(tok, nextAssign, tok->astOperand1()->expressionString(), inconclusive);
            }
        }
    }
}


void CheckOther::redundantAssignmentError(const Token *tok1, const Token* tok2, const std::string& var, bool inconclusive)
{
    const std::list<const Token *> callstack = { tok1, tok2 };
    if (inconclusive)
        reportError(callstack, Severity::style, "redundantAssignment",
                    "$symbol:" + var + "\n"
                                       "Variable is reassigned a value before the old one has been used if variable is no semaphore variable: '$symbol'\n"
                                       "Variable is reassigned a value before the old one has been used: '$symbol'. Make sure that this variable is not used like a semaphore in a threading environment before simplifying this code.",
                    CWE563, true);
    else
        reportError(callstack, Severity::style, "redundantAssignment",
                    "$symbol:" + var + "\n"
                                       "Variable is reassigned a value before the old one has been used: '$symbol'",
                    CWE563, false);
}

void CheckOther::redundantAssignmentInSwitchError(const Token *tok1, const Token* tok2, const std::string &var)
{
    const std::list<const Token *> callstack = { tok1, tok2 };
    reportError(callstack, Severity::warning, "redundantAssignInSwitch",
                "$symbol:" + var + "\n"
                                   "Variable is reassigned a value before the old one has been used: '$symbol' 'break;' missing?",
                CWE563, false);
}


//---------------------------------------------------------------------------
//    switch (x)
//    {
//        case 2:
//            y = a;        // <- this assignment is redundant
//        case 3:
//            y = b;        // <- case 2 falls through and sets y twice
//    }
//---------------------------------------------------------------------------
static inline bool isFunctionOrBreakPattern(const Token *tok)
{
    if (Token::Match(tok, "%name% (") || Token::Match(tok, "break|continue|return|exit|goto|throw"))
        return true;

    return false;
}

void CheckOther::checkRedundantAssignmentInSwitch()
{
    if (!mSettings->isEnabled(Settings::WARNING))
        return;

    const SymbolDatabase *symbolDatabase = mTokenizer->getSymbolDatabase();

    // Find the beginning of a switch. E.g.:
    //   switch (var) { ...
    for (const Scope &switchScope : symbolDatabase->scopeList) {
        if (switchScope.type != Scope::eSwitch || !switchScope.bodyStart)
            continue;

        // Check the contents of the switch statement
        std::map<unsigned int, const Token*> varsWithBitsSet;
        std::map<unsigned int, std::string> bitOperations;

        for (const Token *tok2 = switchScope.bodyStart->next(); tok2 != switchScope.bodyEnd; tok2 = tok2->next()) {
            if (tok2->str() == "{") {
                // Inside a conditional or loop. Don't mark variable accesses as being redundant. E.g.:
                //   case 3: b = 1;
                //   case 4: if (a) { b = 2; }    // Doesn't make the b=1 redundant because it's conditional
                if (Token::Match(tok2->previous(), ")|else {") && tok2->link()) {
                    const Token* endOfConditional = tok2->link();
                    for (const Token* tok3 = tok2; tok3 != endOfConditional; tok3 = tok3->next()) {
                        if (tok3->varId() != 0) {
                            varsWithBitsSet.erase(tok3->varId());
                            bitOperations.erase(tok3->varId());
                        } else if (isFunctionOrBreakPattern(tok3)) {
                            varsWithBitsSet.clear();
                            bitOperations.clear();
                        }
                    }
                    tok2 = endOfConditional;
                }
            }

            // Variable assignment. Report an error if it's assigned to twice before a break. E.g.:
            //    case 3: b = 1;    // <== redundant
            //    case 4: b = 2;

            if (Token::Match(tok2->previous(), ";|{|}|: %var% = %any% ;")) {
                varsWithBitsSet.erase(tok2->varId());
                bitOperations.erase(tok2->varId());
            }

            // Bitwise operation. Report an error if it's performed twice before a break. E.g.:
            //    case 3: b |= 1;    // <== redundant
            //    case 4: b |= 1;
            else if (Token::Match(tok2->previous(), ";|{|}|: %var% %assign% %num% ;") &&
                     (tok2->strAt(1) == "|=" || tok2->strAt(1) == "&=") &&
                     Token::Match(tok2->next()->astOperand2(), "%num%")) {
                const std::string bitOp = tok2->strAt(1)[0] + tok2->strAt(2);
                const std::map<unsigned int, const Token*>::const_iterator i2 = varsWithBitsSet.find(tok2->varId());

                // This variable has not had a bit operation performed on it yet, so just make a note of it
                if (i2 == varsWithBitsSet.end()) {
                    varsWithBitsSet[tok2->varId()] = tok2;
                    bitOperations[tok2->varId()] = bitOp;
                }

                // The same bit operation has been performed on the same variable twice, so report an error
                else if (bitOperations[tok2->varId()] == bitOp)
                    redundantBitwiseOperationInSwitchError(i2->second, i2->second->str());

                // A different bit operation was performed on the variable, so clear it
                else {
                    varsWithBitsSet.erase(tok2->varId());
                    bitOperations.erase(tok2->varId());
                }
            }

            // Bitwise operation. Report an error if it's performed twice before a break. E.g.:
            //    case 3: b = b | 1;    // <== redundant
            //    case 4: b = b | 1;
            else if (Token::Match(tok2->previous(), ";|{|}|: %var% = %name% %or%|& %num% ;") &&
                     tok2->varId() == tok2->tokAt(2)->varId()) {
                const std::string bitOp = tok2->strAt(3) + tok2->strAt(4);
                const std::map<unsigned int, const Token*>::const_iterator i2 = varsWithBitsSet.find(tok2->varId());

                // This variable has not had a bit operation performed on it yet, so just make a note of it
                if (i2 == varsWithBitsSet.end()) {
                    varsWithBitsSet[tok2->varId()] = tok2;
                    bitOperations[tok2->varId()] = bitOp;
                }

                // The same bit operation has been performed on the same variable twice, so report an error
                else if (bitOperations[tok2->varId()] == bitOp)
                    redundantBitwiseOperationInSwitchError(i2->second, i2->second->str());

                // A different bit operation was performed on the variable, so clear it
                else {
                    varsWithBitsSet.erase(tok2->varId());
                    bitOperations.erase(tok2->varId());
                }
            }

            // Not a simple assignment so there may be good reason if this variable is assigned to twice. E.g.:
            //    case 3: b = 1;
            //    case 4: b++;
            else if (tok2->varId() != 0 && tok2->strAt(1) != "|" && tok2->strAt(1) != "&") {
                varsWithBitsSet.erase(tok2->varId());
                bitOperations.erase(tok2->varId());
            }

            // Reset our record of assignments if there is a break or function call. E.g.:
            //    case 3: b = 1; break;
            if (isFunctionOrBreakPattern(tok2)) {
                varsWithBitsSet.clear();
                bitOperations.clear();
            }
        }
    }
}

void CheckOther::redundantBitwiseOperationInSwitchError(const Token *tok, const std::string &varname)
{
    reportError(tok, Severity::warning,
                "redundantBitwiseOperationInSwitch",
                "$symbol:" + varname + "\n"
                "Redundant bitwise operation on '$symbol' in 'switch' statement. 'break;' missing?");
}


//---------------------------------------------------------------------------
// Check for statements like case A||B: in switch()
//---------------------------------------------------------------------------
void CheckOther::checkSuspiciousCaseInSwitch()
{
    if (!mSettings->inconclusive || !mSettings->isEnabled(Settings::WARNING))
        return;

    const SymbolDatabase *symbolDatabase = mTokenizer->getSymbolDatabase();

    for (const Scope & scope : symbolDatabase->scopeList) {
        if (scope.type != Scope::eSwitch)
            continue;

        for (const Token* tok = scope.bodyStart->next(); tok != scope.bodyEnd; tok = tok->next()) {
            if (tok->str() == "case") {
                const Token* finding = nullptr;
                for (const Token* tok2 = tok->next(); tok2; tok2 = tok2->next()) {
                    if (tok2->str() == ":")
                        break;
                    if (Token::Match(tok2, "[;}{]"))
                        break;

                    if (tok2->str() == "?")
                        finding = nullptr;
                    else if (Token::Match(tok2, "&&|%oror%"))
                        finding = tok2;
                }
                if (finding)
                    suspiciousCaseInSwitchError(finding, finding->str());
            }
        }
    }
}

void CheckOther::suspiciousCaseInSwitchError(const Token* tok, const std::string& operatorString)
{
    reportError(tok, Severity::warning, "suspiciousCase",
                "Found suspicious case label in switch(). Operator '" + operatorString + "' probably doesn't work as intended.\n"
                "Using an operator like '" + operatorString + "' in a case label is suspicious. Did you intend to use a bitwise operator, multiple case labels or if/else instead?", CWE398, true);
}

//---------------------------------------------------------------------------
//    Find consecutive return, break, continue, goto or throw statements. e.g.:
//        break; break;
//    Detect dead code, that follows such a statement. e.g.:
//        return(0); foo();
//---------------------------------------------------------------------------
void CheckOther::checkUnreachableCode()
{
    if (!mSettings->isEnabled(Settings::STYLE))
        return;
    const bool printInconclusive = mSettings->inconclusive;
    const SymbolDatabase* symbolDatabase = mTokenizer->getSymbolDatabase();
    for (const Scope * scope : symbolDatabase->functionScopes) {
        for (const Token* tok = scope->bodyStart; tok && tok != scope->bodyEnd; tok = tok->next()) {
            const Token* secondBreak = nullptr;
            const Token* labelName = nullptr;
            if (tok->link() && Token::Match(tok, "(|[|<"))
                tok = tok->link();
            else if (Token::Match(tok, "break|continue ;"))
                secondBreak = tok->tokAt(2);
            else if (Token::Match(tok, "[;{}:] return|throw")) {
                if (Token::simpleMatch(tok->astParent(), "?"))
                    continue;
                tok = tok->next(); // tok should point to return or throw
                for (const Token *tok2 = tok->next(); tok2; tok2 = tok2->next()) {
                    if (tok2->str() == "(" || tok2->str() == "{")
                        tok2 = tok2->link();
                    if (tok2->str() == ";") {
                        secondBreak = tok2->next();
                        break;
                    }
                }
            } else if (Token::Match(tok, "goto %any% ;")) {
                secondBreak = tok->tokAt(3);
                labelName = tok->next();
            } else if (Token::Match(tok, "%name% (") && mSettings->library.isnoreturn(tok) && !Token::Match(tok->next()->astParent(), "?|:")) {
                if ((!tok->function() || (tok->function()->token != tok && tok->function()->tokenDef != tok)) && tok->linkAt(1)->strAt(1) != "{")
                    secondBreak = tok->linkAt(1)->tokAt(2);
            }

            // Statements follow directly, no line between them. (#3383)
            // TODO: Try to find a better way to avoid false positives due to preprocessor configurations.
            const bool inconclusive = secondBreak && (secondBreak->linenr() - 1 > secondBreak->previous()->linenr());

            if (secondBreak && (printInconclusive || !inconclusive)) {
                if (Token::Match(secondBreak, "continue|goto|throw") ||
                    (secondBreak->str() == "return" && (tok->str() == "return" || secondBreak->strAt(1) == ";"))) { // return with value after statements like throw can be necessary to make a function compile
                    duplicateBreakError(secondBreak, inconclusive);
                    tok = Token::findmatch(secondBreak, "[}:]");
                } else if (secondBreak->str() == "break") { // break inside switch as second break statement should not issue a warning
                    if (tok->str() == "break") // If the previous was a break, too: Issue warning
                        duplicateBreakError(secondBreak, inconclusive);
                    else {
                        if (tok->scope()->type != Scope::eSwitch) // Check, if the enclosing scope is a switch
                            duplicateBreakError(secondBreak, inconclusive);
                    }
                    tok = Token::findmatch(secondBreak, "[}:]");
                } else if (!Token::Match(secondBreak, "return|}|case|default") && secondBreak->strAt(1) != ":") { // TODO: No bailout for unconditional scopes
                    // If the goto label is followed by a loop construct in which the label is defined it's quite likely
                    // that the goto jump was intended to skip some code on the first loop iteration.
                    bool labelInFollowingLoop = false;
                    if (labelName && Token::Match(secondBreak, "while|do|for")) {
                        const Token *scope2 = Token::findsimplematch(secondBreak, "{");
                        if (scope2) {
                            for (const Token *tokIter = scope2; tokIter != scope2->link() && tokIter; tokIter = tokIter->next()) {
                                if (Token::Match(tokIter, "[;{}] %any% :") && labelName->str() == tokIter->strAt(1)) {
                                    labelInFollowingLoop = true;
                                    break;
                                }
                            }
                        }
                    }

                    // hide FP for statements that just hide compiler warnings about unused function arguments
                    bool silencedCompilerWarningOnly = false;
                    const Token *silencedWarning = secondBreak;
                    for (;;) {
                        if (Token::Match(silencedWarning, "( void ) %name% ;")) {
                            silencedWarning = silencedWarning->tokAt(5);
                            continue;
                        } else if (silencedWarning && silencedWarning == scope->bodyEnd)
                            silencedCompilerWarningOnly = true;

                        break;
                    }
                    if (silencedWarning)
                        secondBreak = silencedWarning;

                    if (!labelInFollowingLoop && !silencedCompilerWarningOnly)
                        unreachableCodeError(secondBreak, inconclusive);
                    tok = Token::findmatch(secondBreak, "[}:]");
                } else
                    tok = secondBreak;

                if (!tok)
                    break;
                tok = tok->previous(); // Will be advanced again by for loop
            }
        }
    }
}

void CheckOther::duplicateBreakError(const Token *tok, bool inconclusive)
{
    reportError(tok, Severity::style, "duplicateBreak",
                "Consecutive return, break, continue, goto or throw statements are unnecessary.\n"
                "Consecutive return, break, continue, goto or throw statements are unnecessary. "
                "The second statement can never be executed, and so should be removed.", CWE561, inconclusive);
}

void CheckOther::unreachableCodeError(const Token *tok, bool inconclusive)
{
    reportError(tok, Severity::style, "unreachableCode",
                "Statements following return, break, continue, goto or throw will never be executed.", CWE561, inconclusive);
}

//---------------------------------------------------------------------------
// Check scope of variables..
//---------------------------------------------------------------------------
void CheckOther::checkVariableScope()
{
    if (!mSettings->isEnabled(Settings::STYLE))
        return;

    const SymbolDatabase *symbolDatabase = mTokenizer->getSymbolDatabase();

    for (const Variable* var : symbolDatabase->variableList()) {
        if (!var || !var->isLocal() || (!var->isReference() && !var->typeStartToken()->isStandardType()))
            continue;

        if (var->isConst())
            continue;

        bool forHead = false; // Don't check variables declared in header of a for loop
        for (const Token* tok = var->typeStartToken(); tok; tok = tok->previous()) {
            if (tok->str() == "(") {
                forHead = true;
                break;
            } else if (Token::Match(tok, "[;{}]"))
                break;
        }
        if (forHead)
            continue;

        const Token* tok = var->nameToken()->next();
        if (Token::Match(tok, "; %varid% = %any% ;", var->declarationId())) {
            tok = tok->tokAt(3);
            if (!tok->isNumber() && tok->tokType() != Token::eString && tok->tokType() != Token::eChar && !tok->isBoolean())
                continue;
        }
        // bailout if initialized with function call that has possible side effects
        if (Token::Match(tok, "[(=]") && Token::simpleMatch(tok->astOperand2(), "("))
            continue;
        bool reduce = true;
        bool used = false; // Don't warn about unused variables
        for (; tok && tok != var->scope()->bodyEnd; tok = tok->next()) {
            if (tok->str() == "{" && tok->scope() != tok->previous()->scope())
            {
                if (used) {
                    bool used2 = false;
                    if (!checkInnerScope(tok, var, used2) || used2) {
                        reduce = false;
                        break;
                    }
                } else if (!checkInnerScope(tok, var, used)) {
                    reduce = false;
                    break;
                }

                tok = tok->link();

                // parse else if blocks..
            } else if (Token::simpleMatch(tok, "else { if (") && Token::simpleMatch(tok->linkAt(3), ") {")) {
                const Token *endif = tok->linkAt(3)->linkAt(1);
                bool elseif = false;
                if (Token::simpleMatch(endif, "} }"))
                    elseif = true;
                else if (Token::simpleMatch(endif, "} else {") && Token::simpleMatch(endif->linkAt(2),"} }"))
                    elseif = true;
                if (elseif && Token::findmatch(tok->next(), "%varid%", tok->linkAt(1), var->declarationId())) {
                    reduce = false;
                    break;
                }
            } else if (tok->varId() == var->declarationId() || tok->str() == "goto") {
                reduce = false;
                break;
            }
        }

        if (reduce && used)
            variableScopeError(var->nameToken(), var->name());
    }
}

bool CheckOther::checkInnerScope(const Token *tok, const Variable* var, bool& used)
{
    const Scope* scope = tok->next()->scope();
    bool loopVariable = scope->type == Scope::eFor || scope->type == Scope::eWhile || scope->type == Scope::eDo;
    bool noContinue = true;
    const Token* forHeadEnd = nullptr;
    const Token* end = tok->link();
    if (scope->type == Scope::eUnconditional && (tok->strAt(-1) == ")" || tok->previous()->isName())) // Might be an unknown macro like BOOST_FOREACH
        loopVariable = true;

    if (scope->type == Scope::eDo) {
        end = end->linkAt(2);
    } else if (loopVariable && tok->strAt(-1) == ")") {
        tok = tok->linkAt(-1); // Jump to opening ( of for/while statement
    } else if (scope->type == Scope::eSwitch) {
        for (const Scope* innerScope : scope->nestedList) {
            if (used) {
                bool used2 = false;
                if (!checkInnerScope(innerScope->bodyStart, var, used2) || used2) {
                    return false;
                }
            } else if (!checkInnerScope(innerScope->bodyStart, var, used)) {
                return false;
            }
        }
    }

    bool bFirstAssignment=false;
    for (; tok && tok != end; tok = tok->next()) {
        if (tok->str() == "goto")
            return false;
        if (tok->str() == "continue")
            noContinue = false;

        if (Token::simpleMatch(tok, "for ("))
            forHeadEnd = tok->linkAt(1);
        if (tok == forHeadEnd)
            forHeadEnd = nullptr;

        if (loopVariable && noContinue && tok->scope() == scope && !forHeadEnd && scope->type != Scope::eSwitch && Token::Match(tok, "%varid% =", var->declarationId())) { // Assigned in outer scope.
            loopVariable = false;
            unsigned int indent = 0;
            for (const Token* tok2 = tok->tokAt(2); tok2; tok2 = tok2->next()) { // Ensure that variable isn't used on right side of =, too
                if (tok2->str() == "(")
                    indent++;
                else if (tok2->str() == ")") {
                    if (indent == 0)
                        break;
                    indent--;
                } else if (tok2->str() == ";")
                    break;
                else if (tok2->varId() == var->declarationId()) {
                    loopVariable = true;
                    break;
                }
            }
        }

        if (loopVariable && Token::Match(tok, "%varid% !!=", var->declarationId())) // Variable used in loop
            return false;

        if (Token::Match(tok, "& %varid%", var->declarationId())) // Taking address of variable
            return false;

        if (Token::Match(tok, "%varid% =", var->declarationId()))
            bFirstAssignment = true;

        if (!bFirstAssignment && Token::Match(tok, "* %varid%", var->declarationId())) // dereferencing means access to previous content
            return false;

        if (Token::Match(tok, "= %varid%", var->declarationId()) && (var->isArray())) // Create a copy of array/pointer. Bailout, because the memory it points to might be necessary in outer scope
            return false;

        if (tok->varId() == var->declarationId()) {
            used = true;
            if (scope->type == Scope::eSwitch && scope == tok->scope())
                return false; // Used in outer switch scope - unsafe or impossible to reduce scope
        }
    }

    return true;
}

void CheckOther::variableScopeError(const Token *tok, const std::string &varname)
{
    reportError(tok,
                Severity::style,
                "variableScope",
                "$symbol:" + varname + "\n"
                                       "Scope of the variable can be reduced: '$symbol'\n"
                "The scope of the variable '$symbol' can be reduced. Warning: Be careful "
                "when fixing this message, especially when there are inner loops. Here is an "
                "example where cppcheck will write that the scope for 'i' can be reduced:\n"
                "void f(int x)\n"
                "{\n"
                "    int i = 0;\n"
                "    if (x) {\n"
                "        // it's safe to move 'int i = 0;' here\n"
                "        for (int n = 0; n < 10; ++n) {\n"
                "            // it is possible but not safe to move 'int i = 0;' here\n"
                "            do_something(&i);\n"
                "        }\n"
                "    }\n"
                "}\n"
                "When you see this message it is always safe to reduce the variable scope 1 level.", CWE398, false);
}

//---------------------------------------------------------------------------
// Comma in return statement: return a+1, b++;. (experimental)
//---------------------------------------------------------------------------
void CheckOther::checkCommaSeparatedReturn()
{
    // This is experimental for now. See #5076
    if (!mSettings->experimental)
        return;

    if (!mSettings->isEnabled(Settings::STYLE))
        return;

    for (const Token *tok = mTokenizer->tokens(); tok; tok = tok->next()) {
        if (tok->str() == "return") {
            tok = tok->next();
            while (tok && tok->str() != ";") {
                if (tok->link() && Token::Match(tok, "[([{<]"))
                    tok = tok->link();

                if (tok->str() == "," && tok->linenr() != tok->next()->linenr())
                    commaSeparatedReturnError(tok);

                tok = tok->next();
            }
            // bailout: missing semicolon (invalid code / bad tokenizer)
            if (!tok)
                break;
        }
    }
}

void CheckOther::commaSeparatedReturnError(const Token *tok)
{
    reportError(tok,
                Severity::style,
                "commaSeparatedReturn",
                "Comma is used in return statement. The comma can easily be misread as a ';'.\n"
                "Comma is used in return statement. When comma is used in a return statement it can "
                "easily be misread as a semicolon. For example in the code below the value "
                "of 'b' is returned if the condition is true, but it is easy to think that 'a+1' is "
                "returned:\n"
                "    if (x)\n"
                "        return a + 1,\n"
                "    b++;\n"
                "However it can be useful to use comma in macros. Cppcheck does not warn when such a "
                "macro is then used in a return statement, it is less likely such code is misunderstood.", CWE398, false);
}

//---------------------------------------------------------------------------
// Check for function parameters that should be passed by const reference
//---------------------------------------------------------------------------
static std::size_t estimateSize(const Type* type, const Settings* settings, const SymbolDatabase* symbolDatabase, std::size_t recursionDepth = 0)
{
    if (recursionDepth > 20)
        return 0;

    std::size_t cumulatedSize = 0;
    for (const Variable&var : type->classScope->varlist) {
        std::size_t size = 0;
        if (var.isStatic())
            continue;
        if (var.isReference())
            size = settings->sizeof_pointer;
        else if (var.type() && var.type()->classScope)
            size = estimateSize(var.type(), settings, symbolDatabase, recursionDepth+1);
        else
            size = symbolDatabase->sizeOfType(var.typeStartToken());

        if (var.isArray())
            cumulatedSize += size * var.dimension(0);
        else
            cumulatedSize += size;
    }
    for (const Type::BaseInfo &baseInfo : type->derivedFrom) {
        if (baseInfo.type && baseInfo.type->classScope)
            cumulatedSize += estimateSize(baseInfo.type, settings, symbolDatabase, recursionDepth+1);
    }
    return cumulatedSize;
}

static bool canBeConst(const Variable *var)
{
    {
        // check initializer list. If variable is moved from it can't be const.
        const Function* func_scope = var->scope()->function;
        if (func_scope->type == Function::Type::eConstructor) {
            //could be initialized in initializer list
            if (func_scope->arg->link()->next()->str() == ":") {
                for (const Token* tok2 = func_scope->arg->link()->next()->next(); tok2 != var->scope()->bodyStart; tok2 = tok2->next()) {
                    if (tok2->varId() != var->declarationId())
                        continue;
                    const Token* parent = tok2->astParent();
                    if (parent && Token::simpleMatch(parent->previous(), "move ("))
                        return false;
                }
            }
        }
    }
    for (const Token* tok2 = var->scope()->bodyStart; tok2 != var->scope()->bodyEnd; tok2 = tok2->next()) {
        if (tok2->varId() != var->declarationId())
            continue;

        const Token* parent = tok2->astParent();
        if (!parent)
            continue;
        if (parent->str() == "<<" || isLikelyStreamRead(parent))       
	    {
            if (parent->str() == "<<" && parent->astOperand1() == tok2)
                return false;
            if (parent->str() == ">>" && parent->astOperand2() == tok2)
                return false;
        } else if (parent->str() == "," || parent->str() == "(") { // function argument
            const Token* tok3 = tok2->previous();
            unsigned int argNr = 0;
            while (tok3 && tok3->str() != "(") {
                if (tok3->link() && Token::Match(tok3, ")|]|}|>"))
                    tok3 = tok3->link();
                else if (tok3->link())
                    break;
                else if (tok3->str() == ";")
                    break;
                else if (tok3->str() == ",")
                    argNr++;
                tok3 = tok3->previous();
            }
            if (!tok3 || tok3->str() != "(" || !tok3->astOperand1() || !tok3->astOperand1()->function())
                return false;
            else {
                const Variable* argVar = tok3->astOperand1()->function()->getArgumentVar(argNr);
                if (!argVar|| (!argVar->isConst() && argVar->isReference()))
                    return false;
            }
        } else if (parent->isUnaryOp("&")) {
            // TODO: check how pointer is used
            return false;
        } else if (parent->isConstOp())
            continue;
        else if (parent->isAssignmentOp()) {
            if (parent->astOperand1() == tok2)
                return false;
            else if (parent->astOperand1()->str() == "&") {
                const Variable* assignedVar = parent->previous()->variable();
                if (!assignedVar || !assignedVar->isConst())
                    return false;
            }
        } else if (Token::Match(tok2, "%var% . %name% (")) {
            const Function* func = tok2->tokAt(2)->function();
            if (func && (func->isConst() || func->isStatic()))
                continue;
            else
                return false;
        } else
            return false;
    }

    return true;
}

void CheckOther::checkPassByReference()
{
    if (!mSettings->isEnabled(Settings::PERFORMANCE))
        return;

    const SymbolDatabase * const symbolDatabase = mTokenizer->getSymbolDatabase();

    for (const Variable* var : symbolDatabase->variableList()) {
        if (!var || !var->isArgument() || !var->isClass() || var->isArray() || var->isReference() || var->isEnumType())
            continue;

        if (var->scope() && var->scope()->function && 
            var->scope()->function->arg && var->scope()->function->arg->link() && // some paranoid check of null pointers
            var->scope()->function->arg->link()->strAt(-1) == ".")
            continue; // references could not be used as va_start parameters (#5824)

        bool inconclusive = false;

        if (var->type() && !var->type()->isEnumType())
        { // Check if type is a struct or class.
            // Ensure that it is a large object.
            if (!var->type()->classScope)
                inconclusive = true;
            else
            {
                //@todo I am not sure if this calculation is correct in ctrl
                size_t size = estimateSize(var->type(), mSettings, symbolDatabase); 
                if ( size <= 2 * mSettings->sizeof_pointer)
                    continue;
            }
        }
        else
            continue;

        if (inconclusive && !mSettings->inconclusive)
            continue;

        const bool isConst = var->isConst();
        if (isConst) {
            passedByValueError(var->nameToken(), var->name(), inconclusive);
            continue;
        }

        // Check if variable could be const
        if (!var->scope())
            continue;

        if (canBeConst(var)) {
            passedByValueError(var->nameToken(), var->name(), inconclusive);
        }
    }
}

void CheckOther::passedByValueError(const Token *tok, const std::string &parname, bool inconclusive)
{
    reportError(tok, Severity::performance, "passedByValue",
                "$symbol:" + parname + "\n"
                                       "Function parameter '$symbol' should be passed by const reference.\n"
                                       "Parameter '$symbol' is passed by value. It could be passed "
                                       "as a const reference (const type & $symbol) which is usually faster and recommended in ctrl.",
                CWE398, inconclusive);
}

//---------------------------------------------------------------------------
// Incomplete statement..
//---------------------------------------------------------------------------

static bool isType(const Token * tok, bool unknown)
{
    if (Token::Match(tok, "%type%"))
        return true;
    if (Token::simpleMatch(tok, "::"))
        return isType(tok->astOperand2(), unknown);
    if (Token::simpleMatch(tok, "<") && tok->link())
        return true;
    if (unknown && Token::Match(tok, "%name% !!("))
        return true;
    return false;
}

static bool isVarDeclOp(const Token* tok)
{
    if (!tok)
        return false;
    const Token * vartok = tok->astOperand2();
    if (vartok && vartok->variable() && vartok->variable()->nameToken() == vartok)
        return true;
    const Token * typetok = tok->astOperand1();
    return isType(typetok, Token::Match(vartok, "%var%"));
}

static bool isConstStatement(const Token *tok)
{
    if (!tok)
        return false;
    if (Token::Match(tok, "%bool%|%num%|%str%|%char%|nullptr|NULL"))
        return true;
    if (Token::Match(tok, "%var%"))
        return true;
    if (Token::Match(tok, "*|&|&&") &&
        (Token::Match(tok->previous(), "::|.|const") || isVarDeclOp(tok)))
        return false;
    if (Token::Match(tok, "<<|>>") && !astIsIntegral(tok, false))
        return false;
    if (Token::Match(tok, "!|~|%cop%") && (tok->astOperand1() || tok->astOperand2()))
        return true;
    if (Token::simpleMatch(tok->previous(), "sizeof ("))
        return true;
    if (Token::Match(tok, "( %type%"))
        return isConstStatement(tok->astOperand1());
    if (Token::simpleMatch(tok, ","))
        return isConstStatement(tok->astOperand2());
    return false;
}

static bool isVoidStmt(const Token *tok)
{
    if (Token::simpleMatch(tok, "( void"))
        return true;
    const Token *tok2 = tok;
    while (tok2->astOperand1())
        tok2 = tok2->astOperand1();
    if (Token::simpleMatch(tok2->previous(), ")") && Token::simpleMatch(tok2->previous()->link(), "( void"))
        return true;
    if (Token::simpleMatch(tok2, "( void"))
        return true;
    return Token::Match(tok2->previous(), "delete|throw|return");
}

static bool isConstTop(const Token *tok)
{
    if (!tok)
        return false;
    if (tok == tok->astTop())
        return true;
    if (Token::simpleMatch(tok->astParent(), ";") && tok->astTop() &&
        Token::Match(tok->astTop()->previous(), "for|if (") && Token::simpleMatch(tok->astTop()->astOperand2(), ";")) {
        if (Token::simpleMatch(tok->astParent()->astParent(), ";"))
            return tok->astParent()->astOperand2() == tok;
        else
            return tok->astParent()->astOperand1() == tok;
    }
    return false;
}

void CheckOther::checkIncompleteStatement()
{
    if (!mSettings->isEnabled(Settings::WARNING))
        return;

    for (const Token *tok = mTokenizer->tokens(); tok; tok = tok->next()) {
        const Scope *scope = tok->scope();
        if (scope && !scope->isExecutable())
            continue;
        if (!isConstTop(tok))
            continue;
        const Token *rtok = nextAfterAstRightmostLeaf(tok);
        if (!Token::simpleMatch(tok->astParent(), ";") && !Token::simpleMatch(rtok, ";") &&
            !Token::Match(tok->previous(), ";|}|{ %any% ;"))
            continue;
        // Skipe statement expressions
        if (Token::simpleMatch(rtok, "; } )"))
            continue;
        if (!isConstStatement(tok))
            continue;
        if (isVoidStmt(tok))
            continue;
        bool inconclusive = Token::Match(tok, "%cop%");
        if (mSettings->inconclusive || !inconclusive)
            constStatementError(tok, tok->isNumber() ? "numeric" : "string", inconclusive);
    }
}

void CheckOther::constStatementError(const Token *tok, const std::string &type, bool inconclusive)
{
    std::string msg;
    if (Token::simpleMatch(tok, "=="))
        msg = "Found suspicious equality comparison. Did you intend to assign a value instead?";
    else if (Token::Match(tok, ",|!|~|%cop%"))
        msg = "Found suspicious operator '" + tok->str() + "'";
    else if (Token::Match(tok, "%var%"))
        msg = "Unused variable value '" + tok->str() + "'";
    else
        msg = "Redundant code: Found a statement that begins with " + type + " constant.";
    reportError(tok, Severity::warning, "constStatement", msg, CWE398, inconclusive);
}

//---------------------------------------------------------------------------
// Detect division by zero.
//---------------------------------------------------------------------------
void CheckOther::checkZeroDivision()
{
    for (const Token *tok = mTokenizer->tokens(); tok; tok = tok->next()) {
        if (!tok->astOperand2() || !tok->astOperand1())
            continue;
        if (tok->str() != "%" && tok->str() != "/" && tok->str() != "%=" && tok->str() != "/=")
            continue;
        if (!tok->valueType() || !tok->valueType()->isIntegral())
            continue;
        if (tok->astOperand1()->isNumber()) {
            if (MathLib::isFloat(tok->astOperand1()->str()))
                continue;
        } else if (tok->astOperand1()->isName()) {
            if (!tok->astOperand1()->valueType()->isIntegral())
                continue;
        } else if (!tok->astOperand1()->isArithmeticalOp())
            continue;

        // Value flow..
        const ValueFlow::Value *value = tok->astOperand2()->getValue(0LL);
        if (value && mSettings->isEnabled(value, false))
            zerodivError(tok, value);
    }
}

void CheckOther::zerodivError(const Token *tok, const ValueFlow::Value *value)
{
    if (!tok && !value) {
        reportError(tok, Severity::error, "zerodiv", "Division by zero.", CWE369, false);
        reportError(tok, Severity::error, "zerodivcond", ValueFlow::eitherTheConditionIsRedundant(nullptr) + " or there is division by zero.", CWE369, false);
        return;
    }

    const ErrorPath errorPath = getErrorPath(tok, value, "Division by zero");

    std::ostringstream errmsg;
    if (value->condition) {
        const unsigned int line = tok ? tok->linenr() : 0;
        errmsg << ValueFlow::eitherTheConditionIsRedundant(value->condition)
               << " or there is division by zero at line " << line << ".";
    } else
        errmsg << "Division by zero.";

    reportError(errorPath,
                value->errorSeverity() ? Severity::error : Severity::warning,
                value->condition ? "zerodivcond" : "zerodiv",
                errmsg.str(), CWE369, value->isInconclusive());
}

//---------------------------------------------------------------------------
// Check for NaN (not-a-number) in an arithmetic expression, e.g.
// double d = 1.0 / 0.0 + 100.0;
//---------------------------------------------------------------------------

void CheckOther::checkNanInArithmeticExpression()
{
    if (!mSettings->isEnabled(Settings::STYLE))
        return;
    for (const Token *tok = mTokenizer->tokens(); tok; tok = tok->next()) {
        if (tok->str() != "/")
            continue;
        if (!Token::Match(tok->astParent(), "[+-]"))
            continue;
        if (Token::simpleMatch(tok->astOperand2(), "0.0"))
            nanInArithmeticExpressionError(tok);
    }
}

void CheckOther::nanInArithmeticExpressionError(const Token *tok)
{
    reportError(tok, Severity::style, "nanInArithmeticExpression",
                "Using NaN/Inf in a computation.\n"
                "Using NaN/Inf in a computation. "
                "Although nothing bad really happens, it is suspicious.", CWE369, false);
}

//---------------------------------------------------------------------------
// Creating instance of classes which are destroyed immediately
//---------------------------------------------------------------------------
void CheckOther::checkMisusedScopedObject()
{
    if (!mSettings->isEnabled(Settings::STYLE))
        return;

    const SymbolDatabase * const symbolDatabase = mTokenizer->getSymbolDatabase();
    for (const Scope * scope : symbolDatabase->functionScopes) {
        for (const Token *tok = scope->bodyStart; tok && tok != scope->bodyEnd; tok = tok->next()) {
            if ((tok->next()->type() || (tok->next()->function() && tok->next()->function()->isConstructor())) // TODO: The rhs of || should be removed; It is a workaround for a symboldatabase bug
                && Token::Match(tok, "[;{}] %name% (")
                && Token::Match(tok->linkAt(2), ") ; !!}")
                && (!tok->next()->function() || // is not a function on this scope
                                                                                                      tok->next()->function()->isConstructor()))
            { // or is function in this scope and it's a ctor
                tok = tok->next();
                misusedScopeObjectError(tok, tok->str());
                tok = tok->next();
            }
        }
    }
}

void CheckOther::misusedScopeObjectError(const Token *tok, const std::string& varname)
{
    reportError(tok, Severity::style,
                "unusedScopedObject",
                "$symbol:" + varname + "\n"
                "Instance of '$symbol' object is destroyed immediately.", CWE563, false);
}

static const Token * getSingleExpressionInBlock(const Token * tok)
{
    if (!tok)
        return nullptr;
    const Token * top = tok->astTop();
    if (!top)
        return nullptr;
    const Token * nextExpression = nextAfterAstRightmostLeaf(top);
    if (!Token::simpleMatch(nextExpression, "; }"))
        return nullptr;
    return top;
}

//-----------------------------------------------------------------------------
// check for duplicate code in if and else branches
// if (a) { b = true; } else { b = true; }
//-----------------------------------------------------------------------------
void CheckOther::checkDuplicateBranch()
{
    // This is inconclusive since in practice most warnings are noise:
    // * There can be unfixed low-priority todos. The code is fine as it
    //   is but it could be possible to enhance it. Writing a warning
    //   here is noise since the code is fine (see cppcheck, abiword, ..)
    // * There can be overspecified code so some conditions can't be true
    //   and their conditional code is a duplicate of the condition that
    //   is always true just in case it would be false. See for instance
    //   abiword.
    if (!mSettings->isEnabled(Settings::STYLE) || !mSettings->inconclusive)
        return;

    const SymbolDatabase *symbolDatabase = mTokenizer->getSymbolDatabase();

    for (const Scope & scope : symbolDatabase->scopeList) {
        if (scope.type != Scope::eIf)
            continue;

        // check all the code in the function for if (..) else
        if (Token::simpleMatch(scope.bodyEnd, "} else {")) {

            // save if branch code
            const std::string branch1 = scope.bodyStart->next()->stringifyList(scope.bodyEnd);

            if (branch1.empty())
                continue;

            // save else branch code
            const std::string branch2 = scope.bodyEnd->tokAt(3)->stringifyList(scope.bodyEnd->linkAt(2));

            ErrorPath errorPath;
            // check for duplicates
            if (branch1 == branch2) {
                duplicateBranchError(scope.classDef, scope.bodyEnd->next(), errorPath);
                continue;
            }

            // check for duplicates using isSameExpression
            const Token * branchTop1 = getSingleExpressionInBlock(scope.bodyStart->next());
            const Token * branchTop2 = getSingleExpressionInBlock(scope.bodyEnd->tokAt(3));
            if (!branchTop1 || !branchTop2)
                continue;
            if (branchTop1->str() != branchTop2->str())
                continue;
            if (isSameExpression(branchTop1->astOperand1(), branchTop2->astOperand1(), mSettings->library, true, true, &errorPath) &&
                isSameExpression(branchTop1->astOperand2(), branchTop2->astOperand2(), mSettings->library, true, true, &errorPath))
                duplicateBranchError(scope.classDef, scope.bodyEnd->next(), errorPath);
        }
    }
}

void CheckOther::duplicateBranchError(const Token *tok1, const Token *tok2, ErrorPath errors)
{
    errors.emplace_back(tok2, "");
    errors.emplace_back(tok1, "");

    reportError(errors, Severity::style, "duplicateBranch", "Found duplicate branches for 'if' and 'else'.\n"
                "Finding the same code in an 'if' and related 'else' branch is suspicious and "
                "might indicate a cut and paste or logic error. Please examine this code "
                "carefully to determine if it is correct.", CWE398, true);
}

//---------------------------------------------------------------------------
// check for the same expression on both sides of an operator
// (x == x), (x && x), (x || x)
// (x.y == x.y), (x.y && x.y), (x.y || x.y)
//---------------------------------------------------------------------------

namespace {
    bool notconst(const Function* func)
    {
        return !func->isConst();
    }

    void getConstFunctions(const SymbolDatabase *symbolDatabase, std::list<const Function*> &constFunctions)
    {
        for (const Scope &scope : symbolDatabase->scopeList) {
            // only add const functions that do not have a non-const overloaded version
            // since it is pretty much impossible to tell which is being called.
            typedef std::map<std::string, std::list<const Function*> > StringFunctionMap;
            StringFunctionMap functionsByName;
            for (const Function &func : scope.functionList) {
                functionsByName[func.tokenDef->str()].push_back(&func);
            }
            for (StringFunctionMap::iterator it = functionsByName.begin();
                 it != functionsByName.end(); ++it) {
                const std::list<const Function*>::const_iterator nc = std::find_if(it->second.begin(), it->second.end(), notconst);
                if (nc == it->second.end()) {
                    // ok to add all of them
                    constFunctions.splice(constFunctions.end(), it->second);
                }
            }
        }
    }
}

void CheckOther::checkDuplicateExpression()
{
    const bool styleEnabled = mSettings->isEnabled(Settings::STYLE);
    const bool warningEnabled = mSettings->isEnabled(Settings::WARNING);
    if (!styleEnabled && !warningEnabled)
        return;

    // Parse all executing scopes..
    const SymbolDatabase *symbolDatabase = mTokenizer->getSymbolDatabase();

    std::list<const Function*> constFunctions;
    getConstFunctions(symbolDatabase, constFunctions);

    for (const Scope &scope : symbolDatabase->scopeList) {
        // only check functions
        if (scope.type != Scope::eFunction)
            continue;

        for (const Token *tok = scope.bodyStart; tok && tok != scope.bodyEnd; tok = tok->next()) {
            if (tok->str() == "=" && Token::Match(tok->astOperand1(), "%var%")) {
                const Token * endStatement = Token::findsimplematch(tok, ";");
                if (Token::Match(endStatement, "; %type% %var% ;")) {
                    endStatement = endStatement->tokAt(4);
                }
                if (Token::Match(endStatement, "%var% %assign%")) {
                    const Token * nextAssign = endStatement->tokAt(1);
                    const Token * var1 = tok->astOperand1();
                    const Token * var2 = nextAssign->astOperand1();
                    if (var1 && var2 &&
                        Token::Match(var1->previous(), ";|{|} %var%") &&
                        Token::Match(var2->previous(), ";|{|} %var%") &&
                        var2->valueType() && var1->valueType() &&
                        var2->valueType()->originalTypeName == var1->valueType()->originalTypeName &&
                        var2->valueType()->constness == var1->valueType()->constness &&
                        var2->varId() != var1->varId() && (
                            tok->astOperand2()->isArithmeticalOp() ||
                            tok->astOperand2()->str() == "." ||
                            Token::Match(tok->astOperand2()->previous(), "%name% (")
                        ) &&
                        tok->next()->tokType() != Token::eType &&
                        isSameExpression(tok->next(), nextAssign->next(), mSettings->library, true, false) &&
                        isSameExpression(tok->astOperand2(), nextAssign->astOperand2(), mSettings->library, true, false) &&
                        tok->astOperand2()->expressionString() == nextAssign->astOperand2()->expressionString())
                    {
                        bool assigned = false;
                        const Scope * varScope = var1->scope() ? var1->scope() : &scope;
                        for (const Token *assignTok = Token::findsimplematch(var2, ";"); assignTok && assignTok != varScope->bodyEnd; assignTok = assignTok->next()) {
                            if (Token::Match(assignTok, "%varid% = %var%", var1->varId()) && Token::Match(assignTok, "%var% = %varid%", var2->varId())) {
                                assigned = true;
                            }
                            if (Token::Match(assignTok, "%varid% = %var%", var2->varId()) && Token::Match(assignTok, "%var% = %varid%", var1->varId())) {
                                assigned = true;
                            }
                        }
                        if (!assigned && !isUniqueExpression(tok->astOperand2()))
                            duplicateAssignExpressionError(var1, var2, false);
                        else if (mSettings->inconclusive)
                            duplicateAssignExpressionError(var1, var2, true);
                    }
                }
            }
            ErrorPath errorPath;
            if (tok->isOp() && tok->astOperand1() && !Token::Match(tok, "+|*|<<|>>|+=|*=|<<=|>>=")) {
                if (Token::Match(tok, "==|!=|-") && astIsFloat(tok->astOperand1(), true))
                    continue;
                if (isSameExpression(tok->astOperand1(), tok->astOperand2(), mSettings->library, true, true, &errorPath))
                {
                    if (isWithoutSideEffects(tok->astOperand1()))
                    {
                        const bool assignment = tok->str() == "=";
                        if (assignment && warningEnabled)
                            selfAssignmentError(tok, tok->astOperand1()->expressionString());
                        else if (styleEnabled) {
                            if (tok->str() == "==")
                            {
                                const Token* parent = tok->astParent();
                                while (parent && parent->astParent()) {
                                    parent = parent->astParent();
                                }
                                if (parent && parent->previous() && parent->previous()->str() == "static_assert") {
                                    continue;
                                }
                            }
                            duplicateExpressionError(tok->astOperand1(), tok->astOperand2(), tok, errorPath);
                        }
                    }
                } else if (styleEnabled &&
                         isOppositeExpression(tok->astOperand1(), tok->astOperand2(), mSettings->library, false, true, &errorPath) &&
                         !Token::Match(tok, "=|-|-=|/|/=") &&
                         isWithoutSideEffects(tok->astOperand1()))
                {
                    oppositeExpressionError(tok, errorPath);
                } else if (!Token::Match(tok, "[-/%]")) { // These operators are not associative
                    if (styleEnabled && tok->astOperand2() && tok->str() == tok->astOperand1()->str() && isSameExpression(tok->astOperand2(), tok->astOperand1()->astOperand2(), mSettings->library, true, true, &errorPath) && isWithoutSideEffects(tok->astOperand2()))
                        duplicateExpressionError(tok->astOperand2(), tok->astOperand1()->astOperand2(), tok, errorPath);
                    else if (tok->astOperand2() && isConstExpression(tok->astOperand1(), mSettings->library, true))
                    {
                        const Token *ast1 = tok->astOperand1();
                        while (ast1 && tok->str() == ast1->str()) {
                            if (isSameExpression(ast1->astOperand1(), tok->astOperand2(), mSettings->library, true, true, &errorPath) &&
                                isWithoutSideEffects(ast1->astOperand1()) &&
                                isWithoutSideEffects(ast1->astOperand2()))
                                // Probably the message should be changed to 'duplicate expressions X in condition or something like that'.
                                duplicateExpressionError(ast1->astOperand1(), tok->astOperand2(), tok, errorPath);
                            ast1 = ast1->astOperand1();
                        }
                    }
                }
            } else if (styleEnabled && tok->astOperand1() && tok->astOperand2() && tok->str() == ":" && tok->astParent() && tok->astParent()->str() == "?") {
                if (!tok->astOperand1()->values().empty() && !tok->astOperand2()->values().empty() && isEqualKnownValue(tok->astOperand1(), tok->astOperand2()))
                    duplicateValueTernaryError(tok);
                else if (isSameExpression(tok->astOperand1(), tok->astOperand2(), mSettings->library, false, true, &errorPath))
                    duplicateExpressionTernaryError(tok, errorPath);
            }
        }
    }
}

void CheckOther::oppositeExpressionError(const Token *opTok, ErrorPath errors)
{
    errors.emplace_back(opTok, "");

    const std::string& op = opTok ? opTok->str() : "&&";

    reportError(errors, Severity::style, "oppositeExpression", "Opposite expression on both sides of \'" + op + "\'.\n"
                "Finding the opposite expression on both sides of an operator is suspicious and might "
                "indicate a cut and paste or logic error. Please examine this code carefully to "
                "determine if it is correct.", CWE398, false);
}

void CheckOther::duplicateExpressionError(const Token *tok1, const Token *tok2, const Token *opTok, ErrorPath errors)
{
    errors.emplace_back(opTok, "");

    const std::string& expr1 = tok1 ? tok1->expressionString() : "x";
    const std::string& expr2 = tok2 ? tok2->expressionString() : "x";

    const std::string& op = opTok ? opTok->str() : "&&";
    std::string msg = "Same expression on both sides of \'" + op + "\'";
    std::string id = "duplicateExpression";
    if (expr1 != expr2) {
        id = "knownConditionTrueFalse";
        std::string exprMsg = "The expression \'" + expr1 + " " + op +  " " + expr2 + "\' is always ";
        if (Token::Match(opTok, "==|>=|<="))
            msg = exprMsg + "true";
        else if (Token::Match(opTok, "!=|>|<"))
            msg = exprMsg + "false";
        if (!Token::Match(tok1, "%num%|NULL|nullptr") && !Token::Match(tok2, "%num%|NULL|nullptr"))
            msg += " because '" + expr1 + "' and '" + expr2 + "' represent the same value";
    }

    reportError(errors, Severity::style, id.c_str(), msg + ".\n"
                "Finding the same expression on both sides of an operator is suspicious and might "
                "indicate a cut and paste or logic error. Please examine this code carefully to "
                "determine if it is correct.", CWE398, false);
}

void CheckOther::duplicateAssignExpressionError(const Token *tok1, const Token *tok2, bool inconclusive)
{
    const std::list<const Token *> toks = { tok2, tok1 };

    const std::string& var1 = tok1 ? tok1->str() : "x";
    const std::string& var2 = tok2 ? tok2->str() : "x";

    reportError(toks, Severity::style, "duplicateAssignExpression",
                "Same expression used in consecutive assignments of '" + var1 + "' and '" + var2 + "'.\n"
                "Finding variables '" + var1 + "' and '" + var2 + "' that are assigned the same expression "
                "is suspicious and might indicate a cut and paste or logic error. Please examine this code carefully to "
                "determine if it is correct.", CWE398, inconclusive);
}

void CheckOther::duplicateExpressionTernaryError(const Token *tok, ErrorPath errors)
{
    errors.emplace_back(tok, "");
    reportError(errors, Severity::style, "duplicateExpressionTernary", "Same expression in both branches of ternary operator.\n"
                "Finding the same expression in both branches of ternary operator is suspicious as "
                "the same code is executed regardless of the condition.", CWE398, false);
}

void CheckOther::duplicateValueTernaryError(const Token *tok)
{
    reportError(tok, Severity::style, "duplicateValueTernary", "Same value in both branches of ternary operator.\n"
                "Finding the same value in both branches of ternary operator is suspicious as "
                "the same code is executed regardless of the condition.", CWE398, false);
}

void CheckOther::selfAssignmentError(const Token *tok, const std::string &varname)
{
    reportError(tok, Severity::warning,
                "selfAssignment",
                "$symbol:" + varname + "\n"
                "Redundant assignment of '$symbol' to itself.", CWE398, false);
}

//---------------------------------------------------------------------------
// Check testing sign of unsigned variables and pointers.
//---------------------------------------------------------------------------
//@todo sign - activate when there is ctrl sign implementaion
/*void CheckOther::checkSignOfUnsignedVariable()
{
    if (!mSettings->isEnabled(Settings::STYLE))
        return;

    const SymbolDatabase *symbolDatabase = mTokenizer->getSymbolDatabase();

    for (const Scope *scope : symbolDatabase->functionScopes)
    {
        // check all the code in the function
        for (const Token *tok = scope->bodyStart->next(); tok != scope->bodyEnd; tok = tok->next())
        {
            if (!tok->isComparisonOp() || !tok->astOperand1() || !tok->astOperand2())
                continue;

            const ValueFlow::Value *v1 = tok->astOperand1()->getValue(0);
            const ValueFlow::Value *v2 = tok->astOperand2()->getValue(0);

            if (Token::Match(tok, "<|<=") && v2 && v2->isKnown())
            {
                const ValueType *vt = tok->astOperand1()->valueType();
                if (vt && vt->sign == ValueType::UNSIGNED)
                    unsignedLessThanZeroError(tok, v2, tok->astOperand1()->expressionString());
            }
            else if (Token::Match(tok, ">|>=") && v1 && v1->isKnown())
            {
                const ValueType *vt = tok->astOperand2()->valueType();
                if (vt && vt->sign == ValueType::UNSIGNED)
                    unsignedLessThanZeroError(tok, v1, tok->astOperand2()->expressionString());
            }
            else if (Token::simpleMatch(tok, ">=") && v2 && v2->isKnown())
            {
                const ValueType *vt = tok->astOperand1()->valueType();
                if (vt && vt->sign == ValueType::UNSIGNED)
                    unsignedPositiveError(tok, v2, tok->astOperand1()->expressionString());
            }
            else if (Token::simpleMatch(tok, "<=") && v1 && v1->isKnown())
            {
                const ValueType *vt = tok->astOperand2()->valueType();
                if (vt && vt->sign == ValueType::UNSIGNED)
                    unsignedPositiveError(tok, v1, tok->astOperand2()->expressionString());
            }
        }
    }
}*/

void CheckOther::unsignedLessThanZeroError(const Token *tok, const ValueFlow::Value * v, const std::string &varname)
{
    reportError(getErrorPath(tok, v, "Unsigned less than zero"), Severity::style, "unsignedLessThanZero",
                "$symbol:" + varname + "\n"
                "Checking if unsigned expression '$symbol' is less than zero.\n"
                "The unsigned expression '$symbol' will never be negative so it "
                "is either pointless or an error to check if it is.", CWE570, false);
}

void CheckOther::unsignedPositiveError(const Token *tok, const ValueFlow::Value * v, const std::string &varname)
{
    reportError(getErrorPath(tok, v, "Unsigned positive"), Severity::style, "unsignedPositive",
                "$symbol:" + varname + "\n"
                "Unsigned expression '$symbol' can't be negative so it is unnecessary to test it.", CWE570, false);
}

/* check if a constructor in given class scope takes a reference */
static bool constructorTakesReference(const Scope * const classScope)
{
    for (const Function &constructor : classScope->functionList) {
        if (constructor.isConstructor()) {
            for (std::size_t argnr = 0U; argnr < constructor.argCount(); argnr++) {
                const Variable * const argVar = constructor.getArgumentVar(argnr);
                if (argVar && argVar->isReference()) {
                    return true;
                }
            }
        }
    }
    return false;
}

//---------------------------------------------------------------------------
// Checking for shift by negative values
//---------------------------------------------------------------------------

static bool isNegative(const Token *tok, const Settings *settings)
{
    //@todo sign - activate when there is ctrl sign implementaion -> && tok->valueType()->sign == ValueType::SIGNED
    return tok->valueType() && tok->getValueLE(-1LL, settings);
}

void CheckOther::checkNegativeBitwiseShift()
{
    const bool portability = mSettings->isEnabled(Settings::PORTABILITY);

    for (const Token* tok = mTokenizer->tokens(); tok; tok = tok->next()) {
        if (!tok->astOperand1() || !tok->astOperand2())
            continue;

        if (!Token::Match(tok, "<<|>>|<<=|>>="))
            continue;

        // don't warn if lhs is a class. this is an overloaded operator then
        const ValueType *lhsType = tok->astOperand1()->valueType();
        if (!lhsType || !lhsType->isIntegral())
            continue;

        // bailout if operation is protected by ?:
        bool ternary = false;
        for (const Token *parent = tok; parent; parent = parent->astParent()) {
            if (Token::Match(parent, "?|:")) {
                ternary = true;
                break;
            }
        }
        if (ternary)
            continue;

        // Get negative rhs value. preferably a value which doesn't have 'condition'.
        if (portability && isNegative(tok->astOperand1(), mSettings))
            negativeBitwiseShiftError(tok, 1);
        else if (isNegative(tok->astOperand2(), mSettings))
            negativeBitwiseShiftError(tok, 2);
    }
}


void CheckOther::negativeBitwiseShiftError(const Token *tok, int op)
{
    if (op == 1)
        // LHS - this is used by intention in various software, if it
        // is used often in a project and works as expected then this is
        // a portability issue
        reportError(tok, Severity::portability, "shiftNegativeLHS", "Shifting a negative value is technically undefined behaviour", CWE758, false);
    else // RHS
        reportError(tok, Severity::error, "shiftNegative", "Shifting by a negative value is undefined behaviour", CWE758, false);
}

//---------------------------------------------------------------------------
// Detect NULL being passed to variadic function.
//---------------------------------------------------------------------------

void CheckOther::checkVarFuncNullUB()
{
    if (!mSettings->isEnabled(Settings::PORTABILITY))
        return;

    const SymbolDatabase *symbolDatabase = mTokenizer->getSymbolDatabase();
    for (const Scope * scope : symbolDatabase->functionScopes) {
        for (const Token* tok = scope->bodyStart; tok != scope->bodyEnd; tok = tok->next()) {
            // Is NULL passed to a function?
            if (Token::Match(tok,"[(,] NULL [,)]")) {
                // Locate function name in this function call.
                const Token *ftok = tok;
                std::size_t argnr = 1;
                while (ftok && ftok->str() != "(") {
                    if (ftok->str() == ")")
                        ftok = ftok->link();
                    else if (ftok->str() == ",")
                        ++argnr;
                    ftok = ftok->previous();
                }
                ftok = ftok ? ftok->previous() : nullptr;
                if (ftok && ftok->isName()) {
                    // If this is a variadic function then report error
                    const Function *f = ftok->function();
                    if (f && f->argCount() <= argnr) {
                        const Token *tok2 = f->argDef;
                        tok2 = tok2 ? tok2->link() : nullptr; // goto ')'
                        if (tok2 && Token::simpleMatch(tok2->tokAt(-3), ". . ."))
                            varFuncNullUBError(tok);
                    }
                }
            }
        }
    }
}

void CheckOther::varFuncNullUBError(const Token *tok)
{
    reportError(tok,
                Severity::portability,
                "varFuncNullUB",
                "Passing NULL after the last typed argument to a variadic function leads to undefined behaviour.\n"
                "Passing NULL after the last typed argument to a variadic function leads to undefined behaviour.\n"
                "The C99 standard, in section 7.15.1.1, states that if the type used by va_arg() is not compatible with the type of the actual next argument (as promoted according to the default argument promotions), the behavior is undefined.\n"
                "The value of the NULL macro is an implementation-defined null pointer constant (7.17), which can be any integer constant expression with the value 0, or such an expression casted to (void*) (6.3.2.3). This includes values like 0, 0L, or even 0LL.\n"
                "In practice on common architectures, this will cause real crashes if sizeof(int) != sizeof(void*), and NULL is defined to 0 or any other null pointer constant that promotes to int.\n"
                "To reproduce you might be able to use this little code example on 64bit platforms. If the output includes \"ERROR\", the sentinel had only 4 out of 8 bytes initialized to zero and was not detected as the final argument to stop argument processing via va_arg(). Changing the 0 to (void*)0 or 0L will make the \"ERROR\" output go away.\n"
                "#include <stdarg.h>\n"
                "#include <stdio.h>\n"
                "\n"
                "void f(char *s, ...) {\n"
                "    va_list ap;\n"
                "    va_start(ap,s);\n"
                "    for (;;) {\n"
                "        char *p = va_arg(ap,char*);\n"
                "        printf(\"%018p, %s\\n\", p, (long)p & 255 ? p : \"\");\n"
                "        if(!p) break;\n"
                "    }\n"
                "    va_end(ap);\n"
                "}\n"
                "\n"
                "void g() {\n"
                "    char *s2 = \"x\";\n"
                "    char *s3 = \"ERROR\";\n"
                "\n"
                "    // changing 0 to 0L for the 7th argument (which is intended to act as sentinel) makes the error go away on x86_64\n"
                "    f(\"first\", s2, s2, s2, s2, s2, 0, s3, (char*)0);\n"
                "}\n"
                "\n"
                "void h() {\n"
                "    int i;\n"
                "    volatile unsigned char a[1000];\n"
                "    for (i = 0; i<sizeof(a); i++)\n"
                "        a[i] = -1;\n"
                "}\n"
                "\n"
                "int main() {\n"
                "    h();\n"
                "    g();\n"
                "    return 0;\n"
                "}", CWE475, false);
}

void CheckOther::checkUnusedLabel()
{
    if (!mSettings->isEnabled(Settings::STYLE) && !mSettings->isEnabled(Settings::WARNING))
        return;

    const SymbolDatabase *symbolDatabase = mTokenizer->getSymbolDatabase();
    for (const Scope * scope : symbolDatabase->functionScopes) {
        for (const Token* tok = scope->bodyStart; tok != scope->bodyEnd; tok = tok->next()) {
            if (!tok->scope()->isExecutable())
                tok = tok->scope()->bodyEnd;

            if (Token::Match(tok, "{|}|; %name% :") && tok->strAt(1) != "default") {
                if (!Token::findsimplematch(scope->bodyStart->next(), ("goto " + tok->strAt(1)).c_str(), scope->bodyEnd->previous()))
                    unusedLabelError(tok->next(), tok->next()->scope()->type == Scope::eSwitch);
            }
        }
    }
}

void CheckOther::unusedLabelError(const Token* tok, bool inSwitch)
{
    if (inSwitch) {
        if (!tok || mSettings->isEnabled(Settings::WARNING))
            reportError(tok, Severity::warning, "unusedLabelSwitch",
                        "$symbol:" + (tok ? tok->str() : emptyString) + "\n"
                        "Label '$symbol' is not used. Should this be a 'case' of the enclosing switch()?", CWE398, false);
    } else {
        if (!tok || mSettings->isEnabled(Settings::STYLE))
            reportError(tok, Severity::style, "unusedLabel",
                        "$symbol:" + (tok ? tok->str() : emptyString) + "\n"
                        "Label '$symbol' is not used.", CWE398, false);
    }
}


void CheckOther::checkEvaluationOrder()
{
    if (!mSettings->isEnabled(Settings::WARNING))
        return;

    const SymbolDatabase *symbolDatabase = mTokenizer->getSymbolDatabase();
    for (const Scope * functionScope : symbolDatabase->functionScopes) {
        for (const Token* tok = functionScope->bodyStart; tok != functionScope->bodyEnd; tok = tok->next()) {
            if (!Token::Match(tok, "++|--") && !tok->isAssignmentOp())
                continue;
            if (!tok->astOperand1())
                continue;
            for (const Token *tok2 = tok;; tok2 = tok2->astParent()) {
                // If ast parent is a sequence point then break
                const Token * const parent = tok2->astParent();
                if (!parent)
                    break;
                if (Token::Match(parent, "%oror%|&&|?|:|;"))
                    break;
                if (parent->str() == ",") {
                    const Token *par = parent;
                    while (Token::simpleMatch(par,","))
                        par = par->astParent();
                    // not function or in a while clause => break
                    if (!(par && par->str() == "(" && par->astOperand2() && par->strAt(-1) != "while"))
                        break;
                    // control flow (if|while|etc) => break
                    if (Token::simpleMatch(par->link(),") {"))
                        break;
                    // sequence point in function argument: dostuff((1,2),3) => break
                    par = par->next();
                    while (par && (par->previous() != parent))
                        par = par->nextArgument();
                    if (!par)
                        break;
                }
                if (parent->str() == "(" && parent->astOperand2())
                    break;

                // self assignment..
                if (tok2 == tok &&
                    tok->str() == "=" &&
                    parent->str() == "=" &&
                    isSameExpression(tok->astOperand1(), parent->astOperand1(), mSettings->library, true, false))
                {
                    if (mSettings->isEnabled(Settings::WARNING) &&
                        isSameExpression(tok->astOperand1(), parent->astOperand1(), mSettings->library, true, false))
                        selfAssignmentError(parent, tok->astOperand1()->expressionString());
                    break;
                }

                // Is expression used?
                bool foundError = false;
                visitAstNodes((parent->astOperand1() != tok2) ? parent->astOperand1() : parent->astOperand2(),
                [&](const Token *tok3) {
                    if (tok3->str() == "&" && !tok3->astOperand2())
                        return ChildrenToVisit::none; // don't handle address-of for now
                    if (tok3->str() == "(" && Token::simpleMatch(tok3->previous(), "sizeof"))
                        return ChildrenToVisit::none; // don't care about sizeof usage
                                  if (isSameExpression(tok->astOperand1(), tok3, mSettings->library, true, false))
                        foundError = true;
                    return foundError ? ChildrenToVisit::done : ChildrenToVisit::op1_and_op2;
                });

                if (foundError && mSettings->inconclusive)
                {
                    unknownEvaluationOrder(parent);
                    break;
                }
            }
        }
    }
}

void CheckOther::unknownEvaluationOrder(const Token* tok)
{
    reportError(tok, Severity::warning, "unknownEvaluationOrder",
                "Expression '" + (tok ? tok->expressionString() : std::string("x = x++;")) + "' depends on order of evaluation of side effects", CWE768, false);
}

//----------------------------------------------------------------------------------------------------------------------------------------
// check for shadow variables and not unique function attribues
//----------------------------------------------------------------------------------------------------------------------------------------
static const Token *findShadowed(const Scope *scope, const std::string &varname, int linenr)
{
    if (!scope)
        return nullptr;
    for (const Variable &var : scope->varlist)
    {
        if (scope->isExecutable() && var.nameToken()->linenr() > linenr)
            continue;
        if (var.name() == varname)
            return var.nameToken();
    }
    for (const Function &f : scope->functionList)
    {
        if (f.name() == varname)
            return f.tokenDef;
    }

    return findShadowed(scope->nestedIn, varname, linenr);
}

//----------------------------------------------------------------------------------------------------------------------------------------
void CheckOther::checkShadowVariables()
{
    if (!mSettings->isEnabled(Settings::STYLE))
        return;
    const SymbolDatabase *symbolDatabase = mTokenizer->getSymbolDatabase();
    for (const Scope &scope : symbolDatabase->scopeList)
    {

        if (!scope.isExecutable())
            continue;

        if (scope.function && scope.function->argumentList.size() > 0)
        {
            std::list<std::string> restArgs;
            for (const Variable &var : scope.function->argumentList)
            {
                restArgs.push_back(var.name());
            }
            restArgs.erase(restArgs.begin());

            for (const Variable &var : scope.function->argumentList)
            {
                // check functions argument for shadow vars
                checkShadowVar(scope, var);

                // check unique argument names
                if (std::find(restArgs.begin(), restArgs.end(), var.name()) != restArgs.end())
                    notUniqueArgNameError(var.nameToken());

                if (restArgs.size() > 0)
                    restArgs.erase(restArgs.begin());
            }
        }

        for (const Variable &var : scope.varlist)
        {
            checkShadowVar(scope, var);
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------------------------
void CheckOther::checkShadowVar(const Scope &scope, const Variable &var)
{
    if (!var.nameToken())
        return;

    const Token *shadowed = findShadowed(scope.nestedIn, var.name(), var.nameToken()->linenr());
    if (!shadowed)
        return;

    if (scope.type == Scope::eFunction && scope.className == var.name())
        return;
    shadowError(var.nameToken(), shadowed, shadowed->varId() != 0);
}

//----------------------------------------------------------------------------------------------------------------------------------------
void CheckOther::shadowError(const Token *var, const Token *shadowed, bool shadowVar)
{
    ErrorPath errorPath;
    errorPath.push_back(ErrorPathItem(shadowed, "Shadowed declaration"));
    errorPath.push_back(ErrorPathItem(var, "Shadow variable"));
    const std::string varname = var ? var->str() : (shadowVar ? "var" : "f");
    const char *id = shadowVar ? "shadowVar" : "shadowFunction";
    std::string message = "$symbol:" + varname + "\n" +
                          "Local variable shadows outer " + (shadowVar ? "variable" : "function") + ": \'$symbol\'";
    reportError(errorPath, Severity::style, id, message, CWE398, false);
}

//----------------------------------------------------------------------------------------------------------------------------------------
void CheckOther::notUniqueArgNameError(const Token *var)
{
    const std::string varname = var ? var->str() : "var";
    std::string message = "$symbol:" + varname + "\nThe function parameter is not unique: \'$symbol\'";
    reportError(var, Severity::warning, "notUniqueFunctionAttributeName", message, CWE398, false);
}

//----------------------------------------------------------------------------------------------------------------------------------------

void CheckOther::checkConstArgument()
{
    if (!mSettings->isEnabled(Settings::STYLE))
        return;
    const SymbolDatabase *symbolDatabase = mTokenizer->getSymbolDatabase();
    for (const Scope *functionScope : symbolDatabase->functionScopes) {
        for (const Token *tok = functionScope->bodyStart; tok != functionScope->bodyEnd; tok = tok->next()) {
            if (!Token::simpleMatch(tok->astParent(), "("))
                continue;
            if (!Token::Match(tok->astParent()->previous(), "%name%"))
                continue;
            if (Token::Match(tok->astParent()->previous(), "if|while|switch|sizeof"))
                continue;
            if (tok == tok->astParent()->previous())
                continue;
            if (!tok->hasKnownIntValue())
                continue;
            if (Token::Match(tok, "++|--"))
                continue;
            if (isConstVarExpression(tok))
                continue;
            const Token * tok2 = tok;
            if (Token::Match(tok2, "%var%"))
                continue;
            constArgumentError(tok, tok->astParent()->previous(), &tok->values().front());
        }
    }
}

void CheckOther::constArgumentError(const Token *tok, const Token *ftok, const ValueFlow::Value *value)
{
    MathLib::bigint intvalue = value ? value->intvalue : 0;
    const std::string expr = tok ? tok->expressionString() : std::string("x");
    const std::string fun = ftok ? ftok->str() : std::string("f");

    const std::string errmsg = "Argument '" + expr + "' to function " + fun + " is always " + std::to_string(intvalue);
    const ErrorPath errorPath = getErrorPath(tok, value, errmsg);
    reportError(errorPath, Severity::style, "constArgument", errmsg, CWE570, false);
}

static ValueFlow::Value getLifetimeObjValue(const Token *tok)
{
    ValueFlow::Value result;
    auto pred = [](const ValueFlow::Value &v) {
        if (!v.isLocalLifetimeValue())
            return false;
        if (!v.tokvalue->variable())
            return false;
        return true;
    };
    auto it = std::find_if(tok->values().begin(), tok->values().end(), pred);
    if (it == tok->values().end())
        return result;
    result = *it;
    // There should only be one lifetime
    if (std::find_if(std::next(it), tok->values().end(), pred) != tok->values().end())
        return result;
    return result;
}

//---------------------------------------------------------------------------------------------------------------------------------------
void CheckOther::checkUndeclaredVar()
{
    for (const Token *tok = mTokenizer->tokens(); tok; tok = tok->next())
    {
        if (Token::Match(tok, "%name% %name%"))
            continue;

        if (isUndeclaredVar(tok))
        {
            //std::cout << __FUNCTION__ << " tok is not def" << std::endl;
            checkUndeclaredVarError(tok);
        }
    }
}

//---------------------------------------------------------------------------------------------------------------------------------------
bool CheckOther::isUndeclaredVar(const Token *tok)
{
    if (!tok || tok->str() == "")
        return false; // empty token given

    if (!tok->scope()->isExecutable())
        return false; // enum, struct, global scope... can not be undefined variable there

    //std::cout << "next***\ntok: " << tok->str() << std::endl;

    // this is known variable
    if ((tok->varId() != 0) || (tok->valueType() != 0))
        return false;

    // this is standard type
    if (tok->isStandardType())
    {
        return false;
    }
    // this is some casting
    if (tok->isCast())
    {
        return false;
    }

    // numbers
    if (tok->isNumber())
    {
        return false;
    }

    if (tok->hasKnownValue())
        return false; // some constant or defines via xml config file

    if (tok->Match(tok, "%op%"))
        return false; // this operand

    if (tok->isKeyword())
        return false; // is keyword

    if (tok->isControlFlowKeyword() || tok->Match(tok, "%name% {"))
        return false; // if,else,while,do....
    if (isFunctionOrBreakPattern(tok))
        return false; // is function like

    // new and this are standard keywords
    if (tok->str() == "new" || tok->str() == "this")
        return false;

    if (Token::Match(tok, "(|)|{|}|;|[|]|:|?|::|.|#|<|>|,"))
        return false; // not possible char in variable name

    if (tok->str().at(0) == '$') // it does not work with ::Match(), beccouse the $ is just resereved now
        return false;            // ignore dollar parameters

    const Token *previous = tok->previous();
    if (previous && ((previous->str() == ".") || (previous->str() == "::")))
        return false; // ignore this.notation and ::notation

    const Token *next = tok->next();
    if (next && ((next->str() == ".") || (next->str() == "::")))
        return false; // ignore this.notation and ::notation

    //ignore variablenames starting with g_ -> should be global variables
    /// @todo remove this hack
    if (mSettings->inconclusive && tok->str().rfind("g_", 0) == 0)
    {
        return false;
    }

    //     std::cout << "tok: " << tok->str() << std::endl;
    //     std::cout << "tokType: " << tok->tokType() << std::endl;
    /* std::cout << "hasKnownValue: " << tok->hasKnownValue() << std::endl;
  std::cout << "valueType: " << tok->valueType() << std::endl;
  std::cout << "varId: " << tok->varId() << std::endl;
  std::cout << "scope type: " << tok->scope()->type << std::endl;*/

    return true;
}

//---------------------------------------------------------------------------------------------------------------------------------------
void CheckOther::checkUndeclaredVarError(const Token *tok)
{
    std::string varName = tok ? tok->str() : "variableName";

    reportError(
        tok,
        Severity::warning,
        "undefinedVariable",
        "$symbol:" + varName + "\n" +
            "Undefined variable: $symbol\n"
            "The variable $symbol is not defined. Maybe it's added by addGlobal() or #uses is missing",
        CWE758, false);
}