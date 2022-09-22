//-----------------------------------------------------------------------------
#ifndef scopeH
#define scopeH
//-----------------------------------------------------------------------------

#include "../token.h"
#include "accesscontrol.h"
#include "enumerator.h"
#include "function.h"
#include "variable.h"

#include <utility>
#include <list>
#include <map>
#include <string>

//-----------------------------------------------------------------------------

class Type;

//-----------------------------------------------------------------------------

class CPPCHECKLIB Scope
{
public:

    //-------------------------------------------------------------------------
    struct UsingInfo
    {
        const Token *start;
        const Scope *scope;
    };

    //-------------------------------------------------------------------------
    enum ScopeType
    {
        eGlobal,
        eClass,
        eStruct,
        eFunction,
        eIf,
        eElse,
        eFor,
        eWhile,
        eDo,
        eSwitch,
        eUnconditional,
        eTry,
        eCatch,
        eEnum
    };

    //-------------------------------------------------------------------------
    Scope(const SymbolDatabase *check_, const Token *classDef_, const Scope *nestedIn_);
    Scope(const SymbolDatabase *check_, const Token *classDef_, const Scope *nestedIn_, ScopeType type_, const Token *start_);

    //-------------------------------------------------------------------------
    const SymbolDatabase *check;
    std::string className;
    const Token *classDef;  ///< class/struct token
    const Token *bodyStart; ///< '{' token
    const Token *bodyEnd;   ///< '}' token
    std::list<Function> functionList;
    std::multimap<std::string, const Function *> functionMap;
    std::list<Variable> varlist;
    const Scope *nestedIn;
    std::list<Scope *> nestedList;
    unsigned int numConstructors;
    unsigned int numCopyOrMoveConstructors;
    std::list<UsingInfo> usingList;
    ScopeType type;
    Type *definedType;
    std::map<std::string, Type *> definedTypesMap;

    // function specific fields
    const Scope *functionOf; ///< scope this function belongs to
    Function *function;      ///< function info for this function

    // enum specific fields
    /// @todo remove this for ctrl lang
    const Token *enumType;
    bool enumClass;

    std::vector<Enumerator> enumeratorList;

    //-------------------------------------------------------------------------
    const Enumerator *findEnumerator(const std::string &name) const
    {
        for (std::size_t i = 0, end = enumeratorList.size(); i < end; ++i)
        {
            if (enumeratorList[i].name->str() == name)
                return &enumeratorList[i];
        }
        return nullptr;
    }

    //-------------------------------------------------------------------------
    bool isNestedIn(const Scope *outer) const
    {
        if (!outer)
            return false;
        if (outer == this)
            return true;
        const Scope *parent = nestedIn;
        while (outer != parent && parent)
            parent = parent->nestedIn;
        if (parent && parent == outer)
            return true;
        return false;
    }

    //-------------------------------------------------------------------------
    bool isClassOrStruct() const
    {
        return (type == eClass || type == eStruct);
    }

    //-------------------------------------------------------------------------
    bool isExecutable() const
    {
        return type != eClass && type != eStruct && type != eGlobal && type != eEnum;
    }

    //-------------------------------------------------------------------------
    bool isLocal() const
    {
        return (type == eIf || type == eElse ||
                type == eFor || type == eWhile || type == eDo ||
                type == eSwitch || type == eUnconditional ||
                type == eTry || type == eCatch);
    }

    //-------------------------------------------------------------------------
    // Is there lambda/inline function(s) in this scope?
    bool hasInlineOrLambdaFunction() const;

    //-------------------------------------------------------------------------
    /**
     * @brief find a function
     * @param tok token of function call
     * @param requireConst if const refers to a const variable only const methods should be matched
     * @return pointer to function if found or NULL if not found
     */
    const Function *findFunction(const Token *tok, bool requireConst = false) const;

    //-------------------------------------------------------------------------
    const Scope *findRecordInNestedList(const std::string &name) const;

    //-------------------------------------------------------------------------
    Scope *findRecordInNestedList(const std::string &name)
    {
        return const_cast<Scope *>(const_cast<const Scope *>(this)->findRecordInNestedList(name));
    }

    //-------------------------------------------------------------------------
    const Type *findType(const std::string &name) const;

    //-------------------------------------------------------------------------
    Type *findType(const std::string &name)
    {
        return const_cast<Type *>(const_cast<const Scope *>(this)->findType(name));
    }

    //-------------------------------------------------------------------------
    /**
     * @brief find if name is in nested list
     * @param name name of nested scope
     */
    Scope *findInNestedListRecursive(const std::string &name);

    //-------------------------------------------------------------------------
    void addVariable(const Token *token_, const Token *start_,
                     const Token *end_, AccessControl access_, const Type *type_,
                     const Scope *scope_, const Settings *settings);






    /** @brief initialize varlist */
    void getVariableList(const Settings *settings);

    //-------------------------------------------------------------------------
    void addFunction(const Function &func);

    //-------------------------------------------------------------------------
    AccessControl defaultAccess() const;

    //-------------------------------------------------------------------------
    /**
     * @brief check if statement is variable declaration and add it if it is
     * @param tok pointer to start of statement
     * @param varaccess access control of statement
     * @param settings Settings
     * @return pointer to last token
     */
    const Token *checkVariable(const Token *tok, AccessControl varaccess, const Settings *settings)
    {
        bool isAdded;
        return checkVariable(tok, varaccess, settings, isAdded);
    }
    const Token *checkVariable(const Token *tok, AccessControl varaccess, const Settings *settings, bool &isAdded);

    //-------------------------------------------------------------------------
    /**
     * @brief get variable from name
     * @param varname name of variable
     * @return pointer to variable
     */
    const Variable *getVariable(const std::string &varname) const;

    //-------------------------------------------------------------------------
    const Token *addEnum(const Token *tok);

    //-------------------------------------------------------------------------

private:

    //-------------------------------------------------------------------------
    /**
     * @brief helper function for getVariableList()
     * @param tok pointer to token to check
     * @param vartok populated with pointer to the variable token, if found
     * @param typetok populated with pointer to the type token, if found
     * @return true if tok points to a variable declaration, false otherwise
     */
    bool isVariableDeclaration(const Token *const tok, const Token *&vartok, const Token *&typetok) const;

    //-------------------------------------------------------------------------
    void findFunctionInBase(const std::string &name, size_t args, std::vector<const Function *> &matches) const;

    //-------------------------------------------------------------------------
};

//-----------------------------------------------------------------------------
#endif // scopeH