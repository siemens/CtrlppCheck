//-----------------------------------------------------------------------------
#include "type.h"

#include "scope.h"

//-----------------------------------------------------------------------------
bool Type::hasCircularDependencies(std::set<BaseInfo> *ancestors) const
{
    std::set<BaseInfo> knownAncestors;
    if (!ancestors)
    {
        ancestors = &knownAncestors;
    }
    for (std::vector<BaseInfo>::const_iterator parent = derivedFrom.begin(); parent != derivedFrom.end(); ++parent)
    {
        if (!parent->type)
            continue;
        else if (this == parent->type)
            return true;
        else if (ancestors->find(*parent) != ancestors->end())
            return true;
        else
        {
            ancestors->insert(*parent);
            if (parent->type->hasCircularDependencies(ancestors))
                return true;
        }
    }
    return false;
}

//-----------------------------------------------------------------------------
bool Type::findDependency(const Type *ancestor) const
{
    if (this == ancestor)
        return true;
    for (std::vector<BaseInfo>::const_iterator parent = derivedFrom.begin(); parent != derivedFrom.end(); ++parent)
    {
        if (parent->type && parent->type->findDependency(ancestor))
            return true;
    }
    return false;
}

//-----------------------------------------------------------------------------
bool Type::isDerivedFrom(const std::string &ancestor) const
{
    for (std::vector<BaseInfo>::const_iterator parent = derivedFrom.begin(); parent != derivedFrom.end(); ++parent)
    {
        if (parent->name == ancestor)
            return true;
        if (parent->type && parent->type->isDerivedFrom(ancestor))
            return true;
    }
    return false;
}

//-----------------------------------------------------------------------------
const Token *Type::initBaseInfo(const Token *tok, const Token *tok1)
{
    // goto initial '{'
    const Token *tok2 = tok1;
    while (tok2 && tok2->str() != "{")
    {
        if (tok2->str() == "<")
        {
            // it looks like cpp template. But is is definitive not ctrl code
            return nullptr;
        }

        // check for base classes
        else if (Token::Match(tok2, ":|,"))
        {
            tok2 = tok2->next();

            // check for invalid code
            if (!tok2 || !tok2->next())
                return nullptr;

            Type::BaseInfo base;

            if (tok->str() == "class")
                base.access = Private;
            else if (tok->str() == "struct")
                base.access = Public;

            base.nameTok = tok2;
            // handle global namespace
            if (tok2->str() == "::")
            {
                tok2 = tok2->next();
            }

            // handle derived base classes
            while (Token::Match(tok2, "%name% ::"))
            {
                tok2 = tok2->tokAt(2);
            }
            if (!tok2)
                return nullptr;

            base.name = tok2->str();

            // save pattern for base class name
            derivedFrom.push_back(base);
        }
        else
            tok2 = tok2->next();
    }

    return tok2;
}

//-----------------------------------------------------------------------------
const std::string &Type::name() const
{
    const Token *next = classDef->next();
    if (classScope && classScope->enumClass && isEnumType())
        return next->strAt(1);
    else if (next->str() == "class")
        return next->strAt(1);
    else if (next->isName())
        return next->str();
    return emptyString;
}


//-----------------------------------------------------------------------------