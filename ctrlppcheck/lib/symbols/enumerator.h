//-----------------------------------------------------------------------------
#ifndef enumeratorH
#define enumeratorH
//-----------------------------------------------------------------------------

#include "../token.h"
#include "scope.h"

//-----------------------------------------------------------------------------
class CPPCHECKLIB Enumerator
{
public:

    //-------------------------------------------------------------------------
    explicit Enumerator(const Scope *scope_) : scope(scope_), name(nullptr), value(0), start(nullptr), end(nullptr), value_known(false) {}
    const Scope *scope;
    const Token *name;
    MathLib::bigint value;
    const Token *start;
    const Token *end;
    bool value_known;

    //-------------------------------------------------------------------------
};

//-----------------------------------------------------------------------------
#endif // enumeratorH
