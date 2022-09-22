//---------------------------------------------------------------------------
#ifndef variableH
#define variableH
//---------------------------------------------------------------------------

#include "../library.h"
#include "../settings.h"
#include "../token.h"

#include "type.h"
#include "scope.h"
#include "accesscontrol.h"
#include "dimension.h"

/** @brief Information about a member variable. */
class CPPCHECKLIB Variable
{

    //-------------------------------------------------------------------------

    /** @brief flags mask used to access specific bit. */
    enum
    {
        fIsStatic = (1 << 1),     /** @brief static variable */
        fIsConst = (1 << 2),      /** @brief const variable */
        fIsClass = (1 << 4),      /** @brief user defined type */
        fIsArray = (1 << 5),      /** @brief array variable */
        fIsReference = (1 << 7),  /** @brief reference variable */
        fIsRValueRef = (1 << 8),  /** @brief rvalue reference variable */
        fHasDefault = (1 << 9),   /** @brief function argument with default value */
        fIsStlString = (1 << 11), /** @brief string */
        fIsFloatType = (1 << 12), /** @brief Floating point type */
    };

    //-------------------------------------------------------------------------

    /**
     * Get specified flag state.
     * @param flag_ flag to get state of
     * @return true if flag set or false in flag not set
     */
    bool getFlag(unsigned int flag_) const
    {
        return ((mFlags & flag_) != 0);
    }

    //-------------------------------------------------------------------------

    /**
     * Set specified flag state.
     * @param flag_ flag to set state
     * @param state_ new state of flag
     */
    void setFlag(unsigned int flag_, bool state_)
    {
        mFlags = state_ ? mFlags | flag_ : mFlags & ~flag_;
    }

    //-------------------------------------------------------------------------

    /**
     * @brief parse and save array dimension information
     * @param lib Library instance
     * @return true if array, false if not
     */
    bool arrayDimensions(const Library *lib);

    //-------------------------------------------------------------------------

public:
    //-------------------------------------------------------------------------

    Variable(const Token *name_, const Token *start_, const Token *end_,
             std::size_t index_, AccessControl access_, const Type *type_,
             const Scope *scope_, const Settings *settings)
        : mNameToken(name_),
          mTypeStartToken(start_),
          mTypeEndToken(end_),
          mIndex(index_),
          mAccess(access_),
          mFlags(0),
          mType(type_),
          mScope(scope_),
          mValueType(nullptr)
    {
        evaluate(settings);
    }

    //-------------------------------------------------------------------------

    ~Variable();

    //-------------------------------------------------------------------------

    /**
     * Get name token.
     * @return name token
     */
    const Token *nameToken() const
    {
        return mNameToken;
    }

    //-------------------------------------------------------------------------

    /**
     * Get type start token.
     * The type start token doesn't account 'static' and 'const' qualifiers
     * E.g.:
     *     static const int * const p = ...;
     * type start token ^
     * @return type start token
     */
    const Token *typeStartToken() const
    {
        return mTypeStartToken;
    }

    //-------------------------------------------------------------------------

    /**
     * Get type end token.
     * The type end token doesn't account the forward 'const' qualifier
     * E.g.:
     *     static const int * const p = ...;
     *       type end token ^
     * @return type end token
     */
    const Token *typeEndToken() const
    {
        return mTypeEndToken;
    }

    //-------------------------------------------------------------------------

    /**
     * Get end token of variable declaration
     * E.g.
     * int i[2][3] = ...
     *   end token ^
     * @return variable declaration end token
     */
    const Token *declEndToken() const;

    //-------------------------------------------------------------------------

    /**
     * Get name string.
     * @return name string
     */
    const std::string &name() const
    {
        // name may not exist for function arguments
        if (mNameToken)
            return mNameToken->str();

        return emptyString;
    }

    //-------------------------------------------------------------------------

    /**
     * Get declaration ID (varId used for variable in its declaration).
     * @return declaration ID
     */
    unsigned int declarationId() const
    {
        // name may not exist for function arguments
        if (mNameToken)
            return mNameToken->varId();

        return 0;
    }

    //-------------------------------------------------------------------------

    /**
     * Get index of variable in declared order.
     * @return variable index
     */
    std::size_t index() const
    {
        return mIndex;
    }

    //-------------------------------------------------------------------------

    /**
     * Is variable public.
     * @return true if public, false if not
     */
    bool isPublic() const
    {
        return mAccess == Public;
    }

    //-------------------------------------------------------------------------

    /**
     * Is variable protected.
     * @return true if protected, false if not
     */
    bool isProtected() const
    {
        return mAccess == Protected;
    }

    //-------------------------------------------------------------------------

    /**
     * Is variable private.
     * @return true if private, false if not
     */
    bool isPrivate() const
    {
        return mAccess == Private;
    }

    //-------------------------------------------------------------------------

    /**
     * Is variable global.
     * @return true if global, false if not
     */
    bool isGlobal() const
    {
        return mAccess == Global;
    }

    //-------------------------------------------------------------------------

    /**
     * Is variable a function argument.
     * @return true if a function argument, false if not
     */
    bool isArgument() const
    {
        return mAccess == Argument;
    }

    //-------------------------------------------------------------------------

    /**
     * Is variable local.
     * @return true if local, false if not
     */
    bool isLocal() const
    {
        return (mAccess == Local);
    }

    //-------------------------------------------------------------------------

    /**
     * Is variable static.
     * @return true if static, false if not
     */
    bool isStatic() const
    {
        return getFlag(fIsStatic);
    }

    //-------------------------------------------------------------------------

    /**
     * Is variable const.
     * @return true if const, false if not
     */
    bool isConst() const
    {
        return getFlag(fIsConst);
    }

    //-------------------------------------------------------------------------

    /**
     * Is variable a throw type.
     * @return true if throw type, false if not
     */
    bool isThrow() const
    {
        return mAccess == Throw;
    }

    //-------------------------------------------------------------------------

    /**
     * Is variable a user defined (or unknown) type.
     * @return true if user defined type, false if not
     */
    bool isClass() const
    {
        return getFlag(fIsClass);
    }

    //-------------------------------------------------------------------------

    /**
     * Is variable an array.
     * @return true if array, false if not
     */
    bool isArray() const
    {
        return getFlag(fIsArray);
    }

    //-------------------------------------------------------------------------

    /**
     * Is array or pointer variable.
     * @return true if pointer or array, false otherwise
     */
    bool isArrayOrPointer() const
    {
    }

    //-------------------------------------------------------------------------

    /**
     * Is reference variable.
     * @return true if reference, false otherwise
     */
    bool isReference() const
    {
        return getFlag(fIsReference);
    }

    //-------------------------------------------------------------------------

    /**
     * Is reference variable.
     * @return true if reference, false otherwise
     */
    bool isRValueReference() const
    {
        return getFlag(fIsRValueRef);
    }

    //-------------------------------------------------------------------------

    /**
     * Does variable have a default value.
     * @return true if has a default falue, false if not
     */
    bool hasDefault() const
    {
        return getFlag(fHasDefault);
    }

    //-------------------------------------------------------------------------

    /**
     * Get Type pointer of known type.
     * @return pointer to type if known, NULL if not known
     */
    const Type *type() const
    {
        return mType;
    }

    //-------------------------------------------------------------------------

    /**
     * Get Scope pointer of known type.
     * @return pointer to type scope if known, NULL if not known
     */
    const Scope *typeScope() const
    {
        return mType ? mType->classScope : nullptr;
    }

    //-------------------------------------------------------------------------

    /**
     * Get Scope pointer of enclosing scope.
     * @return pointer to enclosing scope
     */
    const Scope *scope() const
    {
        return mScope;
    }

    //-------------------------------------------------------------------------

    /**
     * Get array dimensions.
     * @return array dimensions vector
     */
    const std::vector<Dimension> &dimensions() const
    {
        return mDimensions;
    }

    //-------------------------------------------------------------------------

    /**
     * Get array dimension length.
     * @return length of dimension
     */
    MathLib::bigint dimension(std::size_t index_) const
    {
        if ( mDimensions.empty() || ( index_ < 0 ) || (mDimensions.size() <= index_) )
        {
            return 0;
        }
        return mDimensions[index_].num;
    }

    //-------------------------------------------------------------------------

    /**
     * Get array dimension known.
     * @return length of dimension known
     */
    bool dimensionKnown(std::size_t index_) const
    {
        if ( mDimensions.empty() || ( index_ < 0 ) || (mDimensions.size() <= index_) )
        {
            return false;
        }
        return mDimensions[index_].known;
    }

    //-------------------------------------------------------------------------

    /**
     * Checks if the variable is an STL type ('std::')
     * E.g.:
     *   std::string s;
     *   ...
     *   sVar->isStlType() == true
     * @return true if it is an stl type and its type matches any of the types in 'stlTypes'
     */
    bool isStlStringType() const
    {
        return getFlag(fIsStlString);
    }

    //-------------------------------------------------------------------------

    /**
    * Determine whether it's a floating number type
    * @return true if the type is known and it's a floating type (float, double and long double) or a pointer/array to it
    */
    bool isFloatingType() const
    {
        return getFlag(fIsFloatType);
    }

    //-------------------------------------------------------------------------

    /**
    * Determine whether it's an enumeration type
    * @return true if the type is known and it's an enumeration type
    */
    bool isEnumType() const
    {
        return type() && type()->isEnumType();
    }

    //-------------------------------------------------------------------------

    const ValueType *valueType() const
    {
        return mValueType;
    }

    //-------------------------------------------------------------------------

    void setValueType(const ValueType &valueType);

    //-------------------------------------------------------------------------

    AccessControl accessControl() const
    {
        return mAccess;
    }

    //-------------------------------------------------------------------------

private:

    //-------------------------------------------------------------------------
    // only symbol database can change the type
    friend class SymbolDatabase;

    //-------------------------------------------------------------------------

    /**
     * Set Type pointer to known type.
     * @param t type
     */
    void type(const Type *t)
    {
        mType = t;
    }

    //-------------------------------------------------------------------------

    /** @brief variable name token */
    const Token *mNameToken;

    //-------------------------------------------------------------------------

    /** @brief variable type start token */
    const Token *mTypeStartToken;

    //-------------------------------------------------------------------------

    /** @brief variable type end token */
    const Token *mTypeEndToken;

    //-------------------------------------------------------------------------

    /** @brief order declared */
    std::size_t mIndex;

    //-------------------------------------------------------------------------

    /** @brief what section is this variable declared in? */
    AccessControl mAccess; // public/protected/private

    //-------------------------------------------------------------------------

    /** @brief flags */
    unsigned int mFlags;

    //-------------------------------------------------------------------------

    /** @brief pointer to user defined type info (for known types) */
    const Type *mType;

    //-------------------------------------------------------------------------

    /** @brief pointer to scope this variable is in */
    const Scope *mScope;

    //-------------------------------------------------------------------------

    ValueType *mValueType;

    //-------------------------------------------------------------------------

    /** @brief array dimensions */
    std::vector<Dimension> mDimensions;

    //-------------------------------------------------------------------------

    /** @brief fill in information, depending on Tokens given at instantiation */
    void evaluate(const Settings *settings);

    //-------------------------------------------------------------------------
};

//---------------------------------------------------------------------------
#endif // variableH