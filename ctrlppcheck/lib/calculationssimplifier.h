

//---------------------------------------------------------------------------
#ifndef CalculationssimplifierH
#define CalculationssimplifierH
//---------------------------------------------------------------------------

#include "config.h"

#include <ctime>
#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>

class ErrorLogger;
class Settings;
class Token;
class Tokenizer;
class TokenList;

/// @addtogroup Core
/// @{

/** @brief Simplify templates from the preprocessed and partially simplified code. */
class CPPCHECKLIB CalculationsSimplifier {
public:
    explicit CalculationsSimplifier(Tokenizer *tokenizer);
    ~CalculationsSimplifier();


    /**
     * Simplify constant calculations such as "1+2" => "3"
     * @param tok start token
     * @return true if modifications to token-list are done.
     *         false if no modifications are done.
     */
    static bool simplifyNumericCalculations(Token *tok);

    /**
     * Simplify constant calculations such as "1+2" => "3".
     * This also performs simple cleanup of parentheses etc.
     * @return true if modifications to token-list are done.
     *         false if no modifications are done.
     */
    bool simplifyCalculations(Token* frontToken = nullptr, Token *backToken = nullptr);

private:

    /*
     * Same as Token::eraseTokens() but tries to fix up lists with pointers to the deleted tokens.
     * @param begin Tokens after this will be erased.
     * @param end Tokens before this will be erased.
     */
    static void eraseTokens(Token *begin, const Token *end);

    Tokenizer *mTokenizer;
    TokenList &mTokenList;
};

/// @}
//---------------------------------------------------------------------------
#endif // CalculationssimplifierH
