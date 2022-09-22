//-----------------------------------------------------------------------------
#include "scope.h"

#include "symbolutils.h"
#include <list>

//-----------------------------------------------------------------------------
static const Token *skipQualifiers(const Token *tok)
{
    while (Token::Match(tok, "const"))
    {
        tok = tok->next();
    }

    return tok;
}

//-----------------------------------------------------------------------------
static void checkVariableCallMatch(const Variable *callarg, const Variable *funcarg, size_t &same, size_t &fallback1, size_t &fallback2)
{
    if (callarg)
    {
        bool constEquals = (callarg->typeStartToken()->strAt(-1) == "const") == (funcarg->typeStartToken()->strAt(-1) == "const");
        if (constEquals &&
            callarg->typeStartToken()->str() == funcarg->typeStartToken()->str() &&
            callarg->typeStartToken()->isUnsigned() == funcarg->typeStartToken()->isUnsigned() &&
            callarg->typeStartToken()->isLong() == funcarg->typeStartToken()->isLong())
        {
            same++;
        }
        else
        {
            const bool takesInt = Token::Match(funcarg->typeStartToken(), "char|short|int|long");
            const bool takesFloat = Token::Match(funcarg->typeStartToken(), "float|double");
            const bool passesInt = Token::Match(callarg->typeStartToken(), "char|short|int|long");
            const bool passesFloat = Token::Match(callarg->typeStartToken(), "float|double");
            if ((takesInt && passesInt) || (takesFloat && passesFloat))
                fallback1++;
            else if ((takesInt && passesFloat) || (takesFloat && passesInt))
                fallback2++;
        }
    }
}

//-----------------------------------------------------------------------------
static bool valueTypeMatch(const ValueType *valuetype, const Token *type)
{
    if (valuetype->typeScope && type->type() && type->type()->classScope == valuetype->typeScope)
        return true;

    if (valuetype->type == ValueType::typeFromString(type->str()))
        return true;

    if (valuetype->isEnum() && type->isEnumType() && valuetype->typeScope->className == type->str())
        return true;

    return false;
}

//-----------------------------------------------------------------------------
static const Token *skipScopeIdentifiers(const Token *tok)
{
    if (tok && tok->str() == "::")
    {
        tok = tok->next();
    }
    while (Token::Match(tok, "%name% ::") )
    {
        tok = tok->tokAt(2);
    }

    return tok;
}

//-----------------------------------------------------------------------------
Scope::Scope(const SymbolDatabase *check_, const Token *classDef_, const Scope *nestedIn_, ScopeType type_, const Token *start_) : check(check_),
                                                                                                                                   classDef(classDef_),
                                                                                                                                   bodyStart(start_),
                                                                                                                                   bodyEnd(start_->link()),
                                                                                                                                   nestedIn(nestedIn_),
                                                                                                                                   numConstructors(0),
                                                                                                                                   numCopyOrMoveConstructors(0),
                                                                                                                                   type(type_),
                                                                                                                                   definedType(nullptr),
                                                                                                                                   functionOf(nullptr),
                                                                                                                                   function(nullptr),
                                                                                                                                   enumType(nullptr),
                                                                                                                                   enumClass(false)
{
}

//-----------------------------------------------------------------------------
Scope::Scope(const SymbolDatabase *check_, const Token *classDef_, const Scope *nestedIn_) : check(check_),
                                                                                             classDef(classDef_),
                                                                                             bodyStart(nullptr),
                                                                                             bodyEnd(nullptr),
                                                                                             nestedIn(nestedIn_),
                                                                                             numConstructors(0),
                                                                                             numCopyOrMoveConstructors(0),
                                                                                             definedType(nullptr),
                                                                                             functionOf(nullptr),
                                                                                             function(nullptr),
                                                                                             enumType(nullptr),
                                                                                             enumClass(false)
{
    const Token *nameTok = classDef;
    if (!classDef)
    {
        type = Scope::eGlobal;
    }
    else if (classDef->str() == "class")
    {
        type = Scope::eClass;
        nameTok = nameTok->next();
    }
    else if (classDef->str() == "struct")
    {
        type = Scope::eStruct;
        nameTok = nameTok->next();
    }
    else if (classDef->str() == "enum")
    {
        type = Scope::eEnum;
        nameTok = nameTok->next();
    }
    else
    {
        type = Scope::eFunction;
    }
    // skip over qualification if present
    nameTok = skipScopeIdentifiers(nameTok);
    if (nameTok && ((type == Scope::eEnum && Token::Match(nameTok, ":|{")) || nameTok->str() != "{")) // anonymous and unnamed structs don't have a name
        className = nameTok->str();
}

//-----------------------------------------------------------------------------
AccessControl Scope::defaultAccess() const
{
    switch (type)
    {
    case eGlobal:
        return Global;
    case eClass:
        return Private;
    case eStruct:
        return Public;
    default:
        return Local;
    }
}

//-----------------------------------------------------------------------------
// Get variable list..
void Scope::getVariableList(const Settings *settings)
{
    const Token *start;

    if (bodyStart)
    {
        start = bodyStart->next();
    }
    // global scope
    else if (className.empty())
    {
        start = check->mTokenizer->tokens();
    }

    // forward declaration
    else
    {
        return;
    }

    AccessControl varaccess = defaultAccess();
    for (const Token *tok = start; tok && tok != bodyEnd; tok = tok->next())
    {
        // syntax error?
      //  std::cout << "getVariableList tok " << tok->str() <<  std::endl;
        if (tok->next() == nullptr)
        {
            //std::cout << "syntax error " << tok->str() <<  std::endl;
            break;
        }

        // Is it a function?
        else if (tok->str() == "{")
        {
            tok = tok->link();
            continue;
        }


        // "private" "public" "protected" etc
        else if (tok->str() == "public")
        {
            varaccess = Public;
            continue;
        }
        else if (tok->str() == "protected")
        {
            varaccess = Protected;
            continue;
        }
        else if (tok->str() == "private")
        {
            varaccess = Private;
            continue;
        }

        // skip return and delete
        else if (Token::Match(tok, "return|delete"))
        {
            while (tok->next() &&
                   tok->next()->str() != ";" &&
                   tok->next()->str() != "}" /* ticket #4994 */)
            {
                tok = tok->next();
            }
            continue;
        }

        // skip case/default
        if (Token::Match(tok, "case|default"))
        {
            while (tok->next() && !Token::Match(tok->next(), "[:;{}]"))
                tok = tok->next();
            continue;
        }

        // Search for start of statement..
        else if (tok->previous() && !Token::Match(tok->previous(), ";|{|}|public|protected|private"))
            continue;
        else if (tok->str() == ";")
            continue;

        bool isAdded;
     //   std::cout << "try to add " << tok->str() << std::endl;
        tok = checkVariable(tok, varaccess, settings, isAdded);

        if (isAdded)
        {
            // change back to default ariable type
            varaccess = defaultAccess();
        }

        if (!tok)
            break;
    }
}

//-----------------------------------------------------------------------------
const Token *Scope::checkVariable(const Token *tok, AccessControl varaccess, const Settings *settings, bool &isAdded)
{
    // skip const|static
    while (Token::Match(tok, "const|static"))
    {
        tok = tok->next();
    }

    const Token *typestart = tok;

    if (Token::Match(tok, "class|struct|enum"))
    {
        tok = tok->next();
    }

    // This is the start of a statement
    const Token *vartok = nullptr;
    const Token *typetok = nullptr;

    if (tok && isVariableDeclaration(tok, vartok, typetok))
    {   
     //   std::cout << "tok " << tok->str() << std::endl;
      //  std::cout << "vartok " << vartok->str() << std::endl;
      //  std::cout << "typetok " << typetok->str() << std::endl;
        // If the vartok was set in the if-blocks above, create a entry for this variable..
        tok = vartok->next();
        while (Token::Match(tok, "[|{"))
            tok = tok->link()->next();

        if (vartok->varId() == 0)
        {
            if (!vartok->isBoolean())
                check->debugMessage(vartok, "Scope::checkVariable found variable \'" + vartok->str() + "\' with varid 0.");
            return tok;
        }

        const Type *vType = nullptr;

        if (typetok)
        {
            vType = findVariableTypeIncludingUsedNamespaces(check, this, typetok);

            const_cast<Token *>(typetok)->type(vType);
        }

        // skip "enum" or "struct"
        if (Token::Match(typestart, "enum|struct|class"))
            typestart = typestart->next();

        addVariable(vartok, typestart, vartok->previous(), varaccess, vType, this, settings);
        isAdded = true;
    }

    return tok;
}

//-----------------------------------------------------------------------------
const Variable *Scope::getVariable(const std::string &varname) const
{
    std::list<Variable>::const_iterator iter;

    for (iter = varlist.begin(); iter != varlist.end(); ++iter)
    {
        if (iter->name() == varname)
            return &*iter;
    }

    return nullptr;
}

//-----------------------------------------------------------------------------
bool Scope::isVariableDeclaration(const Token *const tok, const Token *&vartok, const Token *&typetok) const
{
 //   std::cout << "is var " << tok->str()<<std::endl;
    /// @todo throw is not valid key for ctrl code
    if (Token::Match(tok, "throw|new"))
        return false;

    if (tok->str() == "using")
        return false;

    const Token *localTypeTok = skipScopeIdentifiers(tok);
    const Token *localVarTok = nullptr;

    
    if (Token::Match(localTypeTok, "%type%"))
    {
  //      std::cout << "localTypeTok is type " << localTypeTok->str() << std::endl; 
        localVarTok = skipQualifiers(localTypeTok->next());
    }

    if (!localVarTok)
    {
      //  std::cout << "no  localVarTok" <<std::endl;
        return false;
    }

    if (localVarTok->str() == "const")
        localVarTok = localVarTok->next();

    if (Token::Match(localVarTok, "%name% ;|=") || (localVarTok && localVarTok->varId() && localVarTok->strAt(1) == ":"))
    {
        vartok = localVarTok;
        typetok = localTypeTok;
    }
    else if (Token::Match(localVarTok, "%name% )|[") && localVarTok->str() != "operator")
    {
        vartok = localVarTok;
        typetok = localTypeTok;
    }
    else if (localVarTok && localVarTok->varId() && Token::Match(localVarTok, "%name% (|{") &&
             Token::Match(localVarTok->next()->link(), ")|} ;"))
    {
        vartok = localVarTok;
        typetok = localTypeTok;
    }
// std::cout << "ret " <<  (nullptr != vartok) << std::endl;
    return nullptr != vartok;
}

//-----------------------------------------------------------------------------
/// @todo refactor this for ctrl lang
const Token *Scope::addEnum(const Token *tok)
{
    const Token *tok2 = tok->next();

    // skip over class if present
    if (tok2->str() == "class")
        tok2 = tok2->next();

    // skip over name
    tok2 = tok2->next();

    // save type if present
    if (tok2->str() == ":")
    {
        tok2 = tok2->next();

        enumType = tok2;
        tok2 = tok2->next();
    }

    // add enumerators
    if (tok2->str() == "{")
    {
        const Token *end = tok2->link();
        tok2 = tok2->next();

        while (Token::Match(tok2, "%name% =|,|}") ||
               (Token::Match(tok2, "%name% (") && Token::Match(tok2->linkAt(1), ") ,|}")))
        {
            Enumerator enumerator(this);

            // save enumerator name
            enumerator.name = tok2;

            // skip over name
            tok2 = tok2->next();

            if (tok2->str() == "=")
            {
                // skip over "="
                tok2 = tok2->next();

                if (tok2->str() == "}")
                    return nullptr;

                enumerator.start = tok2;

                while (!Token::Match(tok2, ",|}"))
                {
                    if (tok2->link())
                        tok2 = tok2->link();
                    enumerator.end = tok2;
                    tok2 = tok2->next();
                }
            }
            else if (tok2->str() == "(")
            {
                // skip over unknown macro
                tok2 = tok2->link()->next();
            }

            if (tok2->str() == ",")
            {
                enumeratorList.push_back(enumerator);
                tok2 = tok2->next();
            }
            else if (tok2->str() == "}")
            {
                enumeratorList.push_back(enumerator);
                break;
            }
        }

        if (tok2 == end)
        {
            tok2 = tok2->next();

            if (tok2 && tok2->str() != ";")
                tok2 = nullptr;
        }
        else
            tok2 = nullptr;
    }
    else
        tok2 = nullptr;

    return tok2;
}

//-----------------------------------------------------------------------------
bool Scope::hasInlineOrLambdaFunction() const
{
    for (std::list<Scope *>::const_iterator it = nestedList.begin(); it != nestedList.end(); ++it)
    {
        const Scope *s = *it;
        // Inline function
        if (s->type == Scope::eUnconditional && Token::simpleMatch(s->bodyStart->previous(), ") {"))
            return true;
    }
    return false;
}

//-----------------------------------------------------------------------------
void Scope::findFunctionInBase(const std::string &name, size_t args, std::vector<const Function *> &matches) const
{
    if (isClassOrStruct() && definedType && !definedType->derivedFrom.empty())
    {
        const std::vector<Type::BaseInfo> &derivedFrom = definedType->derivedFrom;
        for (std::size_t i = 0; i < derivedFrom.size(); ++i)
        {
            const Type *base = derivedFrom[i].type;
            if (base && base->classScope)
            {
                if (base->classScope == this) // Ticket #5120, #5125: Recursive class; tok should have been found already
                    continue;

                for (std::multimap<std::string, const Function *>::const_iterator it = base->classScope->functionMap.find(name); it != base->classScope->functionMap.end() && it->first == name; ++it)
                {
                    const Function *func = it->second;
                    if (args == func->argCount() || (args < func->argCount() && args >= func->minArgCount()))
                    {
                        matches.push_back(func);
                    }
                }

                base->classScope->findFunctionInBase(name, args, matches);
            }
        }
    }
}

//-----------------------------------------------------------------------------
const Function *Scope::findFunction(const Token *tok, bool requireConst) const
{
    // make sure this is a function call
    const Token *end = tok->linkAt(1);
    if (!end)
        return nullptr;

    std::vector<const Token *> arguments;

    // find all the arguments for this function call
    for (const Token *arg = tok->tokAt(2); arg && arg != end; arg = arg->nextArgument())
    {
        arguments.push_back(arg);
    }

    std::vector<const Function *> matches;

    // find all the possible functions that could match
    const std::size_t args = arguments.size();
    for (std::multimap<std::string, const Function *>::const_iterator it = functionMap.find(tok->str()); it != functionMap.cend() && it->first == tok->str(); ++it)
    {
        const Function *func = it->second;
        if (args == func->argCount() ||
            (func->isVariadic() && args >= (func->argCount() - 1)) ||
            (args < func->argCount() && args >= func->minArgCount()))
        {
            matches.push_back(func);
        }
    }

    // check in base classes
    findFunctionInBase(tok->str(), args, matches);

    const Function *fallback1Func = nullptr;
    const Function *fallback2Func = nullptr;

    // check each function against the arguments in the function call for a match
    for (std::size_t i = 0; i < matches.size();)
    {
        bool constFallback = false;
        const Function *func = matches[i];
        size_t same = 0;

        if (!requireConst || !func->isConst())
        {
            // get the function this call is in
            const Scope *scope = tok->scope();

            // check if this function is a member function
            if (scope && scope->functionOf && scope->functionOf->isClassOrStruct() && scope->function)
            {
                // check if isConst mismatches
                if (scope->function->isConst() != func->isConst())
                {
                    if (scope->function->isConst())
                    {
                        ++i;
                        continue;
                    }
                    constFallback = true;
                }
            }
        }

        size_t fallback1 = 0;
        size_t fallback2 = 0;
        bool erased = false;
        for (std::size_t j = 0; j < args; ++j)
        {

            // don't check variadic arguments
            if (func->isVariadic() && j > (func->argCount() - 1))
            {
                break;
            }
            const Variable *funcarg = func->getArgumentVar(j);
            // check for a match with a variable
            if (Token::Match(arguments[j], "%var% ,|)"))
            {
                const Variable *callarg = check->getVariableFromVarId(arguments[j]->varId());
                checkVariableCallMatch(callarg, funcarg, same, fallback1, fallback2);
            }

            // check for a match with reference of a variable
            //todo: shouldn't be possivle to be true in ctrl?
            else if (Token::Match(arguments[j], "* %var% ,|)"))
            {
                const Variable *callarg = check->getVariableFromVarId(arguments[j]->next()->varId());
                if (callarg)
                {
                    const bool funcargref = (funcarg->typeEndToken()->str() == "&");
                    if (funcargref &&
                        (callarg->typeStartToken()->str() == funcarg->typeStartToken()->str() &&
                         callarg->typeStartToken()->isUnsigned() == funcarg->typeStartToken()->isUnsigned() &&
                         callarg->typeStartToken()->isLong() == funcarg->typeStartToken()->isLong()))
                    {
                        same++;
                    }
                    else
                    {
                        // can't match so remove this function from possible matches
                        matches.erase(matches.begin() + i);
                        erased = true;
                        break;
                    }
                }
            }

            // check for a match with address of a variable
            else if (Token::Match(arguments[j], "& %var% ,|)"))
            {
                const Variable *callarg = check->getVariableFromVarId(arguments[j]->next()->varId());
                if (callarg)
                {
                    const bool funcargptr = (funcarg->typeEndToken()->str() == "*");
                    if (funcargptr &&
                        (callarg->typeStartToken()->str() == funcarg->typeStartToken()->str() &&
                         callarg->typeStartToken()->isUnsigned() == funcarg->typeStartToken()->isUnsigned() &&
                         callarg->typeStartToken()->isLong() == funcarg->typeStartToken()->isLong()))
                    {
                        same++;
                    }
                    else if (funcargptr && funcarg->typeStartToken()->str() == "void")
                    {
                        fallback1++;
                    }
                    else
                    {
                        // can't match so remove this function from possible matches
                        matches.erase(matches.begin() + i);
                        erased = true;
                        break;
                    }
                }
            }

            // check for a match with a numeric literal
            else if (Token::Match(arguments[j], "%num% ,|)"))
            {
                if (MathLib::isInt(arguments[j]->str()) && (MathLib::isNullValue(arguments[j]->str())))
                {
                    bool exactMatch = false;
                    if (arguments[j]->str().find('l') != std::string::npos ||
                             arguments[j]->str().find('L') != std::string::npos)
                    {
                        if (arguments[j]->str().find('u') != std::string::npos ||
                            arguments[j]->str().find('U') != std::string::npos)
                        {
                            if (!funcarg->typeStartToken()->isLong() &&
                                funcarg->typeStartToken()->isUnsigned() &&
                                funcarg->typeStartToken()->str() == "long")
                            {
                                exactMatch = true;
                            }
                        }
                        else
                        {
                            if (!funcarg->typeStartToken()->isLong() &&
                                !funcarg->typeStartToken()->isUnsigned() &&
                                funcarg->typeStartToken()->str() == "long")
                            {
                                exactMatch = true;
                            }
                        }
                    }
                    else if (arguments[j]->str().find('u') != std::string::npos ||
                             arguments[j]->str().find('U') != std::string::npos)
                    {
                        if (funcarg->typeStartToken()->isUnsigned() &&
                            funcarg->typeStartToken()->str() == "int")
                        {
                            exactMatch = true;
                        }
                        else if (Token::Match(funcarg->typeStartToken(), "char|short"))
                        {
                            exactMatch = true;
                        }
                    }
                    else
                    {
                        if (Token::Match(funcarg->typeStartToken(), "char|short|int|long"))
                        {
                            exactMatch = true;
                        }
                    }

                    if (exactMatch)
                        same++;
                    else
                    {
                        if (Token::Match(funcarg->typeStartToken(), "char|short|int|long"))
                            fallback1++;
                        else if (Token::Match(funcarg->typeStartToken(), "float|double"))
                            fallback2++;
                    }
                }
                else
                {
                    bool exactMatch = false;
                    if (arguments[j]->str().find('f') != std::string::npos ||
                        arguments[j]->str().find('F') != std::string::npos)
                    {
                        if (funcarg->typeStartToken()->str() == "float")
                        {
                            exactMatch = true;
                        }
                    }
                    else if (arguments[j]->str().find('l') != std::string::npos ||
                             arguments[j]->str().find('L') != std::string::npos)
                    {
                        if (funcarg->typeStartToken()->isLong() &&
                            funcarg->typeStartToken()->str() == "double")
                        {
                            exactMatch = true;
                        }
                    }
                    else
                    {
                        if (!funcarg->typeStartToken()->isLong() &&
                            funcarg->typeStartToken()->str() == "double")
                        {
                            exactMatch = true;
                        }
                    }
                    if (exactMatch)
                        same++;
                    else
                    {
                        if (Token::Match(funcarg->typeStartToken(), "float|double"))
                            fallback1++;
                        else if (Token::Match(funcarg->typeStartToken(), "char|short|int|long"))
                            fallback2++;
                    }
                }
            }

            // check for a match with a string literal
            else if (Token::Match(arguments[j], "%str% ,|)"))
            {
                if (funcarg->typeStartToken() != funcarg->typeEndToken() &&
                    !arguments[j]->isLong() && Token::simpleMatch(funcarg->typeStartToken(), "char *") )
                    same++;
                else if (Token::simpleMatch(funcarg->typeStartToken(), "void *"))
                    fallback1++;
                else if (funcarg->isStlStringType())
                    fallback2++;
            }

            // check for a match with a char literal
            else if (Token::Match(arguments[j], "%char% ,|)"))
            {
                if (!arguments[j]->isLong() && funcarg->typeStartToken()->str() == "char")
                    same++;
                else if (Token::Match(funcarg->typeStartToken(), "char|short|int|long"))
                    fallback1++;
            }

            // check for a match with a boolean literal
            else if (Token::Match(arguments[j], "%bool% ,|)"))
            {
                if (Token::Match(funcarg->typeStartToken(), "bool"))
                    same++;
                else if (Token::Match(funcarg->typeStartToken(), "char|short|int|long"))
                    fallback1++;
            }

            // check that function argument type is not mismatching
            else if (funcarg->isReference() && arguments[j]->str() == "&")
            {
                // can't match so remove this function from possible matches
                matches.erase(matches.begin() + i);
                erased = true;
                break;
            }

            // Try to evaluate the apparently more complex expression
            else
            {
                const Token *argtok = arguments[j];
                while (argtok->astParent() && argtok->astParent() != tok->next() && argtok->astParent()->str() != ",")
                {
                    argtok = argtok->astParent();
                }
                if (argtok && argtok->valueType())
                {
                    const ValueType *valuetype = argtok->valueType();
                    const bool constEquals = ((valuetype->constness > 0) == (funcarg->typeStartToken()->strAt(-1) == "const"));
                    if (constEquals && valueTypeMatch(valuetype, funcarg->typeStartToken()))
                    {
                        same++;
                    }
                    else
                    {
                        const bool takesInt = Token::Match(funcarg->typeStartToken(), "bool|char|short|int|long") ||
                                              funcarg->typeStartToken()->isEnumType();
                        const bool takesFloat = Token::Match(funcarg->typeStartToken(), "float|double");
                        const bool passesInt = valuetype->isIntegral() || valuetype->isEnum();
                        const bool passesFloat = valuetype->isFloat();
                        if ((takesInt && passesInt) || (takesFloat && passesFloat))
                            fallback1++;
                        else if ((takesInt && passesFloat) || (takesFloat && passesInt))
                            fallback2++;
                    }
                }
                else
                {
                    while (Token::Match(argtok, ".|::"))
                        argtok = argtok->astOperand2();

                    if (argtok)
                    {
                        const Variable *callarg = check->getVariableFromVarId(argtok->varId());
                        checkVariableCallMatch(callarg, funcarg, same, fallback1, fallback2);
                    }
                }
            }
        }

        const size_t hasToBe = func->isVariadic() ? (func->argCount() - 1) : args;

        // check if all arguments matched
        if (same == hasToBe)
        {
            if (constFallback)
                fallback1Func = func;
            else
                return func;
        }

        else if (!fallback1Func)
        {
            if (same + fallback1 == hasToBe)
                fallback1Func = func;
            else if (!fallback2Func && same + fallback2 + fallback1 == hasToBe)
                fallback2Func = func;
        }

        if (!erased)
            ++i;
    }

    // Fallback cases
    if (fallback1Func)
        return fallback1Func;

    if (fallback2Func)
        return fallback2Func;

    // Only one candidate left
    if (matches.size() == 1)
        return matches[0];

    return nullptr;
}

//-----------------------------------------------------------------------------
const Scope *Scope::findRecordInNestedList(const std::string &name) const
{
    std::list<Scope *>::const_iterator it;

    for (it = nestedList.begin(); it != nestedList.end(); ++it)
    {
        if ((*it)->className == name && (*it)->type != eFunction)
            return (*it);
    }

    const Type *nested_type = findType(name);

    if (nested_type)
    {
        return nested_type->classScope;
    }

    return nullptr;
}

//-----------------------------------------------------------------------------
const Type *Scope::findType(const std::string &name) const
{
    auto it = definedTypesMap.find(name);

    // Type was found
    if (definedTypesMap.end() != it)
        return (*it).second;

    // is type defined in anonymous namespace..
    it = definedTypesMap.find("");
    if (it != definedTypesMap.end())
    {
        for (const Scope *scope : nestedList)
        {
            if (scope->className.empty() && (scope->isClassOrStruct()))
            {
                const Type *t = scope->findType(name);
                if (t)
                    return t;
            }
        }
    }

    // Type was not found
    return nullptr;
}

//-----------------------------------------------------------------------------
Scope *Scope::findInNestedListRecursive(const std::string &name)
{
    std::list<Scope *>::iterator it;

    for (it = nestedList.begin(); it != nestedList.end(); ++it)
    {
        if ((*it)->className == name)
            return (*it);
    }

    for (it = nestedList.begin(); it != nestedList.end(); ++it)
    {
        Scope *child = (*it)->findInNestedListRecursive(name);
        if (child)
            return child;
    }
    return nullptr;
}

//-----------------------------------------------------------------------------
static std::ostream &operator<<(std::ostream &s, Scope::ScopeType type)
{
    s << (type == Scope::eGlobal ? "Global" : type == Scope::eClass ? "Class" : type == Scope::eStruct ? "Struct" : type == Scope::eFunction ? "Function" : type == Scope::eIf ? "If" : type == Scope::eElse ? "Else" : type == Scope::eFor ? "For" : type == Scope::eWhile ? "While" : type == Scope::eDo ? "Do" : type == Scope::eSwitch ? "Switch" : type == Scope::eTry ? "Try" : type == Scope::eCatch ? "Catch" : type == Scope::eUnconditional ? "Unconditional" : type == Scope::eEnum ? "Enum" : "Unknown");
    return s;
}

//-----------------------------------------------------------------------------
void Scope::addVariable(const Token *token_, const Token *start_,
                        const Token *end_, AccessControl access_, const Type *type_,
                        const Scope *scope_, const Settings *settings)
{
    varlist.emplace_back(token_, start_, end_, varlist.size(),
                         access_,
                         type_, scope_, settings);
}

//-----------------------------------------------------------------------------
void Scope::addFunction(const Function &func)
{
    functionList.push_back(func);

    const Function *back = &functionList.back();

    functionMap.insert(make_pair(back->tokenDef->str(), back));
}

//-----------------------------------------------------------------------------
