//-----------------------------------------------------------------------------
#include "function.h"

#include "symbolutils.h"

//-----------------------------------------------------------------------------
Function::Function(const Tokenizer *mTokenizer, const Token *tok, const Scope *scope, const Token *tokDef, const Token *tokArgDef)
    : tokenDef(tokDef),
      argDef(tokArgDef),
      token(nullptr),
      arg(nullptr),
      retDef(nullptr),
      retType(nullptr),
      functionScope(nullptr),
      nestedIn(scope),
      initArgCount(0),
      type(eFunction),
      access(Public),
      noexceptArg(nullptr),
      throwArg(nullptr),
      mFlags(0)
{

    // class constructor/destructor
    if (tokenDef->str() == scope->className)
    {
        // destructor
        if (tokenDef->previous()->str() == "~")
            type = Function::eDestructor;
        // constructor of any kind
        else
            type = Function::eConstructor;
    }

    const Token *tok1 = tok;

    // look for end of previous statement
    while (tok1->previous() && !Token::Match(tok1->previous(), ";|}|{|public|protected|private|synchronized"))
    {
        tok1 = tok1->previous();

        // static function
        if (tok1->str() == "static")
        {
            isStatic(true);
            if (scope->type != Scope::eGlobal)
                isStaticLocal(true);
        }
    }

    // find the return type
    if (!isConstructor() && !isDestructor())
    {
        while (Token::Match(tok1, "static|struct|enum"))
            tok1 = tok1->next();
        retDef = tok1;
    }

    const Token *end = argDef->link();

    // parse function attributes..
    tok = end->next();
    while (tok)
    {
        if (tok->str() == "const")
            isConst(true);
        else
            break;
        if (tok)
            tok = tok->next();
    }

    if (mTokenizer->isFunctionHead(end, ":{"))
    {
        // assume implementation is inline (definition and implementation same)
        token = tokenDef;
        arg = argDef;
        isInline(true);
        hasBody(true);
    }
}

//-----------------------------------------------------------------------------
bool Function::argsMatch(const Scope *scope, const Token *first, const Token *second, const std::string &path, unsigned int path_length)
{
    //@todo adapt the function to ctrl
    return true;

    unsigned int arg_path_length = path_length;

    /// @todo this does not works for ctrl-lang. 'long signed bla;' is not valid ctrl-syntax
    while (first->str() == second->str() &&
           first->isLong() == second->isLong() &&
           first->isUnsigned() == second->isUnsigned())
    {

        // skip optional type information
        if (Token::Match(first->next(), "struct|enum|class"))
            first = first->next();
        if (Token::Match(second->next(), "struct|enum|class"))
            second = second->next();

        // skip const on type passed by value
        if (Token::Match(first->next(), "const %type% %name%|,|)") &&
            !Token::Match(first->next(), "const %type% %name%| ["))
            first = first->next();
        if (Token::Match(second->next(), "const %type% %name%|,|)") &&
            !Token::Match(second->next(), "const %type% %name%| ["))
            second = second->next();

        // at end of argument list
        if (first->str() == ")")
        {
            return true;
        }

        // skip default value assignment
        else if (first->next()->str() == "=")
        {
            first = first->nextArgument();
            if (first)
                first = first->tokAt(-2);
            if (second->next()->str() == "=")
            {
                second = second->nextArgument();
                if (second)
                    second = second->tokAt(-2);
                if (!first || !second)
                { // End of argument list (first or second)
                    return !first && !second;
                }
            }
            else if (!first)
            {                                   // End of argument list (first)
                return !second->nextArgument(); // End of argument list (second)
            }
        }
        else if (second->next()->str() == "=")
        {
            second = second->nextArgument();
            if (second)
                second = second->tokAt(-2);
            if (!second)
            { // End of argument list (second)
                return false;
            }
        }

        // definition missing variable name
        else if ((first->next()->str() == "," && second->next()->str() != ",") ||
                 (first->next()->str() == ")" && second->next()->str() != ")"))
        {
            second = second->next();
            // skip default value assignment
            if (second->next()->str() == "=")
            {
                do
                {
                    second = second->next();
                } while (!Token::Match(second->next(), ",|)"));
            }
        }
        else if (first->next()->str() == "[" && second->next()->str() != "[")
            second = second->next();

        // function missing variable name
        else if ((second->next()->str() == "," && first->next()->str() != ",") ||
                 (second->next()->str() == ")" && first->next()->str() != ")"))
        {
            first = first->next();
            // skip default value assignment
            if (first->next()->str() == "=")
            {
                do
                {
                    first = first->next();
                } while (!Token::Match(first->next(), ",|)"));
            }
        }
        else if (second->next()->str() == "[" && first->next()->str() != "[")
            first = first->next();

        // argument list has different number of arguments
        else if (second->str() == ")")
            break;

        // ckeck for type * x == type x[]
        else if (Token::Match(first->next(), "* %name%| ,|)|=") &&
                 Token::Match(second->next(), "%name%| [ ] ,|)"))
        {
            do
            {
                first = first->next();
            } while (!Token::Match(first->next(), ",|)"));
            do
            {
                second = second->next();
            } while (!Token::Match(second->next(), ",|)"));
        }

        // variable names are different
        else if ((Token::Match(first->next(), "%name% ,|)|=|[") &&
                  Token::Match(second->next(), "%name% ,|)|[")) &&
                 (first->next()->str() != second->next()->str()))
        {
            // skip variable names
            first = first->next();
            second = second->next();

            // skip default value assignment
            if (first->next()->str() == "=")
            {
                do
                {
                    first = first->next();
                } while (!Token::Match(first->next(), ",|)"));
            }
        }

        // variable with class path
        else if (arg_path_length && Token::Match(first->next(), "%name%") && first->strAt(1) != "const")
        {
            std::string param = path;

            if (Token::simpleMatch(second->next(), param.c_str()))
            {
                second = second->tokAt(int(arg_path_length));
                arg_path_length = 0;
            }

            // nested or base class variable
            else if (arg_path_length <= 2 && Token::Match(first->next(), "%name%") &&
                     (Token::Match(second->next(), "%name% :: %name%") ||
                      (Token::Match(second->next(), "%name% <") &&
                       Token::Match(second->linkAt(1), "> :: %name%"))) &&
                     ((second->next()->str() == scope->className) ||
                      (scope->definedType && scope->definedType->isDerivedFrom(second->next()->str()))) &&
                     (first->next()->str() == second->strAt(3)))
            {
                if (Token::Match(second->next(), "%name% <"))
                    second = second->linkAt(1)->next();
                else
                    second = second->tokAt(2);
            }

            // remove class name
            else if (arg_path_length > 2 && first->strAt(1) != second->strAt(1))
            {
                std::string short_path = path;
                unsigned int short_path_length = arg_path_length;

                // remove last " :: "
                short_path.resize(short_path.size() - 4);
                short_path_length--;

                // remove last name
                std::string::size_type lastSpace = short_path.find_last_of(' ');
                if (lastSpace != std::string::npos)
                {
                    short_path.resize(lastSpace + 1);
                    short_path_length--;
                    if (short_path[short_path.size() - 1] == '>')
                    {
                        short_path.resize(short_path.size() - 3);
                        while (short_path[short_path.size() - 1] == '<')
                        {
                            lastSpace = short_path.find_last_of(' ');
                            short_path.resize(lastSpace + 1);
                            short_path_length--;
                        }
                    }
                }

                param = short_path;
                if (Token::simpleMatch(second->next(), param.c_str()))
                {
                    second = second->tokAt(int(short_path_length));
                    arg_path_length = 0;
                }
            }
        }

        first = first->next();
        second = second->next();

        // reset path length
        if (first->str() == "," || second->str() == ",")
            arg_path_length = path_length;
    }

    return false;
}

//-----------------------------------------------------------------------------
const Token *Function::constructorMemberInitialization() const
{
    if (!isConstructor() || !functionScope || !functionScope->bodyStart)
        return nullptr;
    if (Token::Match(token, "%name% (") && Token::simpleMatch(token->linkAt(1), ") :"))
        return token->linkAt(1)->next();
    return nullptr;
}

//-----------------------------------------------------------------------------
void Function::addArguments(const SymbolDatabase *symbolDatabase, const Scope *scope)
{
    // check for non-empty argument list "( ... )"
    const Token *start = arg ? arg : argDef;
    if (!(start && start->link() != start->next() && !Token::simpleMatch(start, "( void )")))
        return;

    unsigned int count = 0;

    for (const Token *tok = start->next(); tok; tok = tok->next())
    {
        if (Token::Match(tok, ",|)"))
            return; // Syntax error

        const Token *startTok = tok;
        const Token *endTok = nullptr;
        const Token *nameTok = nullptr;

        do
        {
            if (tok->varId() != 0)
            {
                nameTok = tok;
                endTok = tok->previous();
            }
            else if (tok->str() == "[")
            {
                // skip array dimension(s)
                tok = tok->link();
                while (tok->next()->str() == "[")
                    tok = tok->next()->link();
            }
            else if (tok->str() == "<")
            {
                tok = tok->link();
                if (!tok) // something is wrong so just bail out
                    return;
            }

            tok = tok->next();

            if (!tok) // something is wrong so just bail
                return;
        } while (tok->str() != "," && tok->str() != ")" && tok->str() != "=");

        const Token *typeTok = startTok;
        // skip over stuff to get to type
        while (Token::Match(typeTok, "const|enum|struct|::"))
            typeTok = typeTok->next();
        if (Token::Match(typeTok, ",|)"))
        { // #8333
            symbolDatabase->mTokenizer->syntaxError(typeTok);
            return;
        }
        // skip over qualification
        while (Token::Match(typeTok, "%type% ::"))
            typeTok = typeTok->tokAt(2);

        // check for argument with no name or missing varid
        if (!endTok)
        {
            if (tok->previous()->isName() && !Token::Match(tok->tokAt(-1), "const"))
            {
                if (tok->previous() != typeTok)
                {
                    nameTok = tok->previous();
                    endTok = nameTok->previous();

                    if (hasBody())
                        symbolDatabase->debugMessage(nameTok, "Function::addArguments found argument \'" + nameTok->str() + "\' with varid 0.");
                }
                else
                    endTok = typeTok;
            }
            else
                endTok = tok->previous();
        }

        const ::Type *argType = nullptr;
        if (!typeTok->isStandardType())
        {
            argType = findVariableTypeIncludingUsedNamespaces(symbolDatabase, scope, typeTok);

            // save type
            const_cast<Token *>(typeTok)->type(argType);
        }

        // skip default values
        if (tok->str() == "=")
        {
            do
            {
                if (tok->link() && Token::Match(tok, "[{[(<]"))
                    tok = tok->link();
                tok = tok->next();
            } while (tok->str() != "," && tok->str() != ")");
        }

        // skip over stuff before type
        while (Token::Match(startTok, "enum|struct|const"))
            startTok = startTok->next();

        argumentList.emplace_back(nameTok, startTok, endTok, count++, Argument, argType, functionScope, symbolDatabase->mSettings);

        if (tok->str() == ")")
        {
            // check for a variadic function
            if (Token::simpleMatch(startTok, ". . ."))
                isVariadic(true);

            break;
        }
    }

    // count default arguments
    for (const Token *tok = argDef->next(); tok && tok != argDef->link(); tok = tok->next())
    {
        if (tok->str() == "=")
            initArgCount++;
    }
}

//-----------------------------------------------------------------------------
const Function *Function::getOverriddenFunction(bool *foundAllBaseClasses) const
{
    if (foundAllBaseClasses)
        *foundAllBaseClasses = true;
    if (!nestedIn->isClassOrStruct())
        return nullptr;
    return getOverriddenFunctionRecursive(nestedIn->definedType, foundAllBaseClasses);
}

//-----------------------------------------------------------------------------
const Function *Function::getOverriddenFunctionRecursive(const ::Type *baseType, bool *foundAllBaseClasses) const
{
    // check each base class
    for (std::size_t i = 0; i < baseType->derivedFrom.size(); ++i)
    {
        const ::Type *derivedFromType = baseType->derivedFrom[i].type;
        // check if base class exists in database
        if (!derivedFromType || !derivedFromType->classScope)
        {
            if (foundAllBaseClasses)
                *foundAllBaseClasses = false;
            continue;
        }

        const Scope *parent = derivedFromType->classScope;

        // check if function defined in base class
        for (std::multimap<std::string, const Function *>::const_iterator it = parent->functionMap.find(tokenDef->str()); it != parent->functionMap.end() && it->first == tokenDef->str(); ++it)
        {
            const Function *func = it->second;
        }

        if (!derivedFromType->derivedFrom.empty() && !derivedFromType->hasCircularDependencies())
        {
            // avoid endless recursion, see #5289 Crash: Stack overflow in isImplicitlyVirtual_rec when checking SVN and
            // #5590 with a loop within the class hierarchy.
            const Function *func = getOverriddenFunctionRecursive(derivedFromType, foundAllBaseClasses);
            if (func)
            {
                return func;
            }
        }
    }
    return nullptr;
}

//-----------------------------------------------------------------------------
const Variable *Function::getArgumentVar(std::size_t num) const
{
    for (std::list<Variable>::const_iterator i = argumentList.begin(); i != argumentList.end(); ++i)
    {
        if (i->index() == num)
            return (&*i);
        else if (i->index() > num)
            return nullptr;
    }
    return nullptr;
}

//-----------------------------------------------------------------------------