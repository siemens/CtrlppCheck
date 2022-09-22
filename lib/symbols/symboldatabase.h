//-----------------------------------------------------------------------------
#ifndef symboldatabaseH
#define symboldatabaseH
//-----------------------------------------------------------------------------

#include "type.h"
#include "scope.h"
#include "valuetype.h"
#include "enumerator.h"

#include <list>
#include <iostream>
#include <vector>
#include <map>

//-----------------------------------------------------------------------------

class CPPCHECKLIB SymbolDatabase
{

public:

    //-------------------------------------------------------------------------
    SymbolDatabase(const Tokenizer *tokenizer, const Settings *settings, ErrorLogger *errorLogger);
    
    //-------------------------------------------------------------------------
    ~SymbolDatabase();

    //-------------------------------------------------------------------------
    /** @brief Information about all namespaces/classes/structrues */
    std::list<Scope> scopeList;

    //-------------------------------------------------------------------------
    /** @brief Fast access to function scopes */
    std::vector<const Scope *> functionScopes;

    //-------------------------------------------------------------------------
    /** @brief Fast access to class and struct scopes */
    std::vector<const Scope *> classAndStructScopes;

    //-------------------------------------------------------------------------
    /** @brief Fast access to types */
    std::list<Type> typeList;

    //-------------------------------------------------------------------------
    /** Whether iName is a ctrl keyword*/
    static bool isReservedName(const std::string &iName);

    //-------------------------------------------------------------------------
    /**
     * @brief find a variable type if it's a user defined type
     * @param start scope to start looking in
     * @param typeTok token containing variable type
     * @return pointer to type if found or NULL if not found
     */
    const Type *findVariableType(const Scope *start, const Token *typeTok) const;

    //-------------------------------------------------------------------------
    /**
     * @brief find a function
     * @param tok token of function call
     * @return pointer to function if found or NULL if not found
     */
    const Function *findFunction(const Token *tok) const;

    //-------------------------------------------------------------------------
    const Type *findType(const Token *startTok, const Scope *startScope) const;

    //-------------------------------------------------------------------------
    Type *findType(const Token *startTok, Scope *startScope) const
    {
        return const_cast<Type *>(this->findType(startTok, const_cast<const Scope *>(startScope)));
    }

    //-------------------------------------------------------------------------
    const Scope *findScope(const Token *tok, const Scope *startScope) const;

    //-------------------------------------------------------------------------
    Scope *findScope(const Token *tok, Scope *startScope) const
    {
        return const_cast<Scope *>(this->findScope(tok, const_cast<const Scope *>(startScope)));
    }

    //-------------------------------------------------------------------------
    const Variable *getVariableFromVarId(std::size_t varId) const
    {
        return mVariableList.at(varId);
    }

    //-------------------------------------------------------------------------
    const std::vector<const Variable *> &variableList() const
    {
        return mVariableList;
    }

    //-------------------------------------------------------------------------
    /**
     * @brief output a debug message
     */
    void debugMessage(const Token *tok, const std::string &msg) const;

    //-------------------------------------------------------------------------
    void printOut(const char *title = nullptr) const;

    //-------------------------------------------------------------------------
    void printVariable(const Variable *var, const char *indent) const;

    //-------------------------------------------------------------------------
    void printXml(std::ostream &out) const;

    //-------------------------------------------------------------------------
    /*
     * @brief Do a sanity check
     */
    void validate() const;

    //-------------------------------------------------------------------------
    void validateExecutableScopes() const;

    //-------------------------------------------------------------------------
    /** Set valuetype in provided tokenlist */
    void setValueTypeInTokenList();

    //-------------------------------------------------------------------------
    /**
     * Calculates sizeof value for given type.
     * @param type Token which will contain e.g. "int", "*", or string.
     * @return sizeof for given type, or 0 if it can't be calculated.
     */
    unsigned int sizeOfType(const Token *type) const;

    //-------------------------------------------------------------------------

private:
    friend class Scope;
    friend class Function;

    //-------------------------------------------------------------------------
    // Create symboldatabase...
    void createSymbolDatabaseFindAllScopes();
    bool _isValidEnumCode(const Token *tok);
    bool _isValidStructCode(const Token *tok);
    bool _isValidClassCode(const Token *tok);
    void createSymbolDatabaseClassInfo();
    void createSymbolDatabaseVariableInfo();
    void createSymbolDatabaseFunctionScopes();
    void createSymbolDatabaseClassAndStructScopes();
    void createSymbolDatabaseFunctionReturnTypes();
    void createSymbolDatabaseNeedInitialization();
    void createSymbolDatabaseVariableSymbolTable();
    void createSymbolDatabaseSetScopePointers();
    void createSymbolDatabaseSetFunctionPointers(bool firstPass);
    void createSymbolDatabaseSetVariablePointers();
    void createSymbolDatabaseSetTypePointers();
    void createSymbolDatabaseEnums();
    void createSymbolDatabaseUnknownArrayDimensions();


    //-------------------------------------------------------------------------
    void addClassFunction(Scope **scope, const Token **tok, const Token *argStart);
    Function *addGlobalFunctionDecl(Scope *&scope, const Token *tok, const Token *argStart, const Token *funcStart);
    Function *addGlobalFunction(Scope *&scope, const Token *&tok, const Token *argStart, const Token *funcStart);
    void addNewFunction(Scope **scope, const Token **tok);
    bool isFunction(const Token *tok, const Scope *outerScope, const Token **funcStart, const Token **argStart, const Token **declEnd) const;
    const Type *findTypeInNested(const Token *startTok, const Scope *startScope) const;
    Function *findFunctionInScope(const Token *func, const Scope *ns, const std::string &path, unsigned int path_length);
    const Type *findVariableTypeInBase(const Scope *scope, const Token *typeTok) const;
    
    //-------------------------------------------------------------------------
    typedef std::map<unsigned int, unsigned int> MemberIdMap;
    typedef std::map<unsigned int, MemberIdMap> VarIdMap;

    //-------------------------------------------------------------------------
    void fixVarId(VarIdMap &varIds, const Token *vartok, Token *membertok, const Variable *membervar);

    //-------------------------------------------------------------------------
    const Enumerator *findEnumerator(const Token *tok) const;

    //-------------------------------------------------------------------------
    void setValueType(Token *tok, const ValueType &valuetype);
    void setValueType(Token *tok, const Variable &var);
    void setValueType(Token *tok, const Enumerator &enumerator);

    //-------------------------------------------------------------------------
    const Tokenizer *mTokenizer;
    const Settings *mSettings;
    ErrorLogger *mErrorLogger;

    //-------------------------------------------------------------------------
    /** variable symbol table */
    std::vector<const Variable *> mVariableList;

    //-------------------------------------------------------------------------
    /** list for missing types */
    std::list<Type> mBlankTypes;

    //-------------------------------------------------------------------------
    /** "negative cache" list of tokens that we find are not enumeration values */
    mutable std::set<std::string> mTokensThatAreNotEnumeratorValues;

    //-------------------------------------------------------------------------
};

//-----------------------------------------------------------------------------
#endif // symboldatabaseH