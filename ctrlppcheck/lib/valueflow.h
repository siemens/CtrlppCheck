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
#ifndef valueflowH
#define valueflowH
//---------------------------------------------------------------------------

#include "config.h"

#include <list>
#include <string>
#include <utility>

class ErrorLogger;
class Settings;
class SymbolDatabase;
class Token;
class TokenList;
class Variable;

namespace ValueFlow {
    class CPPCHECKLIB Value {
    public:
        typedef std::pair<const Token *, std::string> ErrorPathItem;
        typedef std::list<ErrorPathItem> ErrorPath;

        explicit Value(long long val = 0)
            : valueType(INT),
              intvalue(val),
              tokvalue(nullptr),
              floatValue(0.0),
              varvalue(val),
              condition(nullptr),
              varId(0U),
              conditional(false),
              defaultArg(false),
              lifetimeKind(Object),
              lifetimeScope(Local),
              valueKind(ValueKind::Possible)
        {}
        Value(const Token *c, long long val);

        bool operator==(const Value &rhs) const {
            if (valueType != rhs.valueType)
                return false;
            switch (valueType) {
            case INT:
                if (intvalue != rhs.intvalue)
                    return false;
                break;
            case TOK:
                if (tokvalue != rhs.tokvalue)
                    return false;
                break;
            case FLOAT:
                // TODO: Write some better comparison
                if (floatValue > rhs.floatValue || floatValue < rhs.floatValue)
                    return false;
                break;
            case UNINIT:
                break;
            case LIFETIME:
                if (tokvalue != rhs.tokvalue)
                    return false;
            };

            return varvalue == rhs.varvalue &&
                   condition == rhs.condition &&
                   varId == rhs.varId &&
                   conditional == rhs.conditional &&
                   defaultArg == rhs.defaultArg &&
                   valueKind == rhs.valueKind;
        }

        std::string infoString() const;

        enum ValueType { INT, TOK, FLOAT, UNINIT, LIFETIME } valueType;
        bool isIntValue() const {
            return valueType == INT;
        }
        bool isTokValue() const {
            return valueType == TOK;
        }
        bool isFloatValue() const {
            return valueType == FLOAT;
        }
        bool isUninitValue() const {
            return valueType == UNINIT;
        }

        //@todo can ve remove it or used in ctrl 'time' variable type?
        bool isLifetimeValue() const {
            return valueType == LIFETIME;
        }

        bool isLocalLifetimeValue() const {
            return valueType == LIFETIME && lifetimeScope == Local;
        }

        bool isArgumentLifetimeValue() const {
            return valueType == LIFETIME && lifetimeScope == Argument;
        }

        /** int value */
        long long intvalue;

        /** token value - the token that has the value. this is used for pointer aliases, strings, etc. */
        const Token *tokvalue;

        /** float value */
        double floatValue;

        /** For calculated values - variable value that calculated value depends on */
        long long varvalue;

        /** Condition that this value depends on */
        const Token *condition;

        ErrorPath errorPath;

        /** For calculated values - varId that calculated value depends on */
        unsigned int varId;

        /** Conditional value */
        bool conditional;

        /** Is this value passed as default parameter to the function? */
        bool defaultArg;

        enum LifetimeKind {Object, Lambda, Iterator} lifetimeKind;

        enum LifetimeScope { Local, Argument } lifetimeScope;

        /** How known is this value */
        enum class ValueKind {
            /** This value is possible, other unlisted values may also be possible */
            Possible,
            /** Only listed values are possible */
            Known,
            /** Inconclusive */
            Inconclusive
        } valueKind;

        void setKnown() {
            valueKind = ValueKind::Known;
        }

        bool isKnown() const {
            return valueKind == ValueKind::Known;
        }

        void setPossible() {
            valueKind = ValueKind::Possible;
        }

        bool isPossible() const {
            return valueKind == ValueKind::Possible;
        }

        void setInconclusive(bool inconclusive = true) {
            if (inconclusive)
                valueKind = ValueKind::Inconclusive;
        }

        bool isInconclusive() const {
            return valueKind == ValueKind::Inconclusive;
        }

        void changeKnownToPossible() {
            if (isKnown())
                valueKind = ValueKind::Possible;
        }

        bool errorSeverity() const {
            return !condition && !defaultArg;
        }
    };

    /// Constant folding of expression. This can be used before the full ValueFlow has been executed (ValueFlow::setValues).
    const ValueFlow::Value * valueFlowConstantFoldAST(const Token *expr, const Settings *settings);

    /// Perform valueflow analysis.
    void setValues(TokenList *tokenlist, SymbolDatabase* symboldatabase, ErrorLogger *errorLogger, const Settings *settings);

    std::string eitherTheConditionIsRedundant(const Token *condition);
}

const Variable *getLifetimeVariable(const Token *tok, ValueFlow::Value::ErrorPath &errorPath);

std::string lifetimeType(const Token *tok, const ValueFlow::Value *val);

#endif // valueflowH
