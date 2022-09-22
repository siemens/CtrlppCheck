//---------------------------------------------------------------------------
#ifndef valuetypeH
#define valuetypeH
//---------------------------------------------------------------------------

//#include "scope.h"

#include "../library.h"
#include "../token.h"

#include <string>

//class Scope;

/** Value type */
class CPPCHECKLIB ValueType
{
public:
    //-------------------------------------------------------------------------
    //simplifyPlatformTypes | simplifyStdType | simplifyBitfields

    //-------------------------------------------------------------------------
    /**
      All supported (ctrl) variable types.
    */
    enum Type
    {
        UNKNOWN_TYPE,
        // types defined by cppcheck
        NONSTD,
        RECORD,    //will be used for class & struct not sure if it will be supported in ctrl lang, http://jcsites.juniata.edu/faculty/rhodes/cs2/ch05a.htm
        VOID,
        // integral types -- isIntegral()
        BOOL,
        CHAR,
        SHORT,
        INT,
        LONG,
        ULONG,
        UINT,
        UNKNOWN_INT, // it looks like a integer but the exact type is not known, I am not sure, but it looks like a defensive code
                     // float types -- isFloat()
        FLOAT,
        DOUBLE,

        // ctrl specific variable types
        STRING,
        LANG_STRING,
        ERR_CLASS,
        MAPPING,
        FUNCTION_PTR,
        SHARED_PTR,
        NULL_PTR,
        TIME,
        ATIME,
        BIT32,
        BIT64,
        FILE,
        BLOB,
        DB_RECORDSET,
        DB_CONNECTION,
        DB_COMMAND,
        SHAPE,
        IDISPATCH,
        VA_LIST,
        ANYTYPE,
        MIXED, // really bad variable types
        SIGNED_T,
        UNSIGNED_T, // what ever for type it is. It shall be equal to int, unit
                    // dyn variables
        DYN_INT,
        DYN_UINT,
        DYN_LONG,
        DYN_ULONG,
        DYN_FLOAT,
        DYN_TIME,
        DYN_ATIME,
        DYN_STRING,
        DYN_LANG_STRING,
        DYN_BOOL,
        DYN_BIT32,
        DYN_BIT64,
        DYN_CHAR,
        DYN_BLOB,
        DYN_ANYTYPE,
        DYN_MIXED,
        DYN_ERR_CLASS,
        DYN_MAPPING,
        DYN_DB_CONNECTION,
        DYN_DB_COMMAND,
        DYN_DB_RECORDSET,
        DYN_SHAPE,
        DYN_FUNCTION_PTR,
        // dyn_dyn variables
        DYN_DYN_INT,
        DYN_DYN_UINT,
        DYN_DYN_LONG,
        DYN_DYN_ULONG,
        DYN_DYN_FLOAT,
        DYN_DYN_TIME,
        DYN_DYN_ATIME,
        DYN_DYN_STRING,
        DYN_DYN_LANG_STRING,
        DYN_DYN_BOOL,
        DYN_DYN_BIT32,
        DYN_DYN_BIT64,
        DYN_DYN_CHAR,
        DYN_DYN_ANYTYPE,
        DYN_DYN_MIXED,
        DYN_DYN_ERR_CLASS,
        // added in WinCC OA 3.17
        VECTOR
    } type;

    //-------------------------------------------------------------------------
    unsigned int bits; ///< bitfield bitcount

    unsigned int constness;              ///< bit 0=data, bit 1=*, bit 2=**
    const Scope *typeScope;              ///< if the type definition is seen this point out the type scope
    std::string originalTypeName;        ///< original type name as written in the source code. eg. this might be "uint8_t" when type is CHAR.

    ValueType() : type(UNKNOWN_TYPE), bits(0), constness(0U), typeScope(nullptr) {}
    ValueType(const ValueType &vt) : type(vt.type), bits(vt.bits), constness(vt.constness), typeScope(vt.typeScope), originalTypeName(vt.originalTypeName) {}
    ValueType(enum Type t, unsigned int p) : type(t), bits(0), constness(0U), typeScope(nullptr) {}
    ValueType(enum Type t, unsigned int p, unsigned int c) : type(t), bits(0), constness(c), typeScope(nullptr) {}
    ValueType(enum Type t, unsigned int p, unsigned int c, const std::string &otn) : type(t), bits(0), constness(c), typeScope(nullptr), originalTypeName(otn) {}
    ValueType &operator=(const ValueType &other) = delete;

    static ValueType parseDecl(const Token *type, const Settings *settings);

    //-------------------------------------------------------------------------
    /**
       Convert variable type from string to Type.
       Used to read ctrl code.
       @param typestr type string (string, dyn_dyn_float...)
    */
    static Type typeFromString(const std::string &typestr);

    //-------------------------------------------------------------------------
    /// @todo clarify why shall be bool and char integral?
    /// and documentig the fucking code.
    bool isIntegral() const
    {
        return (type >= ValueType::Type::BOOL && type <= ValueType::Type::UNKNOWN_INT);
    }

    //-------------------------------------------------------------------------
    /**
      @brief Check if variable type is floating type.
      @details Only float and double variable types are floating in ctrl lanuage.
      @note anytype and mixed are ignored.
      @return Return TRUE when variable is floating type.
    */
    bool isFloat() const
    {
        return (type == ValueType::Type::FLOAT || type == ValueType::Type::DOUBLE);
    }


    //---------------------------------------------------------------------------
    /**
     * @brief Function check if the variable is of type Wincc OA shape variable 
     */
    bool isShape() const
    {
      return ( type == ValueType::Type::SHAPE );
    }

    //---------------------------------------------------------------------------
    /**
     * @brief Function check if the variable is known wincc oa type
     */
    bool isKnownType() const
    {
      return ( type != ValueType::Type::UNKNOWN_TYPE );
    }

    //---------------------------------------------------------------------------
    /**
     * Function check if the variable is a dyn_* type (array) or vector<>
     * To check if is pure dyn_ var use also function isVectorVar()
     */
    bool isDynVar() const
    {
      return (type >= ValueType::Type::DYN_INT && type <= ValueType::Type::DYN_FUNCTION_PTR) || (type == ValueType::Type::VECTOR);
    }

    //---------------------------------------------------------------------------
    /**
     * Function check if the variable is wincc oa vector variable
     */
    bool isVectorVar() const
    {
      return (type == ValueType::Type::VECTOR);
    }

    //---------------------------------------------------------------------------
    /**
     * Function check if the variable is dyn_dyn_* type (2 x array).
     */
    bool isDynDynVar() const
    {
      return (type >= ValueType::Type::DYN_DYN_INT && type <= ValueType::Type::DYN_DYN_ERR_CLASS);
    }
    
    //---------------------------------------------------------------------------
    /**
     * Function check if the variable is wincc oa mapping variable
     */
    bool isMappingVar() const
    {
      return (type == ValueType::Type::MAPPING);
    }
    
    //---------------------------------------------------------------------------
    /**
     * Function check if the variable is of give type
     */
    bool isType(ValueType::Type checkedType) const
    {
      return (type == checkedType);
    }

    //---------------------------------------------------------------------------

    bool fromLibraryType(const std::string &typestr, const Settings *settings);

    //---------------------------------------------------------------------------

    bool isEnum() const;

    //---------------------------------------------------------------------------

    bool canCastTo(const std::string &otherType) const;

    //-------------------------------------------------------------------------
    /**
    * Convert variable type to human readable string.
    */
    std::string str() const;

    //-------------------------------------------------------------------------
    /**
    * Convert variable type to human readable string.
    * @param pure. Shall be print in pure format.
    */
    std::string str(const bool pure) const;

    //-------------------------------------------------------------------------
    /**
    * Convert variable type dump string.
    */
    std::string dump() const;
    std::string typeToString() const;
};

//---------------------------------------------------------------------------
#endif // valuetypeH
