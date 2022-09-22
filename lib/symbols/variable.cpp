//---------------------------------------------------------------------------
#include "variable.h"

#include "astutils.h"
#include <cassert>

//---------------------------------------------------------------------------

Variable::~Variable()
{
    delete mValueType;
}

//---------------------------------------------------------------------------

const Token *Variable::declEndToken() const
{
    Token const *declEnd = typeStartToken();
    while (declEnd && !Token::Match(declEnd, "[;,)={]"))
    {
        if (declEnd->link() && Token::Match(declEnd, "(|["))
            declEnd = declEnd->link();
        declEnd = declEnd->next();
    }
    return declEnd;
}

//---------------------------------------------------------------------------

void Variable::evaluate(const Settings *settings)
{
    const Library *const lib = settings ? &settings->library : nullptr;

    if (mNameToken)
        setFlag(fIsArray, arrayDimensions(lib));

    if (mTypeStartToken)
        setValueType(ValueType::parseDecl(mTypeStartToken, settings));

    const Token *tok = mTypeStartToken;
    while (tok && tok->previous() && tok->previous()->isName())
        tok = tok->previous();
    const Token *end = mTypeEndToken;
    if (end)
        end = end->next();
    while (tok != end)
    {
        if (tok->str() == "static")
            setFlag(fIsStatic, true);
        else if (tok->str() == "const")
            setFlag(fIsConst, true);
        else if (tok->str() == "&")
        {
            if (isReference())
                setFlag(fIsRValueRef, true);
            setFlag(fIsReference, true);
        }

        if (tok->str() == "<" && tok->link())
//        ds sieht nach vector aus
            tok = tok->link();
        else
            tok = tok->next();
    }

    while (Token::Match(mTypeStartToken, "static|const %any%"))
        mTypeStartToken = mTypeStartToken->next();
    while (mTypeEndToken && mTypeEndToken->previous() && Token::Match(mTypeEndToken, "const"))
        mTypeEndToken = mTypeEndToken->previous();

    if (mTypeStartToken)
    {
        std::string strtype = mTypeStartToken->str();
        for (const Token *typeToken = mTypeStartToken; Token::Match(typeToken, "%type% :: %type%"); typeToken = typeToken->tokAt(2))
            strtype += "::" + typeToken->strAt(2);
        // @todo check oa dyn type here
        setFlag(fIsClass, !lib->podtype(strtype) && !mTypeStartToken->isStandardType() && !isEnumType() && !isReference());
    }
    if (mAccess == Argument)
    {
        tok = mNameToken;
        if (!tok)
        {
            // Argument without name
            tok = mTypeEndToken;
            // back up to start of array dimensions
            while (tok && tok->str() == "]")
                tok = tok->link()->previous();
            // add array dimensions if present
            if (tok && tok->next()->str() == "[")
                setFlag(fIsArray, arrayDimensions(lib));
        }
        if (!tok)
            return;
        tok = tok->next();
        while (tok->str() == "[")
            tok = tok->link();
        setFlag(fHasDefault, tok->str() == "=");
    }
    // check for C++11 member initialization
    if (mScope && mScope->isClassOrStruct())
    {
        // type var = x or
        // type var = {x}
        // type var = x; gets simplified to: type var ; var = x ;
        Token const *declEnd = declEndToken();
        if ((Token::Match(declEnd, "; %name% =") && declEnd->strAt(1) == mNameToken->str()) ||
            Token::Match(declEnd, "=|{"))
            setFlag(fHasDefault, true);
    }

    if (mTypeStartToken)
    {
        if (Token::Match(mTypeStartToken, "float|double"))
            setFlag(fIsFloatType, true);
    }
}

//---------------------------------------------------------------------------
void Variable::setValueType(const ValueType &valueType)
{
  delete mValueType;
  mValueType = new ValueType(valueType);
  setFlag(fIsConst, mValueType->constness != 0);
}

//---------------------------------------------------------------------------
bool Variable::arrayDimensions(const Library *lib)
{

// make happy for vector

    if (Token::Match(mTypeStartToken, "%type%") && mTypeStartToken->str().rfind("dyn_", 0) == 0)
    {
        // const Variable *var = mTypeStartToken->variable();
        // return (var && var->valueType()->type >= ValueType::Type::DYN_INT);
        const Token *makeDynTok = declEndToken();
        if (makeDynTok)
        {
            //3*next -> Reason: Declartion will be split
            //More Information: tokenize.cpp LN: 2779
            makeDynTok = makeDynTok->tokAt(3);
        }

        if (makeDynTok && makeDynTok->str().rfind("makeDyn", 0) == 0)
        {
            int argnr = numberOfArguments(makeDynTok);

            Dimension dimension_;
            dimension_.num = argnr;
            dimension_.known = true;
            mDimensions.push_back(dimension_);
        }

        return true;
    }

    return false;
}

//---------------------------------------------------------------------------