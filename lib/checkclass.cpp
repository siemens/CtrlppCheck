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
#include "checkclass.h"

#include "astutils.h"
#include "errorlogger.h"
#include "library.h"
#include "settings.h"
#include "symbols/symbols.h"
#include "token.h"
#include "tokenize.h"
#include "utils.h"

#include <algorithm>
#include <cstdlib>
#include <stack>
#include <utility>
//---------------------------------------------------------------------------

// Register CheckClass..
namespace {
    CheckClass instance;
}

static const CWE CWE398(398U);  // Indicator of Poor Code Quality
static const CWE CWE665(665U);  // Improper Initialization
static const CWE CWE758(758U);  // Reliance on Undefined, Unspecified, or Implementation-Defined Behavior
static const CWE CWE762(762U);  // Mismatched Memory Management Routines

static bool isVariableCopyNeeded(const Variable &var)
{
    return (var.type() && var.type()->needInitialization == Type::True) || (var.valueType()->type >= ValueType::Type::CHAR);
}

//---------------------------------------------------------------------------

CheckClass::CheckClass(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger)
    : Check(myName(), tokenizer, settings, errorLogger),
      mSymbolDatabase(tokenizer?tokenizer->getSymbolDatabase():nullptr)
{

}

//---------------------------------------------------------------------------
// ClassCheck: Check that all class constructors are ok.
//---------------------------------------------------------------------------

void CheckClass::constructors()
{
    const bool printStyle = mSettings->isEnabled(Settings::STYLE);
    const bool printWarnings = mSettings->isEnabled(Settings::WARNING);
    if (!printStyle && !printWarnings)
        return;

    const bool printInconclusive = mSettings->inconclusive;
    for (const Scope * scope : mSymbolDatabase->classAndStructScopes) {

        bool usedInUnion = false;
        for (const Scope &unionScope : mSymbolDatabase->scopeList) {
            for (const Variable &var : unionScope.varlist) {
                if (var.type() && var.type()->classScope == scope) {
                    usedInUnion = true;
                    break;
                }
            }
        }

        // There are no constructors.
        if (scope->numConstructors == 0 && printStyle && !usedInUnion) {
            // If there is a private variable, there should be a constructor..
            for (const Variable &var : scope->varlist) {
                const Token *initTok = var.nameToken();
                while (Token::simpleMatch(initTok->next(), "["))
                    initTok = initTok->linkAt(1);
                if (var.isPrivate() && !var.isStatic() && !Token::Match(var.nameToken(), "%varid% ; %varid% =", var.declarationId()) &&
                    !Token::Match(initTok, "%var%|] {|=") &&
                    (!var.isClass() || (var.type() && var.type()->needInitialization == Type::True))) {
                    noConstructorError(scope->classDef, scope->className, scope->classDef->str() == "struct");
                    break;
                }
            }
        }

        if (!printWarnings)
            continue;

        std::vector<Usage> usage(scope->varlist.size());

        for (const Function &func : scope->functionList) {
            if (!func.hasBody() || !func.isConstructor())
                continue;

            // Mark all variables not used
            clearAllVar(usage);

            std::list<const Function *> callstack;
            initializeVarList(func, callstack, scope, usage);

            // Check if any variables are uninitialized
            int count = -1;
            for (const Variable &var : scope->varlist) {
                ++count;

                // check for C++11 initializer
                if (var.hasDefault()) {
                    usage[count].init = true;
                    continue;
                }

                if (usage[count].assign || usage[count].init || var.isStatic())
                    continue;

                if ( var.type() && var.type()->needInitialization == Type::False && var.type()->derivedFrom.empty())
                    continue;

                if (var.isConst()) // We can't set const members in assignment operator
                    continue;

                // Check if this is a class constructor
                if (var.isClass() && func.type == Function::eConstructor) {
                    // Unknown type so assume it is initialized
                    if (!var.type())
                        continue;

                    // Known type that doesn't need initialization or
                    // known type that has member variables of an unknown type
                    else if (var.type()->needInitialization != Type::True)
                        continue;
                }

                bool inconclusive = false;
                // Don't warn about unknown types in copy constructors since we
                // don't know if they can be copied or not..
                if (!isVariableCopyNeeded(var))
                    inconclusive = true;

                if (!printInconclusive && inconclusive)
                    continue;
            }
        }
    }
}

static bool isNonCopyable(const Scope *scope, bool *unknown)
{
    bool u = false;
    // check if there is base class that is not copyable
    for (const Type::BaseInfo &baseInfo : scope->definedType->derivedFrom) {
        if (!baseInfo.type || !baseInfo.type->classScope) {
            u = true;
            continue;
        }

        if (isNonCopyable(baseInfo.type->classScope, &u))
            return true;

        for (const Function &func : baseInfo.type->classScope->functionList) {
            if (func.access == Private)
                return true;
        }
    }
    *unknown = u;
    return false;
}

void CheckClass::assignVar(unsigned int varid, const Scope *scope, std::vector<Usage> &usage)
{
    unsigned int count = 0;

    for (std::list<Variable>::const_iterator var = scope->varlist.begin(); var != scope->varlist.end(); ++var, ++count) {
        if (var->declarationId() == varid) {
            usage[count].assign = true;
            return;
        }
    }
}

void CheckClass::initVar(unsigned int varid, const Scope *scope, std::vector<Usage> &usage)
{
    unsigned int count = 0;

    for (std::list<Variable>::const_iterator var = scope->varlist.begin(); var != scope->varlist.end(); ++var, ++count) {
        if (var->declarationId() == varid) {
            usage[count].init = true;
            return;
        }
    }
}

void CheckClass::assignAllVar(std::vector<Usage> &usage)
{
    for (std::size_t i = 0; i < usage.size(); ++i)
        usage[i].assign = true;
}

void CheckClass::clearAllVar(std::vector<Usage> &usage)
{
    for (std::size_t i = 0; i < usage.size(); ++i) {
        usage[i].assign = false;
        usage[i].init = false;
    }
}

bool CheckClass::isBaseClassFunc(const Token *tok, const Scope *scope)
{
    // Iterate through each base class...
    for (std::size_t i = 0; i < scope->definedType->derivedFrom.size(); ++i) {
        const Type *derivedFrom = scope->definedType->derivedFrom[i].type;

        // Check if base class exists in database
        if (derivedFrom && derivedFrom->classScope) {
            const std::list<Function>& functionList = derivedFrom->classScope->functionList;

            for (const Function &func : functionList) {
                if (func.tokenDef->str() == tok->str())
                    return true;
            }
        }

        // Base class not found so assume it is in it.
        else
            return true;
    }

    return false;
}

void CheckClass::initializeVarList(const Function &func, std::list<const Function *> &callstack, const Scope *scope, std::vector<Usage> &usage)
{
    if (!func.functionScope)
        throw InternalError(nullptr, "Internal Error: Invalid syntax"); // #5702
    bool initList = func.isConstructor();
    const Token *ftok = func.arg->link()->next();
    int level = 0;
    for (; ftok && ftok != func.functionScope->bodyEnd; ftok = ftok->next()) {
        // Class constructor.. initializing variables like this
        // clKalle::clKalle() : var(value) { }
        if (initList) {
            if (level == 0 && Token::Match(ftok, "%name% {|(") && Token::Match(ftok->linkAt(1), "}|) ,|{")) {
                if (ftok->str() != func.name()) {
                    initVar(ftok->varId(), scope, usage);
                } else { // c++11 delegate constructor
                    const Function *member = ftok->function();
                    // member function not found => assume it initializes all members
                    if (!member) {
                        assignAllVar(usage);
                        return;
                    }

                    // recursive call
                    // assume that all variables are initialized
                    if (std::find(callstack.begin(), callstack.end(), member) != callstack.end()) {
                        /** @todo false negative: just bail */
                        assignAllVar(usage);
                        return;
                    }

                    // member function has implementation
                    if (member->hasBody()) {
                        // initialize variable use list using member function
                        callstack.push_back(member);
                        initializeVarList(*member, callstack, scope, usage);
                        callstack.pop_back();
                    }

                    // there is a called member function, but it has no implementation, so we assume it initializes everything
                    else {
                        assignAllVar(usage);
                    }
                }
            } else if (level != 0 && Token::Match(ftok, "%name% =")) // assignment in the initializer: var(value = x)
                assignVar(ftok->varId(), scope, usage);

            // Level handling
            if (ftok->link() && Token::Match(ftok, "(|<"))
                level++;
            else if (ftok->str() == "{") {
                if (level != 0 ||
                    (Token::Match(ftok->previous(), "%name%|>") && Token::Match(ftok->link(), "} ,|{")))
                    level++;
                else
                    initList = false;
            } else if (ftok->link() && Token::Match(ftok, ")|>|}"))
                level--;
        }

        if (initList)
            continue;

        // Variable getting value from stream?
        if (Token::Match(ftok, ">>|& %name%") && isLikelyStreamRead(ftok)) {
            assignVar(ftok->next()->varId(), scope, usage);
        }

        // If assignment comes after an && or || this is really inconclusive because of short circuiting
        if (Token::Match(ftok, "%oror%|&&"))
            continue;

        if (Token::simpleMatch(ftok, "( !"))
            ftok = ftok->next();

        // Using the operator= function to initialize all variables..
        if (Token::Match(ftok->next(), "return| (| * this )| =")) {
            assignAllVar(usage);
            break;
        }

        // Using swap to assign all variables..
        if (Token::Match(ftok, "[;{}] %name% (") && Token::Match(ftok->linkAt(2), ") . %name% ( *| this ) ;")) {
            assignAllVar(usage);
            break;
        }

        // Calling member variable function?
        if (Token::Match(ftok->next(), "%var% . %name% (")) {
            for (const Variable &var : scope->varlist) {
                if (var.declarationId() == ftok->next()->varId()) {
                    /** @todo false negative: we assume function changes variable state */
                    assignVar(ftok->next()->varId(), scope, usage);
                    break;
                }
            }

            ftok = ftok->tokAt(2);
        }

        if (!Token::Match(ftok->next(), "::| %name%") &&
            !Token::Match(ftok->next(), "*| this . %name%") &&
            !Token::Match(ftok->next(), "* %name% =") &&
            !Token::Match(ftok->next(), "( * this ) . %name%"))
            continue;

        // Goto the first token in this statement..
        ftok = ftok->next();

        // skip "return"
        if (ftok->str() == "return")
            ftok = ftok->next();

        // Skip "( * this )"
        if (Token::simpleMatch(ftok, "( * this ) .")) {
            ftok = ftok->tokAt(5);
        }

        // Skip "this->"
        if (Token::simpleMatch(ftok, "this ."))
            ftok = ftok->tokAt(2);

        // Skip "classname :: "
        if (Token::Match(ftok, ":: %name%"))
            ftok = ftok->next();
        while (Token::Match(ftok, "%name% ::"))
            ftok = ftok->tokAt(2);

        // Clearing all variables..
        if (Token::Match(ftok, "::| memset ( this ,")) {
            assignAllVar(usage);
            return;
        }

        // Ticket #7068
        else if (Token::Match(ftok, "::| memset ( &| this . %name%")) {
            if (ftok->str() == "::")
                ftok = ftok->next();
            int offsetToMember = 4;
            if (ftok->strAt(2) == "&")
                ++offsetToMember;
            assignVar(ftok->tokAt(offsetToMember)->varId(), scope, usage);
            ftok = ftok->linkAt(1);
            continue;
        }

        // Clearing array..
        else if (Token::Match(ftok, "::| memset ( %name% ,")) {
            if (ftok->str() == "::")
                ftok = ftok->next();
            assignVar(ftok->tokAt(2)->varId(), scope, usage);
            ftok = ftok->linkAt(1);
            continue;
        }

        // Calling member function?
        else if (Token::simpleMatch(ftok, "operator= (") &&
                 ftok->previous()->str() != "::") {
            if (ftok->function() && ftok->function()->nestedIn == scope) {
                const Function *member = ftok->function();
                // recursive call
                // assume that all variables are initialized
                if (std::find(callstack.begin(), callstack.end(), member) != callstack.end()) {
                    /** @todo false negative: just bail */
                    assignAllVar(usage);
                    return;
                }

                // member function has implementation
                if (member->hasBody()) {
                    // initialize variable use list using member function
                    callstack.push_back(member);
                    initializeVarList(*member, callstack, scope, usage);
                    callstack.pop_back();
                }

                // there is a called member function, but it has no implementation, so we assume it initializes everything
                else {
                    assignAllVar(usage);
                }
            }

            // using default operator =, assume everything initialized
            else {
                assignAllVar(usage);
            }
        } else if (Token::Match(ftok, "::| %name% (") && !Token::Match(ftok, "if|while|for")) {
            if (ftok->str() == "::")
                ftok = ftok->next();

            // Passing "this" => assume that everything is initialized
            for (const Token *tok2 = ftok->next()->link(); tok2 && tok2 != ftok; tok2 = tok2->previous()) {
                if (tok2->str() == "this") {
                    assignAllVar(usage);
                    return;
                }
            }

            // check if member function
            if (ftok->function() && ftok->function()->nestedIn == scope &&
                !ftok->function()->isConstructor()) {
                const Function *member = ftok->function();

                // recursive call
                // assume that all variables are initialized
                if (std::find(callstack.begin(), callstack.end(), member) != callstack.end()) {
                    assignAllVar(usage);
                    return;
                }

                // member function has implementation
                if (member->hasBody()) {
                    // initialize variable use list using member function
                    callstack.push_back(member);
                    initializeVarList(*member, callstack, scope, usage);
                    callstack.pop_back();

                    // Assume that variables that are passed to it are initialized..
                    for (const Token *tok2 = ftok; tok2; tok2 = tok2->next()) {
                        if (Token::Match(tok2, "[;{}]"))
                            break;
                        if (Token::Match(tok2, "[(,] &| %name% [,)]")) {
                            tok2 = tok2->next();
                            if (tok2->str() == "&")
                                tok2 = tok2->next();
                            assignVar(tok2->varId(), scope, usage);
                        }
                    }
                }

                // there is a called member function, but it has no implementation, so we assume it initializes everything
                else {
                    assignAllVar(usage);
                }
            }

            // not member function
            else {
                // could be a base class virtual function, so we assume it initializes everything
                if (!func.isConstructor() && isBaseClassFunc(ftok, scope)) {
                    /** @todo False Negative: we should look at the base class functions to see if they
                     *  call any derived class virtual functions that change the derived class state
                     */
                    assignAllVar(usage);
                }
            }
        }

        // Assignment of member variable?
        else if (Token::Match(ftok, "%name% =")) {
            assignVar(ftok->varId(), scope, usage);
            bool bailout = ftok->variable() && ftok->variable()->isReference();
            const Token* tok2 = ftok->tokAt(2);
            if (tok2->str() == "&") {
                tok2 = tok2->next();
                bailout = true;
            }
            if (tok2->variable() && (bailout || tok2->variable()->isArray()) && tok2->strAt(1) != "[")
                assignVar(tok2->varId(), scope, usage);
        }

        // Assignment of array item of member variable?
        else if (Token::Match(ftok, "%name% [|.")) {
            const Token *tok2 = ftok;
            while (tok2) {
                if (tok2->strAt(1) == "[")
                    tok2 = tok2->next()->link();
                else if (Token::Match(tok2->next(), ". %name%"))
                    tok2 = tok2->tokAt(2);
                else
                    break;
            }
            if (tok2 && tok2->strAt(1) == "=")
                assignVar(ftok->varId(), scope, usage);
        }

        // Assignment of array item of member variable?
        else if (Token::Match(ftok, "* %name% =")) {
            assignVar(ftok->next()->varId(), scope, usage);
        } else if (Token::Match(ftok, "* this . %name% =")) {
            assignVar(ftok->tokAt(3)->varId(), scope, usage);
        }

        // The functions 'clear' and 'Clear' are supposed to initialize variable.
        if (Token::Match(ftok, "%name% . clear|Clear (")) {
            assignVar(ftok->varId(), scope, usage);
        }
    }
}

void CheckClass::noConstructorError(const Token *tok, const std::string &classname, bool isStruct)
{
    // For performance reasons the constructor might be intentionally missing. Therefore this is not a "warning"
    reportError(tok, Severity::style, "noConstructor",
                "$symbol:" + classname + "\n" +
                "The " + std::string(isStruct ? "struct" : "class") + " '$symbol' does not have a constructor although it has private member variables.\n"
                "The " + std::string(isStruct ? "struct" : "class") + " '$symbol' does not have a constructor "
                "although it has private member variables. Member variables of builtin types are left "
                "uninitialized when the class is instantiated. That may cause bugs or undefined behavior.", CWE398, false);
}

//---------------------------------------------------------------------------
// ClassCheck: Use initialization list instead of assignment
//---------------------------------------------------------------------------

void CheckClass::initializationListUsage()
{
    if (!mSettings->isEnabled(Settings::PERFORMANCE))
        return;

    for (const Scope *scope : mSymbolDatabase->functionScopes) {
        // Check every constructor
        if (!scope->function || (!scope->function->isConstructor()))
            continue;

        const Scope* owner = scope->functionOf;
        for (const Token* tok = scope->bodyStart; tok != scope->bodyEnd; tok = tok->next()) {
            if (Token::Match(tok, "%name% (")) // Assignments might depend on this function call or if/for/while/switch statement from now on.
                break;
            if (Token::Match(tok, "try|do {"))
                break;
            if (!Token::Match(tok, "%var% =") || tok->strAt(-1) == "*")
                continue;

            const Variable* var = tok->variable();
            if (!var || var->scope() != owner || var->isStatic())
                continue;
            if (var->isReference() || var->isEnumType())
                continue;

            // Access local var member in rhs => do not warn
            bool localmember = false;
            visitAstNodes(tok->next()->astOperand2(),
            [&](const Token *rhs) {
                if (rhs->str() == "." && rhs->astOperand1() && rhs->astOperand1()->variable() && rhs->astOperand1()->variable()->isLocal())
                    localmember = true;
                return ChildrenToVisit::op1_and_op2;
            });
            if (localmember)
                continue;

            bool allowed = true;
            visitAstNodes(tok->next()->astOperand2(),
            [&](const Token *tok2) {
                const Variable* var2 = tok2->variable();
                if (var2) {
                    if (var2->scope() == owner && tok2->strAt(-1)!=".") { // Is there a dependency between two member variables?
                        allowed = false;
                        return ChildrenToVisit::done;
                    } else if (var2->isArray() && var2->isLocal()) { // Can't initialize with a local array
                        allowed = false;
                        return ChildrenToVisit::done;
                    }
                } else if (tok2->str() == "this") { // 'this' instance is not completely constructed in initialization list
                    allowed = false;
                    return ChildrenToVisit::done;
                } else if (Token::Match(tok2, "%name% (") && tok2->strAt(-1) != "." && isMemberFunc(owner, tok2)) { // Member function called?
                    allowed = false;
                    return ChildrenToVisit::done;
                }
                return ChildrenToVisit::op1_and_op2;
            });
            if (!allowed)
                continue;

            suggestInitializationList(tok, tok->str());
        }
    }
}

void CheckClass::suggestInitializationList(const Token* tok, const std::string& varname)
{
    reportError(tok, Severity::performance, "useInitializationList", "$symbol:" + varname + "\nVariable '$symbol' is assigned in constructor body. Consider performing initialization in initialization list.\n"
                "When an object of a class is created, the constructors of all member variables are called consecutively "
                "in the order the variables are declared, even if you don't explicitly write them to the initialization list. You "
                "could avoid assigning '$symbol' a value by passing the value to the constructor in the initialization list.", CWE398, false);
}

//---------------------------------------------------------------------------
// ClassCheck: Unused private functions
//---------------------------------------------------------------------------

static bool checkFunctionUsage(const Function *privfunc, const Scope* scope)
{
    if (!scope)
        return true; // Assume it is used, if scope is not seen

    for (std::list<Function>::const_iterator func = scope->functionList.begin(); func != scope->functionList.end(); ++func) {
        if (func->functionScope) {
            // check if the function is used in parameter default value
            //Ex:  int i = foo(privfunc());
            if (Token::Match(func->tokenDef, "%name% (")) {

                for (const Token *ftok = func->tokenDef->tokAt(2); ftok && ftok->str() != ")"; ftok = ftok->next()) {

                    if (Token::Match(ftok, "= %name% [(,)]") && ftok->strAt(1) == privfunc->name())
                        return true;
                    if (ftok->str() == "(")
                        ftok = ftok->link();
                }
            }
            // find in functiona scope if there is some where called
            for (const Token *ftok = func->functionScope->classDef->linkAt(1); ftok != func->functionScope->bodyEnd; ftok = ftok->next()) {
                
                if (ftok->function() == privfunc)
                    return true;
                if (ftok->varId() == 0U && ftok->str() == privfunc->name()) // TODO: This condition should be redundant
                    return true;

                // check if is given as parameter (callback function ...).
                // ex: dpConnect("privfunc", ...)
                if ( Token::Match(ftok, "%name% (") )
                {
                  const std::vector<const Token *> &callArguments = getArguments(ftok);
                  for (unsigned int argnr = 0U; argnr < callArguments.size(); ++argnr)
                  {
                    const Token *argtok = callArguments[argnr];
                    // get unqouted argument value, because "callBackFunction" == callBackFunction
                    std::string unqouted = argtok->unquoteStr();

                    ///@todo: here shoudl be checked also for arg type. because this is worng
                    ///       dpConnet(true, "callBackFunction");
                    ///       but for 99% uf usage it is OK, because primary shoud checks if the function is called
                    ///       an not if the arguments are passed.

                    // when is in class the function must contains class name like:
                    // C::foo(); 
                    if ( scope->isClassOrStruct() && ((scope->className + "::" + unqouted) == privfunc->name()) )
                    {
                        return true;
                    }
                    // non class function call
                    if ( unqouted == privfunc->name() )
                    {
                        return true;
                    }
                  }
                }
            }
        } else if (func->access != Private) // Assume it is used, if a function implementation isn't seen, but empty private copy constructors and assignment operators are OK
            return true;
    }

// user defines types.
/// @todo: I think this is obsolete code, try to remove it
    const std::map<std::string, Type*>::const_iterator end = scope->definedTypesMap.end();
    for (std::map<std::string, Type*>::const_iterator iter = scope->definedTypesMap.begin(); iter != end; ++ iter) {
        const Type *type = (*iter).second;
        if (type->enclosingScope == scope && checkFunctionUsage(privfunc, type->classScope))
            return true;
    }

// assigned to variable like
// int c = privfunc();
// main(){ Debug(c); }
    for (const Variable &var : scope->varlist) {
        if (var.isStatic()) {
            const Token* tok = Token::findmatch(scope->bodyEnd, "%varid% =|(|{", var.declarationId());
            if (tok)
                tok = tok->tokAt(2);
            while (tok && tok->str() != ";") {
                if (tok->function() == privfunc)
                    return true;
                tok = tok->next();
            }
        }
    }

    return false; // Unused in this scope
}

void CheckClass::privateFunctions()
{
    if (!mSettings->isEnabled(Settings::STYLE))
        return;

    for (const Scope * scope : mSymbolDatabase->classAndStructScopes) {

        // do not check borland classes with properties..
        /// @todo remove it, We does not need it in ctrl
        if (Token::findsimplematch(scope->bodyStart, "; __property ;", scope->bodyEnd))
            continue;

        std::list<const Function*> privateFuncs;
        for (const Function &func : scope->functionList) {
            // Get private functions..
            if (func.type == Function::eFunction && func.access == Private) // TODO: There are smarter ways to check private operator usage
                privateFuncs.push_back(&func);
        }

        while (!privateFuncs.empty()) {
            // Check that all private functions are used
            bool used = checkFunctionUsage(privateFuncs.front(), scope); // Usage in this class

            if (!used)
                unusedPrivateFunctionError(privateFuncs.front()->tokenDef, scope->className, privateFuncs.front()->name());

            privateFuncs.pop_front();
        }
    }
}

void CheckClass::unusedPrivateFunctionError(const Token *tok, const std::string &classname, const std::string &funcname)
{
    reportError(tok, Severity::style, "unusedPrivateFunction", "$symbol:" + classname + "::" + funcname + "\nUnused private function: '$symbol'", CWE398, false);
}

//---------------------------------------------------------------------------
// ClassCheck: Check that memset is not used on classes
//---------------------------------------------------------------------------

static const Scope* findFunctionOf(const Scope* scope)
{
    while (scope) {
        if (scope->type == Scope::eFunction)
            return scope->functionOf;
        scope = scope->nestedIn;
    }
    return nullptr;
}

//---------------------------------------------------------------------------
// ClassCheck: "C& operator=(const C&) { ... return *this; }"
// operator= should return a reference to *this
//---------------------------------------------------------------------------

void CheckClass::operatorEqRetRefThis()
{
    if (!mSettings->isEnabled(Settings::STYLE))
        return;

    for (const Scope * scope : mSymbolDatabase->classAndStructScopes) {
        for (std::list<Function>::const_iterator func = scope->functionList.begin(); func != scope->functionList.end(); ++func) {
            if (func->hasBody()) {
                // make sure return signature is correct
                if (func->retType == func->nestedIn->definedType && func->tokenDef->strAt(-1) == "&") {
                    checkReturnPtrThis(scope, &(*func), func->functionScope->bodyStart, func->functionScope->bodyEnd);
                }
            }
        }
    }
}

void CheckClass::checkReturnPtrThis(const Scope *scope, const Function *func, const Token *tok, const Token *last)
{
    std::set<const Function*> analyzedFunctions;
    checkReturnPtrThis(scope, func, tok, last, analyzedFunctions);
}

void CheckClass::checkReturnPtrThis(const Scope *scope, const Function *func, const Token *tok, const Token *last, std::set<const Function*>& analyzedFunctions)
{
    bool foundReturn = false;

    const Token* const startTok = tok;

    for (; tok && tok != last; tok = tok->next()) {
        // check for return of reference to this
        if (tok->str() != "return")
            continue;

        foundReturn = true;
        std::string cast("( " + scope->className + " & )");
        if (Token::simpleMatch(tok->next(), cast.c_str()))
            tok = tok->tokAt(4);

        // check if a function is called
        if (tok->strAt(2) == "(" &&
            tok->linkAt(2)->next()->str() == ";") {
            // check if it is a member function
            for (std::list<Function>::const_iterator it = scope->functionList.begin(); it != scope->functionList.end(); ++it) {
                // check for a regular function with the same name and a body
                if (it->type == Function::eFunction && it->hasBody() &&
                    it->token->str() == tok->next()->str()) {
                    // check for the proper return type
                    if (it->tokenDef->previous()->str() == "&" &&
                        it->tokenDef->strAt(-2) == scope->className) {
                        // make sure it's not a const function
                        if (!it->isConst()) {
                            /** @todo make sure argument types match */
                            // avoid endless recursions
                            if (analyzedFunctions.find(&*it) == analyzedFunctions.end()) {
                                analyzedFunctions.insert(&*it);
                                checkReturnPtrThis(scope, &*it, it->arg->link()->next(), it->arg->link()->next()->link(),
                                                   analyzedFunctions);
                            }
                            // just bail for now
                            else
                                return;
                        }
                    }
                }
            }
        }

        // check if *this is returned
        else if (!(Token::Match(tok->next(), "(| * this ;|=") ||
                   Token::simpleMatch(tok->next(), "operator= (") ||
                   Token::simpleMatch(tok->next(), "this . operator= (") ||
                   (Token::Match(tok->next(), "%type% :: operator= (") &&
                    tok->next()->str() == scope->className)))
            operatorEqRetRefThisError(func->token);
    }
    if (foundReturn) {
        return;
    }
    if (startTok->next() == last) {
        if (Token::simpleMatch(func->argDef, std::string("( const " + scope->className + " &").c_str())) {
            // Typical wrong way to suppress default assignment operator by declaring it and leaving empty
            operatorEqMissingReturnStatementError(func->token, func->access == Public);
        } else {
            operatorEqMissingReturnStatementError(func->token, true);
        }
        return;
    }
    if (mSettings->library.isScopeNoReturn(last, nullptr)) {
        // Typical wrong way to prohibit default assignment operator
        // by always throwing an exception or calling a noreturn function
        operatorEqShouldBeLeftUnimplementedError(func->token);
        return;
    }

    operatorEqMissingReturnStatementError(func->token, func->access == Public);
}

void CheckClass::operatorEqRetRefThisError(const Token *tok)
{
    reportError(tok, Severity::style, "operatorEqRetRefThis", "'operator=' should return reference to 'this' instance.", CWE398, false);
}

void CheckClass::operatorEqShouldBeLeftUnimplementedError(const Token *tok)
{
    reportError(tok, Severity::style, "operatorEqShouldBeLeftUnimplemented", "'operator=' should either return reference to 'this' instance or be declared private and left unimplemented.", CWE398, false);
}

void CheckClass::operatorEqMissingReturnStatementError(const Token *tok, bool error)
{
    if (error) {
        reportError(tok, Severity::error, "operatorEqMissingReturnStatement", "No 'return' statement in non-void function causes undefined behavior.", CWE398, false);
    } else {
        operatorEqRetRefThisError(tok);
    }
}

//---------------------------------------------------------------------------
// ClassCheck: "C& operator=(const C& rhs) { if (this == &rhs) ... }"
// operator= should check for assignment to self
//
// For simple classes, an assignment to self check is only a potential optimization.
//
// For classes that allocate dynamic memory, assignment to self can be a real error
// if it is deallocated and allocated again without being checked for.
//
// This check is not valid for classes with multiple inheritance because a
// class can have multiple addresses so there is no trivial way to check for
// assignment to self.
//---------------------------------------------------------------------------

void CheckClass::operatorEqToSelf()
{
    if (!mSettings->isEnabled(Settings::WARNING))
        return;

    for (const Scope * scope : mSymbolDatabase->classAndStructScopes) {
        // skip classes with multiple inheritance
        if (scope->definedType->derivedFrom.size() > 1)
            continue;

        for (const Function &func : scope->functionList) {
            if (func.hasBody()) {
                // make sure that the operator takes an object of the same type as *this, otherwise we can't detect self-assignment checks
                if (func.argumentList.empty())
                    continue;
                const Token* typeTok = func.argumentList.front().typeEndToken();
                while (typeTok->str() == "const" || typeTok->str() == "&" || typeTok->str() == "*")
                    typeTok = typeTok->previous();
                if (typeTok->str() != scope->className)
                    continue;

                // make sure return signature is correct
                if (Token::Match(func.retDef, "%type% &") && func.retDef->str() == scope->className) {
                    // find the parameter name
                    const Token *rhs = func.argumentList.begin()->nameToken();

                    if (!hasAssignSelf(&func, rhs)) {
                        if (hasAllocation(&func, scope))
                            operatorEqToSelfError(func.token);
                    }
                }
            }
        }
    }
}

bool CheckClass::hasAllocation(const Function *func, const Scope* scope) const
{
    // This function is called when no simple check was found for assignment
    // to self.  We are currently looking for:
    //    - deallocate member ; ... member =
    //    - alloc member
    // That is not ideal because it can cause false negatives but its currently
    // necessary to prevent false positives.
    const Token *last = func->functionScope->bodyEnd;
    for (const Token *tok = func->functionScope->bodyStart; tok && (tok != last); tok = tok->next()) {
        if (Token::Match(tok, "%var% = malloc|realloc|calloc|new") && isMemberVar(scope, tok))
            return true;

        // check for deallocating memory
        const Token *var;
        if (Token::Match(tok, "free ( %var%"))
            var = tok->tokAt(2);
        else if (Token::Match(tok, "delete [ ] %var%"))
            var = tok->tokAt(3);
        else if (Token::Match(tok, "delete %var%"))
            var = tok->next();
        else
            continue;
        // Check for assignment to the deleted pointer (only if its a member of the class)
        if (isMemberVar(scope, var)) {
            for (const Token *tok1 = var->next(); tok1 && (tok1 != last); tok1 = tok1->next()) {
                if (Token::Match(tok1, "%varid% =", var->varId()))
                    return true;
            }
        }
    }

    return false;
}

bool CheckClass::hasAssignSelf(const Function *func, const Token *rhs)
{
    if (!rhs)
        return false;
    const Token *last = func->functionScope->bodyEnd;
    for (const Token *tok = func->functionScope->bodyStart; tok && tok != last; tok = tok->next()) {
        if (!Token::simpleMatch(tok, "if ("))
            continue;

        bool ret = false;
        visitAstNodes(tok->next()->astOperand2(),
        [&](const Token *tok2) {
            if (!Token::Match(tok2, "==|!="))
                return ChildrenToVisit::op1_and_op2;
            if (Token::simpleMatch(tok2->astOperand1(), "this"))
                tok2 = tok2->astOperand2();
            else if (Token::simpleMatch(tok2->astOperand2(), "this"))
                tok2 = tok2->astOperand1();
            else
                return ChildrenToVisit::op1_and_op2;
            if (tok2 && tok2->isUnaryOp("&") && tok2->astOperand1()->str() == rhs->str())
                ret = true;
            return ret ? ChildrenToVisit::done : ChildrenToVisit::op1_and_op2;
        });
        if (ret)
            return ret;
    }

    return false;
}

void CheckClass::operatorEqToSelfError(const Token *tok)
{
    reportError(tok, Severity::warning, "operatorEqToSelf",
                "'operator=' should check for assignment to self to avoid problems with dynamic memory.\n"
                "'operator=' should check for assignment to self to ensure that each block of dynamically "
                "allocated memory is owned and managed by only one instance of the class.", CWE398, false);
}


//---------------------------------------------------------------------------
// warn for "this-x". The indented code may be "this->x"
//---------------------------------------------------------------------------

void CheckClass::thisSubtraction()
{
    if (!mSettings->isEnabled(Settings::WARNING))
        return;

    const Token *tok = mTokenizer->tokens();
    for (;;) {
        tok = Token::findmatch(tok, "this - %name%");
        if (!tok)
            break;

        if (tok->strAt(-1) != "*")
            thisSubtractionError(tok);

        tok = tok->next();
    }
}

void CheckClass::thisSubtractionError(const Token *tok)
{
    reportError(tok, Severity::warning, "thisSubtraction", "Suspicious pointer subtraction. Did you intend to write '->'?", CWE398, false);
}

//---------------------------------------------------------------------------
// can member function be const?
//---------------------------------------------------------------------------

void CheckClass::checkConst()
{
    // This is an inconclusive check. False positives: #3322.
    if (!mSettings->inconclusive)
        return;

    if (!mSettings->isEnabled(Settings::STYLE))
        return;

    for (const Scope * scope : mSymbolDatabase->classAndStructScopes) {
        for (const Function &func : scope->functionList) {
            // does the function have a body?
            if (func.type != Function::eFunction || !func.hasBody())
                continue;
            // don't warn for static functions
            if (func.isStatic())
                continue;
            // get last token of return type
            const Token *previous = func.tokenDef->previous();

            // does the function return a pointer or reference?
            if (Token::Match(previous, "*|&")) {
                if (func.retDef->str() != "const")
                    continue;
            } else if (Token::Match(previous->previous(), "*|& >")) {
                const Token *temp = previous->previous();

                bool foundConst = false;
                while (!Token::Match(temp->previous(), ";|}|{|public|protected|private")) {
                    temp = temp->previous();
                    if (temp->str() == "const") {
                        foundConst = true;
                        break;
                    }
                }

                if (!foundConst)
                    continue;
            } else if (Token::simpleMatch(func.retDef, "shared_ptr <")) {
                // Don't warn if a shared_ptr is returned
                continue;
            } else {
                // don't warn for unknown types..
                // LPVOID, HDC, etc
                if (previous->str().size() > 2 && !previous->type() && previous->isUpperCaseName())
                    continue;
            }

            bool memberAccessed = false;
            // if nothing non-const was found. write error..
            if (!checkConstFunc(scope, &func, memberAccessed))
                continue;

            std::string classname = scope->className;
            const Scope *nest = scope->nestedIn;
            while (nest && nest->type != Scope::eGlobal) {
                classname = std::string(nest->className + "::" + classname);
                nest = nest->nestedIn;
            }

            // get function name
            std::string functionName = (func.tokenDef->isName() ? "" : "operator") + func.tokenDef->str();

            if (func.tokenDef->str() == "(")
                functionName += ")";
            else if (func.tokenDef->str() == "[")
                functionName += "]";

            if (func.isInline())
                checkConstError(func.token, classname, functionName, !memberAccessed);
            else // not inline
                checkConstError2(func.token, func.tokenDef, classname, functionName, !memberAccessed);
        }
    }
}

bool CheckClass::isMemberVar(const Scope *scope, const Token *tok) const
{
    bool again = false;

    // try to find the member variable
    do {
        again = false;

        if (tok->str() == "this") {
            return true;
        } else if (Token::simpleMatch(tok->tokAt(-3), "( * this )")) {
            return true;
        } else if (Token::Match(tok->tokAt(-2), "%name% . %name%")) {
            tok = tok->tokAt(-2);
            again = true;
        } else if (Token::Match(tok->tokAt(-2), "] . %name%")) {
            tok = tok->linkAt(-2)->previous();
            again = true;
        } else if (tok->str() == "]") {
            tok = tok->link()->previous();
            again = true;
        }
    } while (again);

    for (const Variable &var : scope->varlist) {
        if (var.name() == tok->str()) {
            if (tok->varId() == 0)
                mSymbolDatabase->debugMessage(tok, "CheckClass::isMemberVar found used member variable \'" + tok->str() + "\' with varid 0");

            return !var.isStatic();
        }
    }

    // not found in this class
    if (!scope->definedType->derivedFrom.empty()) {
        // check each base class
        for (std::size_t i = 0; i < scope->definedType->derivedFrom.size(); ++i) {
            // find the base class
            const Type *derivedFrom = scope->definedType->derivedFrom[i].type;

            // find the function in the base class
            if (derivedFrom && derivedFrom->classScope) {
                if (isMemberVar(derivedFrom->classScope, tok))
                    return true;
            }
        }
    }

    return false;
}

bool CheckClass::isMemberFunc(const Scope *scope, const Token *tok) const
{
    if (!tok->function()) {
        for (const Function &func : scope->functionList) {
            if (func.name() == tok->str()) {
                const Token* tok2 = tok->tokAt(2);
                size_t argsPassed = tok2->str() == ")" ? 0 : 1;
                for (;;) {
                    tok2 = tok2->nextArgument();
                    if (tok2)
                        argsPassed++;
                    else
                        break;
                }
                if (argsPassed == func.argCount() || (argsPassed < func.argCount() && argsPassed >= func.minArgCount()))
                    return true;
            }
        }
    } else if (tok->function()->nestedIn == scope)
        return !tok->function()->isStatic();

    // not found in this class
    if (!scope->definedType->derivedFrom.empty()) {
        // check each base class
        for (std::size_t i = 0; i < scope->definedType->derivedFrom.size(); ++i) {
            // find the base class
            const Type *derivedFrom = scope->definedType->derivedFrom[i].type;

            // find the function in the base class
            if (derivedFrom && derivedFrom->classScope) {
                if (isMemberFunc(derivedFrom->classScope, tok))
                    return true;
            }
        }
    }

    return false;
}

bool CheckClass::checkConstFunc(const Scope *scope, const Function *func, bool& memberAccessed) const
{
    // if the function doesn't have any assignment nor function call,
    // it can be a const function..
    for (const Token *tok1 = func->functionScope->bodyStart; tok1 && tok1 != func->functionScope->bodyEnd; tok1 = tok1->next()) {
        if (tok1->isName() && isMemberVar(scope, tok1)) {
            memberAccessed = true;

            if (tok1->str() == "this" && tok1->previous()->isAssignmentOp())
                return false;


            const Token* lhs = tok1->previous();
            if (lhs->str() == ":" && lhs->astParent() && lhs->astParent()->str() == "(" && tok1->strAt(1) == ")") { // range-based for-loop (C++11)
                // TODO: We could additionally check what is done with the elements to avoid false negatives. Here we just rely on "const" keyword being used.
                if (lhs->astParent()->strAt(1) != "const")
                    return false;
            } else {
                if (lhs->isAssignmentOp()) {
                    const Variable* lhsVar = lhs->previous()->variable();
                    if (lhsVar && !lhsVar->isConst() && lhsVar->isReference() && lhs == lhsVar->nameToken()->next())
                        return false;
                }
            }

            const Token* jumpBackToken = nullptr;
            const Token *lastVarTok = tok1;
            const Token *end = tok1;
            for (;;) {
                if (Token::Match(end->next(), ". %name%")) {
                    end = end->tokAt(2);
                    if (end->varId())
                        lastVarTok = end;
                } else if (end->strAt(1) == "[") {
                    if (!jumpBackToken)
                        jumpBackToken = end->next(); // Check inside the [] brackets
                    end = end->linkAt(1);
                } else if (end->strAt(1) == ")")
                    end = end->next();
                else
                    break;
            }

            if (end->strAt(1) == "(") {
                const Variable *var = lastVarTok->variable();
                if (!var || !var->typeScope())
                    return false;
            }

            // Assignment
            else if (end->next()->isAssignmentOp())
                return false;

            // ++/--
            else if (end->next()->tokType() == Token::eIncDecOp || tok1->previous()->tokType() == Token::eIncDecOp)
                return false;


            const Token* start = tok1;
            while (tok1->strAt(-1) == ")")
                tok1 = tok1->linkAt(-1);


            tok1 = jumpBackToken?jumpBackToken:end; // Jump back to first [ to check inside, or jump to end of expression
        }

        // function call..
        else if (Token::Match(tok1, "%name% (") && !tok1->isStandardType() &&
                 !Token::Match(tok1, "return|if|string|switch|while|for")) {
            if (isMemberFunc(scope, tok1) && tok1->strAt(-1) != ".") {
                return false;
            }
            // Member variable given as parameter
            const Token *lpar = tok1->next();
            if (Token::simpleMatch(lpar, "( ) ("))
                lpar = lpar->tokAt(2);
            for (const Token* tok2 = lpar->next(); tok2 && tok2 != tok1->next()->link(); tok2 = tok2->next()) {
                if (tok2->str() == "(")
                    tok2 = tok2->link();
            }
        } 
    }

    return true;
}

void CheckClass::checkConstError(const Token *tok, const std::string &classname, const std::string &funcname, bool suggestStatic)
{
    checkConstError2(tok, nullptr, classname, funcname, suggestStatic);
}

void CheckClass::checkConstError2(const Token *tok1, const Token *tok2, const std::string &classname, const std::string &funcname, bool suggestStatic)
{
    std::list<const Token *> toks;
    toks.push_back(tok1);
    if (tok2)
        toks.push_back(tok2);
    if (suggestStatic)
        reportError(toks, Severity::performance, "functionStatic",
                    "$symbol:" + classname + "::" + funcname +"\n"
                    "Technically the member function '$symbol' can be static.\n"
                    "The member function '$symbol' can be made a static "
                    "function. Making a function static can bring a performance benefit since no 'this' instance is "
                    "passed to the function. This change should not cause compiler errors but it does not "
                    "necessarily make sense conceptually. Think about your design and the task of the function first - "
                    "is it a function that must not access members of class instances? And maybe it is more appropriate "
                    "to move this function to a new library", CWE398, true);
}

//---------------------------------------------------------------------------
// ClassCheck: Check that initializer list is in declared order.
//---------------------------------------------------------------------------

namespace { // avoid one-definition-rule violation
    struct VarInfo {
        VarInfo(const Variable *_var, const Token *_tok)
            : var(_var), tok(_tok) { }

        const Variable *var;
        const Token *tok;
    };
}

void CheckClass::initializerListOrder()
{
    if (!mSettings->isEnabled(Settings::STYLE))
        return;

    // This check is not inconclusive.  However it only determines if the initialization
    // order is incorrect.  It does not determine if being out of order causes
    // a real error.  Out of order is not necessarily an error but you can never
    // have an error if the list is in order so this enforces defensive programming.
    if (!mSettings->inconclusive)
        return;

    for (const Scope * scope : mSymbolDatabase->classAndStructScopes) {

        // iterate through all member functions looking for constructors
        for (std::list<Function>::const_iterator func = scope->functionList.begin(); func != scope->functionList.end(); ++func) {
            if (func->isConstructor() && func->hasBody()) {
                // check for initializer list
                const Token *tok = func->arg->link()->next();

                if (tok->str() == ":") {
                    std::vector<VarInfo> vars;
                    tok = tok->next();

                    // find all variable initializations in list
                    while (tok && tok != func->functionScope->bodyStart) {
                        if (Token::Match(tok, "%name% (|{")) {
                            const Variable *var = scope->getVariable(tok->str());
                            if (var)
                                vars.emplace_back(var, tok);

                            if (Token::Match(tok->tokAt(2), "%name% =")) {
                                var = scope->getVariable(tok->strAt(2));

                                if (var)
                                    vars.emplace_back(var, tok->tokAt(2));
                            }
                            tok = tok->next()->link()->next();
                        } else
                            tok = tok->next();
                    }

                    // need at least 2 members to have out of order initialization
                    for (std::size_t j = 1; j < vars.size(); j++) {
                        // check for out of order initialization
                        if (vars[j].var->index() < vars[j - 1].var->index())
                            initializerListError(vars[j].tok,vars[j].var->nameToken(), scope->className, vars[j].var->name());
                    }
                }
            }
        }
    }
}

void CheckClass::initializerListError(const Token *tok1, const Token *tok2, const std::string &classname, const std::string &varname)
{
    std::list<const Token *> toks = { tok1, tok2 };
    reportError(toks, Severity::style, "initializerList",
                "$symbol:" + classname + "::" + varname +"\n"
                "Member variable '$symbol' is in the wrong place in the initializer list.\n"
                "Member variable '$symbol' is in the wrong place in the initializer list. "
                "Members are initialized in the order they are declared, not in the "
                "order they are in the initializer list.  Keeping the initializer list "
                "in the same order that the members were declared prevents order dependent "
                "initialization errors.", CWE398, true);
}


//---------------------------------------------------------------------------
// Check for self initialization in initialization list
//---------------------------------------------------------------------------

void CheckClass::checkSelfInitialization()
{
    for (const Scope *scope : mSymbolDatabase->functionScopes) {
        const Function* function = scope->function;
        if (!function || !function->isConstructor())
            continue;

        const Token* tok = function->arg->link()->next();
        if (tok->str() != ":")
            continue;

        for (; tok != scope->bodyStart; tok = tok->next()) {
            if (Token::Match(tok, "[:,] %var% (|{ %var% )|}") && tok->next()->varId() == tok->tokAt(3)->varId()) {
                selfInitializationError(tok, tok->strAt(1));
            }
        }
    }
}

void CheckClass::selfInitializationError(const Token* tok, const std::string& varname)
{
    reportError(tok, Severity::error, "selfInitialization", "$symbol:" + varname + "\nMember variable '$symbol' is initialized by itself.", CWE665, false);
}

//---------------------------------------------------------------------------
// Check for members hiding inherited members with the same name
//---------------------------------------------------------------------------

void CheckClass::checkDuplInheritedMembers()
{
    if (!mSettings->isEnabled(Settings::WARNING))
        return;

    // Iterate over all classes
    for (const Type &classIt : mSymbolDatabase->typeList) {
        // Iterate over the parent classes
        for (const Type::BaseInfo &parentClassIt : classIt.derivedFrom) {
            // Check if there is info about the 'Base' class
            if (!parentClassIt.type || !parentClassIt.type->classScope)
                continue;
            // Check if they have a member variable in common
            for (const Variable &classVarIt : classIt.classScope->varlist) {
                for (const Variable &parentClassVarIt : parentClassIt.type->classScope->varlist) {
                    if (classVarIt.name() == parentClassVarIt.name() && !parentClassVarIt.isPrivate()) { // Check if the class and its parent have a common variable
                        duplInheritedMembersError(classVarIt.nameToken(), parentClassVarIt.nameToken(),
                                                  classIt.name(), parentClassIt.type->name(), classVarIt.name(),
                                                  classIt.classScope->type == Scope::eStruct,
                                                  parentClassIt.type->classScope->type == Scope::eStruct);
                    }
                }
            }
        }
    }
}

void CheckClass::duplInheritedMembersError(const Token *tok1, const Token* tok2,
        const std::string &derivedName, const std::string &baseName,
        const std::string &variableName, bool derivedIsStruct, bool baseIsStruct)
{
    ErrorPath errorPath;
    errorPath.emplace_back(tok2, "Parent variable '" + baseName + "::" + variableName + "'");
    errorPath.emplace_back(tok1, "Derived variable '" + derivedName + "::" + variableName + "'");

    const std::string symbols = "$symbol:" + derivedName + "\n$symbol:" + variableName + "\n$symbol:" + baseName;

    const std::string message = "The " + std::string(derivedIsStruct ? "struct" : "class") + " '" + derivedName +
                                "' defines member variable with name '" + variableName + "' also defined in its parent " +
                                std::string(baseIsStruct ? "struct" : "class") + " '" + baseName + "'.";
    reportError(errorPath, Severity::warning, "duplInheritedMember", symbols + '\n' + message, CWE398, false);
}


//---------------------------------------------------------------------------
// Check that copy constructor and operator defined together
//---------------------------------------------------------------------------

enum CtorType {
    NO,
    WITHOUT_BODY,
    WITH_BODY
};

void CheckClass::checkUnsafeClassDivZero(bool test)
{
    // style severity: it is a style decision if classes should be safe or
    // if users should be required to be careful. I expect that many users
    // will disagree about these reports.
    if (!mSettings->isEnabled(Settings::STYLE))
        return;

    for (const Scope * classScope : mSymbolDatabase->classAndStructScopes) {
        if (!test && classScope->classDef->fileIndex() != 1)
            continue;
        for (const Function &func : classScope->functionList) {
            if (func.access != AccessControl::Public)
                continue;
            if (!func.hasBody())
                continue;
            if (func.name().compare(0,8,"operator")==0)
                continue;
            for (const Token *tok = func.functionScope->bodyStart; tok; tok = tok->next()) {
                if (Token::Match(tok, "if|switch|while|for|do|}"))
                    break;
                if (tok->str() != "/")
                    continue;
                if (!tok->valueType() || !tok->valueType()->isIntegral())
                    continue;
                if (!tok->astOperand2())
                    continue;
                const Variable *var = tok->astOperand2()->variable();
                if (!var || !var->isArgument())
                    continue;
                unsafeClassDivZeroError(tok, classScope->className, func.name(), var->name());
                break;
            }
        }
    }
}

void CheckClass::unsafeClassDivZeroError(const Token *tok, const std::string &className, const std::string &methodName, const std::string &varName)
{
    const std::string symbols = "$symbol:" + className + "\n$symbol:" + methodName + "\n$symbol:" + varName + '\n';
    const std::string s = className + "::" + methodName + "()";
    reportError(tok, Severity::style, "unsafeClassDivZero", symbols + "Public interface of " + className + " is not safe. When calling " + s + ", if parameter " + varName + " is 0 that leads to division by zero.");
}
