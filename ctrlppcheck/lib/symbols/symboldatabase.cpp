
//-----------------------------------------------------------------------------
#include "symboldatabase.h"

#include "symbolutils.h"
#include "variable.h"

#include <cassert>
#include <iomanip>

//-----------------------------------------------------------------------------

namespace
{

/// reseved keyowrds
/// not used, but reserved keywords
#define CTRL_NOTUSED_KEYWORDS          \
    "inline", "extern", "restrict",    \
        "auto", "typedef", "volatile", \
        "register", "goto"

const std::set<std::string> ctrl_reserved_notUsed_keywords = {CTRL_NOTUSED_KEYWORDS};

// all keywords
const std::set<std::string> ctrl_reserved_keywords = {
    CTRL_NOTUSED_KEYWORDS,

    "break", "case", "const", "continue", "default", "do",
    "else", "enum", "for", "goto", "if",
    "return",
    "static", "struct", "switch", "void",
    "while",
    "bool", "catch", "class",
    "delete",
    "false",
    "global",
    "new",
    "nullptr",
    "private", "protected", "public",
    "synchronized",
    "true", "try",
    "this"};
} // namespace

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
bool SymbolDatabase::isReservedName(const std::string &iName)
{
    return (ValueType::typeFromString(iName) == ValueType::Type::UNKNOWN_TYPE) &&
           (ctrl_reserved_keywords.find(iName) != ctrl_reserved_keywords.cend());
}

//-----------------------------------------------------------------------------
static AccessControl accessControlFromString(const std::string &access)
{
    return (access == "public" ? Public : access == "private" ? Private : access == "protected" ? Protected : Unkown);
}

//-----------------------------------------------------------------------------
static std::string accessControlToString(const AccessControl &access)
{
    switch (access)
    {
    case Public:
        return "Public";
    case Protected:
        return "Protected";
    case Private:
        return "Private";
    case Global:
        return "Global";
    case Argument:
        return "Argument";
    case Local:
        return "Local";
    case Throw:
        return "Throw";
    }
    return "Unknown";
}

//-----------------------------------------------------------------------------
static std::string tokenToString(const Token *tok, const Tokenizer *tokenizer)
{
    std::ostringstream oss;
    if (tok)
    {
        oss << tok->str() << " ";
        oss << tokenizer->list.fileLine(tok) << " ";
    }
    oss << tok;
    return oss.str();
}

//-----------------------------------------------------------------------------
static std::string scopeToString(const Scope *scope, const Tokenizer *tokenizer)
{
    std::ostringstream oss;
    if (scope)
    {
        oss << scope->type << " ";
        if (scope->classDef)
            oss << tokenizer->list.fileLine(scope->classDef) << " ";
    }
    oss << scope;
    return oss.str();
}

//-----------------------------------------------------------------------------
static std::string tokenType(const Token *tok)
{
    std::ostringstream oss;
    if (tok)
    {
        oss << tok->str();
    }
    return oss.str();
}

//-----------------------------------------------------------------------------
SymbolDatabase::SymbolDatabase(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger)
    : mTokenizer(tokenizer), mSettings(settings), mErrorLogger(errorLogger)
{
    createSymbolDatabaseFindAllScopes();
    createSymbolDatabaseClassInfo();
    createSymbolDatabaseVariableInfo();
    createSymbolDatabaseFunctionScopes();
    createSymbolDatabaseClassAndStructScopes();
    createSymbolDatabaseFunctionReturnTypes();
    createSymbolDatabaseNeedInitialization();
    createSymbolDatabaseVariableSymbolTable();
    createSymbolDatabaseSetScopePointers();
    createSymbolDatabaseSetFunctionPointers(true);
    createSymbolDatabaseSetVariablePointers();
    createSymbolDatabaseSetTypePointers();
    createSymbolDatabaseEnums();
    createSymbolDatabaseUnknownArrayDimensions();
}

//-----------------------------------------------------------------------------
bool SymbolDatabase::_isValidEnumCode(const Token *tok)
{
    if (tok->str() != "enum" || (false == Token::Match(tok, "enum %name% {")) || Token::Match(tok, "enum {"))
    {
        throw InternalError(tok, "Syntax error. Enum definition is wrong.", InternalError::SYNTAX);
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
bool SymbolDatabase::_isValidStructCode(const Token *tok)
{
    if (tok->str() != "struct")
    {
        throw InternalError(tok, "Syntax error. Struct definition is wrong.", InternalError::SYNTAX);
        return false;
    }

    if (Token::Match(tok, "struct %name% {") || Token::Match(tok, "struct %name% : %name% {"))
    {
        return true;
    }

    throw InternalError(tok, "Syntax error. Struct definition is wrong.", InternalError::SYNTAX);
    return false;
}

//-----------------------------------------------------------------------------
bool SymbolDatabase::_isValidClassCode(const Token *tok)
{
    if (tok->str() != "class")
    {
        throw InternalError(tok, "Syntax error. Class definition is wrong.", InternalError::SYNTAX);
        return false;
    }

    if (Token::Match(tok, "class %name% {") || Token::Match(tok, "class %name% : %name% {"))
    {
        return true;
    }

    throw InternalError(tok, "Syntax error. Class definition is wrong.", InternalError::SYNTAX);
    return false;
}

//-----------------------------------------------------------------------------
void SymbolDatabase::createSymbolDatabaseFindAllScopes()
{
    // create global scope
    scopeList.emplace_back(this, nullptr, nullptr);

    // pointer to current scope
    Scope *scope = &scopeList.back();

    // Store current access in each scope (depends on evaluation progress)
    std::map<const Scope *, AccessControl> access;

    // find all scopes
    for (const Token *tok = mTokenizer->tokens(); tok; tok = tok ? tok->next() : nullptr)
    {
        // #5593 suggested to add here:
        if (mErrorLogger)
        {
            mErrorLogger->reportProgress(mTokenizer->list.getSourceFilePath(),
                                         "SymbolDatabase",
                                         tok->progressValue());
        }
        // Locate next class or enum
        if (Token::Match(tok, "enum|class|struct"))
        {
            // make sure we have valid code
            // error message is throwed in _isValid* funcion
            if (tok->str() == "enum" && false == _isValidEnumCode(tok))
            {
                break;
            }
            else if (tok->str() == "struct" && false == _isValidStructCode(tok))
            {
                break;
            }
            else if (tok->str() == "class" && false == _isValidClassCode(tok))
            {
                break;
            }

            const Token *tokBodyStart = tok->tokAt(2);
            const Token *name = tok->next();
            scopeList.emplace_back(this, tok, scope);
            Scope *new_scope = &scopeList.back();

            if (tok->str() == "class")
                access[new_scope] = Private;
            else if (tok->str() == "struct")
                access[new_scope] = Public;

            // fill typeList...
            Type *new_type = findType(name, scope);
            if (!new_type)
            {
                /// @todo am not sure, but i think this can not happends in ctrl lang
                typeList.emplace_back(new_scope->classDef, new_scope, scope);
                new_type = &typeList.back();
                scope->definedTypesMap[new_type->name()] = new_type;
            }
            else
                new_type->classScope = new_scope;

            new_scope->definedType = new_type;

            // only create base list for classes and structures
            if (new_scope->isClassOrStruct())
            {
                // goto initial '{'
                tokBodyStart = new_scope->definedType->initBaseInfo(tok, tokBodyStart);

                // make sure we have valid code
                if (!tokBodyStart)
                { // defensive, but no body know
                    throw InternalError(tok, "Can not found start of body", InternalError::SYNTAX);
                    break;
                }
            }
            else if (new_scope->type == Scope::eEnum)
            {
                if (tokBodyStart->str() == ":")
                    tokBodyStart = tokBodyStart->tokAt(2);
            }

            new_scope->bodyStart = tokBodyStart;
            new_scope->bodyEnd = tokBodyStart->link();

            // make sure we have valid code
            if (!new_scope->bodyEnd)
            {
                throw InternalError(tok, "Can not found end of body", InternalError::SYNTAX);
                break;
            }

            if (new_scope->type == Scope::eEnum)
            {
                tokBodyStart = new_scope->addEnum(tok);
                scope->nestedList.push_back(new_scope);

                if (!tokBodyStart)
                {
                    throw InternalError(tok, "Can not found start of body", InternalError::SYNTAX);
                    break;
                }
            }
            else
            {
                // make the new scope the current scope
                scope->nestedList.push_back(new_scope);
                scope = new_scope;
            }

            tok = tokBodyStart;
        }

        // check for end of scope
        else if (tok == scope->bodyEnd)
        {
            access.erase(scope);
            scope = const_cast<Scope *>(scope->nestedIn);
            continue;
        }

        // check if in class or structure
        else if (scope->isClassOrStruct())
        {
            const Token *funcStart = nullptr;
            const Token *argStart = nullptr;
            const Token *declEnd = nullptr;

            // get access
            if (Token::Match(tok, "public|protected|private"))
            {
                access[scope] = accessControlFromString(tok->str());
            }

            // class function?
            else if (isFunction(tok, scope, &funcStart, &argStart, &declEnd))
            {
                if (tok->previous()->str() != "::" || tok->strAt(-2) == scope->className)
                {
                    Function function(mTokenizer, tok, scope, funcStart, argStart);

                    // save the access type
                    function.access = access[scope];

                    // go back to default scope
                    access[scope] = scope->defaultAccess();

                    const Token *end = function.argDef->link();

                    // count the number of constructors
                    if (function.isConstructor())
                        scope->numConstructors++;

                    // assume implementation is inline (definition and implementation same)
                    function.token = function.tokenDef;
                    function.arg = function.argDef;

                    // out of line function
                    if (const Token *endTok = mTokenizer->isFunctionHead(end, ";"))
                    {
                        tok = endTok;
                        scope->addFunction(function);
                    }

                    // inline function
                    else
                    {
                        // find start of function '{'
                        bool foundInitList = false;
                        while (end && end->str() != "{" && end->str() != ";")
                        {
                            if (end->link() && Token::Match(end, "(|<"))
                            {
                                end = end->link();
                            }
                            else if (foundInitList &&
                                     Token::Match(end, "%name%|> {") &&
                                     Token::Match(end->linkAt(1), "} ,|{"))
                            {
                                end = end->linkAt(1);
                            }
                            else
                            {
                                if (end->str() == ":")
                                    foundInitList = true;
                                end = end->next();
                            }
                        }

                        if (!end || end->str() == ";")
                            continue;

                        scope->addFunction(function);

                        Function *funcptr = &scope->functionList.back();
                        const Token *tok2 = funcStart;

                        addNewFunction(&scope, &tok2);
                        if (scope)
                        {
                            scope->functionOf = function.nestedIn;
                            scope->function = funcptr;
                            scope->function->functionScope = scope;
                        }

                        tok = tok2;
                    }
                }

                // nested class?
                else
                {
                    /** @todo check entire qualification for match */
                    const Scope *const nested = scope->findInNestedListRecursive(tok->strAt(-2));

                    if (nested)
                        addClassFunction(&scope, &tok, argStart);
                }
            }
        }
        else if (scope->type == Scope::eGlobal)
        {
            const Token *funcStart = nullptr;
            const Token *argStart = nullptr;
            const Token *declEnd = nullptr;

            // function?
            if (isFunction(tok, scope, &funcStart, &argStart, &declEnd))
            {
                // has body?
                if (declEnd && declEnd->str() == "{")
                {
                    tok = funcStart;

                    // class destructor
                    if (tok->previous() && tok->previous()->str() == "~")
                    {
                        addClassFunction(&scope, &tok, argStart);
                    }

                    // regular function
                    else
                    {
                        const Function *const function = addGlobalFunction(scope, tok, argStart, funcStart);

                        if (!function)
                            mTokenizer->syntaxError(tok);
                    }

                    // syntax error?
                    if (!scope)
                        mTokenizer->syntaxError(tok);
                }
            }
        }
        else if (scope->isExecutable())
        {
            if (Token::Match(tok, "else|try|do|catch {"))
            {
                const Token *tok1 = tok->next();
                if (tok->str() == "else")
                    scopeList.emplace_back(this, tok, scope, Scope::eElse, tok1);
                else if (tok->str() == "do")
                    scopeList.emplace_back(this, tok, scope, Scope::eDo, tok1);
                else if (tok->str() == "catch")
                    scopeList.emplace_back(this, tok, scope, Scope::eCatch, tok1);
                else //if (tok->str() == "try")
                    scopeList.emplace_back(this, tok, scope, Scope::eTry, tok1);

                tok = tok1;
                scope->nestedList.push_back(&scopeList.back());
                scope = &scopeList.back();
            }
            else if (Token::Match(tok, "if|for|while|switch (") && Token::simpleMatch(tok->next()->link(), ") {"))
            {
                const Token *scopeStartTok = tok->next()->link()->next();
                if (tok->str() == "if")
                    scopeList.emplace_back(this, tok, scope, Scope::eIf, scopeStartTok);
                else if (tok->str() == "for")
                {
                    scopeList.emplace_back(this, tok, scope, Scope::eFor, scopeStartTok);
                }
                else if (tok->str() == "while")
                {
                    scopeList.emplace_back(this, tok, scope, Scope::eWhile, scopeStartTok);
                }
                else // if (tok->str() == "switch")
                    scopeList.emplace_back(this, tok, scope, Scope::eSwitch, scopeStartTok);

                scope->nestedList.push_back(&scopeList.back());
                scope = &scopeList.back();
                if (scope->type == Scope::eFor)
                    scope->checkVariable(tok->tokAt(2), Local, mSettings); // check for variable declaration and add it to new scope if found
                tok = scopeStartTok;
            }
            else if (tok->str() == "{")
            {
                if (tok->previous()->varId())
                    tok = tok->link();
                else
                {
                    const Token *tok2 = tok->previous();
                    while (!Token::Match(tok2, ";|}|{|)"))
                        tok2 = tok2->previous();
                    if (tok2->next() != tok && tok2->strAt(1) != ".")
                        tok2 = nullptr; // No lambda

                    if (!Token::Match(tok->previous(), "=|,|(|return") && !(tok->strAt(-1) == ")" && Token::Match(tok->linkAt(-1)->previous(), "=|,|(|return")))
                    {
                        scopeList.emplace_back(this, tok, scope, Scope::eUnconditional, tok);
                        scope->nestedList.push_back(&scopeList.back());
                        scope = &scopeList.back();
                    }
                    else
                    {
                        tok = tok->link();
                    }
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
void SymbolDatabase::createSymbolDatabaseClassInfo()
{
    // fill in using info
    for (std::list<Scope>::iterator it = scopeList.begin(); it != scopeList.end(); ++it)
    {
        for (std::list<Scope::UsingInfo>::iterator i = it->usingList.begin(); i != it->usingList.end(); ++i)
        {
            // only find if not already found
            if (i->scope == nullptr)
            {
                // check scope for match
                const Scope *const scope = findScope(i->start->tokAt(2), &(*it));
                if (scope)
                {
                    // set found scope
                    i->scope = scope;
                    break;
                }
            }
        }
    }

    // fill in base class info
    for (std::list<Type>::iterator it = typeList.begin(); it != typeList.end(); ++it)
    {
        // finish filling in base class info
        for (unsigned int i = 0; i < it->derivedFrom.size(); ++i)
        {
            const Type *found = findType(it->derivedFrom[i].nameTok, it->enclosingScope);
            if (found && found->findDependency(&(*it)))
            {
                // circular dependency
                //mTokenizer->syntaxError(nullptr);
            }
            else
            {
                it->derivedFrom[i].type = found;
            }
        }
    }
}

//-----------------------------------------------------------------------------
void SymbolDatabase::createSymbolDatabaseVariableInfo()
{
    // fill in variable info
    for (std::list<Scope>::iterator it = scopeList.begin(); it != scopeList.end(); ++it)
    {
        // find variables
        it->getVariableList(mSettings);
    }

    // fill in function arguments
    for (std::list<Scope>::iterator it = scopeList.begin(); it != scopeList.end(); ++it)
    {
        std::list<Function>::iterator func;

        for (func = it->functionList.begin(); func != it->functionList.end(); ++func)
        {
            // add arguments
            func->addArguments(this, &*it);
        }
    }
}

//-----------------------------------------------------------------------------
void SymbolDatabase::createSymbolDatabaseFunctionScopes()
{
    // fill in function scopes
    for (std::list<Scope>::iterator it = scopeList.begin(); it != scopeList.end(); ++it)
    {
        if (it->type == Scope::eFunction)
            functionScopes.push_back(&*it);
    }
}

//-----------------------------------------------------------------------------
void SymbolDatabase::createSymbolDatabaseClassAndStructScopes()
{
    // fill in class and struct scopes
    for (std::list<Scope>::iterator it = scopeList.begin(); it != scopeList.end(); ++it)
    {
        if (it->isClassOrStruct())
            classAndStructScopes.push_back(&*it);
    }
}

//-----------------------------------------------------------------------------
void SymbolDatabase::createSymbolDatabaseFunctionReturnTypes()
{
    // fill in function return types
    for (std::list<Scope>::iterator it = scopeList.begin(); it != scopeList.end(); ++it)
    {
        std::list<Function>::iterator func;

        for (func = it->functionList.begin(); func != it->functionList.end(); ++func)
        {
            // add return types
            if (func->retDef)
            {
                const Token *type = func->retDef;
                while (Token::Match(type, "static|const|struct|class|enum"))
                    type = type->next();
                if (type)
                {
                    func->retType = findVariableTypeInBase(&*it, type);
                    if (!func->retType)
                        func->retType = findTypeInNested(type, func->nestedIn);
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
void SymbolDatabase::createSymbolDatabaseNeedInitialization()
{
    // For C++, it is more difficult: Determine if user defined type needs initialization...
    unsigned int unknowns = 0; // stop checking when there are no unknowns
    unsigned int retry = 0;    // bail if we don't resolve all the variable types for some reason

    do
    {
        unknowns = 0;

        for (std::list<Scope>::iterator it = scopeList.begin(); it != scopeList.end(); ++it)
        {
            Scope *scope = &(*it);

            if (!scope->definedType)
            {
                mBlankTypes.push_back(Type());
                scope->definedType = &mBlankTypes.back();
            }

            if (scope->isClassOrStruct() && scope->definedType->needInitialization == Type::Unknown)
            {
                // check for default constructor
                bool hasDefaultConstructor = false;

                std::list<Function>::const_iterator func;

                for (func = scope->functionList.begin(); func != scope->functionList.end(); ++func)
                {
                    if (func->type == Function::eConstructor)
                    {
                        // check for no arguments: func ( )
                        if (func->argCount() == 0)
                        {
                            hasDefaultConstructor = true;
                            break;
                        }

                        /** check for arguments with default values */
                        else if (func->argCount() == func->initializedArgCount())
                        {
                            hasDefaultConstructor = true;
                            break;
                        }
                    }
                }

                // User defined types with user defined default constructor doesn't need initialization.
                // We assume the default constructor initializes everything.
                // Another check will figure out if the constructor actually initializes everything.
                if (hasDefaultConstructor)
                    scope->definedType->needInitialization = Type::False;

                // check each member variable to see if it needs initialization
                else
                {
                    bool needInitialization = false;
                    bool unknown = false;

                    std::list<Variable>::const_iterator var;
                    for (var = scope->varlist.begin(); var != scope->varlist.end() && !needInitialization; ++var)
                    {
                        if (var->isClass())
                        {
                            if (var->type())
                            {
                                // does this type need initialization?
                                if (var->type()->needInitialization == Type::True)
                                    needInitialization = true;
                            }
                        }
                        else if (!var->hasDefault())
                            needInitialization = true;
                    }

                    if (needInitialization)
                        scope->definedType->needInitialization = Type::True;
                    else if (!unknown)
                        scope->definedType->needInitialization = Type::False;
                    else
                    {
                        if (scope->definedType->needInitialization == Type::Unknown)
                            unknowns++;
                    }
                }
            }
            else if (scope->definedType->needInitialization == Type::Unknown)
                scope->definedType->needInitialization = Type::True;
        }

        retry++;
    } while (unknowns && retry < 100);

    // this shouldn't happen so output a debug warning
    if (retry == 100 && mSettings->debugwarnings)
    {
        for (std::list<Scope>::iterator it = scopeList.begin(); it != scopeList.end(); ++it)
        {
            const Scope *scope = &(*it);

            if (scope->isClassOrStruct() && scope->definedType->needInitialization == Type::Unknown)
                debugMessage(scope->classDef, "SymbolDatabase::SymbolDatabase couldn't resolve all user defined types.");
        }
    }
}

//-----------------------------------------------------------------------------
void SymbolDatabase::createSymbolDatabaseVariableSymbolTable()
{
    // create variable symbol table
    mVariableList.resize(mTokenizer->varIdCount() + 1);
    std::fill_n(mVariableList.begin(), mVariableList.size(), (const Variable *)nullptr);

    // check all scopes for variables
    for (std::list<Scope>::iterator it = scopeList.begin(); it != scopeList.end(); ++it)
    {
        Scope *scope = &(*it);

        // add all variables
        for (std::list<Variable>::iterator var = scope->varlist.begin(); var != scope->varlist.end(); ++var)
        {
            const unsigned int varId = var->declarationId();
            if (varId)
                mVariableList[varId] = &(*var);
            // fix up variables without type
            if (!var->type() && !var->typeStartToken()->isStandardType())
            {
                const Type *type = findType(var->typeStartToken(), scope);
                if (type)
                    var->type(type);
            }
        }

        // add all function parameters
        for (std::list<Function>::iterator func = scope->functionList.begin(); func != scope->functionList.end(); ++func)
        {
            for (std::list<Variable>::iterator arg = func->argumentList.begin(); arg != func->argumentList.end(); ++arg)
            {
                // check for named parameters
                if (arg->nameToken() && arg->declarationId())
                {
                    const unsigned int declarationId = arg->declarationId();
                    if (declarationId > 0U)
                        mVariableList[declarationId] = &(*arg);
                    // fix up parameters without type
                    if (!arg->type() && !arg->typeStartToken()->isStandardType())
                    {
                        const Type *type = findTypeInNested(arg->typeStartToken(), scope);
                        if (type)
                            arg->type(type);
                    }
                }
            }
        }
    }

    // fill in missing variables if possible
    const std::size_t functions = functionScopes.size();
    for (std::size_t i = 0; i < functions; ++i)
    {
        const Scope *func = functionScopes[i];
        for (const Token *tok = func->bodyStart->next(); tok && tok != func->bodyEnd; tok = tok->next())
        {
            // check for member variable
            if (tok->varId() && tok->next() &&
                (tok->next()->str() == "." ||
                 (tok->next()->str() == "[" && tok->linkAt(1)->strAt(1) == ".")))
            {
                const Token *tok1 = tok->next()->str() == "." ? tok->tokAt(2) : tok->linkAt(1)->tokAt(2);
                if (tok1 && tok1->varId() && mVariableList[tok1->varId()] == 0)
                {
                    const Variable *var = mVariableList[tok->varId()];
                    if (var && var->typeScope())
                    {
                        // find the member variable of this variable
                        const Variable *var1 = var->typeScope()->getVariable(tok1->str());
                        if (var1)
                        {
                            // add this variable to the look up table
                            mVariableList[tok1->varId()] = var1;
                        }
                    }
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
void SymbolDatabase::createSymbolDatabaseSetScopePointers()
{
    // Set scope pointers
    for (std::list<Scope>::iterator it = scopeList.begin(); it != scopeList.end(); ++it)
    {
        Token *start = const_cast<Token *>(it->bodyStart);
        Token *end = const_cast<Token *>(it->bodyEnd);
        if (it->type == Scope::eGlobal)
        {
            start = const_cast<Token *>(mTokenizer->list.front());
            end = const_cast<Token *>(mTokenizer->list.back());
        }
        assert(start && end);

        end->scope(&*it);

        for (Token *tok = start; tok != end; tok = tok->next())
        {
            if (start != end && tok->str() == "{")
            {
                bool isEndOfScope = false;
                for (std::list<Scope *>::const_iterator innerScope = it->nestedList.begin(); innerScope != it->nestedList.end(); ++innerScope)
                {
                    if (tok == (*innerScope)->bodyStart)
                    { // Is begin of inner scope
                        tok = tok->link();
                        if (tok->next() == end || !tok->next())
                        {
                            isEndOfScope = true;
                            break;
                        }
                        tok = tok->next();
                        break;
                    }
                }
                if (isEndOfScope)
                    break;
            }
            tok->scope(&*it);
        }
    }
}

//-----------------------------------------------------------------------------
void SymbolDatabase::createSymbolDatabaseSetFunctionPointers(bool firstPass)
{
    if (firstPass)
    {
        // Set function definition and declaration pointers
        for (std::list<Scope>::iterator it = scopeList.begin(); it != scopeList.end(); ++it)
        {
            for (std::list<Function>::const_iterator func = it->functionList.begin(); func != it->functionList.end(); ++func)
            {
                if (func->tokenDef)
                    const_cast<Token *>(func->tokenDef)->function(&*func);

                if (func->token)
                    const_cast<Token *>(func->token)->function(&*func);
            }
        }
    }

    // Set function call pointers
    for (const Token *tok = mTokenizer->list.front(); tok != mTokenizer->list.back(); tok = tok->next())
    {
        if (!tok->function() && tok->varId() == 0 && Token::Match(tok, "%name% (") && !isReservedName(tok->str()))
        {
            const Function *function = findFunction(tok);
            if (function)
                const_cast<Token *>(tok)->function(function);
        }
    }

    // Set C++ 11 delegate constructor function call pointers
    for (std::list<Scope>::iterator it = scopeList.begin(); it != scopeList.end(); ++it)
    {
        for (std::list<Function>::const_iterator func = it->functionList.begin(); func != it->functionList.end(); ++func)
        {
            // look for initializer list
            if (func->isConstructor() && func->functionScope && func->functionScope->functionOf && func->arg)
            {
                const Token *tok = func->arg->link()->next();
                if (tok->str() == "noexcept")
                {
                    const Token *closingParenTok = tok->linkAt(1);
                    if (!closingParenTok || !closingParenTok->next())
                    {
                        continue;
                    }
                    tok = closingParenTok->next();
                }
                if (tok->str() != ":")
                {
                    continue;
                }
                tok = tok->next();
                while (tok && tok != func->functionScope->bodyStart)
                {
                    if (Token::Match(tok, "%name% {|("))
                    {
                        if (tok->str() == func->tokenDef->str())
                        {
                            const Function *function = func->functionScope->functionOf->findFunction(tok);
                            if (function)
                                const_cast<Token *>(tok)->function(function);
                            break;
                        }
                        tok = tok->linkAt(1);
                    }
                    tok = tok->next();
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
void SymbolDatabase::createSymbolDatabaseSetTypePointers()
{
    std::set<std::string> typenames;
    for (const Type &t : typeList)
    {
        typenames.insert(t.name());
    }

    // Set type pointers
    for (const Token *tok = mTokenizer->list.front(); tok != mTokenizer->list.back(); tok = tok->next())
    {
        if (!tok->isName() || tok->varId() || tok->function() || tok->type() || tok->enumerator())
            continue;

        if (typenames.find(tok->str()) == typenames.end())
            continue;

        const Type *type = findVariableType(tok->scope(), tok);
        if (type)
            const_cast<Token *>(tok)->type(type);
    }
}

//-----------------------------------------------------------------------------
void SymbolDatabase::fixVarId(VarIdMap &varIds, const Token *vartok, Token *membertok, const Variable *membervar)
{
    VarIdMap::iterator varId = varIds.find(vartok->varId());
    if (varId == varIds.end())
    {
        MemberIdMap memberId;
        if (membertok->varId() == 0)
        {
            memberId[membervar->nameToken()->varId()] = const_cast<Tokenizer *>(mTokenizer)->newVarId();
            mVariableList.push_back(membervar);
        }
        else
            mVariableList[membertok->varId()] = membervar;
        varIds.insert(std::make_pair(vartok->varId(), memberId));
        varId = varIds.find(vartok->varId());
    }
    MemberIdMap::iterator memberId = varId->second.find(membervar->nameToken()->varId());
    if (memberId == varId->second.end())
    {
        if (membertok->varId() == 0)
        {
            varId->second.insert(std::make_pair(membervar->nameToken()->varId(), const_cast<Tokenizer *>(mTokenizer)->newVarId()));
            mVariableList.push_back(membervar);
            memberId = varId->second.find(membervar->nameToken()->varId());
        }
        else
            mVariableList[membertok->varId()] = membervar;
    }
    if (membertok->varId() == 0)
        membertok->setVarId(memberId->second);
}

//-----------------------------------------------------------------------------
void SymbolDatabase::createSymbolDatabaseSetVariablePointers()
{
    VarIdMap varIds;

    // Set variable pointers
    for (const Token *tok = mTokenizer->list.front(); tok != mTokenizer->list.back(); tok = tok->next())
    {
        if (tok->varId())
            const_cast<Token *>(tok)->variable(getVariableFromVarId(tok->varId()));

        // Set Token::variable pointer for array member variable
        // Since it doesn't point at a fixed location it doesn't have varid
        if (tok->variable() != nullptr &&
            tok->variable()->typeScope() &&
            Token::Match(tok, "%name% [|."))
        {

            Token *tok2 = tok->next();
            // Locate "]"
            while (tok2 && tok2->str() == "[")
                tok2 = tok2->link()->next();

            Token *membertok = nullptr;
            if (Token::Match(tok2, ". %name%"))
                membertok = tok2->next();
            else if (Token::Match(tok2, ") . %name%") && tok->strAt(-1) == "(")
                membertok = tok2->tokAt(2);

            if (membertok)
            {
                const Variable *var = tok->variable();
                if (var && var->typeScope())
                {
                    const Variable *membervar = var->typeScope()->getVariable(membertok->str());
                    if (membervar)
                    {
                        membertok->variable(membervar);
                        if (membertok->varId() == 0 || mVariableList[membertok->varId()] == nullptr)
                            fixVarId(varIds, tok, const_cast<Token *>(membertok), membervar);
                    }
                }
            }
        }

        // check for function returning record type
        // func(...).var
        // func(...)[...].var
        else if (tok->function() && tok->next()->str() == "(" &&
                 (Token::Match(tok->next()->link(), ") . %name% !!(") ||
                  (Token::Match(tok->next()->link(), ") [") && Token::Match(tok->next()->link()->next()->link(), "] . %name% !!("))))
        {
            const Type *type = tok->function()->retType;
            if (type)
            {
                Token *membertok;
                if (tok->next()->link()->next()->str() == ".")
                    membertok = tok->next()->link()->next()->next();
                else
                    membertok = tok->next()->link()->next()->link()->next()->next();
                const Variable *membervar = membertok->variable();
                if (!membervar)
                {
                    if (type->classScope)
                    {
                        membervar = type->classScope->getVariable(membertok->str());
                        if (membervar)
                        {
                            membertok->variable(membervar);
                            if (membertok->varId() == 0 || mVariableList[membertok->varId()] == nullptr)
                            {
                                if (tok->function()->retDef)
                                    fixVarId(varIds, tok->function()->retDef, const_cast<Token *>(membertok), membervar);
                            }
                        }
                    }
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
void SymbolDatabase::createSymbolDatabaseEnums()
{
    // fill in enumerators in enum
    for (std::list<Scope>::iterator it = scopeList.begin(); it != scopeList.end(); ++it)
    {
        if (it->type != Scope::eEnum)
            continue;

        // add enumerators to enumerator tokens
        for (std::size_t i = 0, end = it->enumeratorList.size(); i < end; ++i)
            const_cast<Token *>(it->enumeratorList[i].name)->enumerator(&it->enumeratorList[i]);
    }

    // fill in enumerator values
    for (std::list<Scope>::iterator it = scopeList.begin(); it != scopeList.end(); ++it)
    {
        if (it->type != Scope::eEnum)
            continue;

        MathLib::bigint value = 0;

        for (std::size_t i = 0, end = it->enumeratorList.size(); i < end; ++i)
        {
            Enumerator &enumerator = it->enumeratorList[i];

            // look for initialization tokens that can be converted to enumerators and convert them
            if (enumerator.start)
            {
                if (!enumerator.end)
                    mTokenizer->syntaxError(enumerator.start);
                for (const Token *tok3 = enumerator.start; tok3 && tok3 != enumerator.end->next(); tok3 = tok3->next())
                {
                    if (tok3->tokType() == Token::eName)
                    {
                        const Enumerator *e = findEnumerator(tok3);
                        if (e)
                            const_cast<Token *>(tok3)->enumerator(e);
                    }
                }

                // look for possible constant folding expressions
                // rhs of operator:
                const Token *rhs = enumerator.start->previous()->astOperand2();

                // constant folding of expression:
                ValueFlow::valueFlowConstantFoldAST(rhs, mSettings);

                // get constant folded value:
                if (rhs && rhs->hasKnownIntValue())
                {
                    enumerator.value = rhs->values().front().intvalue;
                    enumerator.value_known = true;
                    value = enumerator.value + 1;
                }
            }

            // not initialized so use default value
            else
            {
                enumerator.value = value++;
                enumerator.value_known = true;
            }
        }
    }

    // find enumerators
    for (const Token *tok = mTokenizer->list.front(); tok != mTokenizer->list.back(); tok = tok->next())
    {
        if (tok->tokType() != Token::eName)
            continue;
        const Enumerator *enumerator = findEnumerator(tok);
        if (enumerator)
            const_cast<Token *>(tok)->enumerator(enumerator);
    }
}

//-----------------------------------------------------------------------------
void SymbolDatabase::createSymbolDatabaseUnknownArrayDimensions()
{
    // set all unknown array dimensions
    for (const Variable *var : mVariableList)
    {
        // check each array variable
        if (!var || !var->isArray())
            continue;
        // check each array dimension
        for (const Dimension &const_dimension : var->dimensions())
        {
            Dimension &dimension = const_cast<Dimension &>(const_dimension);
            if (dimension.num != 0)
                continue;
            dimension.known = false;
            // check for a single token dimension
            if (dimension.start && (dimension.start == dimension.end))
            {
                // check for an enumerator
                if (dimension.start->enumerator())
                {
                    if (dimension.start->enumerator()->value_known)
                    {
                        dimension.num = dimension.start->enumerator()->value;
                        dimension.known = true;
                    }
                }

                // check for a variable
                else if (dimension.start->varId())
                {
                    // get maximum size from type
                    // find where this type is defined
                    const Variable *var = getVariableFromVarId(dimension.start->varId());

                    // make sure it is in the database
                    if (!var)
                        break;
                    // get type token
                    const Token *index_type = var->typeEndToken();

                    if (index_type->str() == "char")
                    {
                        if (index_type->isUnsigned())
                            dimension.num = UCHAR_MAX + 1;
                        else if (index_type->isSigned())
                            dimension.num = SCHAR_MAX + 1;
                        else
                            dimension.num = CHAR_MAX + 1;
                    }
                    else if (index_type->str() == "short")
                    {
                        if (index_type->isUnsigned())
                            dimension.num = USHRT_MAX + 1;
                        else
                            dimension.num = SHRT_MAX + 1;
                    }

                    // checkScope assumes size is signed int so we limit the following sizes to INT_MAX
                    else if (index_type->str() == "int")
                    {
                        if (index_type->isUnsigned())
                            dimension.num = UINT_MAX + 1ULL;
                        else
                            dimension.num = INT_MAX + 1ULL;
                    }
                    else if (index_type->str() == "long")
                    {
                        if (index_type->isUnsigned())
                        {
                            if (index_type->isLong())
                                dimension.num = ULLONG_MAX; // should be ULLONG_MAX + 1ULL
                            else
                                dimension.num = ULONG_MAX; // should be ULONG_MAX + 1ULL
                        }
                        else
                        {
                            if (index_type->isLong())
                                dimension.num = LLONG_MAX; // should be LLONG_MAX + 1LL
                            else
                                dimension.num = LONG_MAX; // should be LONG_MAX + 1LL
                        }
                    }
                }
            }
            // check for qualified enumerator
            else if (dimension.start)
            {
                // rhs of [
                const Token *rhs = dimension.start->previous()->astOperand2();

                // constant folding of expression:
                ValueFlow::valueFlowConstantFoldAST(rhs, mSettings);

                // get constant folded value:
                if (rhs && rhs->hasKnownIntValue())
                {
                    dimension.num = rhs->values().front().intvalue;
                    dimension.known = true;
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
SymbolDatabase::~SymbolDatabase()
{
    // Clear scope, type, function and variable pointers
    for (const Token *tok = mTokenizer->list.front(); tok; tok = tok->next())
    {
        const_cast<Token *>(tok)->scope(nullptr);
        const_cast<Token *>(tok)->type(nullptr);
        const_cast<Token *>(tok)->function(nullptr);
        const_cast<Token *>(tok)->variable(nullptr);
        const_cast<Token *>(tok)->enumerator(nullptr);
        const_cast<Token *>(tok)->setValueType(nullptr);
    }
}

//-----------------------------------------------------------------------------
void SymbolDatabase::validate() const
{
    if (mSettings->debugwarnings)
    {
        validateExecutableScopes();
    }
    //validateVariables();
}

//-----------------------------------------------------------------------------
Function *SymbolDatabase::addGlobalFunction(Scope *&scope, const Token *&tok, const Token *argStart, const Token *funcStart)
{
    Function *function = nullptr;
    for (std::multimap<std::string, const Function *>::iterator i = scope->functionMap.find(tok->str()); i != scope->functionMap.end() && i->first == tok->str(); ++i)
    {
        const Function *f = i->second;
        if (f->hasBody())
            continue;
        if (Function::argsMatch(scope, f->argDef, argStart, emptyString, 0))
        {
            function = const_cast<Function *>(i->second);
            break;
        }
    }

    if (!function)
        function = addGlobalFunctionDecl(scope, tok, argStart, funcStart);

    function->arg = argStart;
    function->token = funcStart;
    function->hasBody(true);

    addNewFunction(&scope, &tok);

    if (scope)
    {
        scope->function = function;
        function->functionScope = scope;
        return function;
    }
    return nullptr;
}

//-----------------------------------------------------------------------------
Function *SymbolDatabase::addGlobalFunctionDecl(Scope *&scope, const Token *tok, const Token *argStart, const Token *funcStart)
{
    Function function(mTokenizer, tok, scope, funcStart, argStart);
    scope->addFunction(function);
    return &scope->functionList.back();
}

//-----------------------------------------------------------------------------
void SymbolDatabase::addClassFunction(Scope **scope, const Token **tok, const Token *argStart)
{
    const bool destructor((*tok)->previous()->str() == "~");

    int count = 0;
    std::string path;
    unsigned int path_length = 0;
    const Token *tok1 = (*tok);

    if (destructor)
        tok1 = tok1->previous();

    // back up to head of path
    while (tok1 && tok1->previous() && tok1->previous()->str() == "::" && tok1->tokAt(-2) &&
           (tok1->tokAt(-2)->isName() ||
            (tok1->strAt(-2) == ">" && tok1->linkAt(-2) && Token::Match(tok1->linkAt(-2)->previous(), "%name%"))))
    {
        count++;
        const Token *tok2 = tok1->tokAt(-2);
        if (tok2->str() == ">")
            tok2 = tok2->link()->previous();

        if (tok2)
        {
            do
            {
                path = tok1->previous()->str() + " " + path;
                tok1 = tok1->previous();
                path_length++;
            } while (tok1 != tok2);
        }
        else
            return; // syntax error ?
    }

    // syntax error?
    if (!tok1)
        return;

    std::list<Scope>::iterator it1;

    // search for match
    for (it1 = scopeList.begin(); it1 != scopeList.end(); ++it1)
    {
        Scope *scope1 = &(*it1);

        bool match = false;

        // check in namespace if using found
        if (*scope == scope1 && !scope1->usingList.empty())
        {
            std::list<Scope::UsingInfo>::const_iterator it2;
            for (it2 = scope1->usingList.begin(); it2 != scope1->usingList.end(); ++it2)
            {
                if (it2->scope)
                {
                    Function *func = findFunctionInScope(tok1, it2->scope, path, path_length);
                    if (func)
                    {
                        if (!func->hasBody())
                        {
                            func->hasBody(true);
                            func->token = *tok;
                            func->arg = argStart;
                            addNewFunction(scope, tok);
                            if (*scope)
                            {
                                (*scope)->functionOf = func->nestedIn;
                                (*scope)->function = func;
                                (*scope)->function->functionScope = *scope;
                            }
                            return;
                        }
                    }
                }
            }
        }

        if (scope1->className == tok1->str() && (scope1->type != Scope::eFunction))
        {
            // do the scopes match (same scope) or do their names match (multiple namespaces)
            if ((*scope == scope1->nestedIn) || (*scope &&
                                                 (*scope)->className == scope1->nestedIn->className &&
                                                 !(*scope)->className.empty() &&
                                                 (*scope)->type == scope1->nestedIn->type))
            {

                // nested scopes => check that they match
                {
                    const Scope *s1 = *scope;
                    const Scope *s2 = scope1->nestedIn;
                    while (s1 && s2)
                    {
                        if (s1->className != s2->className)
                            break;
                        s1 = s1->nestedIn;
                        s2 = s2->nestedIn;
                    }
                    // Not matching scopes
                    if (s1 || s2)
                        continue;
                }

                Scope *scope2 = scope1;

                while (scope2 && count > 1)
                {
                    count--;
                    if (tok1->strAt(1) == "<")
                        tok1 = tok1->linkAt(1)->tokAt(2);
                    else
                        tok1 = tok1->tokAt(2);
                    scope2 = scope2->findRecordInNestedList(tok1->str());
                }

                if (count == 1 && scope2)
                {
                    match = true;
                    scope1 = scope2;
                }
            }
        }

        if (match)
        {
            for (std::multimap<std::string, const Function *>::iterator it = scope1->functionMap.find((*tok)->str()); it != scope1->functionMap.end() && it->first == (*tok)->str(); ++it)
            {
                Function *func = const_cast<Function *>(it->second);
                if (!func->hasBody())
                {
                    if (Function::argsMatch(scope1, func->argDef, (*tok)->next(), path, path_length))
                    {
                        if (func->type == Function::eDestructor && destructor)
                        {
                            func->hasBody(true);
                        }
                        else if (func->type != Function::eDestructor && !destructor)
                        {
                            // normal function?
                            if ((*tok)->next()->link())
                            {
                                const bool hasConstKeyword = (*tok)->next()->link()->next()->str() == "const";
                                if (func->isConst() == hasConstKeyword)
                                {
                                    func->hasBody(true);
                                }
                            }
                        }

                        if (func->hasBody())
                        {
                            func->token = *tok;
                            func->arg = argStart;
                            addNewFunction(scope, tok);
                            if (*scope)
                            {
                                (*scope)->functionOf = scope1;
                                (*scope)->function = func;
                                (*scope)->function->functionScope = *scope;
                            }
                            return;
                        }
                    }
                }
            }
        }
    }

    // class function of unknown class
    addNewFunction(scope, tok);
}

//-----------------------------------------------------------------------------
void SymbolDatabase::addNewFunction(Scope **scope, const Token **tok)
{
    const Token *tok1 = *tok;
    scopeList.emplace_back(this, tok1, *scope);
    Scope *newScope = &scopeList.back();

    // find start of function '{'
    bool foundInitList = false;
    while (tok1 && tok1->str() != "{" && tok1->str() != ";")
    {
        if (tok1->link() && Token::Match(tok1, "(|<"))
        {
            tok1 = tok1->link();
        }
        else if (foundInitList &&
                 Token::Match(tok1, "%name%|> {") &&
                 Token::Match(tok1->linkAt(1), "} ,|{"))
        {
            tok1 = tok1->linkAt(1);
        }
        else
        {
            if (tok1->str() == ":")
                foundInitList = true;
            tok1 = tok1->next();
        }
    }

    if (tok1 && tok1->str() == "{")
    {
        newScope->bodyStart = tok1;
        newScope->bodyEnd = tok1->link();

        // syntax error?
        if (!newScope->bodyEnd)
        {
            scopeList.pop_back();
            while (tok1->next())
                tok1 = tok1->next();
            *scope = nullptr;
            *tok = tok1;
            return;
        }

        (*scope)->nestedList.push_back(newScope);
        *scope = newScope;
        *tok = tok1;
    }
    else
    {
        scopeList.pop_back();
        *scope = nullptr;
        *tok = nullptr;
    }
}

//-----------------------------------------------------------------------------
void SymbolDatabase::debugMessage(const Token *tok, const std::string &msg) const
{
    std::cout << "debugMessage:: " << msg << std::endl;
    if (tok /*&& mSettings->debugwarnings*/)
    {
        const std::list<const Token *> locationList(1, tok);
        const ErrorLogger::ErrorMessage errmsg(locationList, &mTokenizer->list,
                                               Severity::debug,
                                               "debug",
                                               msg,
                                               false);
        if (mErrorLogger)
            mErrorLogger->reportErr(errmsg);
    }
}

//-----------------------------------------------------------------------------
bool SymbolDatabase::isFunction(const Token *tok, const Scope *outerScope, const Token **funcStart, const Token **argStart, const Token **declEnd) const
{
    if (tok->varId())
    {
        return false;
    }
    // regular function?
    else if (Token::Match(tok, "%name% (") && !isReservedName(tok->str()) && tok->previous() &&
             (Token::Match(tok->previous(), "%name%|>|&|*|::|~") || // Either a return type or scope qualifier in front of tok
              outerScope->isClassOrStruct()))
    { // or a ctor/dtor
        const Token *tok1 = tok->previous();
        const Token *tok2 = tok->next()->link()->next();

        if (!mTokenizer->isFunctionHead(tok->next(), ";:{"))
            return false;

        // skip over destructor "~"
        if (tok1->str() == "~")
            tok1 = tok1->previous();

        // skip over qualification
        while (Token::simpleMatch(tok1, "::"))
        {
            tok1 = tok1->previous();
            if (Token::Match(tok1, "%name%"))
                tok1 = tok1->previous();
            else if (tok1 && tok1->str() == ">" && tok1->link() && Token::Match(tok1->link()->previous(), "%name%"))
                tok1 = tok1->link()->tokAt(-2);
        }

        // done if constructor or destructor
        if (!Token::Match(tok1, "{|}|;|public|protected|private") && tok1)
        {
            // skip over pointers and references
            while (Token::Match(tok1, "%type%|*|&") && !endsWith(tok1->str(), ':') && (!isReservedName(tok1->str()) || tok1->str() == "const"))
                tok1 = tok1->previous();

            // skip over template
            if (tok1 && tok1->str() == ">")
            {
                if (tok1->link())
                    tok1 = tok1->link()->previous();
                else
                    return false;
            }

            // function can't have number or variable as return type
            if (tok1 && (tok1->isNumber() || tok1->varId()))
                return false;

            // skip over return type
            if (Token::Match(tok1, "%name%"))
            {
                if (tok1->str() == "return")
                    return false;
                tok1 = tok1->previous();
            }

            // skip over qualification
            while (Token::simpleMatch(tok1, "::"))
            {
                tok1 = tok1->previous();
                if (Token::Match(tok1, "%name%"))
                    tok1 = tok1->previous();
                else if (tok1 && tok1->str() == ">" && tok1->link() && Token::Match(tok1->link()->previous(), "%name%"))
                    tok1 = tok1->link()->tokAt(-2);
            }

            // skip over modifiers and other stuff
            while (Token::Match(tok1, "const|static|struct|class|enum|%name%|synchronized"))
            {
                tok1 = tok1->previous();
            }

            // should be at a sequence point if this is a function
            if (!Token::Match(tok1, ">|{|}|;|public|protected|private") && tok1)
                return false;
        }

        if (tok2 &&
            (Token::Match(tok2, ";|{|=") ||
             (tok2->isUpperCaseName() && Token::Match(tok2, "%name% ;|{")) ||
             (tok2->isUpperCaseName() && Token::Match(tok2, "%name% (") && tok2->next()->link()->strAt(1) == "{") ||
             Token::Match(tok2, ": ::| %name% (|::|<|{") ||
             Token::Match(tok2, "&|&&| ;|{") ||
             Token::Match(tok2, "= delete|default ;")))
        {
            *funcStart = tok;
            *argStart = tok->next();
            *declEnd = Token::findmatch(tok2, "{|;");
            return true;
        }
    }

    // UNKNOWN_MACRO(a,b) { ... }
    else if (outerScope->type == Scope::eGlobal &&
             Token::Match(tok, "%name% (") &&
             tok->isUpperCaseName() &&
             Token::simpleMatch(tok->linkAt(1), ") {") &&
             (!tok->previous() || Token::Match(tok->previous(), "[;{}]")))
    {
        *funcStart = tok;
        *argStart = tok->next();
        *declEnd = tok->linkAt(1)->next();
        return true;
    }

    // regular C function with missing return or invalid C++ ?
    else if (Token::Match(tok, "%name% (") && !isReservedName(tok->str()) &&
             Token::simpleMatch(tok->linkAt(1), ") {") &&
             (!tok->previous() || Token::Match(tok->previous(), ";|}")))
    {
        //debugMessage(tok, "SymbolDatabase::isFunction found CTRL function without a return type: " + tok->str());

        *funcStart = tok;
        *argStart = tok->next();
        *declEnd = tok->linkAt(1)->next();
        return true;
    }

    return false;
}

//-----------------------------------------------------------------------------
void SymbolDatabase::validateExecutableScopes() const
{
    const std::size_t functions = functionScopes.size();
    for (std::size_t i = 0; i < functions; ++i)
    {
        const Scope *const scope = functionScopes[i];
        const Function *const function = scope->function;
        if (scope->isExecutable() && !function)
        {
            const std::list<const Token *> callstack(1, scope->classDef);
            const std::string msg = std::string("Executable scope '") + scope->classDef->str() + "' with unknown function.";
            const ErrorLogger::ErrorMessage errmsg(callstack, &mTokenizer->list, Severity::debug,
                                                   "symbolDatabaseWarning",
                                                   msg,
                                                   false);
            mErrorLogger->reportErr(errmsg);
        }
    }
}

//-----------------------------------------------------------------------------
void SymbolDatabase::printVariable(const Variable *var, const char *indent) const
{
    std::cout << indent << "mNameToken: " << tokenToString(var->nameToken(), mTokenizer) << std::endl;
    if (var->nameToken())
    {
        std::cout << indent << "    declarationId: " << var->declarationId() << std::endl;
    }
    std::cout << indent << "mTypeStartToken: " << tokenToString(var->typeStartToken(), mTokenizer) << std::endl;
    std::cout << indent << "mTypeEndToken: " << tokenToString(var->typeEndToken(), mTokenizer) << std::endl;

    const Token *autoTok = nullptr;
    std::cout << indent << "   ";
    for (const Token *tok = var->typeStartToken(); tok != var->typeEndToken()->next(); tok = tok->next())
    {
        std::cout << " " << tokenType(tok);
        if (tok->str() == "auto")
            autoTok = tok;
    }
    std::cout << std::endl;
    if (autoTok)
    {
        const ValueType *valueType = autoTok->valueType();
        std::cout << indent << "    auto valueType: " << valueType << std::endl;
        if (var->typeStartToken()->valueType())
        {
            std::cout << indent << "        " << valueType->str() << std::endl;
        }
    }
    std::cout << indent << "mIndex: " << var->index() << std::endl;
    std::cout << indent << "mAccess: " << accessControlToString(var->accessControl()) << std::endl;
    std::cout << indent << "mFlags: " << std::endl;
    std::cout << indent << "    isStatic: " << var->isStatic() << std::endl;
    std::cout << indent << "    isLocal: " << var->isLocal() << std::endl;
    std::cout << indent << "    isConst: " << var->isConst() << std::endl;
    std::cout << indent << "    isClass: " << var->isClass() << std::endl;
    std::cout << indent << "    isArray: " << var->isArray() << std::endl;
    std::cout << indent << "    isReference: " << var->isReference() << std::endl;
    std::cout << indent << "    isRValueRef: " << var->isRValueReference() << std::endl;
    std::cout << indent << "    hasDefault: " << var->hasDefault() << std::endl;
    std::cout << indent << "mType: ";
    if (var->type())
    {
        std::cout << var->type()->type() << " " << var->type()->name();
        std::cout << " " << mTokenizer->list.fileLine(var->type()->classDef);
        std::cout << " " << var->type() << std::endl;
    }
    else
        std::cout << "none" << std::endl;

    if (var->nameToken())
    {
        const ValueType *valueType = var->nameToken()->valueType();
        std::cout << indent << "valueType: " << valueType << std::endl;
        if (valueType)
        {
            std::cout << indent << "    " << valueType->str() << std::endl;
        }
    }

    std::cout << indent << "mScope: " << scopeToString(var->scope(), mTokenizer) << std::endl;

    std::cout << indent << "mDimensions:";
    for (std::size_t i = 0; i < var->dimensions().size(); i++)
    {
        std::cout << " " << var->dimension(i);
        if (!var->dimensions()[i].known)
            std::cout << "?";
    }
    std::cout << std::endl;
}

//-----------------------------------------------------------------------------
void SymbolDatabase::printOut(const char *title) const
{
    std::cout << std::setiosflags(std::ios::boolalpha);
    if (title)
        std::cout << "\n### " << title << " ###\n";

    for (std::list<Scope>::const_iterator scope = scopeList.begin(); scope != scopeList.end(); ++scope)
    {
        std::cout << "Scope: " << &*scope << " " << scope->type << std::endl;
        std::cout << "    className: " << scope->className << std::endl;
        std::cout << "    classDef: " << tokenToString(scope->classDef, mTokenizer) << std::endl;
        std::cout << "    bodyStart: " << tokenToString(scope->bodyStart, mTokenizer) << std::endl;
        std::cout << "    bodyEnd: " << tokenToString(scope->bodyEnd, mTokenizer) << std::endl;

        std::list<Function>::const_iterator func;

        // find the function body if not implemented inline
        for (func = scope->functionList.begin(); func != scope->functionList.end(); ++func)
        {
            std::cout << "    Function: " << &*func << std::endl;
            std::cout << "        name: " << tokenToString(func->tokenDef, mTokenizer) << std::endl;
            std::cout << "        type: " << (func->type == Function::eConstructor ? "Constructor" : func->type == Function::eDestructor ? "Destructor" : func->type == Function::eFunction ? "Function" : "Unknown") << std::endl;
            std::cout << "        access: " << accessControlToString(func->access) << std::endl;
            std::cout << "        hasBody: " << func->hasBody() << std::endl;
            std::cout << "        isInline: " << func->isInline() << std::endl;
            std::cout << "        isConst: " << func->isConst() << std::endl;
            std::cout << "        isStatic: " << func->isStatic() << std::endl;
            std::cout << "        isStaticLocal: " << func->isStaticLocal() << std::endl;
            std::cout << "        isVariadic: " << func->isVariadic() << std::endl;
            std::cout << "        attributes:";
            if (func->isAttributeConst())
                std::cout << " const ";
            if (func->isAttributePure())
                std::cout << " pure ";
            if (func->isAttributeNoreturn())
                std::cout << " noreturn ";
            if (func->isAttributeConstructor())
                std::cout << " constructor ";
            if (func->isAttributeDestructor())
                std::cout << " destructor ";
            if (func->isAttributeNodiscard())
                std::cout << " nodiscard ";
            std::cout << std::endl;
            std::cout << "        noexceptArg: " << (func->noexceptArg ? func->noexceptArg->str() : "none") << std::endl;
            std::cout << "        throwArg: " << (func->throwArg ? func->throwArg->str() : "none") << std::endl;
            std::cout << "        tokenDef: " << tokenToString(func->tokenDef, mTokenizer) << std::endl;
            std::cout << "        argDef: " << tokenToString(func->argDef, mTokenizer) << std::endl;
            if (!func->isConstructor() && !func->isDestructor())
                std::cout << "        retDef: " << tokenToString(func->retDef, mTokenizer) << std::endl;
            if (func->retDef)
            {
                std::cout << "           ";
                for (const Token *tok = func->retDef; tok && tok != func->tokenDef && !Token::Match(tok, "{|;|override|final"); tok = tok->next())
                    std::cout << " " << tokenType(tok);
                std::cout << std::endl;
            }
            std::cout << "        retType: " << func->retType << std::endl;

            if (func->tokenDef->next()->valueType())
            {
                const ValueType *valueType = func->tokenDef->next()->valueType();
                std::cout << "        valueType: " << valueType << std::endl;
                if (valueType)
                {
                    std::cout << "            " << valueType->str() << std::endl;
                }
            }

            if (func->hasBody())
            {
                std::cout << "        token: " << tokenToString(func->token, mTokenizer) << std::endl;
                std::cout << "        arg: " << tokenToString(func->arg, mTokenizer) << std::endl;
            }
            std::cout << "        nestedIn: " << scopeToString(func->nestedIn, mTokenizer) << std::endl;
            std::cout << "        functionScope: " << scopeToString(func->functionScope, mTokenizer) << std::endl;

            std::list<Variable>::const_iterator var;

            for (var = func->argumentList.begin(); var != func->argumentList.end(); ++var)
            {
                std::cout << "        Variable: " << &*var << std::endl;
                printVariable(&*var, "            ");
            }
        }

        std::list<Variable>::const_iterator var;

        for (var = scope->varlist.begin(); var != scope->varlist.end(); ++var)
        {
            std::cout << "    Variable: " << &*var << std::endl;
            printVariable(&*var, "        ");
        }

        if (scope->type == Scope::eEnum)
        {
            std::cout << "    enumType: ";
            if (scope->enumType)
                scope->enumType->stringify(std::cout, false, true, false);
            else
                std::cout << "int";
            std::cout << std::endl;
            std::cout << "    enumClass: " << scope->enumClass << std::endl;
            for (std::vector<Enumerator>::const_iterator enumerator = scope->enumeratorList.begin(); enumerator != scope->enumeratorList.end(); ++enumerator)
            {
                std::cout << "        Enumerator: " << enumerator->name->str() << " = ";
                if (enumerator->value_known)
                {
                    std::cout << enumerator->value;
                }

                if (enumerator->start)
                {
                    const Token *tok = enumerator->start;
                    std::cout << (enumerator->value_known ? " " : "") << "[" << tok->str();
                    while (tok && tok != enumerator->end)
                    {
                        if (tok->next())
                            std::cout << " " << tok->next()->str();
                        tok = tok->next();
                    }

                    std::cout << "]";
                }

                std::cout << std::endl;
            }
        }

        std::cout << "    nestedIn: " << scope->nestedIn;
        if (scope->nestedIn)
        {
            std::cout << " " << scope->nestedIn->type << " "
                      << scope->nestedIn->className;
        }
        std::cout << std::endl;

        std::cout << "    definedType: " << scope->definedType << std::endl;

        std::cout << "    nestedList[" << scope->nestedList.size() << "] = (";

        std::list<Scope *>::const_iterator nsi;

        std::size_t count = scope->nestedList.size();
        for (nsi = scope->nestedList.begin(); nsi != scope->nestedList.end(); ++nsi)
        {
            std::cout << " " << (*nsi) << " " << (*nsi)->type << " " << (*nsi)->className;
            if (count-- > 1)
                std::cout << ",";
        }

        std::cout << " )" << std::endl;

        std::list<Scope::UsingInfo>::const_iterator use;

        for (use = scope->usingList.begin(); use != scope->usingList.end(); ++use)
        {
            std::cout << "    using: " << use->scope << " " << use->start->strAt(2);
            const Token *tok1 = use->start->tokAt(3);
            while (tok1 && tok1->str() == "::")
            {
                std::cout << "::" << tok1->strAt(1);
                tok1 = tok1->tokAt(2);
            }
            std::cout << " " << mTokenizer->list.fileLine(use->start) << std::endl;
        }

        std::cout << "    functionOf: " << scopeToString(scope->functionOf, mTokenizer) << std::endl;

        std::cout << "    function: " << scope->function;
        if (scope->function)
            std::cout << " " << scope->function->name();
        std::cout << std::endl;
    }

    for (std::list<Type>::const_iterator type = typeList.begin(); type != typeList.end(); ++type)
    {
        std::cout << "Type: " << &(*type) << std::endl;
        std::cout << "    name: " << type->name() << std::endl;
        std::cout << "    classDef: " << tokenToString(type->classDef, mTokenizer) << std::endl;
        std::cout << "    classScope: " << type->classScope << std::endl;
        std::cout << "    enclosingScope: " << type->enclosingScope;
        if (type->enclosingScope)
        {
            std::cout << " " << type->enclosingScope->type << " "
                      << type->enclosingScope->className;
        }
        std::cout << std::endl;
        std::cout << "    needInitialization: " << (type->needInitialization == Type::Unknown ? "Unknown" : type->needInitialization == Type::True ? "True" : type->needInitialization == Type::False ? "False" : "Invalid") << std::endl;

        std::cout << "    derivedFrom[" << type->derivedFrom.size() << "] = (";
        std::size_t count = type->derivedFrom.size();
        for (std::size_t i = 0; i < type->derivedFrom.size(); ++i)
        {
            std::cout << accessControlToString(type->derivedFrom[i].access);

            if (type->derivedFrom[i].type)
                std::cout << " " << type->derivedFrom[i].type;
            else
                std::cout << " Unknown";

            std::cout << " " << type->derivedFrom[i].name;
            if (count-- > 1)
                std::cout << ",";
        }

        std::cout << " )" << std::endl;

        std::cout << " )" << std::endl;
    }

    for (std::size_t i = 1; i < mVariableList.size(); i++)
    {
        std::cout << "mVariableList[" << i << "]: " << mVariableList[i];
        if (mVariableList[i])
        {
            std::cout << " " << mVariableList[i]->name() << " "
                      << mTokenizer->list.fileLine(mVariableList[i]->nameToken());
        }
        std::cout << std::endl;
    }
    std::cout << std::resetiosflags(std::ios::boolalpha);
}

//-----------------------------------------------------------------------------
void SymbolDatabase::printXml(std::ostream &out) const
{
    out << std::setiosflags(std::ios::boolalpha);

    std::set<const Variable *> variables;

    // Scopes..
    out << "  <scopes>" << std::endl;
    for (std::list<Scope>::const_iterator scope = scopeList.begin(); scope != scopeList.end(); ++scope)
    {
        out << "    <scope";
        out << " id=\"" << &*scope << "\"";
        out << " type=\"" << scope->type << "\"";
        if (!scope->className.empty())
            out << " className=\"" << ErrorLogger::toxml(scope->className) << "\"";

        if (scope->classDef)
            out << " classDef=\"" << ErrorLogger::toxml(scope->classDef->str()) << "\"";
        if (scope->bodyStart)
            out << " bodyStart=\"" << scope->bodyStart << '\"';
        if (scope->bodyEnd)
            out << " bodyEnd=\"" << scope->bodyEnd << '\"';
        if (scope->nestedIn)
            out << " nestedIn=\"" << scope->nestedIn << "\"";
        if (scope->function)
            out << " function=\"" << scope->function << "\"";
        if (scope->functionList.empty() && scope->varlist.empty() && scope->enumeratorList.empty())
            out << "/>" << std::endl;
        else
        {
            out << '>' << std::endl;
            if (!scope->functionList.empty())
            {
                out << "      <functionList>" << std::endl;
                for (std::list<Function>::const_iterator function = scope->functionList.begin(); function != scope->functionList.end(); ++function)
                {
                    out << "        <function id=\"" << &*function << "\" tokenDef=\"" << function->tokenDef << "\" name=\"" << ErrorLogger::toxml(function->name()) << '\"';
                    out << " type=\"" << (function->type == Function::eConstructor ? "Constructor" : function->type == Function::eDestructor ? "Destructor" : function->type == Function::eFunction ? "Function" : "Unknown") << '\"';
                    out << " access=\"" << accessControlToString(function->access) << '\"';
                    if (function->argCount() == 0U)
                        out << "/>" << std::endl;
                    else
                    {
                        out << ">" << std::endl;
                        for (unsigned int argnr = 0; argnr < function->argCount(); ++argnr)
                        {
                            const Variable *arg = function->getArgumentVar(argnr);
                            out << "          <arg nr=\"" << argnr + 1 << "\" variable=\"" << arg << "\"/>" << std::endl;
                            variables.insert(arg);
                        }
                        out << "        </function>" << std::endl;
                    }
                }
                out << "      </functionList>" << std::endl;
            }
            if (!scope->varlist.empty())
            {
                out << "      <varlist>" << std::endl;
                for (std::list<Variable>::const_iterator var = scope->varlist.begin(); var != scope->varlist.end(); ++var)
                    out << "        <var id=\"" << &*var << "\"/>" << std::endl;
                out << "      </varlist>" << std::endl;
            }
            if (!scope->enumeratorList.empty())
            {
                out << "      <enumeratorList>" << std::endl;
                for (std::size_t i = 0, end = scope->enumeratorList.size(); i < end; ++i)
                {
                    out << "        <enumerator id=\"" << scope->enumeratorList[i].name << "\"";
                    out << " name=\"" << scope->enumeratorList[i].name->str() << "\"";
                    out << "/>" << std::endl;
                }
                out << "      </enumeratorList>" << std::endl;
            }

            out << "    </scope>" << std::endl;
        }
    }
    out << "  </scopes>" << std::endl;

    // Variables..
    for (const Variable *var : mVariableList)
        variables.insert(var);
    out << "  <variables>" << std::endl;
    for (const Variable *var : variables)
    {
        if (!var)
            continue;
        out << "    <var id=\"" << var << '\"';
        out << " name=\"" << (var->nameToken() ? var->nameToken()->str() : "NULL") << '\"';
        out << " nameToken=\"" << (var->nameToken() ? var->nameToken() : nullptr) << '\"';
        out << " access=\"" << accessControlToString(var->mAccess) << '\"';
        out << " typeStartToken=\"" << var->typeStartToken() << '\"';
        out << " typeEndToken=\"" << var->typeEndToken() << '\"';
        out << " scope=\"" << var->scope() << '\"';
        out << " constness=\"" << var->valueType()->constness << '\"';
        out << " isArgument=\"" << var->isArgument() << '\"';
        out << " isArray=\"" << var->isArray() << '\"';
        out << " isClass=\"" << var->isClass() << '\"';
        out << " isConst=\"" << var->isConst() << '\"';
        out << " isLocal=\"" << var->isLocal() << '\"';
        out << " isReference=\"" << var->isReference() << '\"';
        out << " isStatic=\"" << var->isStatic() << '\"';
        out << "/>" << std::endl;
    }
    out << "  </variables>" << std::endl;
    out << std::resetiosflags(std::ios::boolalpha);
}

//-----------------------------------------------------------------------------
const Enumerator *SymbolDatabase::findEnumerator(const Token *tok) const
{
    const Scope *scope = tok->scope();

    const std::string &tokStr = tok->str();

    if (mTokensThatAreNotEnumeratorValues.find(tokStr) != mTokensThatAreNotEnumeratorValues.end())
    {
        return nullptr;
    }

    // check for qualified name
    if (tok->strAt(-1) == "::")
    {
        // find first scope
        const Token *tok1 = tok;
        while (Token::Match(tok1->tokAt(-2), "%name% ::"))
            tok1 = tok1->tokAt(-2);

        if (tok1->strAt(-1) == "::")
            scope = &scopeList.front();
        else
        {
            // FIXME search base class here

            // find first scope
            while (scope && scope->nestedIn)
            {
                const Scope *temp = scope->nestedIn->findRecordInNestedList(tok1->str());
                if (temp)
                {
                    scope = temp;
                    break;
                }
                scope = scope->nestedIn;
            }
        }

        if (scope)
        {
            tok1 = tok1->tokAt(2);
            while (scope && Token::Match(tok1, "%name% ::"))
            {
                scope = scope->findRecordInNestedList(tok1->str());
                tok1 = tok1->tokAt(2);
            }

            if (scope)
            {
                const Enumerator *enumerator = scope->findEnumerator(tokStr);

                if (enumerator) // enum class
                    return enumerator;
                // enum
                else
                {
                    for (std::list<Scope *>::const_iterator it = scope->nestedList.begin(), end = scope->nestedList.end(); it != end; ++it)
                    {
                        enumerator = (*it)->findEnumerator(tokStr);

                        if (enumerator)
                            return enumerator;
                    }
                }
            }
        }
    }
    else
    {
        const Enumerator *enumerator = scope->findEnumerator(tokStr);

        if (enumerator)
            return enumerator;

        for (std::list<Scope *>::const_iterator s = scope->nestedList.begin(); s != scope->nestedList.end(); ++s)
        {
            enumerator = (*s)->findEnumerator(tokStr);

            if (enumerator)
                return enumerator;
        }

        if (scope->definedType)
        {
            const std::vector<Type::BaseInfo> &derivedFrom = scope->definedType->derivedFrom;
            for (size_t i = 0, end = derivedFrom.size(); i < end; ++i)
            {
                const Type *derivedFromType = derivedFrom[i].type;
                if (derivedFromType && derivedFromType->classScope)
                {
                    enumerator = derivedFromType->classScope->findEnumerator(tokStr);

                    if (enumerator)
                        return enumerator;
                }
            }
        }

        while (scope->nestedIn)
        {
            if (scope->type == Scope::eFunction && scope->functionOf)
                scope = scope->functionOf;
            else
                scope = scope->nestedIn;

            enumerator = scope->findEnumerator(tokStr);

            if (enumerator)
                return enumerator;

            for (std::list<Scope *>::const_iterator s = scope->nestedList.begin(); s != scope->nestedList.end(); ++s)
            {
                enumerator = (*s)->findEnumerator(tokStr);

                if (enumerator)
                    return enumerator;
            }
        }
    }

    mTokensThatAreNotEnumeratorValues.insert(tokStr);

    return nullptr;
}

//-----------------------------------------------------------------------------
const Type *SymbolDatabase::findVariableTypeInBase(const Scope *scope, const Token *typeTok) const
{
    if (scope && scope->definedType && !scope->definedType->derivedFrom.empty())
    {
        const std::vector<Type::BaseInfo> &derivedFrom = scope->definedType->derivedFrom;
        for (std::size_t i = 0; i < derivedFrom.size(); ++i)
        {
            const Type *base = derivedFrom[i].type;
            if (base && base->classScope)
            {
                const Type *type = base->classScope->findType(typeTok->str());
                if (type)
                    return type;
                type = findVariableTypeInBase(base->classScope, typeTok);
                if (type)
                    return type;
            }
        }
    }

    return nullptr;
}

//-----------------------------------------------------------------------------
const Type *SymbolDatabase::findVariableType(const Scope *start, const Token *typeTok) const
{
    const Scope *scope = start;

    // check if type does not have a namespace
    if (typeTok->strAt(-1) != "::" && typeTok->strAt(1) != "::")
    {
        // check if type same as scope
        if (start->isClassOrStruct() && typeTok->str() == start->className)
            return start->definedType;

        while (scope)
        {
            // look for type in this scope
            const Type *type = scope->findType(typeTok->str());

            if (type)
                return type;

            // look for type in base classes if possible
            if (scope->isClassOrStruct())
            {
                type = findVariableTypeInBase(scope, typeTok);

                if (type)
                    return type;
            }

            // check if in member function class to see if it's present in class
            if (scope->type == Scope::eFunction && scope->functionOf)
            {
                const Scope *scope1 = scope->functionOf;

                type = scope1->findType(typeTok->str());

                if (type)
                    return type;

                type = findVariableTypeInBase(scope1, typeTok);

                if (type)
                    return type;
            }

            scope = scope->nestedIn;
        }
    }

    // check for a qualified name and use it when given
    else if (typeTok->strAt(-1) == "::")
    {
        // check if type is not part of qualification
        if (typeTok->strAt(1) == "::")
            return nullptr;

        // find start of qualified function name
        const Token *tok1 = typeTok;

        while (Token::Match(tok1->tokAt(-2), "%type% ::") ||
               (Token::simpleMatch(tok1->tokAt(-2), "> ::") && tok1->linkAt(-2) && Token::Match(tok1->linkAt(-2)->tokAt(-1), "%type%")))
        {
            if (tok1->strAt(-1) == "::")
                tok1 = tok1->tokAt(-2);
            else
                tok1 = tok1->linkAt(-2)->tokAt(-1);
        }

        // check for global scope
        if (tok1->strAt(-1) == "::")
        {
            scope = &scopeList.front();

            scope = scope->findRecordInNestedList(tok1->str());
        }

        // find start of qualification
        else
        {
            while (scope)
            {
                if (scope->className == tok1->str())
                    break;
                else
                {
                    const Scope *scope1 = scope->findRecordInNestedList(tok1->str());

                    if (scope1)
                    {
                        scope = scope1;
                        break;
                    }
                    else if (scope->type == Scope::eFunction && scope->functionOf)
                        scope = scope->functionOf;
                    else
                        scope = scope->nestedIn;
                }
            }
        }

        if (scope)
        {
            // follow qualification
            while (scope && (Token::Match(tok1, "%type% ::") ||
                             (Token::Match(tok1, "%type% <") && Token::simpleMatch(tok1->linkAt(1), "> ::"))))
            {
                if (tok1->strAt(1) == "::")
                    tok1 = tok1->tokAt(2);
                else
                    tok1 = tok1->linkAt(1)->tokAt(2);
                const Scope *temp = scope->findRecordInNestedList(tok1->str());
                if (!temp)
                {
                    // look in base classes
                    const Type *type = findVariableTypeInBase(scope, tok1);

                    if (type)
                        return type;
                }
                scope = temp;
            }

            if (scope && scope->definedType)
                return scope->definedType;
        }
    }

    return nullptr;
}

//-----------------------------------------------------------------------------
const Function *SymbolDatabase::findFunction(const Token *tok) const
{
    // find the scope this function is in
    const Scope *currScope = tok->scope();
    while (currScope && currScope->isExecutable())
    {
        if (currScope->functionOf)
            currScope = currScope->functionOf;
        else
            currScope = currScope->nestedIn;
    }

    // check for a qualified name and use it when given
    if (tok->strAt(-1) == "::")
    {
        // find start of qualified function name
        const Token *tok1 = tok;

        while (Token::Match(tok1->tokAt(-2), ">|%type% ::"))
        {
            if (tok1->strAt(-2) == ">")
            {
                if (tok1->linkAt(-2))
                    tok1 = tok1->linkAt(-2)->tokAt(-1);
                else
                {
                    if (mSettings->debugwarnings)
                        debugMessage(tok1->tokAt(-2), "SymbolDatabase::findFunction found '>' without link.");
                    return nullptr;
                }
            }
            else
                tok1 = tok1->tokAt(-2);
        }

        // check for global scope
        if (tok1->strAt(-1) == "::")
        {
            currScope = &scopeList.front();

            currScope = currScope->findRecordInNestedList(tok1->str());
        }

        // find start of qualification
        else
        {
            while (currScope)
            {
                if (currScope->className == tok1->str())
                    break;
                else
                {
                    const Scope *scope = currScope->findRecordInNestedList(tok1->str());

                    if (scope)
                    {
                        currScope = scope;
                        break;
                    }
                    else
                        currScope = currScope->nestedIn;
                }
            }
        }

        if (currScope)
        {
            while (currScope && !(Token::Match(tok1, "%type% :: %any% (") ||
                                  (Token::Match(tok1, "%type% <") && Token::Match(tok1->linkAt(1), "> :: %any% ("))))
            {
                if (tok1->strAt(1) == "::")
                    tok1 = tok1->tokAt(2);
                else
                    tok1 = tok1->linkAt(1)->tokAt(2);
                currScope = currScope->findRecordInNestedList(tok1->str());
            }

            tok1 = tok1->tokAt(2);

            if (currScope && tok1)
                return currScope->findFunction(tok1);
        }
    }

    // check for member function
    else if (Token::Match(tok->tokAt(-2), "!!this ."))
    {
        const Token *tok1 = tok->tokAt(-2);
        if (Token::Match(tok1, "%var% ."))
        {
            const Variable *var = getVariableFromVarId(tok1->varId());
            if (var && var->typeScope())
                return var->typeScope()->findFunction(tok, var->isConst());
        }
    }

    // check in enclosing scopes
    else
    {
        while (currScope)
        {
            const Function *func = currScope->findFunction(tok);
            if (func)
                return func;
            currScope = currScope->nestedIn;
        }
    }
    return nullptr;
}

//-----------------------------------------------------------------------------
const Scope *SymbolDatabase::findScope(const Token *tok, const Scope *startScope) const
{
    const Scope *scope = nullptr;
    // absolute path
    if (tok->str() == "::")
    {
        tok = tok->next();
        scope = &scopeList.front();
    }
    // relative path
    else if (tok->isName())
    {
        scope = startScope;
    }

    while (scope && tok && tok->isName())
    {
        if (tok->strAt(1) == "::")
        {
            scope = scope->findRecordInNestedList(tok->str());
            tok = tok->tokAt(2);
        }
        else if (tok->strAt(1) == "<" && Token::simpleMatch(tok->linkAt(1), "> ::"))
        {
            scope = scope->findRecordInNestedList(tok->str());
            tok = tok->linkAt(1)->tokAt(2);
        }
        else
            return scope->findRecordInNestedList(tok->str());
    }

    // not a valid path
    return nullptr;
}

//-----------------------------------------------------------------------------
const Type *SymbolDatabase::findType(const Token *startTok, const Scope *startScope) const
{
    // skip over struct
    if (Token::Match(startTok, "struct|class"))
        startTok = startTok->next();

    // type same as scope
    if (startTok->str() == startScope->className && startScope->isClassOrStruct() && startTok->strAt(1) != "::")
        return startScope->definedType;

    const Scope *start_scope = startScope;

    // absolute path - directly start in global scope
    if (startTok->str() == "::")
    {
        startTok = startTok->next();
        start_scope = &scopeList.front();
    }

    const Token *tok = startTok;
    const Scope *scope = start_scope;

    while (scope && tok && tok->isName())
    {
        if (tok->strAt(1) == "::" || (tok->strAt(1) == "<" && Token::simpleMatch(tok->linkAt(1), "> ::")))
        {
            scope = scope->findRecordInNestedList(tok->str());
            if (scope)
            {
                if (tok->strAt(1) == "::")
                    tok = tok->tokAt(2);
                else
                    tok = tok->linkAt(1)->tokAt(2);
            }
            else
            {
                start_scope = start_scope->nestedIn;
                if (!start_scope)
                    break;
                scope = start_scope;
                tok = startTok;
            }
        }
        else
        {
            const Type *type = scope->findType(tok->str());
            if (type)
                return type;
            else
                break;
        }
    }

    // check using namespaces
    while (startScope)
    {
        for (std::list<Scope::UsingInfo>::const_iterator it = startScope->usingList.begin();
             it != startScope->usingList.end(); ++it)
        {
            tok = startTok;
            scope = it->scope;
            start_scope = startScope;

            while (scope && tok && tok->isName())
            {
                if (tok->strAt(1) == "::" || (tok->strAt(1) == "<" && Token::simpleMatch(tok->linkAt(1), "> ::")))
                {
                    scope = scope->findRecordInNestedList(tok->str());
                    if (scope)
                    {
                        if (tok->strAt(1) == "::")
                            tok = tok->tokAt(2);
                        else
                            tok = tok->linkAt(1)->tokAt(2);
                    }
                    else
                    {
                        start_scope = start_scope->nestedIn;
                        if (!start_scope)
                            break;
                        scope = start_scope;
                        tok = startTok;
                    }
                }
                else
                {
                    const Type *type = scope->findType(tok->str());
                    if (type)
                        return type;
                    else
                        break;
                }
            }
        }
        startScope = startScope->nestedIn;
    }

    // not a valid path
    return nullptr;
}

//-----------------------------------------------------------------------------
const Type *SymbolDatabase::findTypeInNested(const Token *startTok, const Scope *startScope) const
{
    // skip over struct
    if (Token::Match(startTok, "struct|class|enum"))
        startTok = startTok->next();

    // type same as scope
    if (startTok->str() == startScope->className && startScope->isClassOrStruct())
        return startScope->definedType;

    bool hasPath = false;

    const Token *tok = startTok;
    const Scope *scope = startScope;

    while (scope && tok && tok->isName())
    {
        const Type *type = scope->findType(tok->str());
        if (hasPath || type)
            return type;
        else
        {
            scope = scope->nestedIn;
            if (!scope)
                break;
        }
    }

    // not a valid path
    return nullptr;
}

//-----------------------------------------------------------------------------
Function *SymbolDatabase::findFunctionInScope(const Token *func, const Scope *ns, const std::string &path, unsigned int path_length)
{
    const Function *function = nullptr;
    const bool destructor = func->strAt(-1) == "~";

    for (std::multimap<std::string, const Function *>::const_iterator it = ns->functionMap.find(func->str());
         it != ns->functionMap.end() && it->first == func->str(); ++it)
    {

        if (Function::argsMatch(ns, it->second->argDef, func->next(), path, path_length) &&
            it->second->isDestructor() == destructor)
        {
            function = it->second;
            break;
        }
    }

    if (!function)
    {
        const Scope *scope = ns->findRecordInNestedList(func->str());
        if (scope && Token::Match(func->tokAt(1), "::|<"))
        {
            if (func->strAt(1) == "::")
                func = func->tokAt(2);
            else if (func->linkAt(1))
                func = func->linkAt(1)->tokAt(2);
            else
                return nullptr;
            if (func->str() == "~")
                func = func->next();
            function = findFunctionInScope(func, scope, path, path_length);
        }
    }

    return const_cast<Function *>(function);
}

//-----------------------------------------------------------------------------
unsigned int SymbolDatabase::sizeOfType(const Token *type) const
{
    unsigned int size = mTokenizer->sizeOfType(type);

    if (size == 0 && type->type() && type->type()->isEnumType() && type->type()->classScope)
    {
        size = mSettings->sizeof_int;
        const Token *enum_type = type->type()->classScope->enumType;
        if (enum_type)
            size = mTokenizer->sizeOfType(enum_type);
    }

    return size;
}

//-----------------------------------------------------------------------------
void SymbolDatabase::setValueType(Token *tok, const Variable &var)
{
    ValueType valuetype;
    if (var.nameToken())
    {
        valuetype.bits = var.nameToken()->bits();
    }
    valuetype.typeScope = var.typeScope();
    if (parsedecl(var.typeStartToken(), &valuetype, mSettings))
    {
        if (tok->str() == "." && tok->astOperand1())
        {
            const ValueType *const vt = tok->astOperand1()->valueType();
            if (vt && (vt->constness & 1) != 0)
                valuetype.constness |= 1;
        }
        setValueType(tok, valuetype);
    }
}

//-----------------------------------------------------------------------------
void SymbolDatabase::setValueType(Token *tok, const Enumerator &enumerator)
{
    ValueType valuetype;
    valuetype.typeScope = enumerator.scope;
    const Token *type = enumerator.scope->enumType;
    if (type)
    {
        valuetype.type = ValueType::typeFromString(type->str());
        if (valuetype.type == ValueType::Type::UNKNOWN_TYPE && type->isStandardType())
            valuetype.fromLibraryType(type->str(), mSettings);

        if (valuetype.isIntegral())
        {
            //@todo sign - activate when there is ctrl sign implementaion
            /*if (type->isSigned())
                valuetype.sign = ValueType::Sign::SIGNED;
            else if (type->isUnsigned())
                valuetype.sign = ValueType::Sign::UNSIGNED;
            else if (valuetype.type == ValueType::Type::CHAR)
                valuetype.sign = mDefaultSignedness;
            else
                valuetype.sign = ValueType::Sign::SIGNED;*/
        }

        setValueType(tok, valuetype);
    }
    else
    {
        valuetype.type = ValueType::INT;
        setValueType(tok, valuetype);
    }
}

//-----------------------------------------------------------------------------
void SymbolDatabase::setValueType(Token *tok, const ValueType &valuetype)
{
    tok->setValueType(new ValueType(valuetype));
    Token *parent = const_cast<Token *>(tok->astParent());
    if (!parent || parent->valueType())
        return;
    if (!parent->astOperand1())
        return;

    const ValueType *vt1 = parent->astOperand1() ? parent->astOperand1()->valueType() : nullptr;
    const ValueType *vt2 = parent->astOperand2() ? parent->astOperand2()->valueType() : nullptr;

    if (vt1 && Token::Match(parent, "<<|>>"))
    {
        if (vt2 && vt2->isIntegral())
            setValueType(parent, *vt1);
        return;
    }

    if (parent->isAssignmentOp())
    {
        if (vt1)
            setValueType(parent, *vt1);

        return;
    }

    if ((parent->str() == "." || parent->str() == "::") &&
        parent->astOperand2() && parent->astOperand2()->isName())
    {
        const Variable *var = parent->astOperand2()->variable();
        if (!var && valuetype.typeScope && vt1)
        {
            const std::string &name = parent->astOperand2()->str();
            const Scope *typeScope = vt1->typeScope;
            if (!typeScope)
                return;
            for (std::list<Variable>::const_iterator it = typeScope->varlist.begin(); it != typeScope->varlist.end(); ++it)
            {
                if (it->nameToken()->str() == name)
                {
                    var = &*it;
                    break;
                }
            }
        }
        if (var)
            setValueType(parent, *var);
        return;
    }

    if (!vt1)
        return;
    if (parent->astOperand2() && !vt2)
        return;

    const bool ternary = parent->str() == ":" && parent->astParent() && parent->astParent()->str() == "?";
    if (ternary)
    {
        if (vt2 && vt1->type == vt2->type)
            setValueType(parent, *vt2);
        parent = const_cast<Token *>(parent->astParent());
    }

    if (ternary || parent->isArithmeticalOp() || parent->tokType() == Token::eIncDecOp)
    {
        if (vt1->type == ValueType::Type::DOUBLE || (vt2 && vt2->type == ValueType::Type::DOUBLE))
        {
            setValueType(parent, ValueType(ValueType::Type::DOUBLE, 0U));
            return;
        }
        if (vt1->type == ValueType::Type::FLOAT || (vt2 && vt2->type == ValueType::Type::FLOAT))
        {
            setValueType(parent, ValueType(ValueType::Type::FLOAT, 0U));
            return;
        }
    }

    if (vt1->isIntegral() &&
        (!vt2 || vt2->isIntegral()) &&
        (ternary || parent->isArithmeticalOp() || parent->tokType() == Token::eBitOp || parent->tokType() == Token::eIncDecOp || parent->isAssignmentOp()))
    {

        ValueType vt;
        if (!vt2 || vt1->type > vt2->type)
        {
            vt.type = vt1->type;
            vt.originalTypeName = vt1->originalTypeName;
        }
        else if (vt1->type == vt2->type)
        {
            vt.type = vt1->type;
            vt.originalTypeName = (vt1->originalTypeName.empty() ? vt2 : vt1)->originalTypeName;
        }
        else
        {
            vt.type = vt2->type;
            vt.originalTypeName = vt2->originalTypeName;
        }
        if (vt.type < ValueType::Type::INT && vt.type != ValueType::Type::BOOL)
        {
            vt.type = ValueType::Type::INT;
            vt.originalTypeName.clear();
        }

        setValueType(parent, vt);
        return;
    }
}

//-----------------------------------------------------------------------------
/// @todo refactor this code for ctrl lang. AND KEEP IT SIMPLE
void SymbolDatabase::setValueTypeInTokenList()
{
    Token *tokens = const_cast<Tokenizer *>(mTokenizer)->list.front();

    for (Token *tok = tokens; tok; tok = tok->next())
        tok->setValueType(nullptr);

    for (Token *tok = tokens; tok; tok = tok->next())
    {
        if (tok->isNumber())
        {
            if (MathLib::isFloat(tok->str()))
            {
                ValueType::Type type = ValueType::Type::DOUBLE;
                const char suffix = tok->str()[tok->str().size() - 1];
                if (suffix == 'f' || suffix == 'F')
                    type = ValueType::Type::FLOAT;
                else if (suffix == 'L' || suffix == 'l')
                    type = ValueType::Type::LONG;
                setValueType(tok, ValueType(type, 0U));
            }
            else if (MathLib::isInt(tok->str()))
            {
                const bool unsignedSuffix = (tok->str().find_last_of("uU") != std::string::npos);
                ValueType::Type type;
                const MathLib::bigint value = MathLib::toLongNumber(tok->str());
                if (mSettings->platformType == cppcheck::Platform::Unspecified)
                    type = ValueType::Type::INT;
                else if (mSettings->isIntValue(unsignedSuffix ? (value >> 1) : value))
                    type = ValueType::Type::INT;
                /*else if (mSettings->isLongValue(unsignedSuffix ? (value >> 1) : value))
                    type = ValueType::Type::LONG;*/
                else
                    type = ValueType::Type::LONG;
                for (std::size_t pos = tok->str().size() - 1U; pos > 0U; --pos)
                {
                    const char suffix = tok->str()[pos];

                    if (suffix == 'l' || suffix == 'L')
                        //type = (type == ValueType::Type::INT) ? ValueType::Type::LONG : ValueType::Type::LONGLONG;
                        type = ValueType::Type::LONG;
                    /*else if (pos > 2U && suffix == '4' && tok->str()[pos - 1] == '6' && tok->str()[pos - 2] == 'i') {
                        type = ValueType::Type::LONGLONG;
                        pos -= 2;
                    }*/
                    else
                        break;
                }
                setValueType(tok, ValueType(type, 0U));
            }
        }
        else if (tok->isComparisonOp() || tok->tokType() == Token::eLogicalOp)
        {
            /// @todo ctrl does not support operator functions
            /// it can be sefally removed
            /*if (tok->isComparisonOp() && (getClassScope(tok->astOperand1()) || getClassScope(tok->astOperand2())))
            {
                const Function *function = getOperatorFunction(tok);
                if (function)
                {
                    ValueType vt;
                    parsedecl(function->retDef, &vt, mSettings);
                    setValueType(tok, vt);
                    continue;
                }
            }*/
            setValueType(tok, ValueType(ValueType::Type::BOOL, 0U));
        }
        else if (tok->isBoolean())
        {
            setValueType(tok, ValueType(ValueType::Type::BOOL, 0U));
        }
        else if (tok->tokType() == Token::eChar)
            setValueType(tok, ValueType(ValueType::Type::CHAR, 0U));
        else if (tok->tokType() == Token::eString)
        {
            setValueType(tok, ValueType(ValueType::Type::STRING, 1U, 1U));
        }
        else if (tok->str() == "(")
        {
            // cast
            if (tok->isCast() && !tok->astOperand2() && Token::Match(tok, "( %name%"))
            {
                ValueType valuetype;
                if (Token::simpleMatch(parsedecl(tok->next(), &valuetype, mSettings), ")"))
                    setValueType(tok, valuetype);
            }

            // function
            else if (tok->previous() && tok->previous()->function() && tok->previous()->function()->retDef)
            {
                ValueType valuetype;
                if (parsedecl(tok->previous()->function()->retDef, &valuetype, mSettings))
                    setValueType(tok, valuetype);
            }
            /// @todo remove or refactor it for ctrl code
            else if (Token::simpleMatch(tok->previous(), "sizeof ("))
            {
                // TODO: use specified size_t type
                ValueType valuetype(ValueType::Type::LONG, 0U);
                valuetype.originalTypeName = "size_t";
                setValueType(tok, valuetype);

                if (Token::Match(tok, "( %type% %type%| *| *| )"))
                {
                    ValueType vt;
                    if (parsedecl(tok->next(), &vt, mSettings))
                    {
                        setValueType(tok->next(), vt);
                    }
                }
            }

            // function style cast
            /// @todo i think this can not work for ctrl code
            else if (tok->previous() && tok->previous()->isStandardType())
            {
                ValueType valuetype;
                valuetype.type = ValueType::typeFromString(tok->previous()->str());
                setValueType(tok, valuetype);
            }

            // constructor
            else if (tok->previous() && tok->previous()->type() && tok->previous()->type()->classScope)
            {
                ValueType valuetype;
                valuetype.type = ValueType::RECORD;
                valuetype.typeScope = tok->previous()->type()->classScope;
                setValueType(tok, valuetype);
            }

            // library function
            else if (tok->previous())
            {
                const std::string &typestr(mSettings->library.returnValueType(tok->previous()));

                TokenList tokenList(mSettings);
                std::istringstream istr(typestr + ";");
                if (tokenList.createTokens(istr))
                {
                    ValueType vt;
                    assert(tokenList.front());
                    tokenList.simplifyPlatformTypes();
                    tokenList.simplifyStdType();
                    if (parsedecl(tokenList.front(), &vt, mSettings))
                    {
                        setValueType(tok, vt);
                    }
                }
            }
        }
        else if (tok->variable())
        {
            setValueType(tok, *tok->variable());
        }
        else if (tok->enumerator())
        {
            setValueType(tok, *tok->enumerator());
        }
        else if (tok->str() == "new")
        {
            const Token *typeTok = tok->next();
            /// @todo i think this can not work for ctrl lang
            if (Token::Match(typeTok, "( std| ::| )"))
                typeTok = typeTok->link()->next();
            std::string typestr;
            while (Token::Match(typeTok, "%name% :: %name%"))
            {
                typestr += typeTok->str() + "::";
                typeTok = typeTok->tokAt(2);
            }
            if (!Token::Match(typeTok, "%type% ;|[|("))
                continue;
            typestr += typeTok->str();
            ValueType vt;
            if (typeTok->type() && typeTok->type()->classScope)
            {
                vt.type = ValueType::Type::RECORD;
                vt.typeScope = typeTok->type()->classScope;
            }
            else
            {
                vt.type = ValueType::typeFromString(typestr);
                if (vt.type == ValueType::Type::UNKNOWN_TYPE)
                    vt.fromLibraryType(typestr, mSettings);
                if (vt.type == ValueType::Type::UNKNOWN_TYPE)
                    continue;
            }
            setValueType(tok, vt);
        }
    }

    // Update functions with new type information.
    createSymbolDatabaseSetFunctionPointers(false);

    // Update auto variables with new type information.
    createSymbolDatabaseSetVariablePointers();
}

//-----------------------------------------------------------------------------