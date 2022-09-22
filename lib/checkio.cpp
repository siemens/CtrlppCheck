/*
 * Cppcheck - A tool for static C/C++ code analysis
 * Copyright (C) 2007-2019 Cppcheck team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

//---------------------------------------------------------------------------
#include "checkio.h"

#include "library.h"
#include "mathlib.h"
#include "settings.h"
#include "symbols/symbols.h"
#include "token.h"
#include "tokenize.h"
#include "utils.h"
#include "valueflow.h"

#include <cctype>
#include <cstdlib>
#include <list>
#include <map>
#include <set>
#include <utility>
#include <vector>

//---------------------------------------------------------------------------

// Register CheckIO..
namespace {
    CheckIO instance;
}

// CVE ID used:
static const CWE CWE119(119U);  // Improper Restriction of Operations within the Bounds of a Memory Buffer
static const CWE CWE398(398U);  // Indicator of Poor Code Quality
static const CWE CWE664(664U);  // Improper Control of a Resource Through its Lifetime
static const CWE CWE685(685U);  // Function Call With Incorrect Number of Arguments
static const CWE CWE686(686U);  // Function Call With Incorrect Argument Type
static const CWE CWE687(687U);  // Function Call With Incorrectly Specified Argument Value
static const CWE CWE704(704U);  // Incorrect Type Conversion or Cast
static const CWE CWE910(910U);  // Use of Expired File Descriptor

//---------------------------------------------------------------------------
// fflush(stdin) <- fflush only applies to output streams in ANSI C
// fread(); fwrite(); <- consecutive read/write statements require repositioning in between
// fopen("","r"); fwrite(); <- write to read-only file (or vice versa)
// fclose(); fread(); <- Use closed file
//---------------------------------------------------------------------------
enum OpenMode { CLOSED, READ_MODE, WRITE_MODE, RW_MODE, UNKNOWN_OM };
static OpenMode getMode(const std::string& str)
{
    if (str.find('+', 1) != std::string::npos)
        return RW_MODE;
    // check for 'w' 'a' 'wb' && 'ab'
    else if (str == "w" || str == "a" || str == "wb" || str == "ab" )
        return WRITE_MODE;
    // check for 'r' && 'rb'
    else if (str == "r" || str == "rb" )
        return READ_MODE;
    return UNKNOWN_OM;
}

struct Filepointer {
    OpenMode mode;
    unsigned int mode_indent;
    enum Operation {NONE, UNIMPORTANT, READ, WRITE, POSITIONING, OPEN, CLOSE, UNKNOWN_OP} lastOperation;
    unsigned int op_indent;
    enum AppendMode { UNKNOWN_AM, APPEND, APPEND_EX };
    AppendMode append_mode;
    explicit Filepointer(OpenMode mode_ = UNKNOWN_OM)
        : mode(mode_), mode_indent(0), lastOperation(NONE), op_indent(0), append_mode(UNKNOWN_AM) {
    }
};

namespace {
    //for this functions it doesn't matter what status the file has (no matter on CLOSED, OPEN, ... etc.)
    const std::set<std::string> whitelist = {"feof", "ferror", "ftell" };
}

void CheckIO::checkFileUsage()
{
    const bool printPortability = mSettings->isEnabled(Settings::PORTABILITY);
    const bool printWarnings = mSettings->isEnabled(Settings::WARNING);

    std::map<unsigned int, Filepointer> filepointers;

    const SymbolDatabase* symbolDatabase = mTokenizer->getSymbolDatabase();
    for (const Variable* var : symbolDatabase->variableList()) {
        if (!var || !var->declarationId() || var->isArray() || !Token::simpleMatch(var->typeStartToken(), "file"))
            continue;

        if (var->isLocal()) {
                filepointers.insert(std::make_pair(var->declarationId(), Filepointer(CLOSED)));
        } else {
            filepointers.insert(std::make_pair(var->declarationId(), Filepointer(UNKNOWN_OM)));
            // TODO: If all fopen calls we find open the file in the same type, we can set Filepointer::mode
        }
    }

    for (const Scope * scope : symbolDatabase->functionScopes) {
        unsigned int indent = 0;
        for (const Token *tok = scope->bodyStart; tok != scope->bodyEnd; tok = tok->next()) {
            if (tok->str() == "{")
                indent++;
            else if (tok->str() == "}") {
                indent--;
                for (std::map<unsigned int, Filepointer>::iterator i = filepointers.begin(); i != filepointers.end(); ++i) {
                    if (indent < i->second.mode_indent) {
                        i->second.mode_indent = 0;
                        i->second.mode = UNKNOWN_OM;
                    }
                    if (indent < i->second.op_indent) {
                        i->second.op_indent = 0;
                        i->second.lastOperation = Filepointer::UNKNOWN_OP;
                    }
                }
            } else if (tok->str() == "return" || tok->str() == "continue" || tok->str() == "break" || mSettings->library.isnoreturn(tok)) { // Reset upon return, continue or break
                for (std::map<unsigned int, Filepointer>::iterator i = filepointers.begin(); i != filepointers.end(); ++i) {
                    i->second.mode_indent = 0;
                    i->second.mode = UNKNOWN_OM;
                    i->second.op_indent = 0;
                    i->second.lastOperation = Filepointer::UNKNOWN_OP;
                }
            } else if (Token::Match(tok, "%var% =") && tok->strAt(2) != "fopen" ) {
                std::map<unsigned int, Filepointer>::iterator i = filepointers.find(tok->varId());
                if (i != filepointers.end()) {
                    i->second.mode = UNKNOWN_OM;
                    i->second.lastOperation = Filepointer::UNKNOWN_OP;
                }
            } else if (Token::Match(tok, "%name% (") && tok->previous() && (!tok->previous()->isName() || Token::Match(tok->previous(), "return|throw"))) {
                std::string mode;
                const Token* fileTok = nullptr;
                Filepointer::Operation operation = Filepointer::NONE;

                
                if (tok->str() == "fopen" && tok->strAt(-1) == "=") { //OPEN
                    const Token* modeTok = tok->tokAt(2)->nextArgument();
                    if (modeTok && modeTok->tokType() == Token::eString)
                        mode = modeTok->strValue();
                    
                    fileTok = tok->tokAt(-2);
                    operation = Filepointer::OPEN;
                } else if ((tok->str() == "rewind" || tok->str() == "fseek" || tok->str() == "fflush")) { //POSITIONING
                    fileTok = tok->tokAt(2);
                    if (printPortability && fileTok && tok->str() == "fflush") {
                        if (fileTok->str() == "stdin")
                            fflushOnInputStreamError(tok, fileTok->str());
                        else {
                            const Filepointer& f = filepointers[fileTok->varId()];
                            if (f.mode == READ_MODE)
                                fflushOnInputStreamError(tok, fileTok->str());
                        }
                    }
                    operation = Filepointer::POSITIONING;
                } else if (tok->str() == "fgets" || tok->str() == "fread" || tok->str() == "fscanf") { //READ
                    if (tok->str().find("scanf") != std::string::npos || tok->str() == "fread")
                        fileTok = tok->tokAt(2);
                    else
                        fileTok = tok->linkAt(1)->previous();
                    operation = Filepointer::READ;
                } else if (tok->str() == "fputs" || tok->str() == "fwrite" ||tok->str() == "fprintf") { //WRITE
                    if (tok->str().find("printf") != std::string::npos || tok->str() == "fwrite")
                        fileTok = tok->tokAt(2);
                    else
                        fileTok = tok->linkAt(1)->previous();
                    operation = Filepointer::WRITE;
                } else if (tok->str() == "fclose") { //..CLOSE
                    fileTok = tok->tokAt(2);
                    operation = Filepointer::CLOSE;
                } else if (whitelist.find(tok->str()) != whitelist.end()) { //UNIMPORTANT
                    fileTok = tok->tokAt(2);
                    operation = Filepointer::UNIMPORTANT;
                } else if (!Token::Match(tok, "if|for|while|catch|switch") && !mSettings->library.isFunctionConst(tok->str(), true)) {
                    const Token* const end2 = tok->linkAt(1);
                    if (scope->functionOf && scope->functionOf->isClassOrStruct() && !scope->function->isStatic() && ((tok->strAt(-1) != "::" && tok->strAt(-1) != ".") || tok->strAt(-2) == "this")) {
                        if (!tok->function() || (tok->function()->nestedIn && tok->function()->nestedIn->isClassOrStruct())) {
                            for (std::map<unsigned int, Filepointer>::iterator i = filepointers.begin(); i != filepointers.end(); ++i) {
                                const Variable* var = symbolDatabase->getVariableFromVarId(i->first);
                                if (!var || !(var->isLocal() || var->isGlobal() || var->isStatic())) {
                                    i->second.mode = UNKNOWN_OM;
                                    i->second.mode_indent = 0;
                                    i->second.op_indent = indent;
                                    i->second.lastOperation = Filepointer::UNKNOWN_OP;
                                }
                            }
                            continue;
                        }
                    }
                    for (const Token* tok2 = tok->tokAt(2); tok2 != end2; tok2 = tok2->next()) {
                        if (tok2->varId() && filepointers.find(tok2->varId()) != filepointers.end()) {
                            fileTok = tok2;
                            operation = Filepointer::UNKNOWN_OP; // Assume that repositioning was last operation and that the file is opened now
                            break;
                        }
                    }
                }

                while (Token::Match(fileTok, "%name% ."))
                    fileTok = fileTok->tokAt(2);

                if (!fileTok || !fileTok->varId() || fileTok->strAt(1) == "[")
                    continue;

                if (filepointers.find(fileTok->varId()) == filepointers.end()) { // function call indicates: Its a File
                    filepointers.insert(std::make_pair(fileTok->varId(), Filepointer(UNKNOWN_OM)));
                }
                Filepointer& f = filepointers[fileTok->varId()];

                switch (operation) {
                case Filepointer::OPEN:
                    f.mode = getMode(mode);
                    if (mode.find('a') != std::string::npos) {
                        if (f.mode == RW_MODE)
                            f.append_mode = Filepointer::APPEND_EX;
                        else
                            f.append_mode = Filepointer::APPEND;
                    } else
                        f.append_mode = Filepointer::UNKNOWN_AM;
                    f.mode_indent = indent;
                    break;
                case Filepointer::POSITIONING:
                    if (f.mode == CLOSED)
                        useClosedFileError(tok);
                    else if (f.append_mode == Filepointer::APPEND && tok->str() != "fflush" && printWarnings)
                        seekOnAppendedFileError(tok);
                    break;
                case Filepointer::READ:
                    if (f.mode == CLOSED)
                        useClosedFileError(tok);
                    else if (f.mode == WRITE_MODE)
                        readWriteOnlyFileError(tok);
                    else if (f.lastOperation == Filepointer::WRITE)
                        ioWithoutPositioningError(tok);
                    break;
                case Filepointer::WRITE:
                    if (f.mode == CLOSED)
                        useClosedFileError(tok);
                    else if (f.mode == READ_MODE)
                        writeReadOnlyFileError(tok);
                    else if (f.lastOperation == Filepointer::READ)
                        ioWithoutPositioningError(tok);
                    break;
                case Filepointer::CLOSE:
                    if (f.mode == CLOSED)
                        useClosedFileError(tok);
                    else
                        f.mode = CLOSED;
                    f.mode_indent = indent;
                    break;
                case Filepointer::UNIMPORTANT:
                    if (f.mode == CLOSED)
                        useClosedFileError(tok);
                    break;
                case Filepointer::UNKNOWN_OP:
                    f.mode = UNKNOWN_OM;
                    f.mode_indent = 0;
                    break;
                default:
                    break;
                }
                if (operation != Filepointer::NONE && operation != Filepointer::UNIMPORTANT) {
                    f.op_indent = indent;
                    f.lastOperation = operation;
                }
            }
        }
        for (std::map<unsigned int, Filepointer>::iterator i = filepointers.begin(); i != filepointers.end(); ++i) {
            i->second.op_indent = 0;
            i->second.mode = UNKNOWN_OM;
            i->second.lastOperation = Filepointer::UNKNOWN_OP;
        }
    }
}

void CheckIO::fflushOnInputStreamError(const Token *tok, const std::string &varname)
{
    reportError(tok, Severity::portability,
                "fflushOnInputStream", "fflush() called on input stream '" + varname + "' may result in undefined behaviour on non-linux systems.", CWE398, false);
}

void CheckIO::ioWithoutPositioningError(const Token *tok)
{
    reportError(tok, Severity::error,
                "IOWithoutPositioning", "Read and write operations without a call to a positioning function (fseek or rewind) or fflush in between result in undefined behaviour.", CWE664, false);
}

void CheckIO::readWriteOnlyFileError(const Token *tok)
{
    reportError(tok, Severity::error,
                "readWriteOnlyFile", "Read operation on a file that was opened only for writing.", CWE664, false);
}

void CheckIO::writeReadOnlyFileError(const Token *tok)
{
    reportError(tok, Severity::error,
                "writeReadOnlyFile", "Write operation on a file that was opened only for reading.", CWE664, false);
}

void CheckIO::useClosedFileError(const Token *tok)
{
    reportError(tok, Severity::error,
                "useClosedFile", "Used file that is not opened.", CWE910, false);
}

void CheckIO::seekOnAppendedFileError(const Token *tok)
{
    reportError(tok, Severity::warning,
                "seekOnAppendedFile", "Repositioning operation performed on a file opened in append mode has no effect.", CWE398, false);
}

//---------------------------------------------------------------------------
//    printf("%u", "xyz"); // Wrong argument type
//    printf("%u%s", 1); // Too few arguments
//    printf("", 1); // Too much arguments
//---------------------------------------------------------------------------

static bool findFormat(unsigned int arg, const Token *firstArg,
                       const Token **formatStringTok, const Token **formatArgTok)
{
    const Token* argTok = firstArg;

    for (unsigned int i = 0; i < arg && argTok; ++i)
        argTok = argTok->nextArgument();

    if (Token::Match(argTok, "%str% [,)]")) {
        *formatArgTok = argTok->nextArgument();
        *formatStringTok = argTok;
        return true;
    } else if (Token::Match(argTok, "%var% [,)]") &&
               argTok->variable() &&
               Token::Match(argTok->variable()->typeStartToken(), "char") &&
               ((argTok->variable()->dimensions().size() == 1 &&
                 argTok->variable()->dimensionKnown(0) &&
                 argTok->variable()->dimension(0) != 0))) {
        *formatArgTok = argTok->nextArgument();
        if (!argTok->values().empty()) {
            std::list<ValueFlow::Value>::const_iterator value = std::find_if(
                        argTok->values().begin(), argTok->values().end(), std::mem_fn(&ValueFlow::Value::isTokValue));
            if (value != argTok->values().end() && value->isTokValue() && value->tokvalue &&
                value->tokvalue->tokType() == Token::eString) {
                *formatStringTok = value->tokvalue;
            }
        }
        return true;
    }
    return false;
}


void CheckIO::checkWrongPrintfScanfArguments()
{
    const SymbolDatabase *symbolDatabase = mTokenizer->getSymbolDatabase();
    const bool isWindows = mSettings->isWindowsPlatform();

    for (const Scope * scope : symbolDatabase->functionScopes) {
        for (const Token *tok = scope->bodyStart->next(); tok != scope->bodyEnd; tok = tok->next()) {
            if (!tok->isName()) continue;

            const Token* argListTok = nullptr; // Points to first va_list argument
            const Token* formatStringTok = nullptr; // Points to format string token

            bool scan = false;
            int formatStringArgNo = -1;

            if (tok->strAt(1) == "(" && mSettings->library.formatstr_function(tok)) {
                formatStringArgNo = mSettings->library.formatstr_argno(tok);
                scan = mSettings->library.formatstr_scan(tok);
            }

            if (formatStringArgNo >= 0) {
                // formatstring found in library. Find format string and first argument belonging to format string.
                if (!findFormat(static_cast<unsigned int>(formatStringArgNo), tok->tokAt(2), &formatStringTok, &argListTok))
                    continue;
            } else {
                continue;
            }

            if (!formatStringTok)
                continue;

            checkFormatString(tok, formatStringTok, argListTok, scan);
        }
    }
}

void CheckIO::checkFormatString(const Token * const tok,
                                const Token * const formatStringTok,
                                const Token *       argListTok,
                                const bool scan)
{
    const bool printWarning = mSettings->isEnabled(Settings::WARNING);
    const std::string &formatString = formatStringTok->str();

    // Count format string parameters..
    unsigned int numFormat = 0;
    bool percent = false;
    const Token* argListTok2 = argListTok;
    std::set<unsigned int> parameterPositionsUsed;
    for (std::string::const_iterator i = formatString.begin(); i != formatString.end(); ++i) {
        if (*i == '%') {
            percent = !percent;
        } else if (percent && *i == '[') {
            while (i != formatString.end()) {
                if (*i == ']') {
                    numFormat++;
                    if (argListTok)
                        argListTok = argListTok->nextArgument();
                    percent = false;
                    break;
                }
                ++i;
            }
            if (i == formatString.end())
                break;
        } else if (percent) {
            percent = false;

            bool _continue = false;
            bool skip = false;
            std::string width;
            unsigned int parameterPosition = 0;

            while (i != formatString.end() && *i != '[' && !std::isalpha((unsigned char)*i)) {
                if (*i == '*') {
                    skip = true;
                    if (scan)
                        _continue = true;
                    else {
                        numFormat++;
                        if (argListTok)
                            argListTok = argListTok->nextArgument();
                    }
                } else if (std::isdigit(*i)) {
                    width += *i;
                }

                ++i;
            }

            if (i != formatString.end() && *i == '[') {
                while (i != formatString.end()) {
                    if (*i == ']') {
                        if (!skip) {
                            numFormat++;
                            if (argListTok)
                                argListTok = argListTok->nextArgument();
                        }
                        break;
                    }
                    ++i;
                }
                _continue = true;
            }
            if (i == formatString.end())
                break;
            if (_continue)
                continue;

            if (scan || *i != 'm') { // %m is a non-standard extension that requires no parameter on print functions.
                ++numFormat;

                // Perform type checks
                ArgumentInfo argInfo(argListTok, mSettings);

                if (argInfo.typeToken && !argInfo.isLibraryType(mSettings)) {
                    if (scan) {
                        std::string specifier;
                        bool done = false;
                        while (!done) {
                            switch (*i) {
                            case 's':
                                specifier += *i;
                                if (argInfo.variableInfo && argInfo.isKnownType() && argInfo.variableInfo->isArray() && (argInfo.variableInfo->dimensions().size() == 1) && argInfo.variableInfo->dimensions()[0].known) {
                                    if (!width.empty()) {
                                        const int numWidth = std::atoi(width.c_str());
                                        if (numWidth != (argInfo.variableInfo->dimension(0) - 1))
                                            invalidScanfFormatWidthError(tok, numFormat, numWidth, argInfo.variableInfo, 's');
                                    }
                                }
                                if (argListTok && argListTok->tokType() != Token::eString &&
                                    argInfo.isKnownType() &&
                                    (!Token::Match(argInfo.typeToken, "string|anytype|mixed") ||
                                     argInfo.typeToken->strAt(-1) == "const")) {
                                    if (!(argInfo.element && !argInfo.typeToken->isStandardType()))
                                        invalidScanfArgTypeError_s(tok, numFormat, specifier, &argInfo);
                                }
                                done = true;
                                break;
                            case 'c':
                                if (argInfo.variableInfo && argInfo.isKnownType() && argInfo.variableInfo->isArray() && (argInfo.variableInfo->dimensions().size() == 1) && argInfo.variableInfo->dimensions()[0].known) {
                                    if (!width.empty()) {
                                        const int numWidth = std::atoi(width.c_str());
                                        if (numWidth > argInfo.variableInfo->dimension(0))
                                            invalidScanfFormatWidthError(tok, numFormat, numWidth, argInfo.variableInfo, 'c');
                                    }
                                }
                                done = true;
                                break;
                            case 'x':
                            case 'X':
                            case 'u':
                            case 'o':
                            case 'n':
                            case 'd':
                            case 'i':
                                specifier += *i;
                                if (argInfo.typeToken->tokType() == Token::eString)
                                    invalidScanfArgTypeError_int(tok, numFormat, specifier, &argInfo);
                                else if (argInfo.isKnownType()) {
                                    if (!Token::Match(argInfo.typeToken, "char|short|int|long|anytype|mixed")) {
                                        if (argInfo.typeToken->isStandardType() || !argInfo.element)
                                            invalidScanfArgTypeError_int(tok, numFormat, specifier, &argInfo);
                                    } else if (argInfo.typeToken->isUnsigned() ||
                                               argInfo.typeToken->strAt(-1) == "const") {
                                        invalidScanfArgTypeError_int(tok, numFormat, specifier, &argInfo);
                                    } else {
                                        switch (specifier[0]) {
                                        case 'l':
                                            if (argInfo.typeToken->str() != "long" || argInfo.typeToken->isLong())
                                                invalidScanfArgTypeError_int(tok, numFormat, specifier, &argInfo);
                                            break;
                                        default:
                                            if (argInfo.typeToken->str() != "int")
                                                invalidScanfArgTypeError_int(tok, numFormat, specifier, &argInfo);
                                            break;
                                        }
                                    }
                                }
                                done = true;
                                break;
                            case 'e':
                            case 'E':
                            case 'f':
                            case 'g':
                            case 'G':
                            case 'a':
                                specifier += *i;
                                if (argInfo.typeToken->tokType() == Token::eString)
                                    invalidScanfArgTypeError_float(tok, numFormat, specifier, &argInfo);
                                else if (argInfo.isKnownType()) {
                                    if (!Token::Match(argInfo.typeToken, "float|anytype|mixed")) {
                                        if (argInfo.typeToken->isStandardType())
                                            invalidScanfArgTypeError_float(tok, numFormat, specifier, &argInfo);
                                    } else if (argInfo.typeToken->strAt(-1) == "const") {
                                        invalidScanfArgTypeError_float(tok, numFormat, specifier, &argInfo);
                                    } else if (argInfo.typeToken->str() != "float")
                                                invalidScanfArgTypeError_float(tok, numFormat, specifier, &argInfo);
                                }
                                done = true;
                                break;
                            case 'l':
                                if (i+1 != formatString.end() && *(i+1) == *i)
                                    specifier += *i++;
                                // Expect an alphabetical character after these specifiers
                                if (i != formatString.end() && !isalpha(*(i+1))) {
                                    specifier += *i;
                                    invalidLengthModifierError(tok, numFormat, specifier);
                                    done = true;
                                } else {
                                    specifier += *i++;
                                }
                                break;
                            default:
                                done = true;
                                break;
                            }
                        }
                    } else if (printWarning) {
                        std::string specifier;
                        bool done = false;
                        while (!done) {
                            switch (*i) {
                            case 's':
                                if (argListTok->tokType() != Token::eString &&
                                    argInfo.isKnownType()) {
                                    if (!Token::Match(argInfo.typeToken, "string")) {
                                        if (!argInfo.element)
                                            invalidPrintfArgTypeError_s(tok, numFormat, &argInfo);
                                    }
                                }
                                done = true;
                                break;
                            case 'c':
                            case 'x':
                            case 'X':
                            case 'o':
                                specifier += *i;
                                if (argInfo.typeToken->tokType() == Token::eString)
                                    invalidPrintfArgTypeError_uint(tok, numFormat, specifier, &argInfo);
                                else if (argInfo.isKnownType()) {
                                    if (!Token::Match(argInfo.typeToken, "bool|short|long|int|char")) {
                                        if (!argInfo.element)
                                            invalidPrintfArgTypeError_uint(tok, numFormat, specifier, &argInfo);
                                    } else {
                                        switch (specifier[0]) {
                                        case 'l':
                                            if (argInfo.typeToken->str() != "long" || argInfo.typeToken->isLong())
                                                invalidPrintfArgTypeError_uint(tok, numFormat, specifier, &argInfo);
                                            break;
                                        default:
                                            if (!Token::Match(argInfo.typeToken, "bool|char|short|int"))
                                                invalidPrintfArgTypeError_uint(tok, numFormat, specifier, &argInfo);
                                            break;
                                        }
                                    }
                                }
                                done = true;
                                break;
                            case 'd':
                            case 'i':
                                specifier += *i;
                                if (argInfo.typeToken->tokType() == Token::eString) {
                                    invalidPrintfArgTypeError_sint(tok, numFormat, specifier, &argInfo);
                                } else if (argInfo.isKnownType()) {
                                    if (argInfo.typeToken->isUnsigned() && !Token::Match(argInfo.typeToken, "char|short")) {
                                        if (!argInfo.element)
                                            invalidPrintfArgTypeError_sint(tok, numFormat, specifier, &argInfo);
                                    } else if (!Token::Match(argInfo.typeToken, "bool|char|short|int|long")) {
                                        if (!argInfo.element)
                                            invalidPrintfArgTypeError_sint(tok, numFormat, specifier, &argInfo);
                                    } else {
                                        switch (specifier[0]) {
                                        case 'l':
                                            if (argInfo.typeToken->str() != "long" || argInfo.typeToken->isLong())
                                                invalidPrintfArgTypeError_sint(tok, numFormat, specifier, &argInfo);
                                            break;
                                        default:
                                            if (!Token::Match(argInfo.typeToken, "bool|char|short|int"))
                                                invalidPrintfArgTypeError_sint(tok, numFormat, specifier, &argInfo);
                                            break;
                                        }
                                    }
                                }
                                done = true;
                                break;
                            case 'u':
                                specifier += *i;
                                if (argInfo.typeToken->tokType() == Token::eString) {
                                    invalidPrintfArgTypeError_uint(tok, numFormat, specifier, &argInfo);
                                } else if (argInfo.isKnownType()) {
                                    if (!argInfo.typeToken->isUnsigned() && !Token::Match(argInfo.typeToken, "bool")) {
                                        if (!argInfo.element)
                                            invalidPrintfArgTypeError_uint(tok, numFormat, specifier, &argInfo);
                                    } else if (!Token::Match(argInfo.typeToken, "bool|char|short|long|int")) {
                                        if (!argInfo.element)
                                            invalidPrintfArgTypeError_uint(tok, numFormat, specifier, &argInfo);
                                    } else {
                                        switch (specifier[0]) {
                                        case 'l':
                                            if (argInfo.typeToken->str() != "long" || argInfo.typeToken->isLong())
                                                invalidPrintfArgTypeError_uint(tok, numFormat, specifier, &argInfo);
                                            break;
                                        default:
                                            if (!Token::Match(argInfo.typeToken, "bool|char|short|int"))
                                                invalidPrintfArgTypeError_uint(tok, numFormat, specifier, &argInfo);
                                            break;
                                        }
                                    }
                                }
                                done = true;
                                break;

                            case 'e':
                            case 'E':
                            case 'f':
                            case 'g':
                            case 'G':
                                specifier += *i;
                                if (argInfo.typeToken->tokType() == Token::eString)
                                    invalidPrintfArgTypeError_float(tok, numFormat, specifier, &argInfo);
                                else if (argInfo.isKnownType()) {
                                    if (!Token::Match(argInfo.typeToken, "float")) {
                                        if (!argInfo.element)
                                            invalidPrintfArgTypeError_float(tok, numFormat, specifier, &argInfo);
                                    }
                                }
                                done = true;
                                break;
                            case 'l': { // Can be 'll' (long long int or unsigned long long int) or 'l' (long int or unsigned long int)
                                // If the next character is the same (which makes 'hh' or 'll') then expect another alphabetical character
                                const bool isSecondCharAvailable = ((i + 1) != formatString.end());
                                if (i != formatString.end() && isSecondCharAvailable && *(i + 1) == *i) {
                                    if (!isalpha(*(i + 2))) {
                                        std::string modifier;
                                        modifier += *i;
                                        modifier += *(i + 1);
                                        invalidLengthModifierError(tok, numFormat, modifier);
                                        done = true;
                                    } else {
                                        specifier = *i++;
                                        specifier += *i++;
                                    }
                                } else {
                                    if (i != formatString.end()) {
                                        if ((i + 1) != formatString.end() && !isalpha(*(i + 1))) {
                                            std::string modifier;
                                            modifier += *i;
                                            invalidLengthModifierError(tok, numFormat, modifier);
                                            done = true;
                                        } else {
                                            specifier = *i++;
                                        }
                                    } else {
                                        done = true;
                                    }
                                }
                            }
                            break;
                            default:
                                done = true;
                                break;
                            }
                        }
                    }
                }

                if (argListTok)
                    argListTok = argListTok->nextArgument(); // Find next argument
            }
        }
    }

    // Count printf/scanf parameters..
    unsigned int numFunction = 0;
    while (argListTok2) {
        numFunction++;
        argListTok2 = argListTok2->nextArgument(); // Find next argument
    }

    if (printWarning) {
        // Check that all parameter positions reference an actual parameter
        for (unsigned int i : parameterPositionsUsed) {
            if ((i == 0) || (i > numFormat))
                wrongPrintfScanfPosixParameterPositionError(tok, tok->str(), i, numFormat);
        }
    }

    // Mismatching number of parameters => warning
    if (numFormat != numFunction)
        wrongPrintfScanfArgumentsError(tok, tok->originalName().empty() ? tok->str() : tok->originalName(), numFormat, numFunction);
}

// We currently only support string literals, variables, and functions.
/// @todo add non-string literals, and generic expressions

CheckIO::ArgumentInfo::ArgumentInfo(const Token * arg, const Settings *settings)
    : variableInfo(nullptr)
    , typeToken(nullptr)
    , functionInfo(nullptr)
    , tempToken(nullptr)
    , element(false)
    , _template(false)
    , address(false)
{
    if (!arg)
        return;

    // Use AST type info
    // TODO: This is a bailout so that old code is used in simple cases. Remove the old code and always use the AST type.
    if (!Token::Match(arg, "%str% ,|)") && !(Token::Match(arg,"%var%") && arg->variable() && arg->variable()->isArray())) {
        const Token *top = arg;
        while (top->astParent() && top->astParent()->str() != "," && top->astParent() != arg->previous())
            top = top->astParent();
        const ValueType *valuetype = top->argumentType();
        if (valuetype && valuetype->type >= ValueType::Type::BOOL) {
            typeToken = tempToken = new Token();

            tempToken->str(valuetype->typeToString());

            if (!valuetype->originalTypeName.empty())
                tempToken->originalName(valuetype->originalTypeName);
            
            tempToken = const_cast<Token*>(typeToken);
            return;
        }
    }


    if (arg->tokType() == Token::eString) {
        typeToken = arg;
        return;
    } else if (arg->tokType() == Token::eVariable || arg->tokType() == Token::eFunction) {

        const Token *varTok = nullptr;
        const Token *tok1 = arg->next();
        for (; tok1; tok1 = tok1->next()) {
            if (tok1->str() == "," || tok1->str() == ")") {
                if (tok1->previous()->str() == "]") {
                    varTok = tok1->linkAt(-1)->previous();
                    if (varTok->str() == ")" && varTok->link()->previous()->tokType() == Token::eFunction) {
                        const Function * function = varTok->link()->previous()->function();
                        if (function && function->retType && function->retType->isEnumType()) {
                            if (function->retType->classScope->enumType)
                                typeToken = function->retType->classScope->enumType;
                            else {
                                tempToken = new Token();
                                tempToken->fileIndex(tok1->fileIndex());
                                tempToken->linenr(tok1->linenr());
                                tempToken->str("int");
                                typeToken = tempToken;
                            }
                        } else if (function && function->retDef) {
                            typeToken = function->retDef;
                            while (typeToken->str() == "const" || typeToken->str() == "extern")
                                typeToken = typeToken->next();
                            functionInfo = function;
                            element = true;
                        }
                        return;
                    }
                } else if (tok1->previous()->str() == ")" && tok1->linkAt(-1)->previous()->tokType() == Token::eFunction) {
                    const Function * function = tok1->linkAt(-1)->previous()->function();
                    if (function && function->retType && function->retType->isEnumType()) {
                        if (function->retType->classScope->enumType)
                            typeToken = function->retType->classScope->enumType;
                        else {
                            tempToken = new Token();
                            tempToken->fileIndex(tok1->fileIndex());
                            tempToken->linenr(tok1->linenr());
                            tempToken->str("int");
                            typeToken = tempToken;
                        }
                    } else if (function && function->retDef) {
                        typeToken = function->retDef;
                        while (typeToken->str() == "const" || typeToken->str() == "extern")
                            typeToken = typeToken->next();
                        functionInfo = function;
                        element = false;
                    }
                    return;
                } else
                    varTok = tok1->previous();
                break;
            } else if (tok1->str() == "(" || tok1->str() == "{" || tok1->str() == "[")
                tok1 = tok1->link();
            // check for vector.at() and string.at()
            else if (Token::Match(tok1->previous(), "%var% . at (") &&
                     Token::Match(tok1->linkAt(2), ") [,)]")) {
                varTok = tok1->previous();
                variableInfo = varTok->variable();

                if (!variableInfo || !isStdVectorOrString()) {
                    variableInfo = nullptr;
                    typeToken = nullptr;
                }

                return;
            } else if (!(tok1->str() == "." || tok1->tokType() == Token::eVariable || tok1->tokType() == Token::eFunction))
                return;
        }

        if (varTok) {
            variableInfo = varTok->variable();
            element = tok1->previous()->str() == "]";

            // look for std::vector operator [] and use template type as return type
            if (variableInfo) {
                if (element && isStdVectorOrString()) { // isStdVectorOrString sets type token if true
                    element = false;    // not really an array element
                } else if (variableInfo->isEnumType()) {
                    if (variableInfo->type() && variableInfo->type()->classScope && variableInfo->type()->classScope->enumType)
                        typeToken = variableInfo->type()->classScope->enumType;
                    else {
                        tempToken = new Token();
                        tempToken->fileIndex(tok1->fileIndex());
                        tempToken->linenr(tok1->linenr());
                        tempToken->str("int");
                        typeToken = tempToken;
                    }
                } else
                    typeToken = variableInfo->typeStartToken();
            }

            return;
        }
    }
}

CheckIO::ArgumentInfo::~ArgumentInfo()
{
    if (tempToken) {
        while (tempToken->next())
            tempToken->deleteNext();

        delete tempToken;
    }
}

bool CheckIO::ArgumentInfo::isStdVectorOrString()
{
    if (variableInfo->type() && !variableInfo->type()->derivedFrom.empty()) {
        const std::vector<Type::BaseInfo>& derivedFrom = variableInfo->type()->derivedFrom;
        for (std::size_t i = 0, size = derivedFrom.size(); i < size; ++i) {
            const Token* nameTok = derivedFrom[i].nameTok;
            if (Token::Match(nameTok, "vector <")) {
                typeToken = nameTok->tokAt(2);
                //@todo I think we can remove it for ctrl code
                _template = true;
                return true;
            } else if (Token::Match(nameTok, "string")) {
                return true;
            }
        }
    } else if (variableInfo->type()) {
        const Scope * classScope = variableInfo->type()->classScope;
        if (classScope) {
            for (const Function &func : classScope->functionList) {
                if (func.name() == "operator[]") {
                    if (Token::Match(func.retDef, "%type% &")) {
                        typeToken = func.retDef;
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

bool CheckIO::ArgumentInfo::isArrayOrPointer() const
{
    if (address)
        return true;
    else {
        const Token *tok = typeToken;
        while (Token::Match(tok, "const|struct"))
            tok = tok->next();
        if (tok && tok->strAt(1) == "*")
            return true;
    }
    return false;
}

bool CheckIO::ArgumentInfo::isComplexType() const
{
    if (variableInfo->type())
        return (true);

    const Token* varTypeTok = typeToken;

    return ((variableInfo->isStlStringType() || (varTypeTok->strAt(1) == "<" && varTypeTok->linkAt(1) && varTypeTok->linkAt(1)->strAt(1) != "::")));
}

bool CheckIO::ArgumentInfo::isKnownType() const
{
    if (variableInfo)
        return (typeToken->isStandardType() || typeToken->next()->isStandardType() || isComplexType());
    else if (functionInfo)
        return (typeToken->isStandardType() || functionInfo->retType || Token::Match(typeToken, "string"));

    return typeToken->isStandardType() || Token::Match(typeToken, "string");
}

bool CheckIO::ArgumentInfo::isLibraryType(const Settings *settings) const
{
    return typeToken && typeToken->isStandardType() && settings->library.podtype(typeToken->str());
}

void CheckIO::wrongPrintfScanfArgumentsError(const Token* tok,
        const std::string &functionName,
        unsigned int numFormat,
        unsigned int numFunction)
{
    const Severity::SeverityType severity = numFormat > numFunction ? Severity::error : Severity::warning;
    if (severity != Severity::error && !mSettings->isEnabled(Settings::WARNING))
        return;

    std::ostringstream errmsg;
    errmsg << functionName
           << " format string requires "
           << numFormat
           << " parameter" << (numFormat != 1 ? "s" : "") << " but "
           << (numFormat > numFunction ? "only " : "")
           << numFunction
           << (numFunction != 1 ? " are" : " is")
           << " given.";

    reportError(tok, severity, "wrongPrintfScanfArgNum", errmsg.str(), CWE685, false);
}

void CheckIO::wrongPrintfScanfPosixParameterPositionError(const Token* tok, const std::string& functionName,
        unsigned int index, unsigned int numFunction)
{
    if (!mSettings->isEnabled(Settings::WARNING))
        return;
    std::ostringstream errmsg;
    errmsg << functionName << ": ";
    if (index == 0) {
        errmsg << "parameter positions start at 1, not 0";
    } else {
        errmsg << "referencing parameter " << index << " while " << numFunction << " arguments given";
    }
    reportError(tok, Severity::warning, "wrongPrintfScanfParameterPositionError", errmsg.str(), CWE685, false);
}

void CheckIO::invalidScanfArgTypeError_s(const Token* tok, unsigned int numFormat, const std::string& specifier, const ArgumentInfo* argInfo)
{
    const Severity::SeverityType severity = getSeverity(argInfo);
    if (!mSettings->isEnabled(severity))
        return;
    std::ostringstream errmsg;
    errmsg << "%" << specifier << " in format string (no. " << numFormat << ") requires a \'";
    if (specifier[0] == 's')
        errmsg << "string";
    errmsg << "\' but the argument type is ";
    argumentType(errmsg, argInfo);
    errmsg << ".";
    reportError(tok, severity, "invalidScanfArgType_s", errmsg.str(), CWE686, false);
}
void CheckIO::invalidScanfArgTypeError_int(const Token* tok, unsigned int numFormat, const std::string& specifier, const ArgumentInfo* argInfo)
{
    const Severity::SeverityType severity = getSeverity(argInfo);
    if (!mSettings->isEnabled(severity))
        return;
    std::ostringstream errmsg;
    errmsg << "%" << specifier << " in format string (no. " << numFormat << ") requires \'";
    if (specifier[0] == 'h') {
        if (specifier[1] == 'h')
            errmsg << "char";
        else
            errmsg << "short";
    } else if (specifier[0] == 'l') {
        errmsg << "long";
    } else {
        errmsg << "int";
    }
    errmsg << "\' but the argument type is ";
    argumentType(errmsg, argInfo);
    errmsg << ".";
    reportError(tok, severity, "invalidScanfArgType_int", errmsg.str(), CWE686, false);
}
void CheckIO::invalidScanfArgTypeError_float(const Token* tok, unsigned int numFormat, const std::string& specifier, const ArgumentInfo* argInfo)
{
    const Severity::SeverityType severity = getSeverity(argInfo);
    if (!mSettings->isEnabled(severity))
        return;
    std::ostringstream errmsg;
    errmsg << "%" << specifier << " in format string (no. " << numFormat << ") requires \'";
    errmsg << "float";
    errmsg << "\' but the argument type is ";
    argumentType(errmsg, argInfo);
    errmsg << ".";
    reportError(tok, severity, "invalidScanfArgType_float", errmsg.str(), CWE686, false);
}

void CheckIO::invalidPrintfArgTypeError_s(const Token* tok, unsigned int numFormat, const ArgumentInfo* argInfo)
{
    const Severity::SeverityType severity = getSeverity(argInfo);
    if (!mSettings->isEnabled(severity))
        return;
    std::ostringstream errmsg;
    errmsg << "%s in format string (no. " << numFormat << ") requires \'string\' but the argument type is ";
    argumentType(errmsg, argInfo);
    errmsg << ".";
    reportError(tok, severity, "invalidPrintfArgType_s", errmsg.str(), CWE686, false);
}
void CheckIO::invalidPrintfArgTypeError_n(const Token* tok, unsigned int numFormat, const ArgumentInfo* argInfo)
{
    const Severity::SeverityType severity = getSeverity(argInfo);
    if (!mSettings->isEnabled(severity))
        return;
    std::ostringstream errmsg;
    errmsg << "%n in format string (no. " << numFormat << ") requires \'int *\' but the argument type is ";
    argumentType(errmsg, argInfo);
    errmsg << ".";
    reportError(tok, severity, "invalidPrintfArgType_n", errmsg.str(), CWE686, false);
}
void CheckIO::invalidPrintfArgTypeError_p(const Token* tok, unsigned int numFormat, const ArgumentInfo* argInfo)
{
    const Severity::SeverityType severity = getSeverity(argInfo);
    if (!mSettings->isEnabled(severity))
        return;
    std::ostringstream errmsg;
    errmsg << "%p in format string (no. " << numFormat << ") requires an address but the argument type is ";
    argumentType(errmsg, argInfo);
    errmsg << ".";
    reportError(tok, severity, "invalidPrintfArgType_p", errmsg.str(), CWE686, false);
}
static void printfFormatType(std::ostream& os, const std::string& specifier, bool isUnsigned)
{
    os << "\'";
    if (specifier[0] == 'l') {
        os << "long";
    } else if (specifier[0] == 'u' || specifier[1] == 'u') {
        os << "unsigned";
    } else {
        os << "int";
    }
    os << "\'";
}

void CheckIO::invalidPrintfArgTypeError_uint(const Token* tok, unsigned int numFormat, const std::string& specifier, const ArgumentInfo* argInfo)
{
    const Severity::SeverityType severity = getSeverity(argInfo);
    if (!mSettings->isEnabled(severity))
        return;
    std::ostringstream errmsg;
    errmsg << "%" << specifier << " in format string (no. " << numFormat << ") requires ";
    printfFormatType(errmsg, specifier, true);
    errmsg << " but the argument type is ";
    argumentType(errmsg, argInfo);
    errmsg << ".";
    reportError(tok, severity, "invalidPrintfArgType_uint", errmsg.str(), CWE686, false);
}

void CheckIO::invalidPrintfArgTypeError_sint(const Token* tok, unsigned int numFormat, const std::string& specifier, const ArgumentInfo* argInfo)
{
    const Severity::SeverityType severity = getSeverity(argInfo);
    if (!mSettings->isEnabled(severity))
        return;
    std::ostringstream errmsg;
    errmsg << "%" << specifier << " in format string (no. " << numFormat << ") requires ";
    printfFormatType(errmsg, specifier, false);
    errmsg << " but the argument type is ";
    argumentType(errmsg, argInfo);
    errmsg << ".";
    reportError(tok, severity, "invalidPrintfArgType_sint", errmsg.str(), CWE686, false);
}
void CheckIO::invalidPrintfArgTypeError_float(const Token* tok, unsigned int numFormat, const std::string& specifier, const ArgumentInfo* argInfo)
{
    const Severity::SeverityType severity = getSeverity(argInfo);
    if (!mSettings->isEnabled(severity))
        return;
    std::ostringstream errmsg;
    errmsg << "%" << specifier << " in format string (no. " << numFormat << ") requires \'";
    errmsg << "float\' but the argument type is ";
    argumentType(errmsg, argInfo);
    errmsg << ".";
    reportError(tok, severity, "invalidPrintfArgType_float", errmsg.str(), CWE686, false);
}

Severity::SeverityType CheckIO::getSeverity(const CheckIO::ArgumentInfo *argInfo)
{
    return (argInfo && argInfo->typeToken && !argInfo->typeToken->originalName().empty()) ? Severity::portability : Severity::warning;
}

void CheckIO::argumentType(std::ostream& os, const ArgumentInfo * argInfo)
{
    if (argInfo) {
        os << "\'";
        const Token *type = argInfo->typeToken;
        if (type->tokType() == Token::eString) {
            os << "const char *";
        } else {
            if (type->originalName().empty()) {
                if (type->strAt(-1) == "const")
                    os << "const ";
                while (Token::Match(type, "const|struct")) {
                    os << type->str() << " ";
                    type = type->next();
                }
                while (Token::Match(type, "%any% ::")) {
                    os << type->str() << "::";
                    type = type->tokAt(2);
                }
                type->stringify(os, false, true, false);
                if (type->strAt(1) == "*" && !argInfo->element)
                    os << " *";
                else if (argInfo->variableInfo && !argInfo->element && argInfo->variableInfo->isArray())
                    os << " *";
                else if (type->strAt(1) == "*" && argInfo->variableInfo && argInfo->element && argInfo->variableInfo->isArray())
                    os << " *";
                if (argInfo->address)
                    os << " *";
            } else {
                if (type->isUnsigned()) {
                    if (type->originalName() == "__int64" || type->originalName() == "__int32" || type->originalName() == "ptrdiff_t")
                        os << "unsigned ";
                }
                os << type->originalName();
                if (type->strAt(1) == "*" || argInfo->address)
                    os << " *";
                os << " {aka ";
                type->stringify(os, false, true, false);
                if (type->strAt(1) == "*" || argInfo->address)
                    os << " *";
                os << "}";
            }
        }
        os << "\'";
    } else
        os << "Unknown";
}

void CheckIO::invalidLengthModifierError(const Token* tok, unsigned int numFormat, const std::string& modifier)
{
    if (!mSettings->isEnabled(Settings::WARNING))
        return;
    std::ostringstream errmsg;
    errmsg << "'" << modifier << "' in format string (no. " << numFormat << ") is a length modifier and cannot be used without a conversion specifier.";
    reportError(tok, Severity::warning, "invalidLengthModifierError", errmsg.str(), CWE704, false);
}

void CheckIO::invalidScanfFormatWidthError(const Token* tok, unsigned int numFormat, int width, const Variable *var, char c)
{
    MathLib::bigint arrlen = 0;
    std::string varname;

    if (var) {
        arrlen = var->dimension(0);
        varname = var->name();
    }

    std::ostringstream errmsg;
    if (arrlen > width) {
        if (tok != nullptr && (!mSettings->inconclusive || !mSettings->isEnabled(Settings::WARNING)))
            return;
        errmsg << "Width " << width << " given in format string (no. " << numFormat << ") is smaller than destination buffer"
               << " '" << varname << "[" << arrlen << "]'.";
        reportError(tok, Severity::warning, "invalidScanfFormatWidth_smaller", errmsg.str(), CWE(0U), true);
    } else {
        errmsg << "Width " << width << " given in format string (no. " << numFormat << ") is larger than destination buffer '"
               << varname << "[" << arrlen << "]', use %" << (c == 'c' ? arrlen : (arrlen - 1)) << c << " to prevent overflowing it.";
        reportError(tok, Severity::error, "invalidScanfFormatWidth", errmsg.str(), CWE687, false);
    }
}
