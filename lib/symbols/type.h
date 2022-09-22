//-----------------------------------------------------------------------------
#ifndef typeH
#define typeH
//-----------------------------------------------------------------------------

#include "../token.h"
#include "accesscontrol.h"
//#include "scope.h"

#include <vector>



class Scope;

//-----------------------------------------------------------------------------

/** @brief Information about a class type. */
class CPPCHECKLIB Type
{
public:

    //-------------------------------------------------------------------------

    const Token *classDef; ///< Points to "class" token
    const Scope *classScope;
    const Scope *enclosingScope;

    //-------------------------------------------------------------------------
    enum NeedInitialization
    {
        Unknown,
        True,
        False
    } needInitialization;

    //-------------------------------------------------------------------------
    class BaseInfo
    {
    public:

        //---------------------------------------------------------------------
        BaseInfo() : type(nullptr), nameTok(nullptr), access(Public)
        {
        }

        //---------------------------------------------------------------------
        std::string name;
        const Type *type;
        const Token *nameTok;
        AccessControl access; // public/protected/private

        //---------------------------------------------------------------------
        // allow ordering within containers
        bool operator<(const BaseInfo &rhs) const
        {
            return this->type < rhs.type;
        }

        //---------------------------------------------------------------------
    };

    //-------------------------------------------------------------------------
    std::vector<BaseInfo> derivedFrom;

    const Token *typeStart;
    const Token *typeEnd;

    //-------------------------------------------------------------------------
    Type(const Token *classDef_ = nullptr, const Scope *classScope_ = nullptr, const Scope *enclosingScope_ = nullptr) : classDef(classDef_),
                                                                                                                         classScope(classScope_),
                                                                                                                         enclosingScope(enclosingScope_),
                                                                                                                         needInitialization(Unknown),
                                                                                                                         typeStart(nullptr),
                                                                                                                         typeEnd(nullptr)
    {
        if (classDef_ && classDef_->str() == "enum")
            needInitialization = True;
    }

    //-------------------------------------------------------------------------
    const std::string &name() const;

    //-------------------------------------------------------------------------
    const std::string &type() const
    {
        return classDef ? classDef->str() : emptyString;
    }
    
    //-------------------------------------------------------------------------
    bool isClassType() const
    {
        return classDef && classDef->str() == "class";
    }

    //-------------------------------------------------------------------------
    bool isEnumType() const
    {
        return classDef && classDef->str() == "enum";
    }

    //-------------------------------------------------------------------------
    bool isStructType() const
    {
        return classDef && classDef->str() == "struct";
    }

    //-------------------------------------------------------------------------
    const Token *initBaseInfo(const Token *tok, const Token *tok1);


    //-------------------------------------------------------------------------
    /**
    * Check for circulare dependencies, i.e. loops within the class hierarchy
    * @param ancestors list of ancestors. For internal usage only, clients should not supply this argument.
    * @return true if there is a circular dependency
    */
    bool hasCircularDependencies(std::set<BaseInfo> *ancestors = nullptr) const;

    //-------------------------------------------------------------------------
    /**
    * Check for dependency
    * @param ancestor potential ancestor
    * @return true if there is a dependency
    */
    bool findDependency(const Type *ancestor) const;

    //-------------------------------------------------------------------------
    bool isDerivedFrom(const std::string &ancestor) const;

    //-------------------------------------------------------------------------
};


//-----------------------------------------------------------------------------
#endif // typeH