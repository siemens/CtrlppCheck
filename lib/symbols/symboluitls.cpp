//-----------------------------------------------------------------------------
#include "symbolutils.h"

//-----------------------------------------------------------------------------
const Type *findVariableTypeIncludingUsedNamespaces(const SymbolDatabase *symbolDatabase, const Scope *scope, const Token *typeTok)
{
    const Type *argType = symbolDatabase->findVariableType(scope, typeTok);
    if (argType)
        return argType;

    // look for variable type in any using namespace in this scope or above
    while (scope)
    {
        for (const Scope::UsingInfo &ui : scope->usingList)
        {
            if (ui.scope)
            {
                argType = symbolDatabase->findVariableType(ui.scope, typeTok);
                if (argType)
                    return argType;
            }
        }
        scope = scope->nestedIn;
    }
    return nullptr;
}

//-----------------------------------------------------------------------------
const Token *parsedecl(const Token *type, ValueType *const valuetype, const Settings *settings)
{
    // find start of declaration
    if ( !type )
    {
        // defensive
        return nullptr;
    }

    while (Token::Match(type->previous(), "%name%"))
        type = type->previous();

    // set some default type
    if (!valuetype->typeScope)
        valuetype->type = ValueType::Type::UNKNOWN_TYPE;
    else if (valuetype->typeScope->type == Scope::eEnum)
    {
        valuetype->type = ValueType::Type::INT;
    }
    else
        valuetype->type = ValueType::Type::RECORD;

    // try to find type
    while (Token::Match(type, "%name%|&|::") && !type->variable() && !type->function())
    {

        if (Token::Match(type, "synchronized|global"))
        {
            // just skip over
            valuetype->type = ValueType::Type::UNKNOWN_TYPE;
        }
        /// @todo i think this can not work for ctrl lang
        else if (Token::Match(type, "%name% :: %name%"))
        {
            std::string typestr;
            const Token *end = type;
            while (Token::Match(end, "%name% :: %name%"))
            {
                typestr += end->str() + "::";
                end = end->tokAt(2);
            }
            typestr += end->str();
            if (valuetype->fromLibraryType(typestr, settings))
                type = end;
        }
        // this is the regular variable type!
        else if (ValueType::Type::UNKNOWN_TYPE != ValueType::typeFromString(type->str()))
        {
            valuetype->type = ValueType::typeFromString(type->str());
        }

        /// @todo this can not work for ctrl lang, verify it and remove it
        else if (!valuetype->typeScope && (type->str() == "struct" || type->str() == "enum"))
            valuetype->type = type->str() == "struct" ? ValueType::Type::RECORD : ValueType::Type::NONSTD;

        else if (!valuetype->typeScope && type->type() && type->type()->classScope)
        {
            valuetype->type = ValueType::Type::RECORD;
            valuetype->typeScope = type->type()->classScope;
        }
        else if (type->isStandardType())
            valuetype->fromLibraryType(type->str(), settings);
        else if (Token::Match(type->previous(), "!!:: %name% !!::"))
            valuetype->fromLibraryType(type->str(), settings);
        /// @todo originalName shoud be removed in ctrl lang
        if (!type->originalName().empty())
            valuetype->originalTypeName = type->originalName();

        //try next token.
        type = type->next();
    }

    if (!type)
    {
        // defensive
        return nullptr;
    }

    if (valuetype->type == ValueType::Type::UNKNOWN_TYPE)
    {
        if (type->function())
        {
            valuetype->type = ValueType::Type::VOID;
        }
    }

    if (valuetype->type != ValueType::Type::UNKNOWN_TYPE)
    {
        // safe founded type token.
        return type;
    }

    // defensive
    return nullptr;
}

//-----------------------------------------------------------------------------