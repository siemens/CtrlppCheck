//-----------------------------------------------------------------------------
#ifndef symbolsutilsH
#define symbolsutilsH
//-----------------------------------------------------------------------------

#include "../settings.h"
#include "../token.h"

#include "valuetype.h"
#include "scope.h"
#include "symboldatabase.h"

//-----------------------------------------------------------------------------
const Type *findVariableTypeIncludingUsedNamespaces(const SymbolDatabase *symbolDatabase, const Scope *scope, const Token *typeTok);

//-----------------------------------------------------------------------------
const Token *parsedecl(const Token *type, ValueType *const valuetype, const Settings *settings);

//-----------------------------------------------------------------------------
#endif // symbolsutilsH