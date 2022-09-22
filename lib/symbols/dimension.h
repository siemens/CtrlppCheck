//-----------------------------------------------------------------------------
#ifndef dimensionH
#define dimensionH
//-----------------------------------------------------------------------------

#include "../token.h"

//-----------------------------------------------------------------------------
/**
 * @brief Array dimension information.
 */
struct Dimension
{

    //-------------------------------------------------------------------------
    Dimension() : start(nullptr), end(nullptr), num(0), known(true) {}

    const Token *start;  ///< size start token
    const Token *end;    ///< size end token
    MathLib::bigint num; ///< (assumed) dimension length when size is a number, 0 if not known
    bool known;          ///< Known size

    //-------------------------------------------------------------------------
    
};

//-----------------------------------------------------------------------------
#endif // dimensionH