

//---------------------------------------------------------------------------
#include "checkY2038.h"

#include "astutils.h"
#include "errorlogger.h"
#include "mathlib.h"
#include "settings.h"
#include "symbols/symbols.h"
#include "token.h"
#include "tokenize.h"
#include "valueflow.h"

#include <cstddef>
#include <list>
//---------------------------------------------------------------------------

// Register this check class (by creating a static instance of it)
namespace
{
  CheckY2038 instance;
}

//---------------------------------------------------------------------------
// CWEs


static const CWE CWE704(704U); // Incorrect Type Conversion or Cast
static const struct CWE CWE758(758U); // Reliance on Undefined, Unspecified, or Implementation-Defined Behavior


//---------------------------------------------------------------------------
const ValueType* CheckY2038::getValType(const Token *tok)
{
  const ValueType *valType;

  if ( !tok )
  {
    return nullptr;
  }

  // value type is defined
  if ( tok->valueType() )
  {
    return tok->valueType();
  }
  // function call
  else if ( Token::Match(tok, "%name% ("))
  {
    const Function *func = tok->function();
    std::string type;
    if (func)
    {
      if ( func->retDef )
      {
        type = func->retDef->str();
      }
    }
    else
    {
      type = mSettings->library.returnValueType(tok);
    }

    ValueType valType2;
    valType2.type = ValueType::typeFromString(type);

    return new ValueType(valType2);
  }
  // explicit cast
  else if ( Token::Match(tok->previous(), "( %type% )"))
  {
    ValueType valType2;
    valType2.type = ValueType::typeFromString(tok->str());
    return new ValueType(valType2);
  }

  return nullptr;
}

//---------------------------------------------------------------------------
std::string CheckY2038::getVarName(const Token *tok)
{
  if ( !tok )
    return "variable";

  const ValueType *valType = getValType(tok);

  std::string typeDef = "";
  if ( valType )
    typeDef = " of type '" + valType->str() + "'";

  if ( Token::Match(tok, "%name% (") )
  {
    if ( valType )
      typeDef = " with return type '" + valType->str() + "'";
    return "function " + tok->expressionString() + "()" + typeDef;
  }
  // explicit cast
  else if ( Token::Match(tok->previous(), "( %type% )") )
  {
    return tok->previous()->expressionString();
  }
  else if ( tok->variable() )
  {
    return "variable '" + tok->expressionString() + "'" + typeDef;
  }

  // defensive
  return tok->expressionString() + typeDef;
}


//---------------------------------------------------------------------------
void CheckY2038::timeVarCastOperands(const Token *tok)
{
  if (!tok || !tok->isOp() || !tok->astOperand1())
    return;

  const Token *left;
  const Token *right;

  left = tok->astOperand1();
  right = tok->astOperand2();

  if (!left || !right)
    return; // no idea why it happens, but check it here. otherwise it crash

  if ( right->previous() && Token::Match(right->previous(), "%name% (") )
    right = right->previous();


  if ( Token::Match(right, "%name% (") )
  {
    // the right side is a function, so check if the return value from function can be caste to time variable
    checkConversion(left, right);
    return;
  }

  const ValueType *valTypeLeft, *valTypeRight;
  valTypeLeft = left->valueType();
  valTypeRight = right->valueType();

  if (!valTypeLeft || valTypeLeft->str() == "")
  {
    // it looks like issue in cppcheck self, when the variable has no type
    return; // unkown value type
  }

  if (!valTypeRight || valTypeLeft->str() == "")
  {
    // it looks like issue in cppcheck self, when the variable has no type
    return; // unkown value type
  }

  // Conversion check?
  checkConversion(left, right);
}

//---------------------------------------------------------------------------
void CheckY2038::timeVarCastExplCast(const Token *tok)
{
  if ( !Token::Match(tok, "( %type% )"))
    return;

   checkConversion(tok->tokAt(1), tok->tokAt(3));
}

//---------------------------------------------------------------------------
void CheckY2038::timeVarCastFunction(const Token *tok)
{
  const Function *func = tok->function();

  if ( func )
  {
    // ctrl function founded in code
    const std::vector<const Token *> &callArguments = getArguments(tok);
    for (unsigned int argnr = 0U; argnr < callArguments.size(); ++argnr)
    {
      const Token *arg = callArguments[argnr];
      if ( !arg )
        break; // paranoid check (defensive)

      const Variable *argVar = func->getArgumentVar(argnr);

      if ( !argVar )
        break; // there are too much arguments called

      checkConversion(argVar->nameToken(), arg);
    }
    return;
  }

  if ( !mSettings )
    return; // defensive, paranoid check

  // library function (defined in .xml files)
  const std::string &functionName = mSettings->library.getFunctionName(tok);

  if ( (functionName == "")  || (mSettings->library.functions.find(functionName) == mSettings->library.functions.cend()) )
    return; // defensive, function not found in library therefore can not be checked

  const std::vector<const Token *> &callArguments = getArguments(tok);
  for (unsigned int argnr = 0U; argnr < callArguments.size(); ++argnr)
  {
    const Token *arg = callArguments[argnr];
    if ( !arg )
      break; // paranoid check (defensive)

    const unsigned int fArgNr = argnr + 1;
    std::string argValueType = mSettings->library.valueTypeArg(tok, fArgNr);

    Token *argVar = new Token();
    const std::string argName = mSettings->library.getArgName(tok, fArgNr);

    if ( argName.empty() || argName == "variadic" )
      break; // ignore variadic arguments
    
    argVar->str("Argument " + argName + " of function " + tok->str() + "()");
    argVar->linenr(tok->linenr());
   // argVar = tok;
    ValueType *type;
    type = new ValueType();
    type->type = ValueType::typeFromString(argValueType);
    argVar->setValueType(type);
    checkConversion(argVar, arg);
  }
}


//---------------------------------------------------------------------------
/*
 * check variable cast from and into time variable
 */
void CheckY2038::timeVarCast(void)
{
  const SymbolDatabase *symbolDatabase = mTokenizer->getSymbolDatabase();
  for (const Scope * scope : symbolDatabase->functionScopes) {
    for (const Token* tok = scope->bodyStart->next(); tok != scope->bodyEnd; tok = tok->next()) {
//  std::cout << __FUNCTION__ << " " << tok->str() << std::endl;

      // check how function hanlde parameter
      if ( Token::Match(tok, "%name% (")  )
      {
        timeVarCastFunction(tok);
      }

      // check operands
      timeVarCastOperands(tok);

      // check explicit casts
      timeVarCastExplCast(tok);
    }
  }
}


//---------------------------------------------------------------------------
void CheckY2038::checkConversion(const Token *left, const Token *right)
{
    if ( !left || !right )
      return;
    const ValueType *valTypeLeft, *valTypeRight;

    // std::cout << left->str() << " R " << right->str() << std::endl;
    valTypeLeft = getValType(left);

   //   if ( valTypeLeft )
   //   std::cout << left->str() << " valTypeLeft " << valTypeLeft->str() << std::endl;

    valTypeRight = getValType(right);
  //    if ( valTypeRight )
  //      std::cout << right->str() << " valTypeRight " << valTypeRight->str() << std::endl;

     if ( !valTypeLeft || !valTypeRight )
       return; // defensive, no body know, but somehow it happens
 // Do not warn about assignment with NULL
    if (FwdAnalysis::isNullOperand(right))
      return;

    if ( valTypeLeft->isType(ValueType::Type::TIME) && valTypeRight->isType(ValueType::Type::TIME) )
      return; // everthing is fine. Lenf and rith side are time variable types

    if ( !valTypeLeft->isType(ValueType::Type::TIME) && !valTypeRight->isType(ValueType::Type::TIME) &&
         !valTypeLeft->isType(ValueType::Type::MIXED) && !valTypeRight->isType(ValueType::Type::MIXED) &&
         !valTypeLeft->isType(ValueType::Type::ANYTYPE) && !valTypeRight->isType(ValueType::Type::ANYTYPE) )
      return; // everthing is fine. Lenf and rith side are NOT of time variable type

    // if ( valTypeLeft->isDynVar() || valTypeRight->isDynVar() ||
    //      valTypeLeft->isDynDynVar() || valTypeRight->isDynDynVar() )
    // {
    //   // @todo check dyn_time casts ...
    //   return;
    // }

    if ( valTypeRight->isType(ValueType::Type::TIME) )
    {
      // right side is time var. check if can by casted in to other vars, or you can lost sime values

      // safe cast in to
      // float, uint, long, ulong, string, anytype, mixed
      if ( valTypeLeft->isType(ValueType::Type::FLOAT) ||
           valTypeLeft->isType(ValueType::Type::UINT) ||
           valTypeLeft->isType(ValueType::Type::LONG) ||
           valTypeLeft->isType(ValueType::Type::ULONG) ||
           valTypeLeft->isType(ValueType::Type::STRING) ||
           valTypeLeft->isType(ValueType::Type::ANYTYPE) ||
           valTypeLeft->isType(ValueType::Type::MIXED) )
      {
        // safe cast, nothing to do here
      }

      else if ( valTypeLeft->isType(ValueType::Type::UNKNOWN_TYPE) )
      {
        y2038unkownTypeError(left);
      }

      // unsafe cast with value overflow
      // int, short
      else if ( valTypeLeft->isType(ValueType::Type::INT) ||
           valTypeLeft->isType(ValueType::Type::SHORT) ||
           valTypeRight->isType(ValueType::Type::BIT32))
      {
        y2038overflow(left, right, Severity::warning, false);
        return;
      }

      // unpossible cast
      else if ( valTypeLeft->isType(ValueType::Type::LANG_STRING) ||
           valTypeLeft->isType(ValueType::Type::ERR_CLASS) ||
           valTypeLeft->isType(ValueType::Type::MAPPING) ||
           valTypeLeft->isType(ValueType::Type::FUNCTION_PTR) ||
           valTypeLeft->isType(ValueType::Type::SHARED_PTR) ||
           valTypeLeft->isType(ValueType::Type::NULL_PTR) ||
           valTypeLeft->isType(ValueType::Type::FILE) ||
           valTypeLeft->isType(ValueType::Type::BLOB) ||
           valTypeLeft->isType(ValueType::Type::DB_RECORDSET) ||
           valTypeLeft->isType(ValueType::Type::DB_COMMAND) ||
           valTypeLeft->isType(ValueType::Type::DB_CONNECTION) ||
           valTypeLeft->isType(ValueType::Type::SHAPE) ||
           valTypeLeft->isType(ValueType::Type::IDISPATCH) ||
           valTypeLeft->isType(ValueType::Type::VA_LIST) ||
           valTypeLeft->isType(ValueType::Type::VECTOR) ||
           valTypeLeft->isDynDynVar() ||
           valTypeLeft->isDynVar() )
      {
        y2038canNotCastError(left, right, Severity::warning, false);
      }

      else
      {
        // this looks possible but not sure if it works, therefore inconclusive
        y2038canNotCastError(left, right, Severity::warning, true);
      }
    }

    if ( valTypeLeft->isType(ValueType::Type::TIME) )
    {
      // left side is time var. check if can by casted from other vars, or you can lost sime information


      // safe cast from
      // float, uint, long, ulong, string, anytype, mixed
      if ( valTypeRight->isType(ValueType::Type::FLOAT) ||
           valTypeRight->isType(ValueType::Type::UINT) ||
           valTypeRight->isType(ValueType::Type::LONG) ||
           valTypeRight->isType(ValueType::Type::ULONG) ||
           valTypeRight->isType(ValueType::Type::ATIME) )
      {
        // safe cast, nothing to do here
      }

      else if ( valTypeRight->isType(ValueType::Type::UNKNOWN_TYPE) )
      {
        y2038unkownTypeError(right);
      }

      // unsafe cast with value lost
      // int, short
      else if ( valTypeRight->isType(ValueType::Type::INT) ||
           valTypeRight->isType(ValueType::Type::SHORT) )
      {
        y2038valueLost(left, right, Severity::warning, false);
      }

      // unpossible cast
      else if ( valTypeRight->isType(ValueType::Type::STRING) ||
           valTypeRight->isType(ValueType::Type::LANG_STRING) ||
           valTypeRight->isType(ValueType::Type::ERR_CLASS) ||
           valTypeRight->isType(ValueType::Type::MAPPING) ||
           valTypeRight->isType(ValueType::Type::FUNCTION_PTR) ||
           valTypeRight->isType(ValueType::Type::SHARED_PTR) ||
           valTypeRight->isType(ValueType::Type::NULL_PTR) ||
           valTypeRight->isType(ValueType::Type::FILE) ||
           valTypeRight->isType(ValueType::Type::BLOB) ||
           valTypeRight->isType(ValueType::Type::DB_RECORDSET) ||
           valTypeRight->isType(ValueType::Type::DB_COMMAND) ||
           valTypeRight->isType(ValueType::Type::DB_CONNECTION) ||
           valTypeRight->isType(ValueType::Type::SHAPE) ||
           valTypeRight->isType(ValueType::Type::IDISPATCH) ||
           valTypeRight->isType(ValueType::Type::VA_LIST) ||
           valTypeRight->isType(ValueType::Type::VECTOR) ||
           valTypeRight->isDynDynVar() ||
           valTypeRight->isDynVar() )
      {
        // operator+ : time + string will cast in to time therefore well be " bla 2" in the follosiwn
        // example ignored
        /*
         time t = getCurrentTime();
         // wrong
         string s = "bla " + t + " bla 2";
         // riht
         string s = "bla " + (string)t + " bla 2";
         */
        y2038canNotCastError(left, right, Severity::warning, false);
      }

      else
      {
        // this looks possible but not sure if it works, therefore inconclusive
        y2038canNotCastError(left, right, Severity::warning, true);
      }

    }
}


//---------------------------------------------------------------------------
void CheckY2038::y2038unkownTypeError(const Token *tok)
{
  if ( tok && !mSettings->inconclusive )
    return;
  std::string tokName = tok ? tok->expressionString() : "variableName";
  std::string errmsg = "$symbol:" + tokName + "\n"+
                       "Handling of unknown variable type '" + tokName + "' into time variable.\n"
                       "Unknown variable type of '" + tokName + "' leads to undefined scenario. Check if the variable can by casted in to time variable.";
  reportError(tok, Severity::warning, "y2038unkownTypeError", errmsg, CWE704, true);
}

//---------------------------------------------------------------------------
void CheckY2038::y2038canNotCastError(const Token *left, const Token *right, Severity::SeverityType prio, bool inconclusive)
{
  if ( left && (!mSettings->isEnabled(prio) || (inconclusive && !mSettings->inconclusive) ) )
    return;
  reportError(left, prio, "y2038canNotCastError",
              "$symbol:" + right->str() + "\n" + "$symbol:" + left->str() + "\n" +
              "The value of " + getVarName(right) + " can not be safely casted to " + getVarName(left) + ".",
              CWE758, inconclusive);
}

//---------------------------------------------------------------------------
void CheckY2038::y2038overflow(const Token *left, const Token *right, Severity::SeverityType prio, bool inconclusive)
{
  if ( left && (!mSettings->isEnabled(prio) || (inconclusive && !mSettings->inconclusive) ) )
    return;
  reportError(left, prio, "y2038overflow",
              "$symbol:" + right->str() + "\n" + "$symbol:" + left->str() + "\n" +
              "Possible value over-flow from " + getValType(right)->str() + " to " + getValType(left)->str() + "\n" +
              "Possible value over-flow when January 19th, 2038 at 03:14:07 GMT reached. The value of " + getVarName(right) + " is not safely casted to " + getVarName(left) + ".",
              CWE758, inconclusive);
}


//---------------------------------------------------------------------------
void CheckY2038::y2038valueLost(const Token *left, const Token *right, Severity::SeverityType prio, bool inconclusive)
{
  if ( !left || !right || (!mSettings->isEnabled(prio) || (inconclusive && !mSettings->inconclusive) ) )
    return;
  reportError(right, prio, "y2038valueLost",
              "$symbol:" + right->str() + "\n" + "$symbol:" + left->str() + "\n" +
              "Possible value lost from " + getValType(right)->str() + " to " + getValType(left)->str() + "\n" +
              "Possible value lost when January 19th, 2038 at 03:14:07 GMT reached. The value of " + getVarName(right) + " is not safely casted to " + getVarName(left) + ".",
              CWE758, inconclusive);
}