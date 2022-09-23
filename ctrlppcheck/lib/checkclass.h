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
#ifndef checkclassH
#define checkclassH
//---------------------------------------------------------------------------

#include "check.h"
#include "config.h"
#include "tokenize.h"

#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>

class ErrorLogger;
class Function;
class Scope;
class Settings;
class SymbolDatabase;
class Token;

/// @addtogroup Checks
/// @{


/** @brief %Check classes. Uninitialized member variables, non-conforming operators, etc */
class CPPCHECKLIB CheckClass : public Check {
public:
    /** @brief This constructor is used when registering the CheckClass */
    CheckClass() : Check(myName()), mSymbolDatabase(nullptr) {
    }

    /** @brief This constructor is used when running checks. */
    CheckClass(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger);

    /** @brief Run checks on the normal token list */
    void runChecks(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger) OVERRIDE {
        CheckClass checkClass(tokenizer, settings, errorLogger);

        // can't be a simplified check .. the 'sizeof' is used.
        checkClass.checkUnsafeClassDivZero();
        checkClass.constructors();
        checkClass.privateFunctions();
        checkClass.operatorEqRetRefThis();
        checkClass.thisSubtraction();
        checkClass.operatorEqToSelf();
        checkClass.initializerListOrder();
        checkClass.initializationListUsage();
        checkClass.checkSelfInitialization();
        checkClass.checkConst();
        checkClass.checkDuplInheritedMembers();
    }

    /** @brief Run checks on the simplified token list */
    void runSimplifiedChecks(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger) OVERRIDE {
        (void)tokenizer;
        (void)settings;
        (void)errorLogger;
    }


    /** @brief %Check that all class constructors are ok */
    void constructors();

    /** @brief %Check that all private functions are called */
    void privateFunctions();

    /** @brief 'operator=' should return reference to *this */
    void operatorEqRetRefThis();    // Warning upon no "return *this;"

    /** @brief 'operator=' should check for assignment to self */
    void operatorEqToSelf();    // Warning upon no check for assignment to self

    /** @brief warn for "this-x". The indented code may be "this->x"  */
    void thisSubtraction();

    /** @brief can member function be const? */
    void checkConst();

    /** @brief Check initializer list order */
    void initializerListOrder();

    /** @brief Suggest using initialization list */
    void initializationListUsage();

    /** @brief Check for initialization of a member with itself */
    void checkSelfInitialization();

    /** @brief Check duplicated inherited members */
    void checkDuplInheritedMembers();

    /** @brief Check that arbitrary usage of the public interface does not result in division by zero */
    void checkUnsafeClassDivZero(bool test=false);

private:
    const SymbolDatabase *mSymbolDatabase;

    // Reporting errors..
    void noConstructorError(const Token *tok, const std::string &classname, bool isStruct);
    void unusedPrivateFunctionError(const Token *tok, const std::string &classname, const std::string &funcname);
    void thisSubtractionError(const Token *tok);
    void operatorEqRetRefThisError(const Token *tok);
    void operatorEqShouldBeLeftUnimplementedError(const Token *tok);
    void operatorEqMissingReturnStatementError(const Token *tok, bool error);
    void operatorEqToSelfError(const Token *tok);
    void checkConstError(const Token *tok, const std::string &classname, const std::string &funcname, bool suggestStatic);
    void checkConstError2(const Token *tok1, const Token *tok2, const std::string &classname, const std::string &funcname, bool suggestStatic);
    void initializerListError(const Token *tok1,const Token *tok2, const std::string & classname, const std::string &varname);
    void suggestInitializationList(const Token *tok, const std::string& varname);
    void selfInitializationError(const Token* tok, const std::string& varname);
    void duplInheritedMembersError(const Token* tok1, const Token* tok2, const std::string &derivedName, const std::string &baseName, const std::string &variableName, bool derivedIsStruct, bool baseIsStruct);
    void unsafeClassDivZeroError(const Token *tok, const std::string &className, const std::string &methodName, const std::string &varName);

    void getErrorMessages(ErrorLogger *errorLogger, const Settings *settings) const OVERRIDE {
        CheckClass c(nullptr, settings, errorLogger);
        c.noConstructorError(nullptr, "classname", false);
        //c.copyConstructorMallocError(nullptr, 0, "var");
        c.unusedPrivateFunctionError(nullptr, "classname", "funcname");
        c.thisSubtractionError(nullptr);
        c.operatorEqRetRefThisError(nullptr);
        c.operatorEqMissingReturnStatementError(nullptr, true);
        c.operatorEqShouldBeLeftUnimplementedError(nullptr);
        c.operatorEqToSelfError(nullptr);
        c.checkConstError(nullptr, "class", "function", false);
        c.checkConstError(nullptr, "class", "function", true);
        c.initializerListError(nullptr, nullptr, "class", "variable");
        c.suggestInitializationList(nullptr, "variable");
        c.selfInitializationError(nullptr, "var");
        c.duplInheritedMembersError(nullptr, nullptr, "class", "class", "variable", false, false);
        c.unsafeClassDivZeroError(nullptr, "Class", "dostuff", "x");
    }

    static std::string myName() {
        return "Class";
    }

    std::string classInfo() const OVERRIDE {
        return "Check the code for each class.\n"
               "- Missing constructors and copy constructors\n"
               //"- Missing allocation of memory in copy constructor\n"
               "- Constructors which should be explicit\n"
               "- Are all variables initialized by the constructors?\n"
               "- Are all variables assigned by 'operator='?\n"
               "- Warn if memset, memcpy etc are used on a class\n"
               "- Warn if memory for classes is allocated with malloc()\n"
               "- Are there unused private functions?\n"
               "- 'operator=' should return reference to self\n"
               "- 'operator=' should check for assignment to self\n"
               "- Constness for member functions\n"
               "- Order of initializations\n"
               "- Suggest usage of initialization list\n"
               "- Initialization of a member with itself\n"
               "- Suspicious subtraction from 'this'\n"
               "- Duplicated inherited data members\n"
               // disabled for now "- If 'copy constructor' defined, 'operator=' also should be defined and vice versa\n"
               "- Check that arbitrary usage of public interface does not result in division by zero\n"
               "- Check that the 'override' keyword is used when overriding virtual functions\n";
    }

    // operatorEqRetRefThis helper functions
    void checkReturnPtrThis(const Scope *scope, const Function *func, const Token *tok, const Token *last);
    void checkReturnPtrThis(const Scope *scope, const Function *func, const Token *tok, const Token *last, std::set<const Function*>& analyzedFunctions);

    // operatorEqToSelf helper functions
    bool hasAllocation(const Function *func, const Scope* scope) const;
    static bool hasAssignSelf(const Function *func, const Token *rhs);

    // checkConst helper functions
    bool isMemberVar(const Scope *scope, const Token *tok) const;
    bool isMemberFunc(const Scope *scope, const Token *tok) const;
    bool checkConstFunc(const Scope *scope, const Function *func, bool& memberAccessed) const;

    // constructors helper function
    /** @brief Information about a member variable. Used when checking for uninitialized variables */
    struct Usage {
        Usage() : assign(false), init(false) { }

        /** @brief has this variable been assigned? */
        bool assign;

        /** @brief has this variable been initialized? */
        bool init;
    };

    static bool isBaseClassFunc(const Token *tok, const Scope *scope);

    /**
     * @brief assign a variable in the varlist
     * @param varid id of variable to mark assigned
     * @param scope pointer to variable Scope
     * @param usage reference to usage vector
     */
    static void assignVar(unsigned int varid, const Scope *scope, std::vector<Usage> &usage);

    /**
     * @brief initialize a variable in the varlist
     * @param varid id of variable to mark initialized
     * @param scope pointer to variable Scope
     * @param usage reference to usage vector
     */
    static void initVar(unsigned int varid, const Scope *scope, std::vector<Usage> &usage);

    /**
     * @brief set all variables in list assigned
     * @param usage reference to usage vector
     */
    static void assignAllVar(std::vector<Usage> &usage);

    /**
     * @brief set all variables in list not assigned and not initialized
     * @param usage reference to usage vector
     */
    static void clearAllVar(std::vector<Usage> &usage);

    /**
     * @brief parse a scope for a constructor or member function and set the "init" flags in the provided varlist
     * @param func reference to the function that should be checked
     * @param callstack the function doesn't look into recursive function calls.
     * @param scope pointer to variable Scope
     * @param usage reference to usage vector
     */
    void initializeVarList(const Function &func, std::list<const Function *> &callstack, const Scope *scope, std::vector<Usage> &usage);
};
/// @}
//---------------------------------------------------------------------------
#endif // checkclassH
