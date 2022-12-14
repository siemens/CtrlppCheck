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

/**
 * @brief This is the ValueFlow component in Cppcheck.
 *
 * Each @sa Token in the token list has a list of values. These are
 * the "possible" values for the Token at runtime.
 *
 * In the --debug and --debug-normal output you can see the ValueFlow data. For example:
 *
 *     int f()
 *     {
 *         int x = 10;
 *         return 4 * x + 2;
 *     }
 *
 * The --debug-normal output says:
 *
 *     ##Value flow
 *     Line 3
 *       10 always 10
 *     Line 4
 *       4 always 4
 *       * always 40
 *       x always 10
 *       + always 42
 *       2 always 2
 *
 * All value flow analysis is executed in the ValueFlow::setValues() function. The ValueFlow analysis is executed after the tokenizer/ast/symboldatabase/etc..
 * The ValueFlow analysis is done in a series of valueFlow* function calls, where each such function call can only use results from previous function calls.
 * The function calls should be arranged so that valueFlow* that do not require previous ValueFlow information should be first.
 *
 * Type of analysis
 * ================
 *
 * This is "flow sensitive" value flow analysis. We _usually_ track the value for 1 variable at a time.
 *
 * How are calculations handled
 * ============================
 *
 * Here is an example code:
 *
 *   x = 3 + 4;
 *
 * The valueFlowNumber set the values for the "3" and "4" tokens by calling setTokenValue().
 * The setTokenValue() handle the calculations automatically. When both "3" and "4" have values, the "+" can be calculated. setTokenValue() recursively calls itself when parents in calculations can be calculated.
 *
 * Forward / Reverse flow analysis
 * ===============================
 *
 * In forward value flow analysis we know a value and see what happens when we are stepping the program forward. Like normal execution.
 * The valueFlowForward is used in this analysis.
 *
 * In reverse value flow analysis we know the value of a variable at line X. And try to "execute backwards" to determine possible values before line X.
 * The valueFlowReverse is used in this analysis.
 *
 *
 */

#include "valueflow.h"

#include "astutils.h"
#include "errorlogger.h"
#include "library.h"
#include "mathlib.h"
#include "platform.h"
#include "settings.h"
#include "symbols/symbols.h"
#include "token.h"
#include "tokenlist.h"
#include "utils.h"
#include "path.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <limits>
#include <map>
#include <set>
#include <stack>
#include <vector>

static const int TIMEOUT = 10; // Do not repeat ValueFlow analysis more than 10 seconds

namespace {
    struct ProgramMemory {
        std::map<unsigned int, ValueFlow::Value> values;

        void setValue(unsigned int varid, const ValueFlow::Value &value) {
            values[varid] = value;
        }

        bool getIntValue(unsigned int varid, MathLib::bigint* result) const {
            const std::map<unsigned int, ValueFlow::Value>::const_iterator it = values.find(varid);
            const bool found = it != values.end() && it->second.isIntValue();
            if (found)
                *result = it->second.intvalue;
            return found;
        }

        void setIntValue(unsigned int varid, MathLib::bigint value) {
            values[varid] = ValueFlow::Value(value);
        }

        bool getTokValue(unsigned int varid, const Token** result) const {
            const std::map<unsigned int, ValueFlow::Value>::const_iterator it = values.find(varid);
            const bool found = it != values.end() && it->second.isTokValue();
            if (found)
                *result = it->second.tokvalue;
            return found;
        }

        bool hasValue(unsigned int varid) {
            return values.find(varid) != values.end();
        }

        void swap(ProgramMemory &pm) {
            values.swap(pm.values);
        }

        void clear() {
            values.clear();
        }

        bool empty() const {
            return values.empty();
        }
    };
}

static void execute(const Token *expr,
                    ProgramMemory * const programMemory,
                    MathLib::bigint *result,
                    bool *error);

static void bailoutInternal(TokenList *tokenlist, ErrorLogger *errorLogger, const Token *tok, const std::string &what, const std::string &file, int line, const std::string &function)
{
    std::list<ErrorLogger::ErrorMessage::FileLocation> callstack(1, ErrorLogger::ErrorMessage::FileLocation(tok, tokenlist));
    ErrorLogger::ErrorMessage errmsg(callstack, tokenlist->getSourceFilePath(), Severity::debug,
                                     Path::stripDirectoryPart(file) + ":" + MathLib::toString(line) + ":" + function + " bailout: " + what, "valueFlowBailout", false);
    errorLogger->reportErr(errmsg);
}

#if (defined __cplusplus) && __cplusplus >= 201103L
#define bailout(tokenlist, errorLogger, tok, what) bailoutInternal(tokenlist, errorLogger, tok, what, __FILE__, __LINE__, __func__)
#elif (defined __GNUC__) || (defined __clang__) || (defined _MSC_VER)
#define bailout(tokenlist, errorLogger, tok, what) bailoutInternal(tokenlist, errorLogger, tok, what, __FILE__, __LINE__, __FUNCTION__)
#else
#define bailout(tokenlist, errorLogger, tok, what) bailoutInternal(tokenlist, errorLogger, tok, what, __FILE__, __LINE__, "(valueFlow)")
#endif

static void changeKnownToPossible(std::list<ValueFlow::Value> &values)
{
    std::list<ValueFlow::Value>::iterator it;
    for (it = values.begin(); it != values.end(); ++it)
        it->changeKnownToPossible();
}

/**
 * Is condition always false when variable has given value?
 * \param condition   top ast token in condition
 * \param programMemory   program memory
 */
static bool conditionIsFalse(const Token *condition, const ProgramMemory &programMemory)
{
    if (!condition)
        return false;
    if (condition->str() == "&&") {
        return conditionIsFalse(condition->astOperand1(), programMemory) ||
               conditionIsFalse(condition->astOperand2(), programMemory);
    }
    ProgramMemory progmem(programMemory);
    MathLib::bigint result = 0;
    bool error = false;
    execute(condition, &progmem, &result, &error);
    return !error && result == 0;
}

/**
 * Is condition always true when variable has given value?
 * \param condition   top ast token in condition
 * \param programMemory   program memory
 */
static bool conditionIsTrue(const Token *condition, const ProgramMemory &programMemory)
{
    if (!condition)
        return false;
    if (condition->str() == "||") {
        return conditionIsTrue(condition->astOperand1(), programMemory) ||
               conditionIsTrue(condition->astOperand2(), programMemory);
    }
    ProgramMemory progmem(programMemory);
    bool error = false;
    MathLib::bigint result = 0;
    execute(condition, &progmem, &result, &error);
    return !error && result == 1;
}

/**
 * Get program memory by looking backwards from given token.
 */
static ProgramMemory getProgramMemory(const Token *tok, unsigned int varid, const ValueFlow::Value &value)
{
    ProgramMemory programMemory;
    programMemory.setValue(varid, value);
    if (value.varId)
        programMemory.setIntValue(value.varId, value.varvalue);
    const ProgramMemory programMemory1(programMemory);
    int indentlevel = 0;
    for (const Token *tok2 = tok; tok2; tok2 = tok2->previous()) {
        if (Token::Match(tok2, "[;{}] %varid% = %var% ;", varid)) {
            const Token *vartok = tok2->tokAt(3);
            programMemory.setValue(vartok->varId(), value);
        } else if (Token::Match(tok2, "[;{}] %var% =") ||
                   Token::Match(tok2, "[;{}] const| %type% %var% (")) {
            const Token *vartok = tok2->next();
            while (vartok->next()->isName())
                vartok = vartok->next();
            if (!programMemory.hasValue(vartok->varId())) {
                MathLib::bigint result = 0;
                bool error = false;
                execute(vartok->next()->astOperand2(), &programMemory, &result, &error);
                if (!error)
                    programMemory.setIntValue(vartok->varId(), result);
            }
        }

        if (tok2->str() == "{") {
            if (indentlevel <= 0)
                break;
            --indentlevel;
        }
        if (tok2->str() == "}") {
            const Token *cond = tok2->link();
            cond = Token::simpleMatch(cond->previous(), ") {") ? cond->linkAt(-1) : nullptr;
            if (cond && conditionIsFalse(cond->astOperand2(), programMemory1))
                tok2 = cond->previous();
            else if (cond && conditionIsTrue(cond->astOperand2(), programMemory1)) {
                ++indentlevel;
                continue;
            } else
                break;
        }
    }
    return programMemory;
}

/**
 * Should value be skipped because it's hidden inside && || or ?: expression.
 * Example: ((x!=NULL) && (*x == 123))
 * If 'valuetok' points at the x in '(*x == 123)'. Then the '&&' will be returned.
 * @param valuetok original variable token
 * @return NULL=>don't skip, non-NULL=>The operator token that cause the skip. For instance the '&&'.
 * */
static const Token * skipValueInConditionalExpression(const Token * const valuetok)
{
    // Walk up the ast
    const Token *prev = valuetok;
    for (const Token *tok = valuetok->astParent(); tok; tok = tok->astParent()) {
        const bool prevIsLhs = (prev == tok->astOperand1());
        prev = tok;

        if (prevIsLhs || !Token::Match(tok, "%oror%|&&|?|:"))
            continue;

        if (tok->hasKnownIntValue())
            return tok;

        // Is variable protected in LHS..
        bool bailout = false;
        visitAstNodes(tok->astOperand1(), [&](const Token *tok2) {
            if (tok2->str() == ".")
                return ChildrenToVisit::none;
            // A variable is seen..
            if (tok2 != valuetok && tok2->variable() &&
                (tok2->varId() == valuetok->varId() || (!tok2->variable()->isArgument() && !tok2->hasKnownIntValue()))) {
                // TODO: limit this bailout
                bailout = true;
                return ChildrenToVisit::done;
            }
            return ChildrenToVisit::op1_and_op2;
        });
        if (bailout)
            return tok;
    }
    return nullptr;
}

static bool isEscapeScope(const Token* tok, TokenList * tokenlist, bool unknown = false)
{
    if (!Token::simpleMatch(tok, "{"))
        return false;
    const Token * termTok = Token::findmatch(tok, "return|continue|break|throw|goto", tok->link());
    if (termTok && termTok->scope() == tok->scope())
        return true;
    std::string unknownFunction;
    if (tokenlist && tokenlist->getSettings()->library.isScopeNoReturn(tok->link(), &unknownFunction))
        return unknownFunction.empty() || unknown;
    return false;
}

static bool bailoutSelfAssignment(const Token * const tok)
{
    const Token *parent = tok;
    while (parent) {
        const Token *op = parent;
        parent = parent->astParent();

        // Assignment where lhs variable exists in rhs => return true
        if (parent                         != nullptr      &&
            parent->astOperand2()          == op           &&
            parent->astOperand1()          != nullptr      &&
            parent->str()                  == "=") {
            for (const Token *lhs = parent->astOperand1(); lhs; lhs = lhs->astOperand1()) {
                if (lhs->varId() == tok->varId())
                    return true;
                if (lhs->astOperand2() && lhs->astOperand2()->varId() == tok->varId())
                    return true;
            }
        }
    }
    return false;
}

static ValueFlow::Value castValue(ValueFlow::Value value, unsigned int bit)
{
    if (value.isFloatValue()) {
        value.valueType = ValueFlow::Value::INT;
        if (value.floatValue >= std::numeric_limits<int>::min() && value.floatValue <= std::numeric_limits<int>::max()) {
            value.intvalue = value.floatValue;
        } else { // don't perform UB
            value.intvalue = 0;
        }
    }
    if (bit < MathLib::bigint_bits) {
        const MathLib::biguint one = 1;
        value.intvalue &= (one << bit) - 1;

        //@todo sign - activate when there is ctrl sign implementaion
        /*if (sign == ValueType::Sign::SIGNED && value.intvalue & (one << (bit - 1)))
        {
            value.intvalue |= ~((one << bit) - 1ULL);
        }*/
    }
    return value;
}

static void combineValueProperties(const ValueFlow::Value &value1, const ValueFlow::Value &value2, ValueFlow::Value *result)
{
    if (value1.isKnown() && value2.isKnown())
        result->setKnown();
    else if (value1.isInconclusive() || value2.isInconclusive())
        result->setInconclusive();
    else
        result->setPossible();
    result->condition = value1.condition ? value1.condition : value2.condition;
    result->varId = (value1.varId != 0U) ? value1.varId : value2.varId;
    result->varvalue = (result->varId == value1.varId) ? value1.varvalue : value2.varvalue;
    result->errorPath = (value1.errorPath.empty() ? value2 : value1).errorPath;
}


static const Token *getCastTypeStartToken(const Token *parent)
{
    // TODO: This might be a generic utility function?
    if (!parent || parent->str() != "(")
        return nullptr;
    if (!parent->astOperand2() && Token::Match(parent,"( %name%"))
        return parent->next();
   
    return nullptr;
}

/** Set token value for cast */
static void setTokenValueCast(Token *parent, const ValueType &valueType, const ValueFlow::Value &value, const Settings *settings);

/** set ValueFlow value and perform calculations if possible */
static void setTokenValue(Token* tok, const ValueFlow::Value &value, const Settings *settings)
{
    if (!tok->addValue(value))
        return;

    // Don't set parent for uninitialized values
    if (value.isUninitValue())
        return;

    Token *parent = const_cast<Token*>(tok->astParent());
    if (!parent)
        return;

    if (value.isLifetimeValue())
    {
        if (value.lifetimeKind == ValueFlow::Value::Iterator)
        {
            setTokenValue(parent, value, settings);
        }
        
        return;
    }

    // cast..
    if (const Token *castType = getCastTypeStartToken(parent)) {
        const ValueType &valueType = ValueType::parseDecl(castType, settings);
        setTokenValueCast(parent, valueType, value, settings);
    }

    else if (parent->str() == ":") {
        setTokenValue(parent,value,settings);
    }

    else if (parent->str() == "?" && tok->str() == ":" && tok == parent->astOperand2() && parent->astOperand1()) {
        // is condition always true/false?
        if (parent->astOperand1()->hasKnownValue()) {
            const ValueFlow::Value &condvalue = parent->astOperand1()->values().front();
            const bool cond(condvalue.isTokValue() || (condvalue.isIntValue() && condvalue.intvalue != 0));
            if (cond && !tok->astOperand1()) { // true condition, no second operator
                setTokenValue(parent, condvalue, settings);
            } else {
                const Token *op = cond ? tok->astOperand1() : tok->astOperand2();
                if (!op) // #7769 segmentation fault at setTokenValue()
                    return;
                const std::list<ValueFlow::Value> &values = op->values();
                if (std::find(values.begin(), values.end(), value) != values.end())
                    setTokenValue(parent, value, settings);
            }
        } else {
            // is condition only depending on 1 variable?
            unsigned int varId = 0;
            bool ret = false;
            visitAstNodes(parent->astOperand1(),
            [&](const Token *t) {
                if (t->varId()) {
                    if (varId > 0 || value.varId != 0U)
                        ret = true;
                    varId = t->varId();
                } else if (t->str() == "(" && Token::Match(t->previous(), "%name%"))
                    ret = true; // function call
                return ret ? ChildrenToVisit::done : ChildrenToVisit::op1_and_op2;
            });
            if (ret)
                return;

            ValueFlow::Value v(value);
            v.conditional = true;
            v.changeKnownToPossible();

            if (varId)
                v.varId = varId;

            setTokenValue(parent, v, settings);
        }
    }

    // Calculations..
    else if ((parent->isArithmeticalOp() || parent->isComparisonOp() || (parent->tokType() == Token::eBitOp) || (parent->tokType() == Token::eLogicalOp)) &&
             parent->astOperand1() &&
             parent->astOperand2()) {

        // known result when a operand is 0.
        if (Token::Match(parent, "[&*]") && value.isKnown() && value.isIntValue() && value.intvalue==0) {
            setTokenValue(parent, value, settings);
            return;
        }

        // known result when a operand is true.
        if (Token::simpleMatch(parent, "&&") && value.isKnown() && value.isIntValue() && value.intvalue==0) {
            setTokenValue(parent, value, settings);
            return;
        }

        // known result when a operand is false.
        if (Token::simpleMatch(parent, "||") && value.isKnown() && value.isIntValue() && value.intvalue!=0) {
            setTokenValue(parent, value, settings);
            return;
        }

        for (const ValueFlow::Value &value1 : parent->astOperand1()->values()) {
            if (!value1.isIntValue() && !value1.isFloatValue() && !value1.isTokValue())
                continue;
            if (value1.isTokValue() && (!parent->isComparisonOp() || value1.tokvalue->tokType() != Token::eString))
                continue;
            for (const ValueFlow::Value &value2 : parent->astOperand2()->values()) {
                if (!value2.isIntValue() && !value2.isFloatValue() && !value2.isTokValue())
                    continue;
                if (value2.isTokValue() && (!parent->isComparisonOp() || value2.tokvalue->tokType() != Token::eString || value1.isTokValue()))
                    continue;
                if (value1.isKnown() || value2.isKnown() || value1.varId == 0U || value2.varId == 0U ||
                    (value1.varId == value2.varId && value1.varvalue == value2.varvalue && value1.isIntValue() &&
                     value2.isIntValue())) {
                    ValueFlow::Value result(0);
                    combineValueProperties(value1, value2, &result);
                    const float floatValue1 = value1.isIntValue() ? value1.intvalue : value1.floatValue;
                    const float floatValue2 = value2.isIntValue() ? value2.intvalue : value2.floatValue;
                    switch (parent->str()[0]) {
                    case '+':
                        if (value1.isTokValue() || value2.isTokValue())
                            break;
                        if (value1.isFloatValue() || value2.isFloatValue()) {
                            result.valueType = ValueFlow::Value::FLOAT;
                            result.floatValue = floatValue1 + floatValue2;
                        } else {
                            result.intvalue = value1.intvalue + value2.intvalue;
                        }
                        setTokenValue(parent, result, settings);
                        break;
                    case '-':
                        if (value1.isTokValue() || value2.isTokValue())
                            break;
                        if (value1.isFloatValue() || value2.isFloatValue()) {
                            result.valueType = ValueFlow::Value::FLOAT;
                            result.floatValue = floatValue1 - floatValue2;
                        } else {
                            result.intvalue = value1.intvalue - value2.intvalue;
                        }
                        setTokenValue(parent, result, settings);
                        break;
                    case '*':
                        if (value1.isTokValue() || value2.isTokValue())
                            break;
                        if (value1.isFloatValue() || value2.isFloatValue()) {
                            result.valueType = ValueFlow::Value::FLOAT;
                            result.floatValue = floatValue1 * floatValue2;
                        } else {
                            result.intvalue = value1.intvalue * value2.intvalue;
                        }
                        setTokenValue(parent, result, settings);
                        break;
                    case '/':
                        if (value1.isTokValue() || value2.isTokValue() || value2.intvalue == 0)
                            break;
                        if (value1.isFloatValue() || value2.isFloatValue()) {
                            result.valueType = ValueFlow::Value::FLOAT;
                            result.floatValue = floatValue1 / floatValue2;
                        } else {
                            result.intvalue = value1.intvalue / value2.intvalue;
                        }
                        setTokenValue(parent, result, settings);
                        break;
                    case '%':
                        if (!value1.isIntValue() || !value2.isIntValue())
                            break;
                        if (value2.intvalue == 0)
                            break;
                        result.intvalue = value1.intvalue % value2.intvalue;
                        setTokenValue(parent, result, settings);
                        break;
                    case '=':
                        if (parent->str() == "==") {
                            if ((value1.isIntValue() && value2.isTokValue()) ||
                                (value1.isTokValue() && value2.isIntValue())) {
                                result.intvalue = 0;
                                setTokenValue(parent, result, settings);
                            } else if (value1.isIntValue() && value2.isIntValue()) {
                                result.intvalue = value1.intvalue == value2.intvalue;
                                setTokenValue(parent, result, settings);
                            }
                        }
                        break;
                    case '!':
                        if (parent->str() == "!=") {
                            if ((value1.isIntValue() && value2.isTokValue()) ||
                                (value1.isTokValue() && value2.isIntValue())) {
                                result.intvalue = 1;
                                setTokenValue(parent, result, settings);
                            } else if (value1.isIntValue() && value2.isIntValue()) {
                                result.intvalue = value1.intvalue != value2.intvalue;
                                setTokenValue(parent, result, settings);
                            }
                        }
                        break;
                    case '>': {
                        const bool f = value1.isFloatValue() || value2.isFloatValue();
                        if (!f && !value1.isIntValue() && !value2.isIntValue())
                            break;
                        if (parent->str() == ">")
                            result.intvalue = f ? (floatValue1 > floatValue2) : (value1.intvalue > value2.intvalue);
                        else if (parent->str() == ">=")
                            result.intvalue = f ? (floatValue1 >= floatValue2) : (value1.intvalue >= value2.intvalue);
                        else if (!f && parent->str() == ">>" && value1.intvalue >= 0 && value2.intvalue >= 0 && value2.intvalue < MathLib::bigint_bits)
                            result.intvalue = value1.intvalue >> value2.intvalue;
                        else
                            break;
                        setTokenValue(parent, result, settings);
                        break;
                    }
                    case '<': {
                        const bool f = value1.isFloatValue() || value2.isFloatValue();
                        if (!f && !value1.isIntValue() && !value2.isIntValue())
                            break;
                        if (parent->str() == "<")
                            result.intvalue = f ? (floatValue1 < floatValue2) : (value1.intvalue < value2.intvalue);
                        else if (parent->str() == "<=")
                            result.intvalue = f ? (floatValue1 <= floatValue2) : (value1.intvalue <= value2.intvalue);
                        else if (!f && parent->str() == "<<" && value1.intvalue >= 0 && value2.intvalue >= 0 && value2.intvalue < MathLib::bigint_bits)
                            result.intvalue = value1.intvalue << value2.intvalue;
                        else
                            break;
                        setTokenValue(parent, result, settings);
                        break;
                    }
                    case '&':
                        if (!value1.isIntValue() || !value2.isIntValue())
                            break;
                        if (parent->str() == "&")
                            result.intvalue = value1.intvalue & value2.intvalue;
                        else
                            result.intvalue = value1.intvalue && value2.intvalue;
                        setTokenValue(parent, result, settings);
                        break;
                    case '|':
                        if (!value1.isIntValue() || !value2.isIntValue())
                            break;
                        if (parent->str() == "|")
                            result.intvalue = value1.intvalue | value2.intvalue;
                        else
                            result.intvalue = value1.intvalue || value2.intvalue;
                        setTokenValue(parent, result, settings);
                        break;
                    case '^':
                        if (!value1.isIntValue() || !value2.isIntValue())
                            break;
                        result.intvalue = value1.intvalue ^ value2.intvalue;
                        setTokenValue(parent, result, settings);
                        break;
                    default:
                        // unhandled operator, do nothing
                        break;
                    }
                }
            }
        }
    }

    // !
    else if (parent->str() == "!") {
        for (const ValueFlow::Value &val : tok->values()) {
            if (!val.isIntValue())
                continue;
            ValueFlow::Value v(val);
            v.intvalue = !v.intvalue;
            setTokenValue(parent, v, settings);
        }
    }

    // ~
    else if (parent->str() == "~") {
        for (const ValueFlow::Value &val : tok->values()) {
            if (!val.isIntValue())
                continue;
            ValueFlow::Value v(val);
            v.intvalue = ~v.intvalue;
            unsigned int bits = 0;

            //@todo sign - activate when there is ctrl sign implementaion &&
            //   tok->valueType()->sign == ValueType::Sign::UNSIGNED
            if (settings &&
                tok->valueType())
            {
                if (tok->valueType()->type == ValueType::Type::INT)
                    bits = settings->int_bit;
                else if (tok->valueType()->type == ValueType::Type::LONG)
                    bits = settings->long_bit;
            }
            if (bits > 0 && bits < MathLib::bigint_bits)
                v.intvalue &= (((MathLib::biguint)1)<<bits) - 1;
            setTokenValue(parent, v, settings);
        }
    }

    // unary minus
    else if (parent->isUnaryOp("-")) {
        for (const ValueFlow::Value &val : tok->values()) {
            if (!val.isIntValue() && !val.isFloatValue())
                continue;
            ValueFlow::Value v(val);
            if (v.isIntValue())
                v.intvalue = -v.intvalue;
            else
                v.floatValue = -v.floatValue;
            setTokenValue(parent, v, settings);
        }
    }

    // Array element
    else if (parent->str() == "[" && parent->isBinaryOp()) {
        for (const ValueFlow::Value &value1 : parent->astOperand1()->values()) {
            if (!value1.isTokValue())
                continue;
            for (const ValueFlow::Value &value2 : parent->astOperand2()->values()) {
                if (!value2.isIntValue())
                    continue;
                if (value1.varId == 0U || value2.varId == 0U ||
                    (value1.varId == value2.varId && value1.varvalue == value2.varvalue)) {
                    ValueFlow::Value result(0);
                    result.condition = value1.condition ? value1.condition : value2.condition;
                    result.setInconclusive(value1.isInconclusive() | value2.isInconclusive());
                    result.varId = (value1.varId != 0U) ? value1.varId : value2.varId;
                    result.varvalue = (result.varId == value1.varId) ? value1.intvalue : value2.intvalue;
                    if (value1.valueKind == value2.valueKind)
                        result.valueKind = value1.valueKind;
                    if (value1.tokvalue->tokType() == Token::eString) {
                        const std::string s = value1.tokvalue->strValue();
                        const MathLib::bigint index = value2.intvalue;
                        if (index == s.size()) {
                            result.intvalue = 0;
                            setTokenValue(parent, result, settings);
                        } else if (index >= 0 && index < s.size()) {
                            result.intvalue = s[index];
                            setTokenValue(parent, result, settings);
                        }
                    } else if (value1.tokvalue->str() == "{") {
                        MathLib::bigint index = value2.intvalue;
                        const Token *element = value1.tokvalue->next();
                        while (index > 0 && element->str() != "}") {
                            if (element->str() == ",")
                                --index;
                            if (Token::Match(element, "[{}()[]]"))
                                break;
                            element = element->next();
                        }
                        if (Token::Match(element, "%num% [,}]")) {
                            result.intvalue = MathLib::toLongNumber(element->str());
                            setTokenValue(parent, result, settings);
                        }
                    }
                }
            }
        }
    }
}

static void setTokenValueCast(Token *parent, const ValueType &valueType, const ValueFlow::Value &value, const Settings *settings)
{
    if (valueType.type == ValueType::Type::CHAR)
        setTokenValue(parent, castValue(value, settings->char_bit), settings);
    else if (valueType.type == ValueType::Type::SHORT)
        setTokenValue(parent, castValue(value, settings->short_bit), settings);
    else if (valueType.type == ValueType::Type::INT)
        setTokenValue(parent, castValue(value, settings->int_bit), settings);
    else if (valueType.type == ValueType::Type::LONG)
        setTokenValue(parent, castValue(value, settings->long_bit), settings);
    else if (value.isIntValue()) {
        const long long charMax = settings->signedCharMax();
        const long long charMin = settings->signedCharMin();
        if (charMin <= value.intvalue && value.intvalue <= charMax) {
            // unknown type, but value is small so there should be no truncation etc
            setTokenValue(parent,value,settings);
        }
    }
}

// Handle various constants..
static Token *valueFlowSetConstantValue(const Token *tok, const Settings *settings)
{
    if ((tok->isNumber() && MathLib::isInt(tok->str())) || (tok->tokType() == Token::eChar))
    {
        ValueFlow::Value value(MathLib::toLongNumber(tok->str()));
        setTokenValue(const_cast<Token *>(tok), value, settings);
    } else if (tok->isNumber() && MathLib::isFloat(tok->str())) {
        ValueFlow::Value value;
        value.valueType = ValueFlow::Value::FLOAT;
        value.floatValue = MathLib::toDoubleNumber(tok->str());
        setTokenValue(const_cast<Token *>(tok), value, settings);
    }
    else if (tok->enumerator() && tok->enumerator()->value_known)
    {
        ValueFlow::Value value(tok->enumerator()->value);
        setTokenValue(const_cast<Token *>(tok), value, settings);
    }
    else if (tok->str() == "NULL" || tok->str() == "nullptr")
    {
        ValueFlow::Value value(0);
        setTokenValue(const_cast<Token *>(tok), value, settings);
    }
    return tok->next();
}


static void valueFlowNumber(TokenList *tokenlist)
{
    for (Token *tok = tokenlist->front(); tok;) {
        tok = valueFlowSetConstantValue(tok, tokenlist->getSettings());
    }

    for (Token *tok = tokenlist->front(); tok; tok = tok->next())
    {
        if (tok->isName() && !tok->varId() && Token::Match(tok, "false|true"))
        {
            ValueFlow::Value value(tok->str() == "true");
            setTokenValue(tok, value, tokenlist->getSettings());
        }
        else if (Token::Match(tok, "[(,] NULL [,)]"))
        {
            // NULL function parameters are not simplified in the
            // normal tokenlist
            ValueFlow::Value value(0);
            setTokenValue(tok->next(), value, tokenlist->getSettings());
        }
    }
}

static void valueFlowString(TokenList *tokenlist)
{
    for (Token *tok = tokenlist->front(); tok; tok = tok->next()) {
        if (tok->tokType() == Token::eString) {
            ValueFlow::Value strvalue;
            strvalue.valueType = ValueFlow::Value::TOK;
            strvalue.tokvalue = tok;
            strvalue.setKnown();
            setTokenValue(tok, strvalue, tokenlist->getSettings());
        }
    }
}

static void valueFlowArray(TokenList *tokenlist)
{
    std::map<unsigned int, const Token *> constantArrays;

    for (Token *tok = tokenlist->front(); tok; tok = tok->next()) {
        if (tok->varId() > 0U) {
            // array
            const std::map<unsigned int, const Token *>::const_iterator it = constantArrays.find(tok->varId());
            if (it != constantArrays.end()) {
                ValueFlow::Value value;
                value.valueType = ValueFlow::Value::TOK;
                value.tokvalue = it->second;
                value.setKnown();
                setTokenValue(tok, value, tokenlist->getSettings());
            }

            // pointer = array
            else if (tok->variable() &&
                     tok->variable()->isArray() &&
                     Token::simpleMatch(tok->astParent(), "=") &&
                     tok == tok->astParent()->astOperand2() &&
                     tok->astParent()->astOperand1() &&
                     tok->astParent()->astOperand1()->variable())            {
                ValueFlow::Value value;
                value.valueType = ValueFlow::Value::TOK;
                value.tokvalue = tok;
                value.setKnown();
                setTokenValue(tok, value, tokenlist->getSettings());
            }
            continue;
        }

        if (Token::Match(tok, "const %type% %var% [ %num%| ] = {")) {
            const Token *vartok = tok->tokAt(2);
            const Token *rhstok = vartok->next()->link()->tokAt(2);
            constantArrays[vartok->varId()] = rhstok;
            tok = rhstok->link();
            continue;
        }

        else if (Token::Match(tok, "const char %var% [ %num%| ] = %str% ;")) {
            const Token *vartok = tok->tokAt(2);
            const Token *strtok = vartok->next()->link()->tokAt(2);
            constantArrays[vartok->varId()] = strtok;
            tok = strtok->next();
            continue;
        }
    }
}

static bool isNonZero(const Token *tok)
{
    return tok && (!tok->hasKnownIntValue() || tok->values().front().intvalue != 0);
}

static const Token *getOtherOperand(const Token *tok)
{
    if (!tok)
        return nullptr;
    if (!tok->astParent())
        return nullptr;
    if (tok->astParent()->astOperand1() != tok)
        return tok->astParent()->astOperand1();
    if (tok->astParent()->astOperand2() != tok)
        return tok->astParent()->astOperand2();
    return nullptr;
}

static void valueFlowArrayBool(TokenList *tokenlist)
{
    for (Token *tok = tokenlist->front(); tok; tok = tok->next()) {
        if (tok->hasKnownIntValue())
            continue;
        const Variable *var = nullptr;
        bool known = false;
        std::list<ValueFlow::Value>::const_iterator val =
            std::find_if(tok->values().begin(), tok->values().end(), std::mem_fn(&ValueFlow::Value::isTokValue));
        if (val == tok->values().end()) {
            var = tok->variable();
            known = true;
        } else {
            var = val->tokvalue->variable();
            known = val->isKnown();
        }
        if (!var)
            continue;
        if (!var->isArray() || var->isArgument())
            continue;
        if (isNonZero(getOtherOperand(tok)) && Token::Match(tok->astParent(), "%comp%"))
            continue;
        // TODO: Check for function argument
        if ((astIsBool(tok->astParent()) && !Token::Match(tok->astParent(), "(|%name%")) ||
            (tok->astParent() && Token::Match(tok->astParent()->previous(), "if|while|for ("))) {
            ValueFlow::Value value{1};
            if (known)
                value.setKnown();
            setTokenValue(tok, value, tokenlist->getSettings());
        }
    }
}

static void valueFlowPointerAlias(TokenList *tokenlist)
{
    for (Token *tok = tokenlist->front(); tok; tok = tok->next()) {
        // not address of
        if (!tok->isUnaryOp("&"))
            continue;

        // parent should be a '='
        if (!Token::simpleMatch(tok->astParent(), "="))
            continue;

        // child should be some buffer or variable
        const Token *vartok = tok->astOperand1();
        while (vartok) {
            if (vartok->str() == "[")
                vartok = vartok->astOperand1();
            else if (vartok->str() == "." || vartok->str() == "::")
                vartok = vartok->astOperand2();
            else
                break;
        }
        if (!(vartok && vartok->variable()))
            continue;

        ValueFlow::Value value;
        value.valueType = ValueFlow::Value::TOK;
        value.tokvalue = tok;
        setTokenValue(tok, value, tokenlist->getSettings());
    }
}

static void valueFlowBitAnd(TokenList *tokenlist)
{
    for (Token *tok = tokenlist->front(); tok; tok = tok->next()) {
        if (tok->str() != "&")
            continue;

        if (tok->hasKnownValue())
            continue;

        if (!tok->astOperand1() || !tok->astOperand2())
            continue;

        MathLib::bigint number;
        if (MathLib::isInt(tok->astOperand1()->str()))
            number = MathLib::toLongNumber(tok->astOperand1()->str());
        else if (MathLib::isInt(tok->astOperand2()->str()))
            number = MathLib::toLongNumber(tok->astOperand2()->str());
        else
            continue;

        int bit = 0;
        while (bit <= (MathLib::bigint_bits - 2) && ((((MathLib::bigint)1) << bit) < number))
            ++bit;

        if ((((MathLib::bigint)1) << bit) == number) {
            setTokenValue(tok, ValueFlow::Value(0), tokenlist->getSettings());
            setTokenValue(tok, ValueFlow::Value(number), tokenlist->getSettings());
        }
    }
}

static void valueFlowSameExpressions(TokenList *tokenlist)
{
    for (Token *tok = tokenlist->front(); tok; tok = tok->next()) {
        if (tok->hasKnownValue())
            continue;

        if (!tok->astOperand1() || !tok->astOperand2())
            continue;

        if (tok->astOperand1()->isLiteral() || tok->astOperand2()->isLiteral())
            continue;

        if (!astIsIntegral(tok->astOperand1(), false) && !astIsIntegral(tok->astOperand2(), false))
            continue;

        ValueFlow::Value val;

        if (Token::Match(tok, "==|>=|<=|/")) {
            val = ValueFlow::Value(1);
            val.setKnown();
        }

        if (Token::Match(tok, "!=|>|<|%|-")) {
            val = ValueFlow::Value(0);
            val.setKnown();
        }

        if (!val.isKnown())
            continue;

        if (isSameExpression(tok->astOperand1(), tok->astOperand2(), tokenlist->getSettings()->library, true, true, &val.errorPath))
        {
            setTokenValue(tok, val, tokenlist->getSettings());
        }
    }
}


static bool getExpressionRange(const Token *expr, MathLib::bigint *minvalue, MathLib::bigint *maxvalue)
{
    if (expr->hasKnownIntValue()) {
        if (minvalue)
            *minvalue = expr->values().front().intvalue;
        if (maxvalue)
            *maxvalue = expr->values().front().intvalue;
        return true;
    }

    if (expr->str() == "&" && expr->astOperand1() && expr->astOperand2()) {
        MathLib::bigint vals[4];
        bool lhsHasKnownRange = getExpressionRange(expr->astOperand1(), &vals[0], &vals[1]);
        bool rhsHasKnownRange = getExpressionRange(expr->astOperand2(), &vals[2], &vals[3]);
        if (!lhsHasKnownRange && !rhsHasKnownRange)
            return false;
        if (!lhsHasKnownRange || !rhsHasKnownRange) {
            if (minvalue)
                *minvalue = lhsHasKnownRange ? vals[0] : vals[2];
            if (maxvalue)
                *maxvalue = lhsHasKnownRange ? vals[1] : vals[3];
        } else {
            if (minvalue)
                *minvalue = vals[0] & vals[2];
            if (maxvalue)
                *maxvalue = vals[1] & vals[3];
        }
        return true;
    }

    if (expr->str() == "%" && expr->astOperand1() && expr->astOperand2()) {
        MathLib::bigint vals[4];
        if (!getExpressionRange(expr->astOperand2(), &vals[2], &vals[3]))
            return false;
        if (vals[2] <= 0)
            return false;
        bool lhsHasKnownRange = getExpressionRange(expr->astOperand1(), &vals[0], &vals[1]);
        if (lhsHasKnownRange && vals[0] < 0)
            return false;
        // If lhs has unknown value, it must be unsigned
        //@todo sign - activate when there is ctrl sign implementaion
        /*if (!lhsHasKnownRange && (!expr->astOperand1()->valueType() || expr->astOperand1()->valueType()->sign != ValueType::Sign::UNSIGNED))
            return false;*/
        if (minvalue)
            *minvalue = 0;
        if (maxvalue)
            *maxvalue = vals[3] - 1;
        return true;
    }

    return false;
}

static void valueFlowRightShift(TokenList *tokenList, const Settings* settings)
{
    for (Token *tok = tokenList->front(); tok; tok = tok->next()) {
        if (tok->str() != ">>")
            continue;

        if (tok->hasKnownValue())
            continue;

        if (!tok->astOperand1() || !tok->astOperand2())
            continue;

        if (!tok->astOperand2()->hasKnownValue())
            continue;

        const MathLib::bigint rhsvalue = tok->astOperand2()->values().front().intvalue;
        if (rhsvalue < 0)
            continue;

        if (!tok->astOperand1()->valueType() || !tok->astOperand1()->valueType()->isIntegral())
            continue;

        if (!tok->astOperand2()->valueType() || !tok->astOperand2()->valueType()->isIntegral())
            continue;

        MathLib::bigint lhsmax=0;
        if (!getExpressionRange(tok->astOperand1(), nullptr, &lhsmax))
            continue;
        if (lhsmax < 0)
            continue;
        int lhsbits;
        if ((tok->astOperand1()->valueType()->type == ValueType::Type::CHAR) ||
            (tok->astOperand1()->valueType()->type == ValueType::Type::SHORT) ||
            (tok->astOperand1()->valueType()->type == ValueType::Type::BOOL) ||
            (tok->astOperand1()->valueType()->type == ValueType::Type::INT))
            lhsbits = settings->int_bit;
        else if (tok->astOperand1()->valueType()->type == ValueType::Type::LONG)
            lhsbits = settings->long_bit;
        /* else if (tok->astOperand1()->valueType()->type == ValueType::Type::LONGLONG)
            lhsbits = settings->long_long_bit;*/
        else
            continue;
        if (rhsvalue >= lhsbits || rhsvalue >= MathLib::bigint_bits || (1ULL << rhsvalue) <= lhsmax)
            continue;

        ValueFlow::Value val(0);
        val.setKnown();
        setTokenValue(tok, val, tokenList->getSettings());
    }
}

static void valueFlowOppositeCondition(SymbolDatabase *symboldatabase, const Settings *settings)
{
    for (const Scope &scope : symboldatabase->scopeList) {
        if (scope.type != Scope::eIf)
            continue;
        Token *tok = const_cast<Token *>(scope.classDef);
        if (!Token::simpleMatch(tok, "if ("))
            continue;
        const Token *cond1 = tok->next()->astOperand2();
        if (!cond1 || !cond1->isComparisonOp())
            continue;
        const bool cpp = false;
        Token *tok2 = tok->linkAt(1);
        while (Token::simpleMatch(tok2, ") {")) {
            tok2 = tok2->linkAt(1);
            if (!Token::simpleMatch(tok2, "} else { if ("))
                break;
            const Token *ifOpenBraceTok = tok2->tokAt(4);
            const Token *cond2 = ifOpenBraceTok->astOperand2();
            if (!cond2 || !cond2->isComparisonOp())
                continue;
            if (isOppositeCond(true, cond1, cond2, settings->library, true, true))
            {
                ValueFlow::Value value(1);
                value.setKnown();
                setTokenValue(const_cast<Token*>(cond2), value, settings);
            }
            tok2 = ifOpenBraceTok->link();
        }
    }
}

static void valueFlowGlobalStaticVar(TokenList *tokenList, const Settings *settings)
{
    // Get variable values...
    std::map<const Variable *, ValueFlow::Value> vars;
    for (const Token *tok = tokenList->front(); tok; tok = tok->next()) {
        if (!tok->variable())
            continue;
        // Initialization...
        if (tok == tok->variable()->nameToken() &&
            tok->variable()->isStatic() &&
            !tok->variable()->isConst() &&
            tok->valueType() &&
            tok->valueType()->isIntegral() &&
            tok->valueType()->constness == 0 &&
            Token::Match(tok, "%name% =") &&
            tok->next()->astOperand2() &&
            tok->next()->astOperand2()->hasKnownIntValue()) {
            vars[tok->variable()] = tok->next()->astOperand2()->values().front();
        } else {
            // If variable is written anywhere in TU then remove it from vars
            if (!tok->astParent())
                continue;
            if (Token::Match(tok->astParent(), "++|--|&") && !tok->astParent()->astOperand2())
                vars.erase(tok->variable());
            else if (tok->astParent()->isAssignmentOp()) {
                if (tok == tok->astParent()->astOperand1())
                    vars.erase(tok->variable());
                else if (Token::Match(tok->astParent()->tokAt(-2), "& %name% ="))
                    vars.erase(tok->variable());
            }            else if (isLikelyStreamRead(tok->astParent()))            {
                vars.erase(tok->variable());
            } else if (Token::Match(tok->astParent(), "[(,]"))
                vars.erase(tok->variable());
        }
    }

    // Set values..
    for (Token *tok = tokenList->front(); tok; tok = tok->next()) {
        if (!tok->variable())
            continue;
        std::map<const Variable *, ValueFlow::Value>::const_iterator var = vars.find(tok->variable());
        if (var == vars.end())
            continue;
        setTokenValue(tok, var->second, settings);
    }
}

static void valueFlowReverse(TokenList *tokenlist,
                             Token *tok,
                             const Token * const varToken,
                             ValueFlow::Value val,
                             ValueFlow::Value val2,
                             ErrorLogger *errorLogger,
                             const Settings *settings)
{
    const MathLib::bigint    num        = val.intvalue;
    const Variable * const   var        = varToken->variable();
    if (!var)
        return;

    const unsigned int       varid      = varToken->varId();
    const Token * const      startToken = var->nameToken();

    for (Token *tok2 = tok->previous(); ; tok2 = tok2->previous()) {
        if (!tok2 ||
            tok2 == startToken ||
            (tok2->str() == "{" && tok2->scope()->type == Scope::ScopeType::eFunction)) {
            break;
        }

        if (tok2->varId() == varid) {
            // bailout: assignment
            if (Token::Match(tok2->previous(), "!!* %name% =")) {
                if (settings->debugwarnings)
                    bailout(tokenlist, errorLogger, tok2, "assignment of " + tok2->str());
                break;
            }

            // increment/decrement
            int inc = 0;
            if (Token::Match(tok2->previous(), "[;{}] %name% ++|-- ;"))
                inc = (tok2->strAt(1)=="++") ? -1 : 1;
            else if (Token::Match(tok2->tokAt(-2), "[;{}] ++|-- %name% ;"))
                inc = (tok2->strAt(-1)=="++") ? -1 : 1;
            else if (Token::Match(tok2->previous(), "++|-- %name%") || Token::Match(tok2, "%name% ++|--")) {
                if (settings->debugwarnings)
                    bailout(tokenlist, errorLogger, tok2, "increment/decrement of " + tok2->str());
                break;
            }
            if (inc != 0) {
                val.intvalue += inc;
                const std::string info(tok2->str() + " is " + std::string(inc==1 ? "decremented" : "incremented") + ", before this " + (inc==1?"decrement":"increment") + " the value is " + val.infoString());
                val.errorPath.emplace_back(tok2, info);
            }

            // compound assignment
            if (Token::Match(tok2->previous(), "[;{}] %var% %assign%") && tok2->next()->str() != "=") {
                const Token * const assignToken = tok2->next();
                const Token * const rhsToken = assignToken->astOperand2();
                if (!rhsToken || !rhsToken->hasKnownIntValue()) {
                    if (settings->debugwarnings)
                        bailout(tokenlist, errorLogger, tok2, "compound assignment, rhs value is not known");
                    break;
                }
                const MathLib::bigint rhsValue =  rhsToken->values().front().intvalue;
                if (assignToken->str() == "+=")
                    val.intvalue -= rhsValue;
                else if (assignToken->str() == "-=")
                    val.intvalue += rhsValue;
                else if (assignToken->str() == "*=" && rhsValue != 0)
                    val.intvalue /= rhsValue;
                else {
                    if (settings->debugwarnings)
                        bailout(tokenlist, errorLogger, tok2, "compound assignment " + tok2->str());
                    break;
                }

                const std::string info("Compound assignment '" + assignToken->str() + "', before assignment value is " + val.infoString());
                val.errorPath.emplace_back(tok2, info);
            }

            // bailout: variable is used in rhs in assignment to itself
            if (bailoutSelfAssignment(tok2)) {
                if (settings->debugwarnings)
                    bailout(tokenlist, errorLogger, tok2, "variable " + tok2->str() + " is used in rhs in assignment to itself");
                break;
            }

            // assigned by subfunction?
            bool inconclusive = false;
            if (isVariableChangedByFunctionCall(tok2, settings, &inconclusive)) {
                if (settings->debugwarnings)
                    bailout(tokenlist, errorLogger, tok2, "possible assignment of " + tok2->str() + " by subfunction");
                break;
            }
            val.setInconclusive(inconclusive);
            val2.setInconclusive(inconclusive);

            // skip if variable is conditionally used in ?: expression
            if (const Token *parent = skipValueInConditionalExpression(tok2)) {
                if (settings->debugwarnings)
                    bailout(tokenlist,
                            errorLogger,
                            tok2,
                            "no simplification of " + tok2->str() + " within " + (Token::Match(parent,"[?:]") ? "?:" : parent->str()) + " expression");
                continue;
            }

            // do-while condition, break in the loop body
            {
                const Token *parent = tok2->astParent();
                while (parent && !Token::simpleMatch(parent->previous(), "while ("))
                    parent = parent->astParent();
                if (parent && Token::simpleMatch(parent->tokAt(-2), "} while (") && Token::simpleMatch(parent->linkAt(-2)->previous(), "do {")) {
                    bool breakBailout = false;
                    for (const Token *iftok = parent->linkAt(-2); iftok != parent; iftok = iftok->next()) {
                        if (!Token::simpleMatch(iftok, "if ("))
                            continue;
                        if (!Token::simpleMatch(iftok->linkAt(1), ") { break"))
                            continue;
                        ProgramMemory programMemory;
                        programMemory.setIntValue(varid, num);
                        if (conditionIsTrue(iftok->next()->astOperand2(), programMemory)) {
                            breakBailout = true;
                            break;
                        }
                    }
                    if (breakBailout) {
                        if (settings->debugwarnings)
                            bailout(tokenlist,
                                    errorLogger,
                                    tok2,
                                    "no simplification of " + tok2->str() + " in do-while condition since there is a break in the loop body");
                        break;
                    }
                }
            }

            setTokenValue(tok2, val, settings);
            if (val2.condition)
                setTokenValue(tok2,val2, settings);
            if (tok2 == var->nameToken())
                break;
        }

        // goto label
        if (Token::Match(tok2, "[;{}] %name% :")) {
            if (settings->debugwarnings)
                bailout(tokenlist, errorLogger, tok2->next(), "variable " + var->name() + " stopping on goto label");
            break;
        }

        if (tok2->str() == "}") {
            const Token *vartok = Token::findmatch(tok2->link(), "%varid%", tok2, varid);
            while (Token::Match(vartok, "%name% = %num% ;") && !vartok->tokAt(2)->getValue(num))
                vartok = Token::findmatch(vartok->next(), "%varid%", tok2, varid);
            if (vartok) {
                if (settings->debugwarnings) {
                    std::string errmsg = "variable ";
                    errmsg += var->name() + " ";
                    errmsg += "stopping on }";
                    bailout(tokenlist, errorLogger, tok2, errmsg);
                }
                break;
            } else {
                tok2 = tok2->link();
            }
        } else if (tok2->str() == "{") {
            // if variable is assigned in loop don't look before the loop
            if (tok2->previous() &&
                (Token::simpleMatch(tok2->previous(), "do") ||
                 (tok2->strAt(-1) == ")" && Token::Match(tok2->linkAt(-1)->previous(), "for|while (")))) {

                const Token *start = tok2;
                const Token *end   = start->link();
                if (isVariableChanged(start, end, varid, var->isGlobal(), settings))                {
                    if (settings->debugwarnings)
                        bailout(tokenlist, errorLogger, tok2, "variable " + var->name() + " is assigned in loop. so valueflow analysis bailout when start of loop is reached.");
                    break;
                }
            }

            // Global variable : stop when leaving the function scope
            if (!var->isLocal()) {
                if (!Token::Match(tok2->previous(), ")|else|do {"))
                    break;
                if ((tok2->previous()->str() == ")") &&
                    !Token::Match(tok2->linkAt(-1)->previous(), "if|for|while ("))
                    break;
            }
        } else if (tok2->str() == ";") {
            const Token *parent = tok2->previous();
            while (parent && !Token::Match(parent, "return|break|continue|goto"))
                parent = parent->astParent();
            // reaching a break/continue/return
            if (parent) {
                if (settings->debugwarnings)
                    bailout(tokenlist, errorLogger, tok2, "variable " + var->name() + " stopping on " + parent->str());
                break;
            }
        }

        if (Token::Match(tok2, "%name% (") && !Token::simpleMatch(tok2->linkAt(1), ") {")) {
            // bailout: global non-const variables
            if (!(var->isLocal() || var->isArgument()) && !var->isConst()) {
                if (settings->debugwarnings)
                    bailout(tokenlist, errorLogger, tok, "global variable " + var->name());
                return;
            }
        }
    }
}

static void valueFlowBeforeCondition(TokenList *tokenlist, SymbolDatabase *symboldatabase, ErrorLogger *errorLogger, const Settings *settings)
{
    for (const Scope * scope : symboldatabase->functionScopes) {
        for (Token* tok = const_cast<Token*>(scope->bodyStart); tok != scope->bodyEnd; tok = tok->next()) {
            MathLib::bigint num = 0;
            const Token *vartok = nullptr;
            if (tok->isComparisonOp() && tok->astOperand1() && tok->astOperand2()) {
                if (tok->astOperand1()->isName() && tok->astOperand2()->hasKnownIntValue()) {
                    vartok = tok->astOperand1();
                    num = tok->astOperand2()->values().front().intvalue;
                } else if (tok->astOperand1()->hasKnownIntValue() && tok->astOperand2()->isName()) {
                    vartok = tok->astOperand2();
                    num = tok->astOperand1()->values().front().intvalue;
                } else {
                    continue;
                }
            } else if (Token::Match(tok->previous(), "if|while ( %name% %oror%|&&|)") ||
                       Token::Match(tok, "%oror%|&& %name% %oror%|&&|)")) {
                vartok = tok->next();
                num = 0;
            } else if (Token::Match(tok, "[!?]") && Token::Match(tok->astOperand1(), "%name%")) {
                vartok = tok->astOperand1();
                num = 0;
            } else {
                continue;
            }

            unsigned int varid = vartok->varId();
            const Variable * const var = vartok->variable();

            if (varid == 0U || !var)
                continue;

            // bailout: for/while-condition, variable is changed in while loop
            for (const Token *tok2 = tok; tok2; tok2 = tok2->astParent()) {
                if (tok2->astParent() || tok2->str() != "(" || !Token::simpleMatch(tok2->link(), ") {"))
                    continue;

                // Variable changed in 3rd for-expression
                if (Token::simpleMatch(tok2->previous(), "for (")) {
                    if (tok2->astOperand2() && tok2->astOperand2()->astOperand2() && isVariableChanged(tok2->astOperand2()->astOperand2(), tok2->link(), varid, var->isGlobal(), settings))                    {
                        varid = 0U;
                        if (settings->debugwarnings)
                            bailout(tokenlist, errorLogger, tok, "variable " + var->name() + " used in loop");
                    }
                }

                // Variable changed in loop code
                if (Token::Match(tok2->previous(), "for|while (")) {
                    const Token * const start = tok2->link()->next();
                    const Token * const end   = start->link();

                    if (isVariableChanged(start, end, varid, var->isGlobal(), settings))                    {
                        varid = 0U;
                        if (settings->debugwarnings)
                            bailout(tokenlist, errorLogger, tok, "variable " + var->name() + " used in loop");
                    }
                }
            }
            if (varid == 0U)
                continue;

            // extra logic for unsigned variables 'i>=1' => possible value can also be 0
            if (Token::Match(tok, "<|>"))            {
                if (num != 0)
                    continue;
                if (!var->typeStartToken()->isUnsigned())
                    continue;
            }
            ValueFlow::Value val(tok, num);
            val.varId = varid;
            ValueFlow::Value val2;
            if (num==1U && Token::Match(tok,"<=|>=")) {
                if (var->typeStartToken()->isUnsigned()) {
                    val2 = ValueFlow::Value(tok,0);
                    val2.varId = varid;
                }
            }
            valueFlowReverse(tokenlist,
                             tok,
                             vartok,
                             val,
                             val2,
                             errorLogger,
                             settings);

        }
    }
}

static void removeValues(std::list<ValueFlow::Value> &values, const std::list<ValueFlow::Value> &valuesToRemove)
{
    for (std::list<ValueFlow::Value>::iterator it = values.begin(); it != values.end();) {
        bool found = false;
        for (const ValueFlow::Value &v2 : valuesToRemove) {
            if (it->intvalue == v2.intvalue) {
                found = true;
                break;
            }
        }
        if (found)
            values.erase(it++);
        else
            ++it;
    }
}

static void valueFlowAST(Token *tok, unsigned int varid, const ValueFlow::Value &value, const Settings *settings)
{
    if (!tok)
        return;
    if (tok->varId() == varid)
        setTokenValue(tok, value, settings);
    valueFlowAST(const_cast<Token*>(tok->astOperand1()), varid, value, settings);
    if (tok->str() == "&&" && tok->astOperand1() && tok->astOperand1()->getValue(0)) {
        ProgramMemory pm;
        pm.setValue(varid,value);
        if (conditionIsFalse(tok->astOperand1(), pm))
            return;
    } else if (tok->str() == "||" && tok->astOperand1()) {
        const std::list<ValueFlow::Value> &values = tok->astOperand1()->values();
        bool nonzero = false;
        for (const ValueFlow::Value &v : values) {
            if (v.intvalue != 0) {
                nonzero = true;
                break;
            }
        }
        if (!nonzero)
            return;
        ProgramMemory pm;
        pm.setValue(varid,value);
        if (conditionIsTrue(tok->astOperand1(), pm))
            return;
    }
    valueFlowAST(const_cast<Token*>(tok->astOperand2()), varid, value, settings);
}

/** if known variable is changed in loop body, change it to a possible value */
static void handleKnownValuesInLoop(const Token                 *startToken,
                                    const Token                 *endToken,
                                    std::list<ValueFlow::Value> *values,
                                    unsigned int                varid,
                                    bool                        globalvar,
                                    const Settings              *settings)
{
    bool isChanged = false;
    for (std::list<ValueFlow::Value>::iterator it = values->begin(); it != values->end(); ++it) {
        if (it->isKnown()) {
            if (!isChanged) {
                if (!isVariableChanged(startToken, endToken, varid, globalvar, settings))
                    break;
                isChanged = true;
            }

            it->setPossible();
        }
    }
}

static bool evalAssignment(ValueFlow::Value &lhsValue, const std::string &assign, const ValueFlow::Value &rhsValue)
{
    if (lhsValue.isIntValue()) {
        if (assign == "+=")
            lhsValue.intvalue += rhsValue.intvalue;
        else if (assign == "-=")
            lhsValue.intvalue -= rhsValue.intvalue;
        else if (assign == "*=")
            lhsValue.intvalue *= rhsValue.intvalue;
        else if (assign == "/=") {
            if (rhsValue.intvalue == 0)
                return false;
            else
                lhsValue.intvalue /= rhsValue.intvalue;
        } else if (assign == "%=") {
            if (rhsValue.intvalue == 0)
                return false;
            else
                lhsValue.intvalue %= rhsValue.intvalue;
        } else if (assign == "&=")
            lhsValue.intvalue &= rhsValue.intvalue;
        else if (assign == "|=")
            lhsValue.intvalue |= rhsValue.intvalue;
        else if (assign == "^=")
            lhsValue.intvalue ^= rhsValue.intvalue;
        else
            return false;
    } else if (lhsValue.isFloatValue()) {
        if (assign == "+=")
            lhsValue.floatValue += rhsValue.intvalue;
        else if (assign == "-=")
            lhsValue.floatValue -= rhsValue.intvalue;
        else if (assign == "*=")
            lhsValue.floatValue *= rhsValue.intvalue;
        else if (assign == "/=")
            lhsValue.floatValue /= rhsValue.intvalue;
        else
            return false;
    } else {
        return false;
    }
    return true;
}

static bool valueFlowForward(Token * const               startToken,
                             const Token * const         endToken,
                             const Variable * const      var,
                             const unsigned int          varid,
                             std::list<ValueFlow::Value> values,
                             const bool                  constValue,
                             const bool                  subFunction,
                             TokenList * const           tokenlist,
                             ErrorLogger * const         errorLogger,
                             const Settings * const      settings)
{
    int indentlevel = 0;
    unsigned int number_of_if = 0;
    int varusagelevel = -1;
    bool returnStatement = false;  // current statement is a return, stop analysis at the ";"
    bool read = false;  // is variable value read?

    if (values.empty())
        return true;

    for (Token *tok2 = startToken; tok2 && tok2 != endToken; tok2 = tok2->next()) {
        if (indentlevel >= 0 && tok2->str() == "{")
            ++indentlevel;
        else if (indentlevel >= 0 && tok2->str() == "}") {
            --indentlevel;
            if (indentlevel <= 0 && isReturnScope(tok2) && Token::Match(tok2->link()->previous(), "else|) {")) {
                const Token *condition = tok2->link();
                const bool iselse = Token::simpleMatch(condition->tokAt(-2), "} else {");
                if (iselse)
                    condition = condition->linkAt(-2);
                if (condition && Token::simpleMatch(condition->previous(), ") {"))
                    condition = condition->linkAt(-1)->astOperand2();
                else
                    condition = nullptr;
                if (!condition) {
                    if (settings->debugwarnings)
                        bailout(tokenlist, errorLogger, tok2, "variable " + var->name() + " valueFlowForward, bailing out since it's unknown if conditional return is executed");
                    return false;
                }

                bool bailoutflag = false;
                const Token * const start1 = iselse ? tok2->link()->linkAt(-2) : nullptr;
                for (std::list<ValueFlow::Value>::iterator it = values.begin(); it != values.end();) {
                    if (!iselse && conditionIsTrue(condition, getProgramMemory(condition->astParent(), varid, *it))) {
                        bailoutflag = true;
                        break;
                    }
                    if (iselse && conditionIsFalse(condition, getProgramMemory(condition->astParent(), varid, *it))) {
                        bailoutflag = true;
                        break;
                    }
                    if (iselse && it->isPossible() && isVariableChanged(start1, start1->link(), varid, var->isGlobal(), settings))          
					    values.erase(it++);
                    else
                        ++it;
                }
                if (bailoutflag) {
                    if (settings->debugwarnings)
                        bailout(tokenlist, errorLogger, tok2, "variable " + var->name() + " valueFlowForward, conditional return is assumed to be executed");
                    return false;
                }

                if (values.empty())
                    return true;
            } else if (indentlevel <= 0 &&
                       Token::simpleMatch(tok2->link()->previous(), "else {") &&
                       !isReturnScope(tok2->link()->tokAt(-2)) &&
                     isVariableChanged(tok2->link(), tok2, varid, var->isGlobal(), settings))            {
                changeKnownToPossible(values);
            }
        }

        if (Token::Match(tok2, "[;{}] %name% :") || tok2->str() == "case")
        {
            changeKnownToPossible(values);
            tok2 = tok2->tokAt(2);
            continue;
        }

        else if ((var->isGlobal() || tok2->str() == "asm") && Token::Match(tok2, "%name% (") && Token::Match(tok2->linkAt(1), ") !!{")) {
            return false;
        }

        else if (Token::simpleMatch(tok2, "else {"))
        {
            // Should scope be skipped because variable value is checked?
            bool skipelse = false;
            const Token *condition = tok2->linkAt(-1);
            condition = condition ? condition->linkAt(-1) : nullptr;
            condition = condition ? condition->astOperand2() : nullptr;
            for (const ValueFlow::Value &v : values) {
                if (conditionIsTrue(condition, getProgramMemory(tok2, varid, v))) {
                    skipelse = true;
                    break;
                }
            }
            if (skipelse) {
                tok2 = tok2->linkAt(1);
                continue;
            }
        }

        else if (Token::simpleMatch(tok2, "do {")) {
            const Token *start = tok2->next();
            const Token *end   = start->link();
            if (Token::simpleMatch(end, "} while ("))
                end = end->linkAt(2);

            if (isVariableChanged(start, end, varid, var->isGlobal(), settings))
            {
                if (settings->debugwarnings)
                    bailout(tokenlist, errorLogger, tok2, "variable " + var->name() + " valueFlowForward, assignment in do-while");
                return false;
            }

            handleKnownValuesInLoop(start, end, &values, varid, var->isGlobal(), settings);
        }

        // conditional block of code that assigns variable..
        else if (!tok2->varId() && Token::Match(tok2, "%name% (") && Token::simpleMatch(tok2->linkAt(1), ") {")) {
            // is variable changed in condition?
            if (isVariableChanged(tok2->next(), tok2->next()->link(), varid, var->isGlobal(), settings))
            {
                if (settings->debugwarnings)
                    bailout(tokenlist, errorLogger, tok2, "variable " + var->name() + " valueFlowForward, assignment in condition");
                return false;
            }

            // if known variable is changed in loop body, change it to a possible value..
            if (Token::Match(tok2, "for|while"))
                handleKnownValuesInLoop(tok2, tok2->linkAt(1)->linkAt(1), &values, varid, var->isGlobal(), settings);

            // Set values in condition
            for (Token* tok3 = tok2->tokAt(2); tok3 != tok2->next()->link(); tok3 = tok3->next()) {
                if (tok3->varId() == varid) {
                    for (const ValueFlow::Value &v : values)
                        setTokenValue(tok3, v, settings);
                } else if (Token::Match(tok3, "%oror%|&&|?|;")) {
                    break;
                }
            }

            const Token * const condTok = tok2->next()->astOperand2();
            const bool condAlwaysTrue = (condTok && condTok->hasKnownIntValue() && condTok->values().front().intvalue != 0);
            const bool condAlwaysFalse = (condTok && condTok->hasKnownIntValue() && condTok->values().front().intvalue == 0);

            // Should scope be skipped because variable value is checked?
            std::list<ValueFlow::Value> truevalues;
            std::list<ValueFlow::Value> falsevalues;
            for (const ValueFlow::Value &v : values)            {
                if (condAlwaysTrue)                {
                    truevalues.push_back(v);
                    continue;
                }
                if (condAlwaysFalse)                {
                    falsevalues.push_back(v);
                    continue;
                }
                const ProgramMemory &programMemory = getProgramMemory(tok2, varid, v);
                if (subFunction && conditionIsTrue(condTok, programMemory))
                    truevalues.push_back(v);
                else if (!subFunction && !conditionIsFalse(condTok, programMemory))
                    truevalues.push_back(v);
                if (condAlwaysFalse)
                    falsevalues.push_back(v);
                else if (conditionIsFalse(condTok, programMemory))
                    falsevalues.push_back(v);
                else if (!subFunction && !conditionIsTrue(condTok, programMemory))
                    falsevalues.push_back(v);
            }
            if (truevalues.size() != values.size() || condAlwaysTrue) {
                // '{'
                const Token * const startToken1 = tok2->linkAt(1)->next();

                bool vfresult = valueFlowForward(startToken1->next(),
                                                 startToken1->link(),
                                                 var,
                                                 varid,
                                                 truevalues,
                                                 constValue,
                                                 subFunction,
                                                 tokenlist,
                                                 errorLogger,
                                                 settings);

                if (!condAlwaysFalse && isVariableChanged(startToken1, startToken1->link(), varid, var->isGlobal(), settings))  
				{
                    removeValues(values, truevalues);
                    changeKnownToPossible(values);
                }

                // goto '}'
                tok2 = startToken1->link();

                if (isReturnScope(tok2) || !vfresult) {
                    if (condAlwaysTrue)
                        return false;
                    removeValues(values, truevalues);
                }

                if (Token::simpleMatch(tok2, "} else {")) {
                    const Token * const startTokenElse = tok2->tokAt(2);

                    vfresult = valueFlowForward(startTokenElse->next(),
                                                startTokenElse->link(),
                                                var,
                                                varid,
                                                falsevalues,
                                                constValue,
                                                subFunction,
                                                tokenlist,
                                                errorLogger,
                                                settings);

                    if (!condAlwaysTrue && isVariableChanged(startTokenElse, startTokenElse->link(), varid, var->isGlobal(), settings))
                    {
                        removeValues(values, falsevalues);
                        changeKnownToPossible(values);
                    }

                    // goto '}'
                    tok2 = startTokenElse->link();

                    if (isReturnScope(tok2) || !vfresult) {
                        if (condAlwaysFalse)
                            return false;
                        removeValues(values, falsevalues);
                    }
                }

                continue;
            }

            Token * const start = tok2->linkAt(1)->next();
            Token * const end   = start->link();
            const bool varusage = (indentlevel >= 0 && constValue && number_of_if == 0U) ? isVariableChanged(start, end, varid, var->isGlobal(), settings) : (nullptr != Token::findmatch(start, "%varid%", end, varid));
            if (!read) {
                read = bool(nullptr != Token::findmatch(tok2, "%varid% !!=", end, varid));
            }
            if (varusage) {
                varusagelevel = indentlevel;

                if (indentlevel < 0 && tok2->str() == "switch")
                    return false;

                // TODO: don't check noreturn scopes
                if (read && (number_of_if > 0U || Token::findmatch(tok2, "%varid%", start, varid))) {
                    // Set values in condition
                    const Token * const condend = tok2->linkAt(1);
                    for (Token *condtok = tok2; condtok != condend; condtok = condtok->next()) {
                        if (condtok->varId() == varid) {
                            for (const ValueFlow::Value &v : values)
                                setTokenValue(condtok, v, settings);
                        }
                        if (Token::Match(condtok, "%oror%|&&|?|;"))
                            break;
                    }
                    if (settings->debugwarnings)
                        bailout(tokenlist, errorLogger, tok2, "variable " + var->name() + " is assigned in conditional code");
                    return false;
                }

                if (var->isStatic()) {
                    if (settings->debugwarnings)
                        bailout(tokenlist, errorLogger, tok2, "variable " + var->name() + " bailout when conditional code that contains var is seen");
                    return false;
                }

                // Forward known values in the else branch
                if (Token::simpleMatch(end, "} else {")) {
                    std::list<ValueFlow::Value> knownValues;
                    std::copy_if(values.begin(), values.end(), std::back_inserter(knownValues), std::mem_fn(&ValueFlow::Value::isKnown));
                    valueFlowForward(end->tokAt(2),
                                     end->linkAt(2),
                                     var,
                                     varid,
                                     knownValues,
                                     constValue,
                                     subFunction,
                                     tokenlist,
                                     errorLogger,
                                     settings);
                }

                // Remove conditional values
                std::list<ValueFlow::Value>::iterator it;
                for (it = values.begin(); it != values.end();) {
                    if (it->condition || it->conditional)
                        values.erase(it++);
                    else {
                        it->changeKnownToPossible();
                        ++it;
                    }
                }
            }

            // stop after conditional return scopes that are executed
            if (isReturnScope(end)) {
                std::list<ValueFlow::Value>::iterator it;
                for (it = values.begin(); it != values.end();) {
                    if (conditionIsTrue(tok2->next()->astOperand2(), getProgramMemory(tok2, varid, *it)))
                        values.erase(it++);
                    else
                        ++it;
                }
                if (values.empty())
                    return false;
            }

            // noreturn scopes..
            if ((number_of_if > 0 || Token::findmatch(tok2, "%varid%", start, varid)) &&
                (isEscapeScope(start, tokenlist) ||
                 (Token::simpleMatch(end,"} else {") && isEscapeScope(end->tokAt(2), tokenlist)))) {
                if (settings->debugwarnings)
                    bailout(tokenlist, errorLogger, tok2, "variable " + var->name() + ". noreturn conditional scope.");
                return false;
            }

            if (isVariableChanged(start, end, varid, var->isGlobal(), settings))            {
                if ((!read || number_of_if == 0) &&
                    Token::simpleMatch(tok2, "if (") &&
                    !(Token::simpleMatch(end, "} else {") &&
                      isEscapeScope(end->tokAt(2), tokenlist))) {
                    ++number_of_if;
                    tok2 = end;
                } else {
                    // loop that conditionally set variable and then break => either loop condition is
                    // redundant or the variable can be unchanged after the loop.
                    bool loopCondition = false;
                    if (Token::simpleMatch(tok2, "while (") && Token::Match(tok2->next()->astOperand2(), "%op%"))
                        loopCondition = true;
                    else if (Token::simpleMatch(tok2, "for (") &&
                             Token::simpleMatch(tok2->next()->astOperand2(), ";") &&
                             Token::simpleMatch(tok2->next()->astOperand2()->astOperand2(), ";") &&
                             Token::Match(tok2->next()->astOperand2()->astOperand2()->astOperand1(), "%op%"))
                        loopCondition = true;

                    bool bail = true;
                    if (loopCondition) {
                        const Token *tok3 = Token::findmatch(start, "%varid%", end, varid);
                        if (Token::Match(tok3, "%varid% =", varid) &&
                            tok3->scope()->bodyEnd                &&
                            Token::Match(tok3->scope()->bodyEnd->tokAt(-3), "[;}] break ;") &&
                            !Token::findmatch(tok3->next(), "%varid%", end, varid)) {
                            bail = false;
                            tok2 = end;
                        }
                    }

                    if (bail) {
                        if (settings->debugwarnings)
                            bailout(tokenlist, errorLogger, tok2, "variable " + var->name() + " is assigned in conditional code");
                        return false;
                    }
                }
            }
        }


        else if (tok2->str() == "}" && indentlevel == varusagelevel) {
            ++number_of_if;

            // Set "conditional" flag for all values
            std::list<ValueFlow::Value>::iterator it;
            for (it = values.begin(); it != values.end(); ++it) {
                it->conditional = true;
                it->changeKnownToPossible();
            }

            if (Token::simpleMatch(tok2,"} else {"))
                tok2 = tok2->linkAt(2);
        }

        else if (Token::Match(tok2, "break|continue|goto")) {
            const Scope *scope = tok2->scope();
            if (indentlevel > 0) {
                const Token *tok3 = tok2->tokAt(2);
                int indentlevel2 = indentlevel;
                while (indentlevel2 > 0 &&
                       tok3->str() == "}" &&
                       Token::Match(tok3->link()->previous(), "!!)")) {
                    indentlevel2--;
                    tok3 = tok3->next();
                    if (tok3 && tok3->str() == ";")
                        tok3 = tok3->next();
                }
                if (indentlevel2 > 0)
                    continue;
                scope = tok3->scope();
                indentlevel = 0;
            }
            if (tok2->str() == "break") {
                if (scope && scope->type == Scope::eSwitch) {
                    tok2 = const_cast<Token *>(scope->bodyEnd);
                    if (tok2 == endToken)
                        break;
                    --indentlevel;
                    changeKnownToPossible(values);
                    continue;
                }
            }
            if (settings->debugwarnings)
                bailout(tokenlist, errorLogger, tok2, "variable " + var->name() + ". noreturn conditional scope.");
            return false;
        }

        else if (indentlevel <= 0 && Token::Match(tok2, "return|throw"))
            returnStatement = true;

        else if (returnStatement && tok2->str() == ";")
            return false;

        // If a ? is seen and it's known that the condition is true/false..
        else if (tok2->str() == "?") {
            const Token *condition = tok2->astOperand1();
            const Token *op2 = tok2->astOperand2();
            if (!condition || !op2) // Ticket #6713
                continue;

            if (condition->hasKnownIntValue()) {
                const ValueFlow::Value &condValue = condition->values().front();
                const Token *expr = (condValue.intvalue != 0) ? op2->astOperand1() : op2->astOperand2();
                for (const ValueFlow::Value &v : values)
                    valueFlowAST(const_cast<Token*>(expr), varid, v, settings);
                if (isVariableChangedByFunctionCall(expr, varid, settings, nullptr))
                    changeKnownToPossible(values);
            } else {
                for (const ValueFlow::Value &v : values) {
                    const ProgramMemory programMemory(getProgramMemory(tok2, varid, v));
                    if (conditionIsTrue(condition, programMemory))
                        valueFlowAST(const_cast<Token*>(op2->astOperand1()), varid, v, settings);
                    else if (conditionIsFalse(condition, programMemory))
                        valueFlowAST(const_cast<Token*>(op2->astOperand2()), varid, v, settings);
                    else
                        valueFlowAST(const_cast<Token*>(op2), varid, v, settings);
                }

                const Token * const expr0 = op2->astOperand1() ? op2->astOperand1() : tok2->astOperand1();
                const Token * const expr1 = op2->astOperand2();

                const std::pair<const Token *, const Token *> startEnd0 = expr0->findExpressionStartEndTokens();
                const std::pair<const Token *, const Token *> startEnd1 = expr1->findExpressionStartEndTokens();
                const bool changed0 = isVariableChanged(startEnd0.first, startEnd0.second->next(), varid, var->isGlobal(), settings);
                const bool changed1 = isVariableChanged(startEnd1.first, startEnd1.second->next(), varid, var->isGlobal(), settings);

                if (changed0 && changed1) {
                    if (settings->debugwarnings)
                        bailout(tokenlist, errorLogger, tok2, "variable " + var->name() + " valueFlowForward, changed in both : expressions");
                    return false;
                }

                if (changed0 || changed1)
                    changeKnownToPossible(values);
            }

            // Skip conditional expressions..
            const Token * const questionToken = tok2;
            while (tok2->astOperand1() || tok2->astOperand2()) {
                if (tok2->astOperand2())
                    tok2 = const_cast<Token*>(tok2->astOperand2());
                else if (tok2->isUnaryPreOp())
                    tok2 = const_cast<Token*>(tok2->astOperand1());
                else
                    break;
            }
            tok2 = tok2->next();

            if (isVariableChanged(questionToken, questionToken->astOperand2(), varid, false, settings) &&
                isVariableChanged(questionToken->astOperand2(), tok2, varid, false, settings))
            {
                if (settings->debugwarnings)
                    bailout(tokenlist, errorLogger, tok2, "variable " + var->name() + " valueFlowForward, assignment in condition");
                return false;

            }
        }

        else if (tok2->varId() == varid) {
            // compound assignment, known value in rhs
            if (Token::Match(tok2->previous(), "!!* %name% %assign%") &&
                tok2->next()->str() != "=" &&
                tok2->next()->astOperand2() &&
                tok2->next()->astOperand2()->hasKnownIntValue()) {

                const ValueFlow::Value &rhsValue = tok2->next()->astOperand2()->values().front();
                const std::string &assign = tok2->next()->str();
                std::list<ValueFlow::Value>::iterator it;
                // Erase values that are not int values..
                for (it = values.begin(); it != values.end();) {
                    if (!evalAssignment(*it, assign, rhsValue)) {
                        it = values.erase(it);
                    } else {
                        const std::string info("Compound assignment '" + assign + "', assigned value is " + it->infoString());
                        it->errorPath.emplace_back(tok2, info);

                        ++it;
                    }

                }
                if (values.empty()) {
                    if (settings->debugwarnings)
                        bailout(tokenlist, errorLogger, tok2, "compound assignment of " + tok2->str());
                    return false;
                }
            }

            // bailout: assignment
            else if (Token::Match(tok2->previous(), "!!* %name% %assign%")) {
                // simplify rhs
                std::stack<Token *> rhs;
                rhs.push(const_cast<Token *>(tok2->next()->astOperand2()));
                while (!rhs.empty()) {
                    Token *rtok = rhs.top();
                    rhs.pop();
                    if (!rtok)
                        continue;
                    if (Token::Match(rtok, "++|--|?|:|;|,"))
                        continue;
                    if (rtok->varId() == varid) {
                        for (const ValueFlow::Value &v : values)
                            setTokenValue(rtok, v, settings);
                    }
                    rhs.push(const_cast<Token *>(rtok->astOperand1()));
                    rhs.push(const_cast<Token *>(rtok->astOperand2()));
                }
                if (settings->debugwarnings)
                    bailout(tokenlist, errorLogger, tok2, "assignment of " + tok2->str());
                return false;
            }

            // bailout: possible assignment using >>
            if (isLikelyStreamRead(tok2->previous()))
            {
                if (settings->debugwarnings)
                    bailout(tokenlist, errorLogger, tok2, "Possible assignment of " + tok2->str() + " using " + tok2->strAt(-1));
                return false;
            }

            // skip if variable is conditionally used in ?: expression
            if (const Token *parent = skipValueInConditionalExpression(tok2)) {
                if (settings->debugwarnings)
                    bailout(tokenlist,
                            errorLogger,
                            tok2,
                            "no simplification of " + tok2->str() + " within " + (Token::Match(parent,"[?:]") ? "?:" : parent->str()) + " expression");
                const Token *astTop = parent->astTop();
                if (Token::simpleMatch(astTop->astOperand1(), "for ("))
                    tok2 = const_cast<Token*>(astTop->link());

                // bailout if address of var is taken..
                if (tok2->astParent() && tok2->astParent()->isUnaryOp("&")) {
                    if (settings->debugwarnings)
                        bailout(tokenlist, errorLogger, tok2, "Taking address of " + tok2->str());
                    return false;
                }

                continue;
            }

            {
                // Is variable usage protected by && || ?:
                const Token *tok3 = tok2;
                const Token *parent = tok3->astParent();
                while (parent && !Token::Match(parent, "%oror%|&&|:")) {
                    tok3 = parent;
                    parent = parent->astParent();
                }
                const bool conditional = parent && (parent->str() == ":" || parent->astOperand2() == tok3);

                for (const ValueFlow::Value &v : values) {
                    if (!conditional || !v.conditional)
                        setTokenValue(tok2, v, settings);
                }
            }

            // increment/decrement
            if (Token::Match(tok2->previous(), "++|-- %name%") || Token::Match(tok2, "%name% ++|--")) {
                std::list<ValueFlow::Value>::iterator it;
                // Erase values that are not int values..
                for (it = values.begin(); it != values.end();) {
                    if (!it->isIntValue())
                        it = values.erase(it);
                    else
                        ++it;
                }
                if (values.empty()) {
                    if (settings->debugwarnings)
                        bailout(tokenlist, errorLogger, tok2, "increment/decrement of " + tok2->str());
                    return false;
                }
                const bool pre   = Token::Match(tok2->previous(), "++|--");
                Token * const op = pre ? tok2->previous() : tok2->next();
                const bool inc   = (op->str() == "++");
                // Perform increment/decrement..
                for (it = values.begin(); it != values.end(); ++it) {
                    if (!pre)
                        setTokenValue(op, *it, settings);
                    it->intvalue += (inc ? 1 : -1);
                    if (pre)
                        setTokenValue(op, *it, settings);
                    const std::string info(tok2->str() + " is " + std::string(inc ? "incremented" : "decremented") + "', new value is " + it->infoString());
                    it->errorPath.emplace_back(tok2, info);
                }
            }

            // bailout if address of var is taken..
            if (tok2->astParent() && tok2->astParent()->isUnaryOp("&")) {
                if (settings->debugwarnings)
                    bailout(tokenlist, errorLogger, tok2, "Taking address of " + tok2->str());
                return false;
            }

            // bailout if reference is created..
            if (tok2->astParent() && Token::Match(tok2->astParent()->tokAt(-2), "& %name% =")) {
                if (settings->debugwarnings)
                    bailout(tokenlist, errorLogger, tok2, "Reference of " + tok2->str());
                return false;
            }

            // assigned by subfunction?
            bool inconclusive = false;
            if (isVariableChangedByFunctionCall(tok2, settings, &inconclusive)) {
                if (settings->debugwarnings)
                    bailout(tokenlist, errorLogger, tok2, "possible assignment of " + tok2->str() + " by subfunction");
                return false;
            }
            if (inconclusive) {
                for (ValueFlow::Value &v : values)
                    v.setInconclusive();
            }
            if (tok2->strAt(1) == "." && tok2->next()->originalName() != "->") {
                if (settings->inconclusive) {
                    for (ValueFlow::Value &v : values)
                        v.setInconclusive();
                } else {
                    if (settings->debugwarnings)
                        bailout(tokenlist, errorLogger, tok2, "possible assignment of " + tok2->str() + " by member function");
                    return false;
                }
            }
        }

        // Lambda function
        if (Token::simpleMatch(tok2, "= [") &&
            Token::simpleMatch(tok2->linkAt(1), "] (") &&
            Token::simpleMatch(tok2->linkAt(1)->linkAt(1), ") {")) {
            const Token *bodyStart = tok2->linkAt(1)->linkAt(1)->next();
            if (isVariableChanged(bodyStart, bodyStart->link(), varid, var->isGlobal(), settings))
            {
                if (settings->debugwarnings)
                    bailout(tokenlist, errorLogger, tok2, "valueFlowForward, " + var->name() + " is changed in lambda function");
                return false;
            }
        }
    }
    return true;
}

static const Token *findSimpleReturn(const Function *f)
{
    const Scope *scope = f->functionScope;
    if (!scope)
        return nullptr;
    const Token *returnTok = nullptr;
    for (const Token *tok = scope->bodyStart->next(); tok && tok != scope->bodyEnd; tok = tok->next()) {
        if (tok->str() == "{" && tok->scope() &&
            (tok->scope()->type == Scope::eClass))
        {
            tok = tok->link();
            continue;
        }
        if (Token::simpleMatch(tok->astParent(), "return")) {
            // Multiple returns
            if (returnTok)
                return nullptr;
            returnTok = tok;
        }
    }
    return returnTok;
}

static int getArgumentPos(const Variable *var, const Function *f)
{
    auto arg_it = std::find_if(f->argumentList.begin(), f->argumentList.end(), [&](const Variable &v) {
        return v.nameToken() == var->nameToken();
    });
    if (arg_it == f->argumentList.end())
        return -1;
    return std::distance(f->argumentList.begin(), arg_it);
}

std::string lifetimeType(const Token *tok, const ValueFlow::Value *val)
{
    std::string result;
    if (!val)
        return "object";
    switch (val->lifetimeKind) {
    case ValueFlow::Value::Lambda:
        result = "lambda";
        break;
    case ValueFlow::Value::Iterator:
        result = "iterator";
        break;
    case ValueFlow::Value::Object:
        result = "object";
        break;
    }
    return result;
}

static const Token *getLifetimeToken(const Token *tok, ValueFlow::Value::ErrorPath &errorPath, int depth = 20)
{
    if (!tok)
        return nullptr;
    const Variable *var = tok->variable();
    if (depth < 0)
        return tok;
    if (var && var->declarationId() == tok->varId()) {
        if (var->isReference() || var->isRValueReference()) {
            if (!var->declEndToken())
                return tok;
            if (var->isArgument()) {
                errorPath.emplace_back(var->declEndToken(), "Passed to reference.");
                return var->nameToken();
            } else if (Token::simpleMatch(var->declEndToken(), "=")) {
                errorPath.emplace_back(var->declEndToken(), "Assigned to reference.");
                const Token *vartok = var->declEndToken()->astOperand2();
                if (vartok == tok)
                    return tok;
                if (vartok)
                    return getLifetimeToken(vartok, errorPath, depth - 1);
            } else {
                return nullptr;
            }
        }
    } else if (Token::Match(tok->previous(), "%name% (")) {
        const Function *f = tok->previous()->function();
        if (!f)
            return tok;
        const Token *returnTok = findSimpleReturn(f);
        if (!returnTok)
            return tok;
        if (returnTok == tok)
            return tok;
        const Token *argvarTok = getLifetimeToken(returnTok, errorPath, depth - 1);
        if (!argvarTok)
            return tok;
        const Variable *argvar = argvarTok->variable();
        if (!argvar)
            return tok;
        if (argvar->isArgument() && (argvar->isReference() || argvar->isRValueReference())) {
            int n = getArgumentPos(argvar, f);
            if (n < 0)
                return nullptr;
            const Token *argTok = getArguments(tok->previous()).at(n);
            errorPath.emplace_back(returnTok, "Return reference.");
            errorPath.emplace_back(tok->previous(), "Called function passing '" + argTok->str() + "'.");
            return getLifetimeToken(argTok, errorPath, depth - 1);
        }
    } else if (Token::Match(tok, ".|::|[")) {
        const Token *vartok = tok;
        while (vartok) {
            if (vartok->str() == "[" || vartok->originalName() == "->")
                vartok = vartok->astOperand1();
            else if (vartok->str() == "." || vartok->str() == "::")
                vartok = vartok->astOperand2();
            else
                break;
        }

        if (!vartok)
            return tok;
        const Variable *tokvar = vartok->variable();
        if (!(tokvar && tokvar->isArray()) &&
            (Token::Match(vartok->astParent(), "[|*") || vartok->astParent()->originalName() == "->"))
        {
            for (const ValueFlow::Value &v : vartok->values()) {
                if (!v.isLocalLifetimeValue())
                    continue;
                errorPath.insert(errorPath.end(), v.errorPath.begin(), v.errorPath.end());
                return getLifetimeToken(v.tokvalue, errorPath);
            }
        } else {
            return getLifetimeToken(vartok, errorPath);
        }
    }
    return tok;
}

const Variable *getLifetimeVariable(const Token *tok, ValueFlow::Value::ErrorPath &errorPath)
{
    const Token *tok2 = getLifetimeToken(tok, errorPath);
    if (tok2 && tok2->variable())
        return tok2->variable();
    return nullptr;
}

static bool isNotLifetimeValue(const ValueFlow::Value& val)
{
    return !val.isLifetimeValue();
}

static void valueFlowLifetimeFunction(Token *tok, TokenList *tokenlist, ErrorLogger *errorLogger, const Settings *settings);

static void valueFlowForwardLifetime(Token * tok, TokenList *tokenlist, ErrorLogger *errorLogger, const Settings *settings)
{
    const Token *parent = tok->astParent();
    while (parent && (parent->isArithmeticalOp() || parent->str() == ","))
        parent = parent->astParent();
    if (!parent)
        return;
    // Assignment
    if (parent->str() == "=" && (!parent->astParent() || Token::simpleMatch(parent->astParent(), ";"))) {
        // Lhs should be a variable
        if (!parent->astOperand1() || !parent->astOperand1()->varId())
            return;
        const Variable *var = parent->astOperand1()->variable();
        if (!var || (!var->isLocal() && !var->isGlobal() && !var->isArgument()))
            return;

        const Token *const endOfVarScope = var->typeStartToken()->scope()->bodyEnd;

        // Rhs values..
        if (!parent->astOperand2() || parent->astOperand2()->values().empty())
            return;

        std::list<ValueFlow::Value> values = parent->astOperand2()->values();

        // Static variable initialisation?
        if (var->isStatic() && var->nameToken() == parent->astOperand1())
            changeKnownToPossible(values);

        // Skip RHS
        const Token *nextExpression = nextAfterAstRightmostLeaf(parent);

        // Only forward lifetime values
        values.remove_if(&isNotLifetimeValue);
        valueFlowForward(const_cast<Token *>(nextExpression),
                         endOfVarScope,
                         var,
                         var->declarationId(),
                         values,
                         false,
                         false,
                         tokenlist,
                         errorLogger,
                         settings);

        if (tok->astTop() && Token::simpleMatch(tok->astTop()->previous(), "for (") &&
            Token::simpleMatch(tok->astTop()->link(), ") {")) {
            const Token *start = tok->astTop()->link()->next();
            valueFlowForward(const_cast<Token *>(start),
                             start->link(),
                             var,
                             var->declarationId(),
                             values,
                             false,
                             false,
                             tokenlist,
                             errorLogger,
                             settings);
        }
        // Function call
    } else if (Token::Match(parent->previous(), "%name% (")) {
        valueFlowLifetimeFunction(const_cast<Token *>(parent->previous()), tokenlist, errorLogger, settings);
        // Variable
    } else if (tok->variable()) {
        const Variable *var = tok->variable();
        if (!var->typeStartToken() && !var->typeStartToken()->scope())
            return;
        const Token *endOfVarScope = var->typeStartToken()->scope()->bodyEnd;

        std::list<ValueFlow::Value> values = tok->values();
        const Token *nextExpression = nextAfterAstRightmostLeaf(parent);
        // Only forward lifetime values
        values.remove_if(&isNotLifetimeValue);
        valueFlowForward(const_cast<Token *>(nextExpression),
                         endOfVarScope,
                         var,
                         var->declarationId(),
                         values,
                         false,
                         false,
                         tokenlist,
                         errorLogger,
                         settings);
    }
}

struct LifetimeStore {
    const Token *argtok;
    std::string message;
    ValueFlow::Value::LifetimeKind type;
    ErrorPath errorPath;

    LifetimeStore(const Token *argtok,
                  const std::string &message,
                  ValueFlow::Value::LifetimeKind type = ValueFlow::Value::Object)
        : argtok(argtok), message(message), type(type), errorPath()
    {}

    template <class Predicate>
    void byRef(Token *tok, TokenList *tokenlist, ErrorLogger *errorLogger, const Settings *settings, Predicate pred) const {
        ErrorPath er = errorPath;
        const Token *lifeTok = getLifetimeToken(argtok, er);
        if (!lifeTok)
            return;
        if (!pred(lifeTok))
            return;
        er.emplace_back(argtok, message);

        ValueFlow::Value value;
        value.valueType = ValueFlow::Value::LIFETIME;
        value.lifetimeScope = ValueFlow::Value::Local;
        value.tokvalue = lifeTok;
        value.errorPath = er;
        value.lifetimeKind = type;
        // Don't add the value a second time
        if (std::find(tok->values().begin(), tok->values().end(), value) != tok->values().end())
            return;
        setTokenValue(tok, value, tokenlist->getSettings());
        valueFlowForwardLifetime(tok, tokenlist, errorLogger, settings);
    }

    void byRef(Token *tok, TokenList *tokenlist, ErrorLogger *errorLogger, const Settings *settings) const {
        byRef(tok, tokenlist, errorLogger, settings, [](const Token *) {
            return true;
        });
    }

    template <class Predicate>
    void byVal(Token *tok, TokenList *tokenlist, ErrorLogger *errorLogger, const Settings *settings, Predicate pred) const {
        if (argtok->values().empty()) {
            ErrorPath er;
            er.emplace_back(argtok, message);
            const Variable *var = getLifetimeVariable(argtok, er);
            if (var && var->isArgument()) {
                ValueFlow::Value value;
                value.valueType = ValueFlow::Value::LIFETIME;
                value.lifetimeScope = ValueFlow::Value::Argument;
                value.tokvalue = var->nameToken();
                value.errorPath = er;
                value.lifetimeKind = type;
                // Don't add the value a second time
                if (std::find(tok->values().begin(), tok->values().end(), value) != tok->values().end())
                    return;
                setTokenValue(tok, value, tokenlist->getSettings());
                valueFlowForwardLifetime(tok, tokenlist, errorLogger, settings);
            }
        }
        for (const ValueFlow::Value &v : argtok->values()) {
            if (!v.isLifetimeValue())
                continue;
            const Token *tok3 = v.tokvalue;
            ErrorPath er = v.errorPath;
            const Token *lifeTok = getLifetimeToken(tok3, er);
            if (!lifeTok)
                return;
            if (!pred(lifeTok))
                return;
            er.emplace_back(argtok, message);
            er.insert(er.end(), errorPath.begin(), errorPath.end());

            ValueFlow::Value value;
            value.valueType = ValueFlow::Value::LIFETIME;
            value.lifetimeScope = v.lifetimeScope;
            value.tokvalue = lifeTok;
            value.errorPath = er;
            value.lifetimeKind = type;
            // Don't add the value a second time
            if (std::find(tok->values().begin(), tok->values().end(), value) != tok->values().end())
                continue;
            setTokenValue(tok, value, tokenlist->getSettings());
            valueFlowForwardLifetime(tok, tokenlist, errorLogger, settings);
        }
    }

    void byVal(Token *tok, TokenList *tokenlist, ErrorLogger *errorLogger, const Settings *settings) const {
        byVal(tok, tokenlist, errorLogger, settings, [](const Token *) {
            return true;
        });
    }
};

static const Token *endTemplateArgument(const Token *tok)
{
    for (; tok; tok = tok->next()) {
        if (Token::Match(tok, ">|,"))
            return tok;
        else if (tok->link() && Token::Match(tok, "(|{|[|<"))
            tok = tok->link();
        else if (Token::simpleMatch(tok, ";"))
            return nullptr;
    }
    return nullptr;
}

static void valueFlowLifetimeFunction(Token *tok, TokenList *tokenlist, ErrorLogger *errorLogger, const Settings *settings)
{
    if (!Token::Match(tok, "%name% ("))
        return;

    if (tok->function())
    {
        const Function *f = tok->function();
        const Token *returnTok = findSimpleReturn(f);
        if (!returnTok)
            return;
        for (const ValueFlow::Value &v : returnTok->values()) {
            if (!v.isLifetimeValue())
                continue;
            if (!v.tokvalue)
                continue;
            const Variable *var = v.tokvalue->variable();
            if (!var)
                continue;
            if (!var->isArgument())
                continue;
            int n = getArgumentPos(var, f);
            if (n < 0)
                continue;
            std::vector<const Token *> args = getArguments(tok);
            if (n >= args.size()) {
                if (tokenlist->getSettings()->debugwarnings)
                    bailout(tokenlist,
                            errorLogger,
                            tok,
                            "Argument mismatch: Function '" + tok->str() + "' returning lifetime from argument index " +
                            std::to_string(n) + " but only " + std::to_string(args.size()) +
                            " arguments are available.");
                continue;
            }
            const Token *argtok = args[n];
            LifetimeStore ls{argtok, "Passed to '" + tok->str() + "'.", ValueFlow::Value::Object};
            ls.errorPath = v.errorPath;
            ls.errorPath.emplace_front(returnTok, "Return " + lifetimeType(returnTok, &v) + ".");
            if (var->isReference() || var->isRValueReference()) {
                ls.byRef(tok->next(), tokenlist, errorLogger, settings);
            } else if (v.isArgumentLifetimeValue()) {
                ls.byVal(tok->next(), tokenlist, errorLogger, settings);
            }
        }
    }
}

static void valueFlowLifetime(TokenList *tokenlist, SymbolDatabase*, ErrorLogger *errorLogger, const Settings *settings)
{
    for (Token *tok = tokenlist->front(); tok; tok = tok->next()) {
        if (!tok->scope())
            continue;
        if (tok->scope()->type == Scope::eGlobal)
            continue;
        if (tok->isUnaryOp("&"))
        {
            ErrorPath errorPath;
            const Token *lifeTok = getLifetimeToken(tok->astOperand1(), errorPath);
            if (!lifeTok)
                continue;

            errorPath.emplace_back(tok, "Address of variable taken here.");

            ValueFlow::Value value;
            value.valueType = ValueFlow::Value::LIFETIME;
            value.lifetimeScope = ValueFlow::Value::Local;
            value.tokvalue = lifeTok;
            value.errorPath = errorPath;
            setTokenValue(tok, value, tokenlist->getSettings());

            valueFlowForwardLifetime(tok, tokenlist, errorLogger, settings);
        }
        // container lifetimes
        else if (tok->variable() && Token::Match(tok, "%var% . begin|cbegin|rbegin|crbegin|end|cend|rend|crend|data|c_str (")) {
            ErrorPath errorPath;

            bool isIterator = !Token::Match(tok->tokAt(2), "data|c_str");
            if (isIterator)
                errorPath.emplace_back(tok, "Iterator to container is created here.");
            else
                errorPath.emplace_back(tok, "Pointer to container is created here.");

            ValueFlow::Value value;
            value.valueType = ValueFlow::Value::LIFETIME;
            value.lifetimeScope = ValueFlow::Value::Local;
            value.tokvalue = tok;
            value.errorPath = errorPath;
            value.lifetimeKind = isIterator ? ValueFlow::Value::Iterator : ValueFlow::Value::Object;
            setTokenValue(tok->tokAt(3), value, tokenlist->getSettings());

            valueFlowForwardLifetime(tok->tokAt(3), tokenlist, errorLogger, settings);

        }
        // Check function calls
        else if (Token::Match(tok, "%name% (")) {
            valueFlowLifetimeFunction(tok, tokenlist, errorLogger, settings);
        }
    }
}


static void valueFlowAfterAssign(TokenList *tokenlist, SymbolDatabase *symboldatabase, ErrorLogger *errorLogger, const Settings *settings)
{
    for (const Scope *scope : symboldatabase->functionScopes)
    {
        std::set<unsigned int> aliased;
        for (Token *tok = const_cast<Token *>(scope->bodyStart); tok != scope->bodyEnd; tok = tok->next())
        {
            // Alias
            if (tok->isUnaryOp("&"))
            {
                aliased.insert(tok->astOperand1()->varId());
                continue;
            }

            // Assignment
            if ((tok->str() != "=") || (tok->astParent()))
                continue;

            // Lhs should be a variable
            if (!tok->astOperand1() || !tok->astOperand1()->varId() || tok->astOperand1()->hasKnownValue())
                continue;
            const unsigned int varid = tok->astOperand1()->varId();
            if (aliased.find(varid) != aliased.end())
                continue;
            const Variable *var = tok->astOperand1()->variable();
            if (!var || (!var->isLocal() && !var->isGlobal() && !var->isArgument()))
                continue;

            const Token * const endOfVarScope = var->typeStartToken()->scope()->bodyEnd;

            // Rhs values..
            if (!tok->astOperand2() || tok->astOperand2()->values().empty())
                continue;

            std::list<ValueFlow::Value> values = tok->astOperand2()->values();
            if (std::any_of(values.begin(), values.end(), std::mem_fn(&ValueFlow::Value::isLifetimeValue))) {
                valueFlowForwardLifetime(tok, tokenlist, errorLogger, settings);
                values.remove_if(std::mem_fn(&ValueFlow::Value::isLifetimeValue));
            }

            values.remove_if(std::mem_fn(&ValueFlow::Value::isTokValue));

            for (std::list<ValueFlow::Value>::iterator it = values.begin(); it != values.end(); ++it) {
                const std::string info = "Assignment '" + tok->expressionString() + "', assigned value is " + it->infoString();
                it->errorPath.emplace_back(tok->astOperand2(), info);
            }
            const bool constValue = tok->astOperand2()->isNumber();

            if (Token::Match(var->typeStartToken(), "bool"))
            {
                std::list<ValueFlow::Value>::iterator it;
                for (it = values.begin(); it != values.end(); ++it) {
                    if (it->isIntValue())
                        it->intvalue = (it->intvalue != 0);
                    if (it->isTokValue())
                        it ->intvalue = (it->tokvalue != 0);
                }
            }

            // Static variable initialisation?
            if (var->isStatic() && var->nameToken() == tok->astOperand1())
                changeKnownToPossible(values);

            // Skip RHS
            const Token * nextExpression = nextAfterAstRightmostLeaf(tok);

            if (std::any_of(values.begin(), values.end(), std::mem_fn(&ValueFlow::Value::isTokValue))) {
                std::list<ValueFlow::Value> tokvalues;
                std::copy_if(values.begin(),
                             values.end(),
                             std::back_inserter(tokvalues),
                             std::mem_fn(&ValueFlow::Value::isTokValue));
                valueFlowForward(const_cast<Token *>(nextExpression),
                                 endOfVarScope,
                                 var,
                                 varid,
                                 tokvalues,
                                 constValue,
                                 false,
                                 tokenlist,
                                 errorLogger,
                                 settings);
                values.remove_if(std::mem_fn(&ValueFlow::Value::isTokValue));
            }
            valueFlowForward(const_cast<Token *>(nextExpression), endOfVarScope, var, varid, values, constValue, false, tokenlist, errorLogger, settings);
        }
    }
}

struct ValueFlowConditionHandler {
    struct Condition {
        const Token *vartok;
        std::list<ValueFlow::Value> true_values;
        std::list<ValueFlow::Value> false_values;

        Condition() : vartok(nullptr), true_values(), false_values() {}
    };
    std::function<bool(Token *start, const Token *stop, const Variable *var, const std::list<ValueFlow::Value> &values, bool constValue)>
    forward;
    std::function<Condition(Token *tok)> parse;

    void afterCondition(TokenList *tokenlist,
                        SymbolDatabase *symboldatabase,
                        ErrorLogger *errorLogger,
                        const Settings *settings) const {
        for (const Scope *scope : symboldatabase->functionScopes) {
            std::set<unsigned> aliased;
            for (Token *tok = const_cast<Token *>(scope->bodyStart); tok != scope->bodyEnd; tok = tok->next()) {
                if (Token::Match(tok, "= & %var% ;"))
                    aliased.insert(tok->tokAt(2)->varId());

                Condition cond = parse(tok);
                if (!cond.vartok)
                    continue;
                if (cond.true_values.empty() || cond.false_values.empty())
                    continue;
                const unsigned int varid = cond.vartok->varId();
                if (varid == 0U)
                    continue;
                const Variable *var = cond.vartok->variable();
                if (!var || !(var->isLocal() || var->isGlobal() || var->isArgument()))
                    continue;
                if (aliased.find(varid) != aliased.end()) {
                    if (settings->debugwarnings)
                        bailout(tokenlist,
                                errorLogger,
                                cond.vartok,
                                "variable is aliased so we just skip all valueflow after condition");
                    continue;
                }

                if (Token::Match(tok->astParent(), "%oror%|&&")) {
                    Token *parent = const_cast<Token *>(tok->astParent());
                    const std::string &op(parent->str());

                    if (parent->astOperand1() == tok && ((op == "&&" && Token::Match(tok, "==|>=|<=|!")) ||
                                                         (op == "||" && Token::Match(tok, "%name%|!=")))) {
                        for (; parent && parent->str() == op; parent = const_cast<Token *>(parent->astParent())) {
                            std::stack<Token *> tokens;
                            tokens.push(const_cast<Token *>(parent->astOperand2()));
                            bool assign = false;
                            while (!tokens.empty()) {
                                Token *rhstok = tokens.top();
                                tokens.pop();
                                if (!rhstok)
                                    continue;
                                tokens.push(const_cast<Token *>(rhstok->astOperand1()));
                                tokens.push(const_cast<Token *>(rhstok->astOperand2()));
                                if (rhstok->varId() == varid)
                                    setTokenValue(rhstok, cond.true_values.front(), settings);
                                else if (Token::Match(rhstok, "++|--|=") &&
                                         Token::Match(rhstok->astOperand1(), "%varid%", varid)) {
                                    assign = true;
                                    break;
                                }
                            }
                            if (assign)
                                break;
                            while (parent->astParent() && parent == parent->astParent()->astOperand2())
                                parent = const_cast<Token *>(parent->astParent());
                        }
                    }
                }

                const Token *top = tok->astTop();
                if (top && Token::Match(top->previous(), "if|while ("))
                {
                    // does condition reassign variable?
                    if (tok != top->astOperand2() && Token::Match(top->astOperand2(), "%oror%|&&") &&
                        isVariableChanged(top, top->link(), varid, var->isGlobal(), settings))
                    {
                        if (settings->debugwarnings)
                            bailout(tokenlist, errorLogger, tok, "assignment in condition");
                        continue;
                    }

                    // start token of conditional code
                    Token *startTokens[] = {nullptr, nullptr};

                    // based on the comparison, should we check the if or while?
                    bool check_if = false;
                    bool check_else = false;
                    if (Token::Match(tok, "==|>=|<=|!|>|<|("))
                        check_if = true;
                    if (Token::Match(tok, "%name%|!=|>|<"))
                        check_else = true;

                    if (!check_if && !check_else)
                        continue;

                    // if astParent is "!" we need to invert codeblock
                    {
                        const Token *parent = tok->astParent();
                        while (parent && parent->str() == "&&")
                            parent = parent->astParent();
                        if (parent && (parent->str() == "!" || Token::simpleMatch(parent, "== false"))) {
                            check_if = !check_if;
                            check_else = !check_else;
                        }
                    }

                    // determine startToken(s)
                    if (check_if && Token::simpleMatch(top->link(), ") {"))
                        startTokens[0] = top->link()->next();
                    if (check_else && Token::simpleMatch(top->link()->linkAt(1), "} else {"))
                        startTokens[1] = top->link()->linkAt(1)->tokAt(2);

                    bool bail = false;

                    for (int i = 0; i < 2; i++) {
                        const Token *const startToken = startTokens[i];
                        if (!startToken)
                            continue;
                        std::list<ValueFlow::Value> &values = (i == 0 ? cond.true_values : cond.false_values);
                        if (values.size() == 1U && Token::Match(tok, "==|!|(")) {
                            const Token *parent = tok->astParent();
                            while (parent && parent->str() == "&&")
                                parent = parent->astParent();
                            if (parent && parent->str() == "(")
                                values.front().setKnown();
                        }

                        bool changed = forward(startTokens[i], startTokens[i]->link(), var, values, true);
                        values.front().setPossible();
                        if (changed) {
                            // TODO: The endToken should not be startTokens[i]->link() in the valueFlowForward call
                            if (settings->debugwarnings)
                                bailout(tokenlist,
                                        errorLogger,
                                        startTokens[i]->link(),
                                        "valueFlowAfterCondition: " + var->name() + " is changed in conditional block");
                            bail = true;
                            break;
                        }
                    }
                    if (bail)
                        continue;

                    // After conditional code..
                    if (Token::simpleMatch(top->link(), ") {")) {
                        Token *after = top->link()->linkAt(1);
                        std::string unknownFunction;
                        if (settings->library.isScopeNoReturn(after, &unknownFunction)) {
                            if (settings->debugwarnings && !unknownFunction.empty())
                                bailout(tokenlist, errorLogger, after, "possible noreturn scope");
                            continue;
                        }

                        const bool dead_if = isReturnScope(after);
                        bool dead_else = false;

                        if (Token::simpleMatch(after, "} else {")) {
                            after = after->linkAt(2);
                            if (Token::simpleMatch(after->tokAt(-2), ") ; }")) {
                                if (settings->debugwarnings)
                                    bailout(tokenlist, errorLogger, after, "possible noreturn scope");
                                continue;
                            }
                            dead_else = isReturnScope(after);
                        }

                        std::list<ValueFlow::Value> *values = nullptr;
                        if (!dead_if && check_if)
                            values = &cond.true_values;
                        else if (!dead_else && check_else)
                            values = &cond.false_values;

                        if (values) {
                            // TODO: constValue could be true if there are no assignments in the conditional blocks and
                            //       perhaps if there are no && and no || in the condition
                            bool constValue = false;
                            forward(after, top->scope()->bodyEnd, var, *values, constValue);
                        }
                    }
                }
            }
        }
    }
};

static void setConditionalValues(const Token *tok,
                                 bool invert,
                                 MathLib::bigint value,
                                 ValueFlow::Value &true_value,
                                 ValueFlow::Value &false_value)
{
    if (Token::Match(tok, "==|!=|>=|<=")) {
        true_value = ValueFlow::Value{tok, value};
        false_value = ValueFlow::Value{tok, value};
        return;
    }
    const char *greaterThan = ">";
    const char *lessThan = "<";
    if (invert)
        std::swap(greaterThan, lessThan);
    if (Token::simpleMatch(tok, greaterThan)) {
        true_value = ValueFlow::Value{tok, value + 1};
        false_value = ValueFlow::Value{tok, value};
    } else if (Token::simpleMatch(tok, lessThan)) {
        true_value = ValueFlow::Value{tok, value - 1};
        false_value = ValueFlow::Value{tok, value};
    }
}

static const Token *parseCompareInt(const Token *tok, ValueFlow::Value &true_value, ValueFlow::Value &false_value)
{
    if (!tok->astOperand1() || !tok->astOperand2())
        return nullptr;
    if (Token::Match(tok, "%comp%")) {
        if (tok->astOperand1()->hasKnownIntValue()) {
            setConditionalValues(tok, true, tok->astOperand1()->values().front().intvalue, true_value, false_value);
            return tok->astOperand2();
        } else if (tok->astOperand2()->hasKnownIntValue()) {
            setConditionalValues(tok, false, tok->astOperand2()->values().front().intvalue, true_value, false_value);
            return tok->astOperand1();
        }
    }
    return nullptr;
}

static void valueFlowAfterCondition(TokenList *tokenlist,
                                    SymbolDatabase *symboldatabase,
                                    ErrorLogger *errorLogger,
                                    const Settings *settings)
{
    ValueFlowConditionHandler handler;
    handler.forward = [&](Token *start,
                          const Token *stop,
                          const Variable *var,
                          const std::list<ValueFlow::Value> &values,
    bool constValue) {
        valueFlowForward(
            start->next(), stop, var, var->declarationId(), values, constValue, false, tokenlist, errorLogger, settings);
        return isVariableChanged(start, stop, var->declarationId(), var->isGlobal(), settings);
    };
    handler.parse = [&](const Token *tok) {
        ValueFlowConditionHandler::Condition cond;
        ValueFlow::Value true_value;
        ValueFlow::Value false_value;
        const Token *vartok = parseCompareInt(tok, true_value, false_value);
        if (vartok) {
            if (vartok->str() == "=" && vartok->astOperand1() && vartok->astOperand2())
                vartok = vartok->astOperand1();
            if (!vartok->isName())
                return cond;
            cond.true_values.push_back(true_value);
            cond.false_values.push_back(false_value);
            cond.vartok = vartok;
            return cond;
        }

        if (tok->str() == "!") {
            vartok = tok->astOperand1();

        } else if (tok->isName() && (Token::Match(tok->astParent(), "%oror%|&&") ||
                                     Token::Match(tok->tokAt(-2), "if|while ( %var% [)=]"))) {
            vartok = tok;
        }

        if (!vartok || !vartok->isName())
            return cond;
        cond.true_values.emplace_back(tok, 0LL);
        cond.false_values.emplace_back(tok, 0LL);
        cond.vartok = vartok;

        return cond;
    };
    handler.afterCondition(tokenlist, symboldatabase, errorLogger, settings);
}

static void execute(const Token *expr,
                    ProgramMemory * const programMemory,
                    MathLib::bigint *result,
                    bool *error)
{
    if (!expr)
        *error = true;

    else if (expr->hasKnownIntValue()) {
        *result = expr->values().front().intvalue;
    }

    else if (expr->isNumber()) {
        *result = MathLib::toLongNumber(expr->str());
        if (MathLib::isFloat(expr->str()))
            *error = true;
    }

    else if (expr->varId() > 0) {
        if (!programMemory->getIntValue(expr->varId(), result))
            *error = true;
    }

    else if (expr->isComparisonOp()) {
        MathLib::bigint result1(0), result2(0);
        execute(expr->astOperand1(), programMemory, &result1, error);
        execute(expr->astOperand2(), programMemory, &result2, error);
        if (expr->str() == "<")
            *result = result1 < result2;
        else if (expr->str() == "<=")
            *result = result1 <= result2;
        else if (expr->str() == ">")
            *result = result1 > result2;
        else if (expr->str() == ">=")
            *result = result1 >= result2;
        else if (expr->str() == "==")
            *result = result1 == result2;
        else if (expr->str() == "!=")
            *result = result1 != result2;
    }

    else if (expr->isAssignmentOp())
    {
        execute(expr->astOperand2(), programMemory, result, error);
        if (!expr->astOperand1() || !expr->astOperand1()->varId())
            *error = true;
        if (*error)
            return;

        if (expr->str() == "=")
        {
            programMemory->setIntValue(expr->astOperand1()->varId(), *result);
            return;
        }

        long long intValue;
        if (!programMemory->getIntValue(expr->astOperand1()->varId(), &intValue))
        {
            *error = true;
            return;
        }
        if (expr->str() == "+=")
            programMemory->setIntValue(expr->astOperand1()->varId(), intValue + *result);
        else if (expr->str() == "-=")
            programMemory->setIntValue(expr->astOperand1()->varId(), intValue - *result);
        else if (expr->str() == "*=")
            programMemory->setIntValue(expr->astOperand1()->varId(), intValue * *result);
        else if (expr->str() == "/=" && *result != 0)
            programMemory->setIntValue(expr->astOperand1()->varId(), intValue / *result);
        else if (expr->str() == "%=" && *result != 0)
            programMemory->setIntValue(expr->astOperand1()->varId(), intValue % *result);
        else if (expr->str() == "&=")
            programMemory->setIntValue(expr->astOperand1()->varId(), intValue & *result);
        else if (expr->str() == "|=")
            programMemory->setIntValue(expr->astOperand1()->varId(), intValue | *result);
        else if (expr->str() == "^=")
            programMemory->setIntValue(expr->astOperand1()->varId(), intValue ^ *result);
    }

    else if (Token::Match(expr, "++|--")) {
        if (!expr->astOperand1() || expr->astOperand1()->varId() == 0U)
            *error = true;
        else {
            long long intValue;
            if (!programMemory->getIntValue(expr->astOperand1()->varId(), &intValue))
                *error = true;
            else {
                if (intValue == 0 &&
                    expr->str() == "--" &&
                    expr->astOperand1()->variable() &&
                    expr->astOperand1()->variable()->typeStartToken()->isUnsigned())
                    *error = true; // overflow
                *result = intValue + (expr->str() == "++" ? 1 : -1);
                programMemory->setIntValue(expr->astOperand1()->varId(), *result);
            }
        }
    }

    else if (expr->isArithmeticalOp() && expr->astOperand1() && expr->astOperand2()) {
        MathLib::bigint result1(0), result2(0);
        execute(expr->astOperand1(), programMemory, &result1, error);
        execute(expr->astOperand2(), programMemory, &result2, error);
        if (expr->str() == "+")
            *result = result1 + result2;
        else if (expr->str() == "-")
            *result = result1 - result2;
        else if (expr->str() == "*") {
            if (result2 && (result1 > std::numeric_limits<MathLib::bigint>::max()/result2))
                *error = true;
            else
                *result = result1 * result2;
        } else if (result2 == 0)
            *error = true;
        else if (expr->str() == "/")
            *result = result1 / result2;
        else if (expr->str() == "%")
            *result = result1 % result2;
        else if (expr->str() == "<<")  {
            if (result2 < 0 || result1 < 0 || result2 >= MathLib::bigint_bits)  { // don't perform UB
                *error= true;
            } else {
                *result = result1 << result2;
            }
        } else if (expr->str() == ">>") {
            if (result2 < 0) { // don't perform UB
                *error=true;
            } else {
                *result = result1 >> result2;
            }
        }
    }

    else if (expr->str() == "&&") {
        bool error1 = false;
        execute(expr->astOperand1(), programMemory, result, &error1);
        if (!error1 && *result == 0)
            *result = 0;
        else {
            bool error2 = false;
            execute(expr->astOperand2(), programMemory, result, &error2);
            if (error1 && error2)
                *error = true;
            if (error2)
                *result = 1;
            else
                *result = !!*result;
        }
    }

    else if (expr->str() == "||") {
        execute(expr->astOperand1(), programMemory, result, error);
        if (*result == 0 && *error == false)
            execute(expr->astOperand2(), programMemory, result, error);
    }

    else if (expr->str() == "!") {
        execute(expr->astOperand1(), programMemory, result, error);
        *result = !(*result);
    }

    else if (expr->str() == "," && expr->astOperand1() && expr->astOperand2()) {
        execute(expr->astOperand1(), programMemory, result, error);
        execute(expr->astOperand2(), programMemory, result, error);
    }

    else if (expr->str() == "[" && expr->astOperand1() && expr->astOperand2()) {
        const Token *tokvalue = nullptr;
        if (!programMemory->getTokValue(expr->astOperand1()->varId(), &tokvalue)) {
            auto tokvalue_it = std::find_if(expr->astOperand1()->values().begin(),
                                            expr->astOperand1()->values().end(),
                                            std::mem_fn(&ValueFlow::Value::isTokValue));
            if (tokvalue_it == expr->astOperand1()->values().end()) {
                *error = true;
                return;
            }
            tokvalue = tokvalue_it->tokvalue;
        }
        if (!tokvalue || !tokvalue->isLiteral()) {
            *error = true;
            return;
        }
        const std::string strValue = tokvalue->strValue();
        MathLib::bigint index = 0;
        execute(expr->astOperand2(), programMemory, &index, error);
        if (index >= 0 && index < strValue.size())
            *result = strValue[index];
        else if (index == strValue.size())
            *result = 0;
        else
            *error = true;
    }

    else
        *error = true;
}

static bool valueFlowForLoop1(const Token *tok, unsigned int * const varid, MathLib::bigint * const num1, MathLib::bigint * const num2, MathLib::bigint * const numAfter)
{
    tok = tok->tokAt(2);
    if (!Token::Match(tok, "%type%| %var% ="))
        return false;
    const Token * const vartok = Token::Match(tok, "%var% =") ? tok : tok->next();
    *varid = vartok->varId();
    tok = vartok->tokAt(2);
    const Token * const num1tok = Token::Match(tok, "%num% ;") ? tok : nullptr;
    if (num1tok)
        *num1 = MathLib::toLongNumber(num1tok->str());
    while (Token::Match(tok, "%name%|%num%|%or%|+|-|*|/|&|[|]|("))
        tok = (tok->str() == "(") ? tok->link()->next() : tok->next();
    if (!tok || tok->str() != ";")
        return false;
    tok = tok->next();
    const Token *num2tok = nullptr;
    if (Token::Match(tok, "%varid% <|<=|!=", vartok->varId())) {
        tok = tok->next();
        num2tok = tok->astOperand2();
        if (num2tok && num2tok->str() == "(" && !num2tok->astOperand2())
            num2tok = num2tok->astOperand1();
        if (!Token::Match(num2tok, "%num% ;|%oror%")) // TODO: || enlarges the scope of the condition, so it should not cause FP, but it should no lnger be part of this pattern as soon as valueFlowForLoop2 can handle an unknown RHS of || better
            num2tok = nullptr;
    }
    if (!num2tok)
        return false;
    *num2 = MathLib::toLongNumber(num2tok->str()) - ((tok->str()=="<=") ? 0 : 1);
    *numAfter = *num2 + 1;
    if (!num1tok)
        *num1 = *num2;
    while (tok && tok->str() != ";")
        tok = tok->next();
    if (!Token::Match(tok, "; %varid% ++ ) {", vartok->varId()) && !Token::Match(tok, "; ++ %varid% ) {", vartok->varId()))
        return false;
    return true;
}

static bool valueFlowForLoop2(const Token *tok,
                              ProgramMemory *memory1,
                              ProgramMemory *memory2,
                              ProgramMemory *memoryAfter)
{
    // for ( firstExpression ; secondExpression ; thirdExpression )
    const Token *firstExpression  = tok->next()->astOperand2()->astOperand1();
    const Token *secondExpression = tok->next()->astOperand2()->astOperand2()->astOperand1();
    const Token *thirdExpression = tok->next()->astOperand2()->astOperand2()->astOperand2();

    ProgramMemory programMemory;
    MathLib::bigint result(0);
    bool error = false;
    execute(firstExpression, &programMemory, &result, &error);
    if (error)
        return false;
    execute(secondExpression, &programMemory, &result, &error);
    if (result == 0) // 2nd expression is false => no looping
        return false;
    if (error) {
        // If a variable is reassigned in second expression, return false
        bool reassign = false;
        visitAstNodes(secondExpression,
        [&](const Token *t) {
            if (t->str() == "=" && t->astOperand1() && programMemory.hasValue(t->astOperand1()->varId()))
                // TODO: investigate what variable is assigned.
                reassign = true;
            return reassign ? ChildrenToVisit::done : ChildrenToVisit::op1_and_op2;
        });
        if (reassign)
            return false;
    }

    ProgramMemory startMemory(programMemory);
    ProgramMemory endMemory;

    unsigned int maxcount = 10000;
    while (result != 0 && !error && --maxcount) {
        endMemory = programMemory;
        execute(thirdExpression, &programMemory, &result, &error);
        if (!error)
            execute(secondExpression, &programMemory, &result, &error);
    }

    memory1->swap(startMemory);
    if (!error) {
        memory2->swap(endMemory);
        memoryAfter->swap(programMemory);
    }

    return true;
}

static void valueFlowForLoopSimplify(Token * const bodyStart, const unsigned int varid, bool globalvar, const MathLib::bigint value, TokenList *tokenlist, ErrorLogger *errorLogger, const Settings *settings)
{
    const Token * const bodyEnd = bodyStart->link();

    // Is variable modified inside for loop
    if (isVariableChanged(bodyStart, bodyEnd, varid, globalvar, settings))
        return;

    for (Token *tok2 = bodyStart->next(); tok2 != bodyEnd; tok2 = tok2->next()) {
        if (tok2->varId() == varid) {
            const Token * parent = tok2->astParent();
            while (parent) {
                const Token * const p = parent;
                parent = parent->astParent();
                if (!parent || parent->str() == ":")
                    break;
                if (parent->str() == "?") {
                    if (parent->astOperand2() != p)
                        parent = nullptr;
                    break;
                }
            }
            if (parent) {
                if (settings->debugwarnings)
                    bailout(tokenlist, errorLogger, tok2, "For loop variable " + tok2->str() + " stopping on ?");
                continue;
            }

            ValueFlow::Value value1(value);
            value1.varId = tok2->varId();
            setTokenValue(tok2, value1, settings);
        }

        if (Token::Match(tok2, "%oror%|&&")) {
            const ProgramMemory programMemory(getProgramMemory(tok2->astTop(), varid, ValueFlow::Value(value)));
            if ((tok2->str() == "&&" && !conditionIsTrue(tok2->astOperand1(), programMemory)) ||
                (tok2->str() == "||" && !conditionIsFalse(tok2->astOperand1(), programMemory))) {
                // Skip second expression..
                const Token *parent = tok2;
                while (parent && parent->str() == tok2->str())
                    parent = parent->astParent();
                // Jump to end of condition
                if (parent && parent->str() == "(") {
                    tok2 = parent->link();
                    // cast
                    if (Token::simpleMatch(tok2, ") ("))
                        tok2 = tok2->linkAt(1);
                }
            }

        }
        if ((tok2->str() == "&&" && conditionIsFalse(tok2->astOperand1(), getProgramMemory(tok2->astTop(), varid, ValueFlow::Value(value)))) ||
            (tok2->str() == "||" && conditionIsTrue(tok2->astOperand1(), getProgramMemory(tok2->astTop(), varid, ValueFlow::Value(value)))))
            break;

        else if (Token::simpleMatch(tok2, ") {") && Token::findmatch(tok2->link(), "%varid%", tok2, varid)) {
            if (Token::findmatch(tok2, "continue|break|return", tok2->linkAt(1), varid)) {
                if (settings->debugwarnings)
                    bailout(tokenlist, errorLogger, tok2, "For loop variable bailout on conditional continue|break|return");
                break;
            }
            if (settings->debugwarnings)
                bailout(tokenlist, errorLogger, tok2, "For loop variable skipping conditional scope");
            tok2 = tok2->next()->link();
            if (Token::simpleMatch(tok2, "} else {")) {
                if (Token::findmatch(tok2, "continue|break|return", tok2->linkAt(2), varid)) {
                    if (settings->debugwarnings)
                        bailout(tokenlist, errorLogger, tok2, "For loop variable bailout on conditional continue|break|return");
                    break;
                }

                tok2 = tok2->linkAt(2);
            }
        }

        else if (Token::simpleMatch(tok2, ") {")) {
            if (settings->debugwarnings)
                bailout(tokenlist, errorLogger, tok2, "For loop skipping {} code");
            tok2 = tok2->linkAt(1);
            if (Token::simpleMatch(tok2, "} else {"))
                tok2 = tok2->linkAt(2);
        }
    }
}

static void valueFlowForLoopSimplifyAfter(Token *fortok, unsigned int varid, const MathLib::bigint num, TokenList *tokenlist, ErrorLogger *errorLogger, const Settings *settings)
{
    const Token *vartok = nullptr;
    for (const Token *tok = fortok; tok; tok = tok->next()) {
        if (tok->varId() == varid) {
            vartok = tok;
            break;
        }
    }
    if (!vartok || !vartok->variable())
        return;

    const Variable *var = vartok->variable();
    const Token *endToken = nullptr;
    if (var->isLocal())
        endToken = var->typeStartToken()->scope()->bodyEnd;
    else
        endToken = fortok->scope()->bodyEnd;

    std::list<ValueFlow::Value> values;
    values.emplace_back(num);
    values.back().errorPath.emplace_back(fortok,"After for loop, " + var->name() + " has value " + values.back().infoString());

    valueFlowForward(fortok->linkAt(1)->linkAt(1)->next(),
                     endToken,
                     var,
                     varid,
                     values,
                     false,
                     false,
                     tokenlist,
                     errorLogger,
                     settings);
}

static void valueFlowForLoop(TokenList *tokenlist, SymbolDatabase* symboldatabase, ErrorLogger *errorLogger, const Settings *settings)
{
    for (const Scope &scope : symboldatabase->scopeList) {
        if (scope.type != Scope::eFor)
            continue;

        Token* tok = const_cast<Token*>(scope.classDef);
        Token* const bodyStart = const_cast<Token*>(scope.bodyStart);

        if (!Token::simpleMatch(tok->next()->astOperand2(), ";") ||
            !Token::simpleMatch(tok->next()->astOperand2()->astOperand2(), ";"))
            continue;

        unsigned int varid(0);
        MathLib::bigint num1(0), num2(0), numAfter(0);

        if (valueFlowForLoop1(tok, &varid, &num1, &num2, &numAfter)) {
            if (num1 <= num2) {
                valueFlowForLoopSimplify(bodyStart, varid, false, num1, tokenlist, errorLogger, settings);
                valueFlowForLoopSimplify(bodyStart, varid, false, num2, tokenlist, errorLogger, settings);
                valueFlowForLoopSimplifyAfter(tok, varid, numAfter, tokenlist, errorLogger, settings);
            } else
                valueFlowForLoopSimplifyAfter(tok, varid, num1, tokenlist, errorLogger, settings);
        } else {
            ProgramMemory mem1, mem2, memAfter;
            if (valueFlowForLoop2(tok, &mem1, &mem2, &memAfter)) {
                std::map<unsigned int, ValueFlow::Value>::const_iterator it;
                for (it = mem1.values.begin(); it != mem1.values.end(); ++it) {
                    if (!it->second.isIntValue())
                        continue;
                    valueFlowForLoopSimplify(bodyStart, it->first, false, it->second.intvalue, tokenlist, errorLogger, settings);
                }
                for (it = mem2.values.begin(); it != mem2.values.end(); ++it) {
                    if (!it->second.isIntValue())
                        continue;
                    valueFlowForLoopSimplify(bodyStart, it->first, false, it->second.intvalue, tokenlist, errorLogger, settings);
                }
                for (it = memAfter.values.begin(); it != memAfter.values.end(); ++it) {
                    if (!it->second.isIntValue())
                        continue;
                    valueFlowForLoopSimplifyAfter(tok, it->first, it->second.intvalue, tokenlist, errorLogger, settings);
                }
            }
        }
    }
}

static void valueFlowInjectParameter(TokenList* tokenlist, ErrorLogger* errorLogger, const Settings* settings, const Variable* arg, const Scope* functionScope, const std::list<ValueFlow::Value>& argvalues)
{
    // Is argument passed by value or const reference, and is it a known non-class type?
    if (arg->isReference() && !arg->isConst() && !arg->isClass())
        return;

    // Set value in function scope..
    const unsigned int varid2 = arg->declarationId();
    if (!varid2)
        return;

    valueFlowForward(const_cast<Token*>(functionScope->bodyStart->next()), functionScope->bodyEnd, arg, varid2, argvalues, false, true, tokenlist, errorLogger, settings);
}

static void valueFlowSwitchVariable(TokenList *tokenlist, SymbolDatabase* symboldatabase, ErrorLogger *errorLogger, const Settings *settings)
{
    for (const Scope &scope : symboldatabase->scopeList) {
        if (scope.type != Scope::ScopeType::eSwitch)
            continue;
        if (!Token::Match(scope.classDef, "switch ( %var% ) {"))
            continue;
        const Token *vartok = scope.classDef->tokAt(2);
        const Variable *var = vartok->variable();
        if (!var)
            continue;

        // bailout: global non-const variables
        if (!(var->isLocal() || var->isArgument()) && !var->isConst()) {
            if (settings->debugwarnings)
                bailout(tokenlist, errorLogger, vartok, "switch variable " + var->name() + " is global");
            continue;
        }

        for (Token *tok = scope.bodyStart->next(); tok != scope.bodyEnd; tok = tok->next()) {
            if (tok->str() == "{") {
                tok = tok->link();
                continue;
            }
            if (Token::Match(tok, "case %num% :")) {
                std::list<ValueFlow::Value> values;
                values.emplace_back(MathLib::toLongNumber(tok->next()->str()));
                values.back().condition = tok;
                const std::string info("case " + tok->next()->str() + ": " + vartok->str() + " is " + tok->next()->str() + " here.");
                values.back().errorPath.emplace_back(tok, info);
                bool known = false;
                if ((Token::simpleMatch(tok->previous(), "{") || Token::simpleMatch(tok->tokAt(-2), "break ;")) && !Token::Match(tok->tokAt(3), ";| case"))
                    known = true;
                while (Token::Match(tok->tokAt(3), ";| case %num% :")) {
                    known = false;
                    tok = tok->tokAt(3);
                    if (!tok->isName())
                        tok = tok->next();
                    values.emplace_back(MathLib::toLongNumber(tok->next()->str()));
                    values.back().condition = tok;
                    const std::string info2("case " + tok->next()->str() + ": " + vartok->str() + " is " + tok->next()->str() + " here.");
                    values.back().errorPath.emplace_back(tok, info2);
                }
                for (std::list<ValueFlow::Value>::const_iterator val = values.begin(); val != values.end(); ++val) {
                    valueFlowReverse(tokenlist,
                                     const_cast<Token*>(scope.classDef),
                                     vartok,
                                     *val,
                                     ValueFlow::Value(),
                                     errorLogger,
                                     settings);
                }
                if (vartok->variable()->scope()) {
                    if (known)
                        values.back().setKnown();
                    valueFlowForward(tok->tokAt(3), vartok->variable()->scope()->bodyEnd, vartok->variable(), vartok->varId(), values, values.back().isKnown(), false, tokenlist, errorLogger, settings);
                }
            }
        }
    }
}

static void setTokenValues(Token *tok, const std::list<ValueFlow::Value> &values, const Settings *settings)
{
    for (const ValueFlow::Value &value : values) {
        if (value.isIntValue())
            setTokenValue(tok, value, settings);
    }
}

static bool evaluate(const Token *expr, const std::vector<std::list<ValueFlow::Value>> &values, std::list<ValueFlow::Value> *result)
{
    if (!expr)
        return false;

    // strlen(arg)..
    if (expr->str() == "(" && Token::Match(expr->previous(), "strlen ( %name% )")) {
        const Token *arg = expr->next();
        if (arg->str().compare(0,3,"arg") != 0 || arg->str().size() != 4)
            return false;
        const char n = arg->str()[3];
        if (n < '1' || n - '1' >= values.size())
            return false;
        for (const ValueFlow::Value &argvalue : values[n - '1']) {
            if (argvalue.isTokValue() && argvalue.tokvalue->tokType() == Token::eString) {
                ValueFlow::Value res(argvalue); // copy all "inconclusive", "condition", etc attributes
                // set return value..
                res.valueType = ValueFlow::Value::INT;
                res.tokvalue = nullptr;
                res.intvalue = Token::getStrLength(argvalue.tokvalue);
                result->emplace_back(res);
            }
        }
        return !result->empty();
    }

    // unary operands
    if (expr->astOperand1() && !expr->astOperand2()) {
        std::list<ValueFlow::Value> opvalues;
        if (!evaluate(expr->astOperand1(), values, &opvalues))
            return false;
        if (expr->str() == "+") {
            result->swap(opvalues);
            return true;
        }
        if (expr->str() == "-") {
            for (ValueFlow::Value v: opvalues) {
                if (v.isIntValue()) {
                    v.intvalue = -v.intvalue;
                    result->emplace_back(v);
                }
            }
            return true;
        }
        return false;
    }
    // binary/ternary operands
    if (expr->astOperand1() && expr->astOperand2()) {
        std::list<ValueFlow::Value> lhsValues, rhsValues;
        if (!evaluate(expr->astOperand1(), values, &lhsValues))
            return false;
        if (expr->str() != "?" && !evaluate(expr->astOperand2(), values, &rhsValues))
            return false;

        for (const ValueFlow::Value &val1 : lhsValues) {
            if (!val1.isIntValue())
                continue;
            if (expr->str() == "?") {
                rhsValues.clear();
                const Token *expr2 = val1.intvalue ? expr->astOperand2()->astOperand1() : expr->astOperand2()->astOperand2();
                if (!evaluate(expr2, values, &rhsValues))
                    continue;
                result->insert(result->end(), rhsValues.begin(), rhsValues.end());
                continue;
            }

            for (const ValueFlow::Value &val2 : rhsValues) {
                if (!val2.isIntValue())
                    continue;

                if (val1.varId != 0 && val2.varId != 0) {
                    if (val1.varId != val2.varId || val1.varvalue != val2.varvalue)
                        continue;
                }

                if (expr->str() == "+")
                    result->emplace_back(ValueFlow::Value(val1.intvalue + val2.intvalue));
                else if (expr->str() == "-")
                    result->emplace_back(ValueFlow::Value(val1.intvalue - val2.intvalue));
                else if (expr->str() == "*")
                    result->emplace_back(ValueFlow::Value(val1.intvalue * val2.intvalue));
                else if (expr->str() == "/" && val2.intvalue != 0)
                    result->emplace_back(ValueFlow::Value(val1.intvalue / val2.intvalue));
                else if (expr->str() == "%" && val2.intvalue != 0)
                    result->emplace_back(ValueFlow::Value(val1.intvalue % val2.intvalue));
                else if (expr->str() == "&")
                    result->emplace_back(ValueFlow::Value(val1.intvalue & val2.intvalue));
                else if (expr->str() == "|")
                    result->emplace_back(ValueFlow::Value(val1.intvalue | val2.intvalue));
                else if (expr->str() == "^")
                    result->emplace_back(ValueFlow::Value(val1.intvalue ^ val2.intvalue));
                else if (expr->str() == "==")
                    result->emplace_back(ValueFlow::Value(val1.intvalue == val2.intvalue));
                else if (expr->str() == "!=")
                    result->emplace_back(ValueFlow::Value(val1.intvalue != val2.intvalue));
                else if (expr->str() == "<")
                    result->emplace_back(ValueFlow::Value(val1.intvalue < val2.intvalue));
                else if (expr->str() == ">")
                    result->emplace_back(ValueFlow::Value(val1.intvalue > val2.intvalue));
                else if (expr->str() == ">=")
                    result->emplace_back(ValueFlow::Value(val1.intvalue >= val2.intvalue));
                else if (expr->str() == "<=")
                    result->emplace_back(ValueFlow::Value(val1.intvalue <= val2.intvalue));
                else if (expr->str() == "&&")
                    result->emplace_back(ValueFlow::Value(val1.intvalue && val2.intvalue));
                else if (expr->str() == "||")
                    result->emplace_back(ValueFlow::Value(val1.intvalue || val2.intvalue));
                else if (expr->str() == "<<")
                    result->emplace_back(ValueFlow::Value(val1.intvalue << val2.intvalue));
                else if (expr->str() == ">>")
                    result->emplace_back(ValueFlow::Value(val1.intvalue >> val2.intvalue));
                else
                    return false;
                combineValueProperties(val1, val2, &result->back());
            }
        }
        return !result->empty();
    }
    if (expr->str().compare(0,3,"arg")==0) {
        *result = values[expr->str()[3] - '1'];
        return true;
    }
    if (expr->isNumber()) {
        result->emplace_back(ValueFlow::Value(MathLib::toLongNumber(expr->str())));
        result->back().setKnown();
        return true;
    } else if (expr->tokType() == Token::eChar) {
        result->emplace_back(ValueFlow::Value(MathLib::toLongNumber(expr->str())));
        result->back().setKnown();
        return true;
    }
    return false;
}

static std::list<ValueFlow::Value> getFunctionArgumentValues(const Token *argtok)
{
    std::list<ValueFlow::Value> argvalues(argtok->values());
    if (argvalues.empty() && Token::Match(argtok, "%comp%|%oror%|&&|!")) {
        argvalues.emplace_back(0);
        argvalues.emplace_back(1);
    }
    return argvalues;
}

static void valueFlowLibraryFunction(Token *tok, const std::string &returnValue, const Settings *settings)
{
    std::vector<std::list<ValueFlow::Value>> argValues;
    for (const Token *argtok : getArguments(tok->previous())) {
        argValues.emplace_back(getFunctionArgumentValues(argtok));
        if (argValues.back().empty())
            return;
    }
    if (returnValue.find("arg") != std::string::npos && argValues.empty())
        return;

    TokenList tokenList(settings);
    {
        const std::string code = "return " + returnValue + ";";
        std::istringstream istr(code);
        if (!tokenList.createTokens(istr))
            return;
    }

    // combine operators, set links, etc..
    std::stack<Token *> lpar;
    for (Token *tok2 = tokenList.front(); tok2; tok2 = tok2->next()) {
        if (Token::Match(tok2, "[!<>=] =")) {
            tok2->str(tok2->str() + "=");
            tok2->deleteNext();
        } else if (tok2->str() == "(")
            lpar.push(tok2);
        else if (tok2->str() == ")") {
            if (lpar.empty())
                return;
            Token::createMutualLinks(lpar.top(), tok2);
            lpar.pop();
        }
    }
    if (!lpar.empty())
        return;

    // Evaluate expression
    tokenList.createAst();
    std::list<ValueFlow::Value> results;
    if (evaluate(tokenList.front()->astOperand1(), argValues, &results))
        setTokenValues(tok, results, settings);
}

static void valueFlowSubFunction(TokenList *tokenlist, const Settings *settings)
{
    for (Token *tok = tokenlist->front(); tok; tok = tok->next()) {
        if (!Token::Match(tok, "%name% ("))
            continue;

        const Function * const calledFunction = tok->function();
        if (!calledFunction) {
            // library function?
            const std::string& returnValue(settings->library.returnValue(tok));
            if (!returnValue.empty())
                valueFlowLibraryFunction(tok->next(), returnValue, settings);
            continue;
        }

        const Scope * const calledFunctionScope = calledFunction->functionScope;
        if (!calledFunctionScope)
            continue;

        // TODO: Rewrite this. It does not work well to inject 1 argument at a time.
        const std::vector<const Token *> &callArguments = getArguments(tok);
        for (unsigned int argnr = 0U; argnr < callArguments.size(); ++argnr) {
            const Token *argtok = callArguments[argnr];
            // Get function argument
            const Variable * const argvar = calledFunction->getArgumentVar(argnr);
            if (!argvar)
                break;

            // passing value(s) to function
            std::list<ValueFlow::Value> argvalues(getFunctionArgumentValues(argtok));

            // Don't forward lifetime values
            argvalues.remove_if(std::mem_fn(&ValueFlow::Value::isLifetimeValue));

            if (argvalues.empty())
                continue;

            // Error path..
            for (ValueFlow::Value &v : argvalues) {
                const std::string nr = MathLib::toString(argnr + 1) + getOrdinalText(argnr + 1);

                v.errorPath.emplace_back(argtok,
                                         "Calling function '" +
                                         calledFunction->name() +
                                         "', " +
                                         nr +
                                         " argument '" +
                                         argtok->expressionString() +
                                         "' value is " +
                                         v.infoString());
            }

            // passed values are not "known"..
            changeKnownToPossible(argvalues);

            // FIXME: We need to rewrite the valueflow analysis of function calls. This does not work well.
            //valueFlowInjectParameter(tokenlist, errorLogger, settings, argvar, calledFunctionScope, argvalues);
        }
    }
}

static void valueFlowFunctionDefaultParameter(TokenList *tokenlist, SymbolDatabase* symboldatabase, ErrorLogger *errorLogger, const Settings *settings)
{
    for (const Scope* scope : symboldatabase->functionScopes) {
        const Function* function = scope->function;
        if (!function)
            continue;
        for (std::size_t arg = function->minArgCount(); arg < function->argCount(); arg++) {
            const Variable* var = function->getArgumentVar(arg);
            if (var && var->hasDefault() && Token::Match(var->nameToken(), "%var% = %num%|%str% [,)]")) {
                const std::list<ValueFlow::Value> &values = var->nameToken()->tokAt(2)->values();
                std::list<ValueFlow::Value> argvalues;
                for (const ValueFlow::Value &value : values) {
                    ValueFlow::Value v(value);
                    v.defaultArg = true;
                    v.changeKnownToPossible();
                    if (v.isPossible())
                        argvalues.push_back(v);
                }
                if (!argvalues.empty())
                    valueFlowInjectParameter(tokenlist, errorLogger, settings, var, scope, argvalues);
            }
        }
    }
}

static bool isKnown(const Token * tok)
{
    return tok && tok->hasKnownIntValue();
}

static void valueFlowFunctionReturn(TokenList *tokenlist, ErrorLogger *errorLogger)
{
    for (Token *tok = tokenlist->back(); tok; tok = tok->previous()) {
        if (tok->str() != "(" || !tok->astOperand1() || !tok->astOperand1()->function())
            continue;

        if (tok->hasKnownValue())
            continue;

        // Arguments..
        std::vector<MathLib::bigint> parvalues;
        if (tok->astOperand2()) {
            const Token *partok = tok->astOperand2();
            while (partok && partok->str() == "," && isKnown(partok->astOperand2()))
                partok = partok->astOperand1();
            if (!isKnown(partok))
                continue;
            parvalues.push_back(partok->values().front().intvalue);
            partok = partok->astParent();
            while (partok && partok->str() == ",") {
                parvalues.push_back(partok->astOperand2()->values().front().intvalue);
                partok = partok->astParent();
            }
            if (partok != tok)
                continue;
        }

        // Get scope and args of function
        const Function * const function = tok->astOperand1()->function();
        const Scope * const functionScope = function->functionScope;
        if (!functionScope || !Token::simpleMatch(functionScope->bodyStart, "{ return")) {
            if (functionScope && tokenlist->getSettings()->debugwarnings && Token::findsimplematch(functionScope->bodyStart, "return", functionScope->bodyEnd))
                bailout(tokenlist, errorLogger, tok, "function return; nontrivial function body");
            continue;
        }

        ProgramMemory programMemory;
        for (std::size_t i = 0; i < parvalues.size(); ++i) {
            const Variable * const arg = function->getArgumentVar(i);
            if (!arg || !Token::Match(arg->typeStartToken(), "%type% %name% ,|)")) {
                if (tokenlist->getSettings()->debugwarnings)
                    bailout(tokenlist, errorLogger, tok, "function return; unhandled argument type");
                programMemory.clear();
                break;
            }
            programMemory.setIntValue(arg->declarationId(), parvalues[i]);
        }
        if (programMemory.empty() && !parvalues.empty())
            continue;

        // Determine return value of subfunction..
        MathLib::bigint result = 0;
        bool error = false;
        execute(functionScope->bodyStart->next()->astOperand1(),
                &programMemory,
                &result,
                &error);
        if (!error) {
            ValueFlow::Value v(result);
            v.setKnown();
            setTokenValue(tok, v, tokenlist->getSettings());
        }
    }
}

static void valueFlowUninit(TokenList *tokenlist, SymbolDatabase * /*symbolDatabase*/, ErrorLogger *errorLogger, const Settings *settings)
{
    for (Token *tok = tokenlist->front(); tok; tok = tok->next()) {
        if (!Token::Match(tok,"[;{}] %type%"))
            continue;
        if (!tok->scope()->isExecutable())
            continue;
        const Token *vardecl = tok->next();
        bool stdtype = false;
        bool pointer = false;
        while (Token::Match(vardecl, "%name%|::|*") && vardecl->varId() == 0) {
            stdtype |= vardecl->isStandardType();
            pointer |= vardecl->str() == "*";
            vardecl = vardecl->next();
        }
        if (!stdtype && !pointer)
            continue;
        if (!Token::Match(vardecl, "%var% ;"))
            continue;
        if (Token::Match(vardecl, "%varid% ; %varid% =", vardecl->varId()))
            continue;
        const Variable *var = vardecl->variable();
        if (!var || var->nameToken() != vardecl)
            continue;
        if ((var->type() && var->type()->needInitialization != Type::True) ||
            !var->isLocal() || var->isStatic() || var->isReference())
            continue;

        ValueFlow::Value uninitValue;
        uninitValue.setKnown();
        uninitValue.valueType = ValueFlow::Value::UNINIT;
        std::list<ValueFlow::Value> values;
        values.push_back(uninitValue);

        const bool constValue = true;
        const bool subFunction = false;

        valueFlowForward(vardecl->next(), vardecl->scope()->bodyEnd, var, vardecl->varId(), values, constValue, subFunction, tokenlist, errorLogger, settings);
    }
}

static void valueFlowFwdAnalysis(const TokenList *tokenlist, const Settings *settings)
{
    for (const Token *tok = tokenlist->front(); tok; tok = tok->next())
    {
        if (tok->str() != "=" || !tok->astOperand1() || !tok->astOperand2())
            continue;
        if (!tok->scope()->isExecutable())
            continue;
        if (!tok->astOperand2()->hasKnownIntValue())
            continue;
        ValueFlow::Value v(tok->astOperand2()->values().front());
        v.errorPath.emplace_back(tok, tok->astOperand1()->expressionString() + " is assigned value " + MathLib::toString(v.intvalue));
        FwdAnalysis fwdAnalysis(settings->library);
        const Token *startToken = tok->findExpressionStartEndTokens().second->next();
        const Scope *functionScope = tok->scope();
        while (functionScope->nestedIn && functionScope->nestedIn->isExecutable())
            functionScope = functionScope->nestedIn;
        const Token *endToken = functionScope->bodyEnd;
        for (const FwdAnalysis::KnownAndToken read : fwdAnalysis.valueFlow(tok->astOperand1(), startToken, endToken)) {
            v.valueKind = read.known ? ValueFlow::Value::ValueKind::Known : ValueFlow::Value::ValueKind::Possible;
            setTokenValue(const_cast<Token *>(read.token), v, settings);
        }
    }
}

ValueFlow::Value::Value(const Token *c, long long val)
    : valueType(INT),
      intvalue(val),
      tokvalue(nullptr),
      floatValue(0.0),
      varvalue(val),
      condition(c),
      varId(0U),
      conditional(false),
      defaultArg(false),
      lifetimeKind(Object),
      lifetimeScope(Local),
      valueKind(ValueKind::Possible)
{
    errorPath.emplace_back(c, "Assuming that condition '" + c->expressionString() + "' is not redundant");
}

std::string ValueFlow::Value::infoString() const
{
    switch (valueType) {
    case INT:
        return MathLib::toString(intvalue);
    case TOK:
        return tokvalue->str();
    case FLOAT:
        return MathLib::toString(floatValue);
    case UNINIT:
        return "<Uninit>";
    case LIFETIME:
        return "lifetime=" + tokvalue->str();
    };
    throw InternalError(nullptr, "Invalid ValueFlow Value type");
}

const ValueFlow::Value *ValueFlow::valueFlowConstantFoldAST(const Token *expr, const Settings *settings)
{
    if (expr && expr->values().empty()) {
        valueFlowConstantFoldAST(expr->astOperand1(), settings);
        valueFlowConstantFoldAST(expr->astOperand2(), settings);
        valueFlowSetConstantValue(expr, settings);
    }
    return expr && expr->hasKnownValue() ? &expr->values().front() : nullptr;
}

static std::size_t getTotalValues(TokenList *tokenlist)
{
    std::size_t n = 1;
    for (Token *tok = tokenlist->front(); tok; tok = tok->next())
        n += tok->values().size();
    return n;
}

void ValueFlow::setValues(TokenList *tokenlist, SymbolDatabase* symboldatabase, ErrorLogger *errorLogger, const Settings *settings)
{
    for (Token *tok = tokenlist->front(); tok; tok = tok->next())
        tok->clearValueFlow();

    valueFlowNumber(tokenlist);
    valueFlowString(tokenlist);
    valueFlowArray(tokenlist);
    valueFlowGlobalStaticVar(tokenlist, settings);
    valueFlowPointerAlias(tokenlist);
    valueFlowLifetime(tokenlist, symboldatabase, errorLogger, settings);
    valueFlowFunctionReturn(tokenlist, errorLogger);
    valueFlowBitAnd(tokenlist);
    valueFlowSameExpressions(tokenlist);
    valueFlowFwdAnalysis(tokenlist, settings);

    // Temporary hack.. run valueflow until there is nothing to update or timeout expires
    const std::time_t timeout = std::time(0) + TIMEOUT;
    std::size_t values = 0;
    while (std::time(0) < timeout && values < getTotalValues(tokenlist)) {
        values = getTotalValues(tokenlist);
        valueFlowArrayBool(tokenlist);
        valueFlowRightShift(tokenlist, settings);
        valueFlowOppositeCondition(symboldatabase, settings);
        valueFlowBeforeCondition(tokenlist, symboldatabase, errorLogger, settings);
        valueFlowAfterAssign(tokenlist, symboldatabase, errorLogger, settings);
        valueFlowAfterCondition(tokenlist, symboldatabase, errorLogger, settings);
        valueFlowSwitchVariable(tokenlist, symboldatabase, errorLogger, settings);
        valueFlowForLoop(tokenlist, symboldatabase, errorLogger, settings);
        valueFlowSubFunction(tokenlist, settings);
        valueFlowFunctionDefaultParameter(tokenlist, symboldatabase, errorLogger, settings);
        valueFlowUninit(tokenlist, symboldatabase, errorLogger, settings);
    }
}


std::string ValueFlow::eitherTheConditionIsRedundant(const Token *condition)
{
    if (!condition)
        return "Either the condition is redundant";
    if (condition->str() == "case") {
        std::string expr;
        for (const Token *tok = condition; tok && tok->str() != ":"; tok = tok->next()) {
            expr += tok->str();
            if (Token::Match(tok, "%name%|%num% %name%|%num%"))
                expr += ' ';
        }
        return "Either the switch case '" + expr + "' is redundant";
    }
    return "Either the condition '" + condition->expressionString() + "' is redundant";
}
