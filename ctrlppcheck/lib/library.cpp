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

#include "library.h"

#include "astutils.h"
#include "mathlib.h"
#include "path.h"
#include "symbols/symbols.h"
#include "tinyxml2.h"
#include "token.h"
#include "tokenlist.h"
#include "utils.h"

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <list>

static std::vector<std::string> getnames(const char *names)
{
    std::vector<std::string> ret;
    while (const char *p = std::strchr(names,',')) {
        ret.emplace_back(names, p-names);
        names = p + 1;
    }
    ret.push_back(names);
    return ret;
}

static void gettokenlistfromvalid(const std::string& valid, TokenList& tokenList)
{
    std::istringstream istr(valid + ',');
    tokenList.createTokens(istr);
    for (Token *tok = tokenList.front(); tok; tok = tok->next()) {
        if (Token::Match(tok,"- %num%")) {
            tok->str("-" + tok->strAt(1));
            tok->deleteNext();
        }
    }
}

Library::Library() : mAllocId(0)
{
}

Library::Error Library::load(const char exename[], const char path[])
{
    if (std::strchr(path,',') != nullptr) {
        std::string p(path);
        for (;;) {
            const std::string::size_type pos = p.find(',');
            if (pos == std::string::npos)
                break;
            const Error &e = load(exename, p.substr(0,pos).c_str());
            if (e.errorcode != OK)
                return e;
            p = p.substr(pos+1);
        }
        if (!p.empty())
            return load(exename, p.c_str());
        return Error();
    }

    std::string absolute_path;
    // open file..
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLError error = doc.LoadFile(path);
    if (error == tinyxml2::XML_ERROR_FILE_READ_ERROR && Path::getFilenameExtension(path).empty())
        // Reading file failed, try again...
        error = tinyxml2::XML_ERROR_FILE_NOT_FOUND;
    if (error == tinyxml2::XML_ERROR_FILE_NOT_FOUND) {
        // failed to open file.. is there no extension?
        std::string fullfilename(path);
        if (Path::getFilenameExtension(fullfilename).empty()) {
            fullfilename += ".cfg";
            error = doc.LoadFile(fullfilename.c_str());
            if (error != tinyxml2::XML_ERROR_FILE_NOT_FOUND)
                absolute_path = Path::getAbsoluteFilePath(fullfilename);
        }

        std::list<std::string> cfgfolders;
#ifdef CFGDIR
        cfgfolders.push_back(CFGDIR);
#endif
        if (exename) {
            const std::string exepath(Path::fromNativeSeparators(Path::getPathFromFilename(exename)));
            cfgfolders.push_back(exepath + "cfg");
            cfgfolders.push_back(exepath);
        }

        while (error == tinyxml2::XML_ERROR_FILE_NOT_FOUND && !cfgfolders.empty()) {
            const std::string cfgfolder(cfgfolders.front());
            cfgfolders.pop_front();
            const char *sep = (!cfgfolder.empty() && endsWith(cfgfolder,'/') ? "" : "/");
            const std::string filename(cfgfolder + sep + fullfilename);
            error = doc.LoadFile(filename.c_str());
            if (error != tinyxml2::XML_ERROR_FILE_NOT_FOUND)
                absolute_path = Path::getAbsoluteFilePath(filename);
        }
    } else
        absolute_path = Path::getAbsoluteFilePath(path);

    if (error == tinyxml2::XML_SUCCESS) {
        if (mFiles.find(absolute_path) == mFiles.end()) {
            Error err = load(doc);
            if (err.errorcode == OK)
                mFiles.insert(absolute_path);
            return err;
        }

        return Error(OK); // ignore duplicates
    }

    if (error == tinyxml2::XML_ERROR_FILE_NOT_FOUND)
        return Error(FILE_NOT_FOUND);
    else {
        doc.PrintError();
        return Error(BAD_XML);
    }
}

bool Library::loadxmldata(const char xmldata[], std::size_t len)
{
    tinyxml2::XMLDocument doc;
    return (tinyxml2::XML_SUCCESS == doc.Parse(xmldata, len)) && (load(doc).errorcode == OK);
}

Library::Error Library::load(const tinyxml2::XMLDocument &doc)
{
    const tinyxml2::XMLElement * const rootnode = doc.FirstChildElement();

    if (rootnode == nullptr) {
        doc.PrintError();
        return Error(BAD_XML);
    }

    if (strcmp(rootnode->Name(),"def") != 0)
        return Error(UNSUPPORTED_FORMAT, rootnode->Name());

    const char* format_string = rootnode->Attribute("format");
    int format = 1; // Assume format version 1 if nothing else is specified (very old .cfg files had no 'format' attribute)
    if (format_string)
        format = atoi(format_string);

    if (format > 2 || format <= 0)
        return Error(UNSUPPORTED_FORMAT);

    std::set<std::string> unknown_elements;

    for (const tinyxml2::XMLElement *node = rootnode->FirstChildElement(); node; node = node->NextSiblingElement()) {
        const std::string nodename = node->Name();
        if (nodename == "memory" || nodename == "resource") {
            // get allocationId to use..
            int allocationId = 0;
            for (const tinyxml2::XMLElement *memorynode = node->FirstChildElement(); memorynode; memorynode = memorynode->NextSiblingElement()) {
                if (strcmp(memorynode->Name(),"dealloc")==0) {
                    const std::map<std::string, AllocFunc>::const_iterator it = mDealloc.find(memorynode->GetText());
                    if (it != mDealloc.end()) {
                        allocationId = it->second.groupId;
                        break;
                    }
                }
            }
            if (allocationId == 0) {
                if (nodename == "memory")
                    while (!ismemory(++mAllocId));
                else
                    while (!isresource(++mAllocId));
                allocationId = mAllocId;
            }

            // add alloc/dealloc/use functions..
            for (const tinyxml2::XMLElement *memorynode = node->FirstChildElement(); memorynode; memorynode = memorynode->NextSiblingElement()) {
                const std::string memorynodename = memorynode->Name();
                if (memorynodename == "alloc") {
                    AllocFunc temp;
                    temp.groupId = allocationId;

                    if (memorynode->Attribute("init", "false"))
                        returnuninitdata.insert(memorynode->GetText());

                    const char *arg = memorynode->Attribute("arg");
                    if (arg)
                        temp.arg = atoi(arg);
                    else
                        temp.arg = -1;
                    mAlloc[memorynode->GetText()] = temp;
                } else if (memorynodename == "dealloc") {
                    AllocFunc temp;
                    temp.groupId = allocationId;
                    const char *arg = memorynode->Attribute("arg");
                    if (arg)
                        temp.arg = atoi(arg);
                    else
                        temp.arg = 1;
                    mDealloc[memorynode->GetText()] = temp;
                } else if (memorynodename == "use")
                    functions[memorynode->GetText()].use = true;
                else
                    unknown_elements.insert(memorynodename);
            }
        }

        else if (nodename == "define") {
            const char *name = node->Attribute("name");
            if (name == nullptr)
                return Error(MISSING_ATTRIBUTE, "name");
            const char *value = node->Attribute("value");
            if (value == nullptr)
                return Error(MISSING_ATTRIBUTE, "value");
            const char *type = node->Attribute("type");

            Library::UserDefinedValue def;
            def.name = std::string(name);

            // check possible value type
            def.type = type ? std::string(type) : "";

            // convert value
            if ( def.type == "string" ){
                def.value = "\"" + std::string(value) + "\"";
            }
            else {
                def.value = value;
            }

            if ( def.type == "" )
            {
              if ( def.value == "true" || def.value == "false")
              {
                  def.type = "bool";
              }
              else
              {
                  def.type = "int";
              }
            }
            
            /// @todo make it somehow configurable
            def.isConst = true;

            defines[std::string(name)] = def;
            
        }

        else if (nodename == "function") {
            const char *name = node->Attribute("name");
            if (name == nullptr)
                return Error(MISSING_ATTRIBUTE, "name");
            for (const std::string &s : getnames(name)) {
                const Error &err = loadFunction(node, s, unknown_elements);
                if (err.errorcode != ErrorCode::OK)
                    return err;
            }
        }

        else if (nodename == "reflection") {
            for (const tinyxml2::XMLElement *reflectionnode = node->FirstChildElement(); reflectionnode; reflectionnode = reflectionnode->NextSiblingElement()) {
                if (strcmp(reflectionnode->Name(), "call") != 0) {
                    unknown_elements.insert(reflectionnode->Name());
                    continue;
                }

                const char * const argString = reflectionnode->Attribute("arg");
                if (!argString)
                    return Error(MISSING_ATTRIBUTE, "arg");

                mReflection[reflectionnode->GetText()] = atoi(argString);
            }
        }

        else if (nodename == "markup") {
            const char * const extension = node->Attribute("ext");
            if (!extension)
                return Error(MISSING_ATTRIBUTE, "ext");
            mMarkupExtensions.insert(extension);

            mReportErrors[extension] = (node->Attribute("reporterrors", "true") != nullptr);
            mProcessAfterCode[extension] = (node->Attribute("aftercode", "true") != nullptr);

            for (const tinyxml2::XMLElement *markupnode = node->FirstChildElement(); markupnode; markupnode = markupnode->NextSiblingElement()) {
                const std::string markupnodename = markupnode->Name();
                if (markupnodename == "keywords") {
                    for (const tinyxml2::XMLElement *librarynode = markupnode->FirstChildElement(); librarynode; librarynode = librarynode->NextSiblingElement()) {
                        if (strcmp(librarynode->Name(), "keyword") == 0) {
                            const char* nodeName = librarynode->Attribute("name");
                            if (nodeName == nullptr)
                                return Error(MISSING_ATTRIBUTE, "name");
                            mKeywords[extension].insert(nodeName);
                        } else
                            unknown_elements.insert(librarynode->Name());
                    }
                }

                else if (markupnodename == "exported") {
                    for (const tinyxml2::XMLElement *exporter = markupnode->FirstChildElement(); exporter; exporter = exporter->NextSiblingElement()) {
                        if (strcmp(exporter->Name(), "exporter") != 0) {
                            unknown_elements.insert(exporter->Name());
                            continue;
                        }

                        const char * const prefix = exporter->Attribute("prefix");
                        if (!prefix)
                            return Error(MISSING_ATTRIBUTE, "prefix");

                        for (const tinyxml2::XMLElement *e = exporter->FirstChildElement(); e; e = e->NextSiblingElement()) {
                            const std::string ename = e->Name();
                            if (ename == "prefix")
                                mExporters[prefix].addPrefix(e->GetText());
                            else if (ename == "suffix")
                                mExporters[prefix].addSuffix(e->GetText());
                            else
                                unknown_elements.insert(ename);
                        }
                    }
                }

                else if (markupnodename == "imported") {
                    for (const tinyxml2::XMLElement *librarynode = markupnode->FirstChildElement(); librarynode; librarynode = librarynode->NextSiblingElement()) {
                        if (strcmp(librarynode->Name(), "importer") == 0)
                            mImporters[extension].insert(librarynode->GetText());
                        else
                            unknown_elements.insert(librarynode->Name());
                    }
                }

                else if (markupnodename == "codeblocks") {
                    for (const tinyxml2::XMLElement *blocknode = markupnode->FirstChildElement(); blocknode; blocknode = blocknode->NextSiblingElement()) {
                        const std::string blocknodename = blocknode->Name();
                        if (blocknodename == "block") {
                            const char * blockName = blocknode->Attribute("name");
                            if (blockName)
                                mExecutableBlocks[extension].addBlock(blockName);
                        } else if (blocknodename == "structure") {
                            const char * start = blocknode->Attribute("start");
                            if (start)
                                mExecutableBlocks[extension].setStart(start);
                            const char * end = blocknode->Attribute("end");
                            if (end)
                                mExecutableBlocks[extension].setEnd(end);
                            const char * offset = blocknode->Attribute("offset");
                            if (offset)
                                mExecutableBlocks[extension].setOffset(atoi(offset));
                        }

                        else
                            unknown_elements.insert(blocknodename);
                    }
                }

                else
                    unknown_elements.insert(markupnodename);
            }
        }

        else if (nodename == "podtype") {
            const char * const name = node->Attribute("name");
            if (!name)
                return Error(MISSING_ATTRIBUTE, "name");
            PodType podType = {0};
            const char * const size = node->Attribute("size");
            if (size)
                podType.size = atoi(size);
            const char * const sign = node->Attribute("sign");
            if (sign)
                podType.sign = *sign;
            for (const std::string &s : getnames(name))
                mPodTypes[s] = podType;
        }

        else if (nodename == "platformtype") {
            const char * const type_name = node->Attribute("name");
            if (type_name == nullptr)
                return Error(MISSING_ATTRIBUTE, "name");
            const char *value = node->Attribute("value");
            if (value == nullptr)
                return Error(MISSING_ATTRIBUTE, "value");
            PlatformType type;
            type.mType = value;
            std::set<std::string> platform;
            for (const tinyxml2::XMLElement *typenode = node->FirstChildElement(); typenode; typenode = typenode->NextSiblingElement()) {
                const std::string typenodename = typenode->Name();
                if (typenodename == "platform") {
                    const char * const type_attribute = typenode->Attribute("type");
                    if (type_attribute == nullptr)
                        return Error(MISSING_ATTRIBUTE, "type");
                    platform.insert(type_attribute);
                } else if (typenodename == "signed")
                    type._signed = true;
                else if (typenodename == "unsigned")
                    type._unsigned = true;
                else if (typenodename == "long")
                    type._long = true;
                else if (typenodename == "pointer")
                    type._pointer= true;
                else if (typenodename == "ptr_ptr")
                    type._ptr_ptr = true;
                else if (typenodename == "const_ptr")
                    type._const_ptr = true;
                else
                    unknown_elements.insert(typenodename);
            }
            if (platform.empty()) {
                const PlatformType * const type_ptr = platform_type(type_name, emptyString);
                if (type_ptr) {
                    if (*type_ptr == type)
                        return Error(DUPLICATE_PLATFORM_TYPE, type_name);
                    return Error(PLATFORM_TYPE_REDEFINED, type_name);
                }
                mPlatformTypes[type_name] = type;
            } else {
                for (const std::string &p : platform) {
                    const PlatformType * const type_ptr = platform_type(type_name, p);
                    if (type_ptr) {
                        if (*type_ptr == type)
                            return Error(DUPLICATE_PLATFORM_TYPE, type_name);
                        return Error(PLATFORM_TYPE_REDEFINED, type_name);
                    }
                    mPlatforms[p].mPlatformTypes[type_name] = type;
                }
            }
        }

        else
            unknown_elements.insert(nodename);
    }
    if (!unknown_elements.empty()) {
        std::string str;
        for (std::set<std::string>::const_iterator i = unknown_elements.begin(); i != unknown_elements.end();) {
            str += *i;
            if (++i != unknown_elements.end())
                str += ", ";
        }
        return Error(UNKNOWN_ELEMENT, str);
    }
    return Error(OK);
}

Library::Error Library::loadFunction(const tinyxml2::XMLElement * const node, const std::string &name, std::set<std::string> &unknown_elements)
{
    if (name.empty())
        return Error(OK);

    Function& func = functions[name];

    for (const tinyxml2::XMLElement *functionnode = node->FirstChildElement(); functionnode; functionnode = functionnode->NextSiblingElement()) {
        const std::string functionnodename = functionnode->Name();
        if (functionnodename == "notInLoop")
        {
          const char *expr = functionnode->GetText();
          func.notInLoop_inconclusive = (expr && strcmp(expr, "inconclusive") == 0);
          func.notInLoop = !func.notInLoop_inconclusive;
        }
        else if (functionnodename == "noreturn")
          mNoReturn[name] = (strcmp(functionnode->GetText(), "true") == 0);
        else if (functionnodename == "pure")
            func.ispure = true;
        else if (functionnodename == "const") {
            func.ispure = true;
            func.isconst = true; // a constant function is pure
        } else if (functionnodename == "leak-ignore")
            func.leakignore = true;
        else if (functionnodename == "use-retval")
            func.useretval = true;
        else if (functionnodename == "returnValue") {
            if (const char *expr = functionnode->GetText())
                mReturnValue[name] = expr;
            if (const char *type = functionnode->Attribute("type"))
                mReturnValueType[name] = type;
        } else if (functionnodename == "arg") {
            const char* argNrString = functionnode->Attribute("nr");
            if (!argNrString)
                return Error(MISSING_ATTRIBUTE, "nr");
            const bool bAnyArg = strcmp(argNrString, "any") == 0;
            const bool bVariadicArg = strcmp(argNrString, "variadic") == 0;
            const int nr = (bAnyArg || bVariadicArg) ? -1 : std::atoi(argNrString);
            ArgumentChecks &ac = func.argumentChecks[nr];
            ac.name = argNrString;

            if (const char *typeattr = functionnode->Attribute("type") )
              ac.valueType = typeattr;
            else if ( const char *nameAtr = functionnode->Attribute("name") )
              ac.name = nameAtr;

            ac.optional  = functionnode->Attribute("default") != nullptr;
            ac.variadic = bVariadicArg;
            const char * const argDirection = functionnode->Attribute("direction");
            if (argDirection) {
                const size_t argDirLen = strlen(argDirection);
                if (!strncmp(argDirection, "in", argDirLen)) {
                    ac.direction = ArgumentChecks::Direction::DIR_IN;
                } else if (!strncmp(argDirection, "out", argDirLen)) {
                    ac.direction = ArgumentChecks::Direction::DIR_OUT;
                } else if (!strncmp(argDirection, "inout", argDirLen)) {
                    ac.direction = ArgumentChecks::Direction::DIR_INOUT;
                }
                else
                {
                  /// @todo throw error here
                  ac.direction = ArgumentChecks::Direction::DIR_IN; // set per defaul as input param
                }
            }
            else
              ac.direction = ArgumentChecks::Direction::DIR_IN; // set per defaul as input param


            for (const tinyxml2::XMLElement *argnode = functionnode->FirstChildElement(); argnode; argnode = argnode->NextSiblingElement()) {
                const std::string argnodename = argnode->Name();
                if (argnodename == "not-bool")
                    ac.notbool = true;
                else if (argnodename == "variadic")
                  ac.variadic = true;
                else if (argnodename == "not-null")
                    ac.notnull = true;
                else if (argnodename == "not-uninit")
                    ac.notuninit = true;
                else if (argnodename == "formatstr")
                    ac.formatstr = true;
                else if (argnodename == "strz")
                    ac.strz = true;
                else if (argnodename == "valid") {
                    // Validate the validation expression
                    const char *p = argnode->GetText();
                    bool error = false;
                    bool range = false;
                    bool has_dot = false;

                    if (!p)
                        return Error(BAD_ATTRIBUTE_VALUE, "\"\"");

                    error = *p == '.';
                    for (; *p; p++) {
                        if (std::isdigit(*p))
                            error |= (*(p+1) == '-');
                        else if (*p == ':') {
                            error |= range | (*(p+1) == '.');
                            range = true;
                            has_dot = false;
                        } else if (*p == '-')
                            error |= (!std::isdigit(*(p+1)));
                        else if (*p == ',') {
                            range = false;
                            error |= *(p+1) == '.';
                            has_dot = false;
                        } else if (*p == '.') {
                            error |= has_dot | (!std::isdigit(*(p+1)));
                            has_dot = true;
                        } else
                            error = true;
                    }
                    if (error)
                        return Error(BAD_ATTRIBUTE_VALUE, argnode->GetText());

                    // Set validation expression
                    ac.valid = argnode->GetText();
                }

                else if (argnodename == "minsize") {
                    const char *typeattr = argnode->Attribute("type");
                    if (!typeattr)
                        return Error(MISSING_ATTRIBUTE, "type");

                    ArgumentChecks::MinSize::Type type;
                    if (strcmp(typeattr,"strlen")==0)
                        type = ArgumentChecks::MinSize::Type::STRLEN;
                    else if (strcmp(typeattr,"argvalue")==0)
                        type = ArgumentChecks::MinSize::Type::ARGVALUE;
                    else if (strcmp(typeattr,"sizeof")==0)
                        type = ArgumentChecks::MinSize::Type::SIZEOF;
                    else if (strcmp(typeattr,"mul")==0)
                        type = ArgumentChecks::MinSize::Type::MUL;
                    else
                        return Error(BAD_ATTRIBUTE_VALUE, typeattr);

                    const char *argattr  = argnode->Attribute("arg");
                    if (!argattr)
                        return Error(MISSING_ATTRIBUTE, "arg");
                    if (strlen(argattr) != 1 || argattr[0]<'0' || argattr[0]>'9')
                        return Error(BAD_ATTRIBUTE_VALUE, argattr);

                    ac.minsizes.reserve(type == ArgumentChecks::MinSize::Type::MUL ? 2 : 1);
                    ac.minsizes.emplace_back(type,argattr[0]-'0');
                    if (type == ArgumentChecks::MinSize::Type::MUL) {
                        const char *arg2attr = argnode->Attribute("arg2");
                        if (!arg2attr)
                            return Error(MISSING_ATTRIBUTE, "arg2");
                        if (strlen(arg2attr) != 1 || arg2attr[0]<'0' || arg2attr[0]>'9')
                            return Error(BAD_ATTRIBUTE_VALUE, arg2attr);
                        ac.minsizes.back().arg2 = arg2attr[0] - '0';
                    }
                }

                else if (argnodename == "iterator") {
                    ac.iteratorInfo.it = true;
                    const char* str = argnode->Attribute("type");
                    ac.iteratorInfo.first = str ? (std::strcmp(str, "first") == 0) : false;
                    ac.iteratorInfo.last = str ? (std::strcmp(str, "last") == 0) : false;
                    str = argnode->Attribute("container");
                    ac.iteratorInfo.container = str ? std::atoi(str) : 0;
                }

                else
                    unknown_elements.insert(argnodename);
            }
        } else if (functionnodename == "ignorefunction") {
            func.ignore = true;
        } else if (functionnodename == "formatstr") {
            func.formatstr = true;
            const tinyxml2::XMLAttribute* scan = functionnode->FindAttribute("scan");
            func.formatstr_scan = scan && scan->BoolValue();
        } else if (functionnodename == "warn") {
            WarnInfo wi;
            const char* const severity = functionnode->Attribute("severity");
            if (severity == nullptr)
                return Error(MISSING_ATTRIBUTE, "severity");
            wi.severity = Severity::fromString(severity);

            const char* const reason = functionnode->Attribute("reason");
            const char* const alternatives = functionnode->Attribute("alternatives");
            if (reason && alternatives) {
                // Construct message
                std::vector<std::string> alt = getnames(alternatives);
                wi.message = std::string(reason) + " function '" + name + "' called.";
                if (  alt.size() > 0 && alt[0] != "" ){
                    wi.message += " It is recommended to use ";
                    for (std::size_t i = 0; i < alt.size(); ++i) {
                        wi.message += "'" + alt[i] + "'";
                        if (i == alt.size() - 1)
                            wi.message += " instead.";
                        else if (i == alt.size() - 2)
                            wi.message += " or ";
                        else
                            wi.message += ", ";
                    }
                }
                else
                {
                    wi.message += " It is recommended to clean up the code instead.";
                }
                
                
            } else {
                const char * const message = functionnode->GetText();
                if (!message) {
                    return Error(MISSING_ATTRIBUTE, "\"reason\" and \"alternatives\" or some text.");
                } else
                    wi.message = message;
            }

            functionwarn[name] = wi;
        } else
            unknown_elements.insert(functionnodename);
    }
    return Error(OK);
}

bool Library::isIntArgValid(const Token *ftok, int argnr, const MathLib::bigint argvalue) const
{
    const ArgumentChecks *ac = getarg(ftok, argnr);
    if (!ac || ac->valid.empty())
        return true;
    else if (ac->valid.find('.') != std::string::npos)
        return isFloatArgValid(ftok, argnr, argvalue);
    TokenList tokenList(nullptr);
    gettokenlistfromvalid(ac->valid, tokenList);
    for (const Token *tok = tokenList.front(); tok; tok = tok->next()) {
        if (tok->isNumber() && argvalue == MathLib::toLongNumber(tok->str()))
            return true;
        if (Token::Match(tok, "%num% : %num%") && argvalue >= MathLib::toLongNumber(tok->str()) && argvalue <= MathLib::toLongNumber(tok->strAt(2)))
            return true;
        if (Token::Match(tok, "%num% : ,") && argvalue >= MathLib::toLongNumber(tok->str()))
            return true;
        if ((!tok->previous() || tok->previous()->str() == ",") && Token::Match(tok,": %num%") && argvalue <= MathLib::toLongNumber(tok->strAt(1)))
            return true;
    }
    return false;
}

bool Library::isFloatArgValid(const Token *ftok, int argnr, double argvalue) const
{
    const ArgumentChecks *ac = getarg(ftok, argnr);
    if (!ac || ac->valid.empty())
        return true;
    TokenList tokenList(nullptr);
    gettokenlistfromvalid(ac->valid, tokenList);
    for (const Token *tok = tokenList.front(); tok; tok = tok->next()) {
        if (Token::Match(tok, "%num% : %num%") && argvalue >= MathLib::toDoubleNumber(tok->str()) && argvalue <= MathLib::toDoubleNumber(tok->strAt(2)))
            return true;
        if (Token::Match(tok, "%num% : ,") && argvalue >= MathLib::toDoubleNumber(tok->str()))
            return true;
        if ((!tok->previous() || tok->previous()->str() == ",") && Token::Match(tok,": %num%") && argvalue <= MathLib::toDoubleNumber(tok->strAt(1)))
            return true;
    }
    return false;
}

std::string Library::getFunctionName(const Token *ftok, bool *error) const
{
    if (!ftok) {
        *error = true;
        return "";
    }
    if (ftok->isName()) {
        for (const Scope *scope = ftok->scope(); scope; scope = scope->nestedIn) {
            if (!scope->isClassOrStruct())
                continue;
            const std::vector<Type::BaseInfo> &derivedFrom = scope->definedType->derivedFrom;
            for (unsigned int i = 0; i < derivedFrom.size(); ++i) {
                const Type::BaseInfo &baseInfo = derivedFrom[i];
                const std::string name(baseInfo.name + "::" + ftok->str());
                if (functions.find(name) != functions.end() && matchArguments(ftok, name))
                    return name;
            }
        }
        return ftok->str();
    }
    if (ftok->str() == "::") {
        if (!ftok->astOperand2())
            return getFunctionName(ftok->astOperand1(), error);
        return getFunctionName(ftok->astOperand1(),error) + "::" + getFunctionName(ftok->astOperand2(),error);
    }
    if (ftok->str() == "." && ftok->astOperand1()) {
        const std::string type = astCanonicalType(ftok->astOperand1());
        if (type.empty()) {
            *error = true;
            return "";
        }

        return type + "::" + getFunctionName(ftok->astOperand2(),error);
    }
    *error = true;
    return "";
}

std::string Library::getFunctionName(const Token *ftok) const
{
    if (!Token::Match(ftok, "%name% (") && (ftok->strAt(-1) != "&" || ftok->previous()->astOperand2()))
        return "";

    // Lookup function name using AST..
    if (ftok->astParent()) {
        bool error = false;
        const std::string ret = getFunctionName(ftok->next()->astOperand1(), &error);
        return error ? std::string() : ret;
    }

    // Lookup function name without using AST..
    if (Token::simpleMatch(ftok->previous(), "."))
        return "";
    if (!Token::Match(ftok->tokAt(-2), "%name% ::"))
        return ftok->str();
    std::string ret(ftok->str());
    ftok = ftok->tokAt(-2);
    while (Token::Match(ftok, "%name% ::")) {
        ret = ftok->str() + "::" + ret;
        ftok = ftok->tokAt(-2);
    }
    return ret;
}

bool Library::isnullargbad(const Token *ftok, int argnr) const
{
    const ArgumentChecks *arg = getarg(ftok, argnr);
    if (!arg) {
        // scan format string argument should not be null
        const std::string funcname = getFunctionName(ftok);
        const std::map<std::string, Function>::const_iterator it = functions.find(funcname);
        if (it != functions.cend() && it->second.formatstr && it->second.formatstr_scan)
            return true;
    }
    return arg && arg->notnull;
}

bool Library::isuninitargbad(const Token *ftok, int argnr) const
{
    const ArgumentChecks *arg = getarg(ftok, argnr);
    if (!arg) {
        // non-scan format string argument should not be uninitialized
        const std::string funcname = getFunctionName(ftok);
        const std::map<std::string, Function>::const_iterator it = functions.find(funcname);
        if (it != functions.cend() && it->second.formatstr && !it->second.formatstr_scan)
            return true;
    }
    return arg && arg->notuninit;
}

const Library::ArgumentChecks * Library::getarg(const Token *ftok, int argnr) const
{
    if (isNotLibraryFunction(ftok))
        return nullptr;
    const std::map<std::string, Function>::const_iterator it1 = functions.find(getFunctionName(ftok));
    if (it1 == functions.cend())
        return nullptr;
    const std::map<int,ArgumentChecks>::const_iterator it2 = it1->second.argumentChecks.find(argnr);
    if (it2 != it1->second.argumentChecks.cend())
        return &it2->second;
    const std::map<int,ArgumentChecks>::const_iterator it3 = it1->second.argumentChecks.find(-1);
    if (it3 != it1->second.argumentChecks.cend())
        return &it3->second;
    return nullptr;
}

bool Library::isScopeNoReturn(const Token *end, std::string *unknownFunc) const
{
    if (unknownFunc)
        unknownFunc->clear();

    if (Token::Match(end->tokAt(-2), "!!{ ; }")) {
        const Token *lastTop = end->tokAt(-2)->astTop();
        if (Token::simpleMatch(lastTop, "<<") &&
            Token::simpleMatch(lastTop->astOperand1(), "(") &&
            Token::Match(lastTop->astOperand1()->previous(), "%name% ("))
            return isnoreturn(lastTop->astOperand1()->previous());
    }

    if (!Token::simpleMatch(end->tokAt(-2), ") ; }"))
        return false;

    const Token *funcname = end->linkAt(-2)->previous();
    const Token *start = funcname;
    if (Token::Match(funcname->tokAt(-3),"( * %name% )")) {
        funcname = funcname->previous();
        start = funcname->tokAt(-3);
    } else if (funcname->isName()) {
        while (Token::Match(start, "%name%|.|::"))
            start = start->previous();
    } else {
        return false;
    }
    if (Token::Match(start,"[;{}]") && Token::Match(funcname, "%name% )| (")) {
        if (funcname->str() == "exit")
            return true;
        if (!isnotnoreturn(funcname)) {
            if (unknownFunc && !isnoreturn(funcname))
                *unknownFunc = funcname->str();
            return true;
        }
    }
    return false;
}

//---------------------------------------------------------------------------------------------------------------------------------------
// returns true if ftok is not a library function
bool Library::isNotLibraryFunction(const Token *ftok) const
{
    if (ftok->function() && ftok->function()->nestedIn && ftok->function()->nestedIn->type != Scope::eGlobal)
        return true;

    // variables are not library functions.
    if (ftok->varId())
        return true;

    return false;
}

//---------------------------------------------------------------------------------------------------------------------------------------
bool Library::matchFunctionArguments(const Token *ftok) const
{
    return matchArguments(ftok, getFunctionName(ftok));
}

bool Library::matchArguments(const Token *ftok, const std::string &functionName) const
{
    const int callargs = numberOfArguments(ftok);
    const std::map<std::string, Function>::const_iterator it = functions.find(functionName);
    if (it == functions.cend())
        return (callargs == 0);
    int args = 0;
    int firstOptionalArg = -1;
    bool variadicOrFormatstr = false;
    for (std::map<int, ArgumentChecks>::const_iterator it2 = it->second.argumentChecks.cbegin(); it2 != it->second.argumentChecks.cend(); ++it2) {
        if (it2->first > args)
            args = it2->first;
        if (it2->second.optional && (firstOptionalArg == -1 || firstOptionalArg > it2->first))
            firstOptionalArg = it2->first;

        if (it2->second.formatstr || it2->second.variadic)
            variadicOrFormatstr = true;
            //return args <= callargs;
    }

    if ( variadicOrFormatstr )
        return args <= callargs;
    else
        return (firstOptionalArg < 0) ? args == callargs : (callargs >= firstOptionalArg-1 && callargs <= args);
}

const Library::WarnInfo* Library::getWarnInfo(const Token* ftok) const
{
    if (isNotLibraryFunction(ftok))
        return nullptr;
    std::map<std::string, WarnInfo>::const_iterator i = functionwarn.find(getFunctionName(ftok));
    if (i == functionwarn.cend())
        return nullptr;
    return &i->second;
}

bool Library::formatstr_function(const Token* ftok) const
{
    if (isNotLibraryFunction(ftok))
        return false;

    const std::map<std::string, Function>::const_iterator it = functions.find(getFunctionName(ftok));
    if (it != functions.cend())
        return it->second.formatstr;
    return false;
}

int Library::formatstr_argno(const Token* ftok) const
{
    const std::map<int, Library::ArgumentChecks>& argumentChecksFunc = functions.at(getFunctionName(ftok)).argumentChecks;
    for (std::map<int, Library::ArgumentChecks>::const_iterator i = argumentChecksFunc.cbegin(); i != argumentChecksFunc.cend(); ++i) {
        if (i->second.formatstr) {
            return i->first - 1;
        }
    }
    return -1;
}

bool Library::formatstr_scan(const Token* ftok) const
{
    return functions.at(getFunctionName(ftok)).formatstr_scan;
}

bool Library::isUseRetVal(const Token* ftok) const
{
    if (isNotLibraryFunction(ftok))
        return false;
    const std::map<std::string, Function>::const_iterator it = functions.find(getFunctionName(ftok));
    if (it != functions.cend())
        return it->second.useretval;
    return false;
}

const std::string& Library::returnValue(const Token *ftok) const
{
    if (isNotLibraryFunction(ftok))
        return emptyString;
    const std::map<std::string, std::string>::const_iterator it = mReturnValue.find(getFunctionName(ftok));
    return it != mReturnValue.end() ? it->second : emptyString;
}

const std::string& Library::returnValueType(const Token *ftok) const
{
    if (isNotLibraryFunction(ftok))
        return emptyString;
    const std::map<std::string, std::string>::const_iterator it = mReturnValueType.find(getFunctionName(ftok));
    return it != mReturnValueType.end() ? it->second : emptyString;
}

bool Library::hasminsize(const Token *ftok) const
{
    if (isNotLibraryFunction(ftok))
        return false;
    const std::map<std::string, Function>::const_iterator it1 = functions.find(getFunctionName(ftok));
    if (it1 == functions.cend())
        return false;
    for (std::map<int, ArgumentChecks>::const_iterator it2 = it1->second.argumentChecks.cbegin(); it2 != it1->second.argumentChecks.cend(); ++it2) {
        if (!it2->second.minsizes.empty())
            return true;
    }
    return false;
}

bool Library::ignorefunction(const std::string& functionName) const
{
    const std::map<std::string, Function>::const_iterator it = functions.find(functionName);
    if (it != functions.cend())
        return it->second.ignore;
    return false;
}
bool Library::isUse(const std::string& functionName) const
{
    const std::map<std::string, Function>::const_iterator it = functions.find(functionName);
    if (it != functions.cend())
        return it->second.use;
    return false;
}
bool Library::isLeakIgnore(const std::string& functionName) const
{
  const  std::map<std::string, Function>::const_iterator it = functions.find(functionName);
  if (it != functions.cend())
    return it->second.leakignore;
  return false;
}

//---------------------------------------------------------------------------
bool Library::isFunctionNotInLoop(const Token *ftok, const bool inconclusive) const
{
  if ( isNotLibraryFunction(ftok) )
    return false;
 
  const std::map<std::string, Function>::const_iterator it = functions.find(getFunctionName(ftok));

  if (it == functions.end())
    return false; // not found

   return ( (it->second.notInLoop ) || (inconclusive && it->second.notInLoop_inconclusive) );
}
bool Library::isFunctionConst(const std::string& functionName, bool pure) const
{
    const std::map<std::string, Function>::const_iterator it = functions.find(functionName);
    if (it != functions.cend())
        return pure ? it->second.ispure : it->second.isconst;
    return false;
}
bool Library::isFunctionConst(const Token *ftok) const
{
    if (ftok->function() && ftok->function()->isAttributeConst())
        return true;
    if (isNotLibraryFunction(ftok))
        return false;
    const std::map<std::string, Function>::const_iterator it = functions.find(getFunctionName(ftok));
    return (it != functions.end() && it->second.isconst);
}
bool Library::isnoreturn(const Token *ftok) const
{
    if (ftok->function() && ftok->function()->isAttributeNoreturn())
        return true;
    if (isNotLibraryFunction(ftok))
        return false;
    const std::map<std::string, bool>::const_iterator it = mNoReturn.find(getFunctionName(ftok));
    return (it != mNoReturn.end() && it->second);
}

bool Library::isnotnoreturn(const Token *ftok) const
{
    if (ftok->function() && ftok->function()->isAttributeNoreturn())
        return false;
    if (isNotLibraryFunction(ftok))
        return false;
    const std::map<std::string, bool>::const_iterator it = mNoReturn.find(getFunctionName(ftok));
    return (it != mNoReturn.end() && !it->second);
}

bool Library::markupFile(const std::string &path) const
{
    return mMarkupExtensions.find(Path::getFilenameExtensionInLowerCase(path)) != mMarkupExtensions.end();
}

bool Library::processMarkupAfterCode(const std::string &path) const
{
    const std::map<std::string, bool>::const_iterator it = mProcessAfterCode.find(Path::getFilenameExtensionInLowerCase(path));
    return (it == mProcessAfterCode.end() || it->second);
}

bool Library::reportErrors(const std::string &path) const
{
    const std::map<std::string, bool>::const_iterator it = mReportErrors.find(Path::getFilenameExtensionInLowerCase(path));
    return (it == mReportErrors.end() || it->second);
}

bool Library::isexecutableblock(const std::string &file, const std::string &token) const
{
    const std::map<std::string, CodeBlock>::const_iterator it = mExecutableBlocks.find(Path::getFilenameExtensionInLowerCase(file));
    return (it != mExecutableBlocks.end() && it->second.isBlock(token));
}

int Library::blockstartoffset(const std::string &file) const
{
    int offset = -1;
    const std::map<std::string, CodeBlock>::const_iterator map_it
        = mExecutableBlocks.find(Path::getFilenameExtensionInLowerCase(file));

    if (map_it != mExecutableBlocks.end()) {
        offset = map_it->second.offset();
    }
    return offset;
}

const std::string& Library::blockstart(const std::string &file) const
{
    const std::map<std::string, CodeBlock>::const_iterator map_it
        = mExecutableBlocks.find(Path::getFilenameExtensionInLowerCase(file));

    if (map_it != mExecutableBlocks.end()) {
        return map_it->second.start();
    }
    return emptyString;
}

const std::string& Library::blockend(const std::string &file) const
{
    const std::map<std::string, CodeBlock>::const_iterator map_it
        = mExecutableBlocks.find(Path::getFilenameExtensionInLowerCase(file));

    if (map_it != mExecutableBlocks.end()) {
        return map_it->second.end();
    }
    return emptyString;
}

bool Library::iskeyword(const std::string &file, const std::string &keyword) const
{
    const std::map<std::string, std::set<std::string> >::const_iterator it =
        mKeywords.find(Path::getFilenameExtensionInLowerCase(file));
    return (it != mKeywords.end() && it->second.count(keyword));
}

bool Library::isimporter(const std::string& file, const std::string &importer) const
{
    const std::map<std::string, std::set<std::string> >::const_iterator it =
        mImporters.find(Path::getFilenameExtensionInLowerCase(file));
    return (it != mImporters.end() && it->second.count(importer) > 0);
}
