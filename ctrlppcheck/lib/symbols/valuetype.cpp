//-----------------------------------------------------------------------------
#include "valuetype.h"

#include "symbolutils.h"
#include "scope.h"

//-----------------------------------------------------------------------------

ValueType ValueType::parseDecl(const Token *type, const Settings *settings)
{
    ValueType vt;
    parsedecl(type, &vt, settings);
    return vt;
}

//-----------------------------------------------------------------------------
// the CCN is terrible big, but it is really easy to read.
ValueType::Type ValueType::typeFromString(const std::string &typestr)
{
    // standard ctrl variables
    if ("anytype" == typestr)
    {
        return ValueType::Type::ANYTYPE;
    }
    if ("mixed" == typestr)
    {
        return ValueType::Type::MIXED;
    }
    if ("char" == typestr)
    {
        return ValueType::Type::CHAR;
    }
    if ("float" == typestr)
    {
        return ValueType::Type::FLOAT;
    }
    if ("int" == typestr)
    {
        return ValueType::Type::INT;
    }
    if ("uint" == typestr)
    {
        return ValueType::Type::UINT;
    }
    if ("long" == typestr)
    {
        return ValueType::Type::LONG;
    }
    if ("ulong" == typestr)
    {
        return ValueType::Type::ULONG;
    }
    if ("short" == typestr)
    {
        return ValueType::Type::SHORT;
    }
    //if ("signed" == typestr) { return ValueType::Type::SIGNED; }
    //if ("unsigned" == typestr) { return ValueType::Type::UNSIGNED; }
    if ("function_ptr" == typestr)
    {
        return ValueType::Type::FUNCTION_PTR;
    }
    if ("shared_ptr" == typestr)
    {
        return ValueType::Type::SHARED_PTR;
    }
    if ("nullptr" == typestr)
    {
        return ValueType::Type::NULL_PTR;
    }
    if ("time" == typestr)
    {
        return ValueType::Type::TIME;
    }
    if ("string" == typestr)
    {
        return ValueType::Type::STRING;
    }
    if ("langString" == typestr)
    {
        return ValueType::Type::LANG_STRING;
    }
    if ("bool" == typestr)
    {
        return ValueType::Type::BOOL;
    }
    if ("bit32" == typestr)
    {
        return ValueType::Type::BIT32;
    }
    if ("bit64" == typestr)
    {
        return ValueType::Type::BIT64;
    }
    if ("file" == typestr)
    {
        return ValueType::Type::FILE;
    }
    if ("blob" == typestr)
    {
        return ValueType::Type::BLOB;
    }
    if ("atime" == typestr)
    {
        return ValueType::Type::ATIME;
    }
    if ("errClass" == typestr)
    {
        return ValueType::Type::ERR_CLASS;
    }
    if ("dbRecordset" == typestr)
    {
        return ValueType::Type::DB_RECORDSET;
    }
    if ("dbConnection" == typestr)
    {
        return ValueType::Type::DB_CONNECTION;
    }
    if ("dbCommand" == typestr)
    {
        return ValueType::Type::DB_COMMAND;
    }
    if ("shape" == typestr)
    {
        return ValueType::Type::SHAPE;
    }
    if ("idispatch" == typestr)
    {
        return ValueType::Type::IDISPATCH;
    }
    if ("mapping" == typestr)
    {
        return ValueType::Type::MAPPING;
    }
    if ("va_list" == typestr)
    {
        return ValueType::Type::VA_LIST;
    }

    // dyn variables
    if ("dyn_int" == typestr)
    {
        return ValueType::Type::DYN_INT;
    }
    if ("dyn_uint" == typestr)
    {
        return ValueType::Type::DYN_UINT;
    }
    if ("dyn_long" == typestr)
    {
        return ValueType::Type::DYN_LONG;
    }
    if ("dyn_ulong" == typestr)
    {
        return ValueType::Type::DYN_ULONG;
    }
    if ("dyn_float" == typestr)
    {
        return ValueType::Type::DYN_FLOAT;
    }
    if ("dyn_time" == typestr)
    {
        return ValueType::Type::DYN_TIME;
    }
    if ("dyn_atime" == typestr)
    {
        return ValueType::Type::DYN_ATIME;
    }
    if ("dyn_string" == typestr)
    {
        return ValueType::Type::DYN_STRING;
    }
    if ("dyn_langString" == typestr)
    {
        return ValueType::Type::DYN_LANG_STRING;
    }
    if ("dyn_bool" == typestr)
    {
        return ValueType::Type::DYN_BOOL;
    }
    if ("dyn_bit32" == typestr)
    {
        return ValueType::Type::DYN_BIT32;
    }
    if ("dyn_bit64" == typestr)
    {
        return ValueType::Type::DYN_BIT64;
    }
    if ("dyn_char" == typestr)
    {
        return ValueType::Type::DYN_CHAR;
    }
    if ("dyn_blob" == typestr)
    {
        return ValueType::Type::DYN_BLOB;
    }
    if ("dyn_anytype" == typestr)
    {
        return ValueType::Type::DYN_ANYTYPE;
    }
    if ("dyn_mixed" == typestr)
    {
        return ValueType::Type::DYN_MIXED;
    }
    if ("dyn_errClass" == typestr)
    {
        return ValueType::Type::DYN_ERR_CLASS;
    }
    if ("dyn_mapping" == typestr)
    {
        return ValueType::Type::DYN_MAPPING;
    }
    if ("dyn_dbConnection" == typestr)
    {
        return ValueType::Type::DYN_DB_CONNECTION;
    }
    if ("dyn_dbCommand" == typestr)
    {
        return ValueType::Type::DYN_DB_COMMAND;
    }
    if ("dyn_dbRecordset" == typestr)
    {
        return ValueType::Type::DYN_DB_RECORDSET;
    }
    if ("dyn_shape" == typestr)
    {
        return ValueType::Type::DYN_SHAPE;
    }
    if ("dyn_function_ptr" == typestr)
    {
        return ValueType::Type::DYN_FUNCTION_PTR;
    }

    // dyn_dyn variables
    if ("dyn_dyn_int" == typestr)
    {
        return ValueType::Type::DYN_DYN_INT;
    }
    if ("dyn_dyn_uint" == typestr)
    {
        return ValueType::Type::DYN_DYN_UINT;
    }
    if ("dyn_dyn_long" == typestr)
    {
        return ValueType::Type::DYN_DYN_LONG;
    }
    if ("dyn_dyn_ulong" == typestr)
    {
        return ValueType::Type::DYN_DYN_ULONG;
    }
    if ("dyn_dyn_float" == typestr)
    {
        return ValueType::Type::DYN_DYN_FLOAT;
    }
    if ("dyn_dyn_time" == typestr)
    {
        return ValueType::Type::DYN_DYN_TIME;
    }
    if ("dyn_dyn_atime" == typestr)
    {
        return ValueType::Type::DYN_DYN_ATIME;
    }
    if ("dyn_dyn_string" == typestr)
    {
        return ValueType::Type::DYN_DYN_STRING;
    }
    if ("dyn_dyn_langString" == typestr)
    {
        return ValueType::Type::DYN_DYN_LANG_STRING;
    }
    if ("dyn_dyn_bool" == typestr)
    {
        return ValueType::Type::DYN_DYN_BOOL;
    }
    if ("dyn_dyn_bit32" == typestr)
    {
        return ValueType::Type::DYN_DYN_BIT32;
    }
    if ("dyn_dyn_bit64" == typestr)
    {
        return ValueType::Type::DYN_DYN_BIT64;
    }
    if ("dyn_dyn_char" == typestr)
    {
        return ValueType::Type::DYN_DYN_CHAR;
    }
    if ("dyn_dyn_anytype" == typestr)
    {
        return ValueType::Type::DYN_DYN_ANYTYPE;
    }
    if ("dyn_dyn_mixed" == typestr)
    {
        return ValueType::Type::DYN_DYN_MIXED;
    }
    if ("dyn_dyn_errClass" == typestr)
    {
        return ValueType::Type::DYN_DYN_ERR_CLASS;
    }

    // added in WinCC OA 3.17
    if ("vector" == typestr)
    {
        return ValueType::Type::VECTOR;
    }

    return ValueType::Type::UNKNOWN_TYPE;
}

//-----------------------------------------------------------------------------
bool ValueType::fromLibraryType(const std::string &typestr, const Settings *settings)
{
    const Library::PodType *podtype = settings->library.podtype(typestr);
    if (podtype && (podtype->sign == 's' || podtype->sign == 'u'))
    {
        if (podtype->size == 1)
            type = ValueType::Type::CHAR;
        else if (podtype->size == settings->sizeof_int)
            type = ValueType::Type::INT;
        else if (podtype->size == settings->sizeof_short)
            type = ValueType::Type::SHORT;
        else if (podtype->size == settings->sizeof_long)
            type = ValueType::Type::LONG;
        else
            type = ValueType::Type::UNKNOWN_INT;

        return true;
    }

    const Library::PlatformType *platformType = settings->library.platform_type(typestr, settings->platformString());
    if (platformType)
    {
        if (platformType->mType == "char")
            type = ValueType::Type::CHAR;
        else if (platformType->mType == "short")
            type = ValueType::Type::SHORT;
        else if (platformType->mType == "int")
            type = ValueType::Type::INT;
        else if (platformType->mType == "long")
            type = ValueType::Type::LONG;

        if (platformType->_const_ptr)
            constness = 1;
        return true;
    }

    return false;
}

//-----------------------------------------------------------------------------
/// @todo add all ctrl variable types here
std::string ValueType::dump() const
{
    /// @todo add ctrl variables here
    std::ostringstream ret;
    switch (type)
    {
    case UNKNOWN_TYPE:
        return "";
    case NONSTD:
        ret << "valueType-type=\"nonstd\"";
        break;
    case RECORD:
        ret << "valueType-type=\"record\"";
        break;
    case VOID:
        ret << "valueType-type=\"void\"";
        break;
    case BOOL:
        ret << "valueType-type=\"bool\"";
        break;
    case CHAR:
        ret << "valueType-type=\"char\"";
        break;
    case SHORT:
        ret << "valueType-type=\"short\"";
        break;
    case INT:
        ret << "valueType-type=\"int\"";
        break;
    case LONG:
        ret << "valueType-type=\"long\"";
        break;
    case UNKNOWN_INT:
        ret << "valueType-type=\"unknown int\"";
        break;
    case FLOAT:
        ret << "valueType-type=\"float\"";
        break;
    case DOUBLE:
        ret << "valueType-type=\"double\"";
        break;
    /*case LONGDOUBLE:
      ret << "valueType-type=\"long double\"";
      break;*/
    case STRING:
        ret << "valueType-type=\"string\"";
        break;
    default:
        ret << "valueType-type=\"" << str() << "\"";
        break;
    };

    if (bits > 0)
        ret << " valueType-bits=\"" << bits << '\"';

    if (constness > 0)
        ret << " valueType-constness=\"" << constness << '\"';

    if (typeScope)
        ret << " valueType-typeScope=\"" << typeScope << '\"';

    if (!originalTypeName.empty())
        ret << " valueType-originalTypeName=\"" << originalTypeName << '\"';

    return ret.str();
}

//-----------------------------------------------------------------------------
std::string ValueType::str() const
{
    return str(false);
}

//-----------------------------------------------------------------------------
// There are some commented lines. Not all variables have dyn_ or dyn_dyn_ variant.
// But i will keep it here. May be it will be supported some how in ctrl language.
std::string ValueType::str(const bool cppstyle) const
{
    // add all ctrl variables here
    std::string ret;
    if (cppstyle && (constness & 1))
        ret = " const ";
    if (type == VOID)
        ret += "void";
    else if (type == ANYTYPE)
        ret += "anytype";
    else if (type == MIXED)
        ret += "mixed";
    else if (type == CHAR)
        ret += "char";
    else if (type == FLOAT)
        ret += "float";
    else if (type == INT)
        ret += "int";
    else if (type == UINT)
        ret += "uint";
    else if (type == LONG)
        ret += "long";
    else if (type == ULONG)
        ret += "ulong";
    else if (type == SHORT)
        ret += "short";
    /*else if (type == SIGNED)
      ret += "signed";
    else if (type == UNSIGNED)
      ret += "unsigned";*/
    else if (type == FUNCTION_PTR)
        ret += "function_ptr";
    else if (type == SHARED_PTR)
        ret += "shared_ptr";
    else if (type == NULL_PTR)
        ret += "nullptr";
    else if (type == TIME)
        ret += "time";
    else if (type == STRING)
        ret += "string";
    else if (type == LANG_STRING)
        ret += "langString";
    else if (type == BOOL)
        ret += "bool";
    else if (type == BIT32)
        ret += "bit32";
    else if (type == BIT64)
        ret += "bit64";
    else if (type == FILE)
        ret += "file";
    else if (type == BLOB)
        ret += "blob";
    else if (type == ATIME)
        ret += "atime";
    else if (type == ERR_CLASS)
        ret += "errClass";
    else if (type == DB_COMMAND)
        ret += "dbCommand";
    else if (type == DB_CONNECTION)
        ret += "dbConnection";
    else if (type == DB_RECORDSET)
        ret += "dbRecordset";
    else if (type == SHAPE)
        ret += "shape";
    else if (type == IDISPATCH)
        ret += "idispatch";
    else if (type == MAPPING)
        ret += "mapping";
    else if (type == VA_LIST)
        ret += "va_list";
    // dyn variables
    else if (type == DYN_ANYTYPE)
        ret += "dyn_anytype";
    else if (type == DYN_MIXED)
        ret += "dyn_mixed";
    else if (type == DYN_CHAR)
        ret += "dyn_char";
    else if (type == DYN_FLOAT)
        ret += "dyn_float";
    else if (type == DYN_INT)
        ret += "dyn_int";
    else if (type == DYN_UINT)
        ret += "dyn_uint";
    else if (type == DYN_LONG)
        ret += "dyn_long";
    else if (type == DYN_ULONG)
        ret += "dyn_ulong";
    /*else if (type == DYN_SHORT)
      ret += "dyn_short";
    else if (type == DYN_SIGNED)
      ret += "dyn_signed";
    else if (type == DYN_UNSIGNED)
      ret += "dyn_unsigned";*/
    else if (type == DYN_FUNCTION_PTR)
        ret += "dyn_function_ptr";
    /*else if (type == DYN_SHARED_PTR)
      ret += "dyn_shared_ptr";
    else if (type == DYN_NULL_PTR)
      ret += "dyn_nullptr";*/
    else if (type == DYN_TIME)
        ret += "dyn_time";
    else if (type == DYN_STRING)
        ret += "dyn_string";
    else if (type == DYN_LANG_STRING)
        ret += "dyn_langString";
    else if (type == DYN_BOOL)
        ret += "dyn_bool";
    else if (type == DYN_BIT32)
        ret += "dyn_bit32";
    else if (type == DYN_BIT64)
        ret += "dyn_bit64";
    /*else if (type == DYN_FILE)
      ret += "dyn_file";*/
    else if (type == DYN_BLOB)
        ret += "dyn_blob";
    else if (type == DYN_ATIME)
        ret += "dyn_atime";
    else if (type == DYN_ERR_CLASS)
        ret += "dyn_errClass";
    else if (type == DYN_DB_COMMAND)
        ret += "dyn_dbCommand";
    else if (type == DYN_DB_CONNECTION)
        ret += "dyn_dbConnection";
    else if (type == DYN_DB_RECORDSET)
        ret += "dyn_dbRecordset";
    else if (type == DYN_SHAPE)
        ret += "dyn_shape";
    /*else if (type == DYN_IDISPATCH)
      ret += "dyn_idispatch";*/
    else if (type == DYN_MAPPING)
        ret += "dyn_mapping";
    /*else if (type == DYN_VA_LIST)
      ret += "dyn_va_list";*/

    // dyn_dyn variables
    else if (type == DYN_DYN_ANYTYPE)
        ret += "dyn_dyn_anytype";
    else if (type == DYN_DYN_MIXED)
        ret += "dyn_dyn_mixed";
    else if (type == DYN_DYN_CHAR)
        ret += "dyn_dyn_char";
    else if (type == DYN_DYN_FLOAT)
        ret += "dyn_dyn_float";
    else if (type == DYN_DYN_INT)
        ret += "dyn_dyn_int";
    else if (type == DYN_DYN_UINT)
        ret += "dyn_dyn_uint";
    else if (type == DYN_DYN_LONG)
        ret += "dyn_dyn_long";
    else if (type == DYN_DYN_ULONG)
        ret += "dyn_dyn_ulong";
    /*else if (type == DYN_DYN_SHORT)
      ret += "dyn_dyn_short";
    else if (type == DYN_DYN_SIGNED)
      ret += "dyn_dyn_signed";
    else if (type == DYN_DYN_UNSIGNED)
      ret += "dyn_dyn_unsigned";
    else if (type == DYN_DYN_FUNCTION_PTR)
      ret += "dyn_dyn_function_ptr";
    else if (type == DYN_DYN_SHARED_PTR)
      ret += "dyn_dyn_shared_ptr";
    else if (type == DYN_DYN_NULL_PTR)
      ret += "dyn_dyn_nullptr";*/
    else if (type == DYN_DYN_TIME)
        ret += "dyn_dyn_time";
    else if (type == DYN_DYN_STRING)
        ret += "dyn_dyn_string";
    else if (type == DYN_DYN_LANG_STRING)
        ret += "dyn_dyn_langString";
    else if (type == DYN_DYN_BOOL)
        ret += "dyn_dyn_bool";
    else if (type == DYN_DYN_BIT32)
        ret += "dyn_dyn_bit32";
    else if (type == DYN_DYN_BIT64)
        ret += "dyn_dyn_bit64";
    /*else if (type == DYN_DYN_FILE)
      ret += "dyn_dyn_file";
    else if (type == DYN_DYN_BLOB)
      ret += "dyn_dyn_blob";*/
    else if (type == DYN_DYN_ATIME)
        ret += "dyn_dyn_atime";
    else if (type == DYN_DYN_ERR_CLASS)
        ret += "dyn_dyn_errClass";
    /*else if (type == DYN_DYN_DB_COMMAND)
      ret += "dyn_dyn_dbCommand";
    else if (type == DYN_DYN_DB_CONNECTION)
      ret += "dyn_dyn_dbConnection";
    else if (type == DYN_DYN_DB_RECORDSET)
      ret += "dyn_dyn_dbRecordset";
    else if (type == DYN_DYN_SHAPE)
      ret += "dyn_dyn_shape";
    else if (type == DYN_DYN_IDISPATCH)
      ret += "dyn_dyn_idispatch";
    else if (type == DYN_DYN_MAPPING)
      ret += "dyn_dyn_mapping";
    else if (type == DYN_DYN_VA_LIST)
      ret += "dyn_dyn_va_list";*/
      
    // added in WinCC OA 3.17
    else if (type == VECTOR)
        ret += "vector";

    /// @todo clarify and refactor this code for ctrl lang
    // some thing
    else if ((type == ValueType::Type::NONSTD || type == ValueType::Type::RECORD) && typeScope)
    {
        std::string className(typeScope->className);
        const Scope *scope = typeScope->definedType ? typeScope->definedType->enclosingScope : typeScope->nestedIn;
        while (scope && scope->type != Scope::eGlobal)
        {
            if (scope->type == Scope::eClass || scope->type == Scope::eStruct)
                className = scope->className + "::" + className;
            scope = scope->definedType ? scope->definedType->enclosingScope : scope->nestedIn;
        }
        ret += ' ' + className;
    }

    return (ret.empty() || !cppstyle) ? ret : ret.substr(1);
}

//-----------------------------------------------------------------------------
std::string ValueType::typeToString() const
{
    std::string ret;

    if (type == VOID)
        return "void";
    else if (isIntegral())
    {
        if (type == BOOL)
            return "bool";
        else if (type == CHAR)
            return "char";
        else if (type == SHORT)
            return "short";
        else if (type == INT)
            return "int";
        else if (type == LONG)
            return "long";
        else if (type == UNKNOWN_INT)
            return "unknown_int";
    }
    else if (type == FLOAT)
        return "float";
    else if (type == DOUBLE)
        return "double";
    else if (type == ANYTYPE)
        return "anytype";
    else if (type == ATIME)
        return "atime";
    else if (type == BIT32)
        return "bit32";
    else if (type == BIT64)
        return "bit64";
    else if (type == BLOB)
        return "blob";
    else if (type == FILE)
        return "file";
    else if (type == FUNCTION_PTR)
        return "function_ptr";
    else if (type == UINT)
        return "uint";
    else if (type == ULONG)
        return "ulong";
    else if (type == MIXED)
        return "mixed";
    else if (type == MAPPING)
        return "mapping";
    else if (type == VA_LIST)
        return "va_list";
    else if (type == STRING)
        return "string";
    else if (type == TIME)
        return "time";
    else if (type == SHAPE)
        return "shape";
    else if (type == DYN_ANYTYPE)
        return "dyn_anytype";
    else if (type == DYN_ATIME)
        return "dyn_atime";
    else if (type == DYN_BIT32)
        return "dyn_bit32";
    else if (type == DYN_BIT64)
        return "dyn_bit64";
    else if (type == DYN_BLOB)
        return "dyn_blob";
    else if (type == DYN_BOOL)
        return "dyn_bool";
    else if (type == DYN_CHAR)
        return "dyn_char";
    else if (type == DYN_FLOAT)
        return "dyn_float";
    else if (type == DYN_INT)
        return "dyn_int";
    else if (type == DYN_UINT)
        return "dyn_uint";
    else if (type == DYN_LONG)
        return "dyn_long";
    else if (type == DYN_ULONG)
        return "dyn_ulong";
    else if (type == DYN_MAPPING)
        return "dyn_mapping";
    else if (type == DYN_STRING)
        return "dyn_string";
    else if (type == DYN_TIME)
        return "dyn_time";
    else if (type == DYN_SHAPE)
        return "dyn_shape";
    else if (type == DYN_DYN_ANYTYPE)
        return "dyn_dyn_anytype";
    else if (type == DYN_DYN_ATIME)
        return "dyn_dyn_atime";
    else if (type == DYN_DYN_BIT32)
        return "dyn_dyn_bit32";
    else if (type == DYN_DYN_BIT64)
        return "dyn_dyn_bit64";
    else if (type == DYN_DYN_BOOL)
        return "dyn_dyn_bool";
    else if (type == DYN_DYN_CHAR)
        return "dyn_dyn_char";
    else if (type == DYN_DYN_FLOAT)
        return "dyn_dyn_float";
    else if (type == DYN_DYN_INT)
        return "dyn_dyn_int";
    else if (type == DYN_DYN_UINT)
        return "dyn_dyn_uint";
    else if (type == DYN_DYN_LONG)
        return "dyn_dyn_long";
    else if (type == DYN_DYN_ULONG)
        return "dyn_dyn_ulong";
    else if (type == DYN_DYN_STRING)
        return "dyn_dyn_string";
    else if (type == DYN_DYN_TIME)
        return "dyn_dyn_time";
    // added in WinCC OA 3.17
    else if (type == VECTOR)
        return "vector";

    else if ((type == ValueType::Type::NONSTD || type == ValueType::Type::RECORD) && typeScope)
    {
        std::string className(typeScope->className);
        const Scope *scope = typeScope->definedType ? typeScope->definedType->enclosingScope : typeScope->nestedIn;
        while (scope && scope->type != Scope::eGlobal)
        {
            if (scope->type == Scope::eClass || scope->type == Scope::eStruct)
                className = scope->className + "::" + className;
            scope = scope->definedType ? scope->definedType->enclosingScope : scope->nestedIn;
        }
        return className;
    }

    return ""; //unkown
}

//-----------------------------------------------------------------------------
bool ValueType::isEnum() const
{
    return typeScope && typeScope->type == Scope::eEnum;
}

//-----------------------------------------------------------------------------
bool ValueType::canCastTo(const std::string &otherType) const
{
    //Any type can be casted to anytype or mixed
    //Also returns true when type == otherType
    if (otherType == "anytype" ||
        otherType == "mixed" || 
        otherType == typeToString())
    {
        return true;
    }

    //Map of types that can be casted
    //ValueType::Type (FROM) -> std::vector<std::string>> (TO)
    //If the ValueType:Type is not in this is map i can't be casted
    std::map<ValueType::Type, std::vector<std::string>> castToMap = {
        {BOOL, {"int", "float", "long", "ulong", "uint", "double", "string"}},
        {CHAR, {"int", "float", "long", "ulong", "uint", "double", "string"}},
        {SHORT, {"int", "float", "long", "ulong", "uint", "double", "string"}},
        {INT, {"float", "long", "ulong", "uint", "double", "string", "time"}},
        {LONG, {"float", "ulong", "double", "string", "time"}},
        {ULONG, {"float", "long", "double", "string", "time"}},
        {UINT, {"int", "float", "long", "ulong", "double", "string", "time"}},
        {FLOAT, {"double", "string", "time"}},
        {DOUBLE, {"float", "string", "time"}},
        {STRING, {"lang_string"}},
        {LANG_STRING, {"string"}},
        {TIME, {"string"}},
        {ATIME, {"string"}}};

    //Find current type int the castToMap
    if (castToMap.find(type) != castToMap.end())
    {
        std::vector<std::string> possibleCasts = castToMap[type];
        return std::find(possibleCasts.begin(), possibleCasts.end(), otherType) != possibleCasts.end();
    }

    //Current value isn't in the castTo Map
    return false;
}

//-----------------------------------------------------------------------------