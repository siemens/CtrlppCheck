//-----------------------------------------------------------------------------
#ifndef functionH
#define functionH
//-----------------------------------------------------------------------------

#include "../tokenize.h"
#include "../token.h"
#include "accesscontrol.h"
#include "scope.h"
#include "symboldatabase.h"
#include "variable.h"

#include <list>

//-----------------------------------------------------------------------------
class CPPCHECKLIB Function
{
    //-------------------------------------------------------------------------
    /** @brief flags mask used to access specific bit. */
    enum
    {
        fHasBody = (1 << 0),       ///< @brief has implementation
        fIsInline = (1 << 1),      ///< @brief implementation in class definition
        fIsConst = (1 << 2),       ///< @brief is const
        fIsStatic = (1 << 5),      ///< @brief is static
        fIsStaticLocal = (1 << 6), ///< @brief is static local
        fIsVariadic = (1 << 19)    ///< @brief is variadic
    };

    //-------------------------------------------------------------------------
    /**
     * Get specified flag state.
     * @param flag flag to get state of
     * @return true if flag set or false in flag not set
     */
    bool getFlag(unsigned int flag) const
    {
        return ((mFlags & flag) != 0);
    }

    //-------------------------------------------------------------------------
    /**
     * Set specified flag state.
     * @param flag flag to set state
     * @param state new state of flag
     */
    void setFlag(unsigned int flag, bool state)
    {
        mFlags = state ? mFlags | flag : mFlags & ~flag;
    }

public:

    //-------------------------------------------------------------------------
    enum Type
    {
        eConstructor,
        eDestructor,
        eFunction
    };

    //-------------------------------------------------------------------------
    Function(const Tokenizer *mTokenizer, const Token *tok, const Scope *scope, const Token *tokDef, const Token *tokArgDef);

    //-------------------------------------------------------------------------
    const std::string &name() const
    {
        return tokenDef->str();
    }

    //-------------------------------------------------------------------------
    std::size_t argCount() const
    {
        return argumentList.size();
    }
    
    //-------------------------------------------------------------------------
    std::size_t minArgCount() const
    {
        return argumentList.size() - initArgCount;
    }
    //-------------------------------------------------------------------------
    const Variable *getArgumentVar(std::size_t num) const;

    //-------------------------------------------------------------------------
    unsigned int initializedArgCount() const
    {
        return initArgCount;
    }

    //-------------------------------------------------------------------------
    void addArguments(const SymbolDatabase *symbolDatabase, const Scope *scope);

    //-------------------------------------------------------------------------
    /** @brief get function in base class that is overridden */
    const Function *getOverriddenFunction(bool *foundAllBaseClasses = nullptr) const;

    //-------------------------------------------------------------------------
    bool isConstructor() const
    {
        return type == eConstructor;
    }

    //-------------------------------------------------------------------------
    bool isDestructor() const
    {
        return type == eDestructor;
    }

    //-------------------------------------------------------------------------
    bool isAttributeConstructor() const
    {
        return tokenDef->isAttributeConstructor();
    }

    //-------------------------------------------------------------------------
    bool isAttributeDestructor() const
    {
        return tokenDef->isAttributeDestructor();
    }

    //-------------------------------------------------------------------------
    bool isAttributePure() const
    {
        return tokenDef->isAttributePure();
    }

    //-------------------------------------------------------------------------
    bool isAttributeConst() const
    {
        return tokenDef->isAttributeConst();
    }

    //-------------------------------------------------------------------------
    bool isAttributeNoreturn() const
    {
        return tokenDef->isAttributeNoreturn();
    }

    //-------------------------------------------------------------------------
    bool isAttributeNodiscard() const
    {
        return tokenDef->isAttributeNodiscard();
    }

    //-------------------------------------------------------------------------
    bool hasBody() const
    {
        return getFlag(fHasBody);
    }

    //-------------------------------------------------------------------------
    bool isInline() const
    {
        return getFlag(fIsInline);
    }

    //-------------------------------------------------------------------------
    bool isConst() const
    {
        return getFlag(fIsConst);
    }

    //-------------------------------------------------------------------------
    bool isStatic() const
    {
        return getFlag(fIsStatic);
    }

    //-------------------------------------------------------------------------
    bool isStaticLocal() const
    {
        return getFlag(fIsStaticLocal);
    }

    //-------------------------------------------------------------------------
    bool isVariadic() const
    {
        return getFlag(fIsVariadic);
    }

    //-------------------------------------------------------------------------
    void hasBody(bool state)
    {
        setFlag(fHasBody, state);
    }

    //-------------------------------------------------------------------------
    const Token *tokenDef;            ///< function name token in class definition
    const Token *argDef;              ///< function argument start '(' in class definition
    const Token *token;               ///< function name token in implementation
    const Token *arg;                 ///< function argument start '('
    const Token *retDef;              ///< function return type token
    const ::Type *retType;            ///< function return type
    const Scope *functionScope;       ///< scope of function body
    const Scope *nestedIn;            ///< Scope the function is declared in
    std::list<Variable> argumentList; ///< argument list
    unsigned int initArgCount;        ///< number of args with default values
    Type type;                        ///< constructor, destructor, ...
    AccessControl access;             ///< public/protected/private
    const Token *noexceptArg;         ///< noexcept token
    const Token *throwArg;            ///< throw token

    //-------------------------------------------------------------------------
    static bool argsMatch(const Scope *scope, const Token *first, const Token *second, const std::string &path, unsigned int path_length);

    //-------------------------------------------------------------------------
    /**
     * @return token to ":" if the function is a constructor
     * and it contains member initialization otherwise a nullptr is returned
     */
    const Token *constructorMemberInitialization() const;

    //-------------------------------------------------------------------------

private:

    //-------------------------------------------------------------------------
    /** Recursively determine if this function overrides a virtual function in a base class */
    const Function *getOverriddenFunctionRecursive(const ::Type *baseType, bool *foundAllBaseClasses) const;

    //-------------------------------------------------------------------------
    unsigned int mFlags;

    //-------------------------------------------------------------------------
    void isInline(bool state)
    {
        setFlag(fIsInline, state);
    }

    //-------------------------------------------------------------------------
    void isConst(bool state)
    {
        setFlag(fIsConst, state);
    }

    //-------------------------------------------------------------------------
    void isStatic(bool state)
    {
        setFlag(fIsStatic, state);
    }

    //-------------------------------------------------------------------------
    void isStaticLocal(bool state)
    {
        setFlag(fIsStaticLocal, state);
    }

    //-------------------------------------------------------------------------
    void isVariadic(bool state)
    {
        setFlag(fIsVariadic, state);
    }

    //-------------------------------------------------------------------------
};

//-----------------------------------------------------------------------------
#endif // functionH