/*
 * Cppcheck - A tool for static C/C++ code analysis
 * Copyright (C) 2007-2018 Cppcheck team.
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
#ifndef libraryH
#define libraryH
//---------------------------------------------------------------------------

#include "config.h"
#include "errorlogger.h"
#include "mathlib.h"

#include <cstddef>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

class Token;

namespace tinyxml2 {
    class XMLDocument;
    class XMLElement;
}

/// @addtogroup Core
/// @{

/**
 * @brief Library definitions handling
 */
class CPPCHECKLIB Library {
    friend class TestSymbolDatabase; // For testing only

public:
    Library();

    enum ErrorCode { OK, FILE_NOT_FOUND, BAD_XML, UNKNOWN_ELEMENT, MISSING_ATTRIBUTE, BAD_ATTRIBUTE_VALUE, UNSUPPORTED_FORMAT, DUPLICATE_PLATFORM_TYPE, PLATFORM_TYPE_REDEFINED };

    class Error {
    public:
        Error() : errorcode(OK) {}
        explicit Error(ErrorCode e) : errorcode(e) {}
        template<typename T>
        Error(ErrorCode e, T&& r) : errorcode(e), reason(r) {}
        ErrorCode     errorcode;
        std::string   reason;
    };

    Error load(const char exename [], const char path []);
    Error load(const tinyxml2::XMLDocument &doc);

    /** this is primarily meant for unit tests. it only returns true/false */
    bool loadxmldata(const char xmldata[], std::size_t len);

    struct AllocFunc {
        int groupId;
        int arg;
    };

    /** is allocation type memory? */
    static bool ismemory(const int id) {
        return ((id > 0) && ((id & 1) == 0));
    }
    static bool ismemory(const AllocFunc* const func) {
        return ((func->groupId > 0) && ((func->groupId & 1) == 0));
    }

    /** is allocation type resource? */
    static bool isresource(const int id) {
        return ((id > 0) && ((id & 1) == 1));
    }
    static bool isresource(const AllocFunc* const func) {
        return ((func->groupId > 0) && ((func->groupId & 1) == 1));
    }

    bool formatstr_function(const Token* ftok) const;
    int formatstr_argno(const Token* ftok) const;
    bool formatstr_scan(const Token* ftok) const;

    struct WarnInfo {
        std::string message;
        Severity::SeverityType severity;
    };
    std::map<std::string, WarnInfo> functionwarn;

    const WarnInfo* getWarnInfo(const Token* ftok) const;

    // returns true if ftok is not a library function
    bool isNotLibraryFunction(const Token *ftok) const;
    bool matchFunctionArguments(const Token *ftok) const;
    bool matchArguments(const Token *ftok, const std::string &functionName) const;

    bool isUseRetVal(const Token* ftok) const;

    const std::string& returnValue(const Token *ftok) const;
    const std::string& returnValueType(const Token *ftok) const;

    bool isnoreturn(const Token *ftok) const;
    bool isnotnoreturn(const Token *ftok) const;

    bool isScopeNoReturn(const Token *end, std::string *unknownFunc) const;

    class ArgumentChecks {
    public:
        ArgumentChecks() :
            notbool(false),
            notnull(false),
            notuninit(false),
            formatstr(false),
            strz(false),
            optional(false),
            variadic(false),
            iteratorInfo(),
            direction(DIR_UNKNOWN) {
        }

        bool         notbool;
        bool         notnull;
        bool         notuninit;
        bool         formatstr;
        bool         strz;
        bool         optional;
        bool         variadic;
        std::string  valid;
        std::string  valueType;
        std::string  name;

        class IteratorInfo {
        public:
            IteratorInfo() : container(0), it(false), first(false), last(false) {}

            int  container;
            bool it;
            bool first;
            bool last;
        };
        IteratorInfo iteratorInfo;

        class MinSize {
        public:
            enum Type { NONE, STRLEN, ARGVALUE, SIZEOF, MUL };
            MinSize(Type t, int a) : type(t), arg(a), arg2(0) {}
            Type type;
            int arg;
            int arg2;
        };
        std::vector<MinSize> minsizes;

        enum Direction { DIR_IN, DIR_OUT, DIR_INOUT, DIR_UNKNOWN };
        Direction direction;
    };


    struct Function {
        std::map<int, ArgumentChecks> argumentChecks; // argument nr => argument data
        bool use;
        bool leakignore;
        bool isconst;
        bool ispure;
        bool useretval;
        bool ignore;  // ignore functions/macros from a library (gtk, qt etc)
        bool formatstr;
        bool formatstr_scan;
        bool notInLoop;
        bool notInLoop_inconclusive;
        Function() : use(false), leakignore(false), isconst(false), ispure(false),
                     useretval(false), ignore(false), formatstr(false), formatstr_scan(false),
                     notInLoop(false), notInLoop_inconclusive(false){}
    };

    std::map<std::string, Function> functions;
    bool isUse(const std::string& functionName) const;
    bool isLeakIgnore(const std::string& functionName) const;
    bool isFunctionConst(const std::string& functionName, bool pure) const;
    bool isFunctionConst(const Token *ftok) const;

    //---------------------------------------------------------------------------
    /** @brief Check if the function can be used in loop
      @param fotk Function token
      @param inconclusive Inconclusive check.
    */
    bool isFunctionNotInLoop(const Token *ftok,  const bool inconclusive) const;

    bool isboolargbad(const Token *ftok, int argnr) const {
        const ArgumentChecks *arg = getarg(ftok, argnr);
        return arg && arg->notbool;
    }

    bool isnullargbad(const Token *ftok, int argnr) const;
    bool isuninitargbad(const Token *ftok, int argnr) const;

    bool isargformatstr(const Token *ftok, int argnr) const {
        const ArgumentChecks *arg = getarg(ftok, argnr);
        return arg && arg->formatstr;
    }

    bool isargstrz(const Token *ftok, int argnr) const {
        const ArgumentChecks *arg = getarg(ftok, argnr);
        return arg && arg->strz;
    }

    bool isIntArgValid(const Token *ftok, int argnr, const MathLib::bigint argvalue) const;
    bool isFloatArgValid(const Token *ftok, int argnr, double argvalue) const;

    const std::string& valueTypeArg(const Token *ftok, int argnr) const {
      const ArgumentChecks *arg = getarg(ftok, argnr);
      return arg ? arg->valueType : emptyString;
    }

    const std::string& getArgName(const Token *ftok, int argnr) const {
      const ArgumentChecks *arg = getarg(ftok, argnr);
      return arg ? arg->name : emptyString;
    }

    const std::string& validarg(const Token *ftok, int argnr) const {
        const ArgumentChecks *arg = getarg(ftok, argnr);
        return arg ? arg->valid : emptyString;
    }

    const ArgumentChecks::IteratorInfo *getArgIteratorInfo(const Token *ftok, int argnr) const {
        const ArgumentChecks *arg = getarg(ftok, argnr);
        return arg && arg->iteratorInfo.it ? &arg->iteratorInfo : nullptr;
    }

    bool hasminsize(const Token *ftok) const;

    const std::vector<ArgumentChecks::MinSize> *argminsizes(const Token *ftok, int argnr) const {
        const ArgumentChecks *arg = getarg(ftok, argnr);
        return arg ? &arg->minsizes : nullptr;
    }

    ArgumentChecks::Direction getArgDirection(const Token *ftok, int argnr) const {
        const ArgumentChecks *arg = getarg(ftok, argnr);
        return arg ? arg->direction : ArgumentChecks::Direction::DIR_UNKNOWN;
    }

    bool markupFile(const std::string &path) const;

    bool processMarkupAfterCode(const std::string &path) const;

    const std::set<std::string> &markupExtensions() const {
        return mMarkupExtensions;
    }

    bool reportErrors(const std::string &path) const;

    bool ignorefunction(const std::string &functionName) const;

    bool isexecutableblock(const std::string &file, const std::string &token) const;

    int blockstartoffset(const std::string &file) const;

    const std::string& blockstart(const std::string &file) const;
    const std::string& blockend(const std::string &file) const;

    bool iskeyword(const std::string &file, const std::string &keyword) const;

    bool isexporter(const std::string &prefix) const {
        return mExporters.find(prefix) != mExporters.end();
    }

    bool isexportedprefix(const std::string &prefix, const std::string &token) const {
        const std::map<std::string, ExportedFunctions>::const_iterator it = mExporters.find(prefix);
        return (it != mExporters.end() && it->second.isPrefix(token));
    }

    bool isexportedsuffix(const std::string &prefix, const std::string &token) const {
        const std::map<std::string, ExportedFunctions>::const_iterator it = mExporters.find(prefix);
        return (it != mExporters.end() && it->second.isSuffix(token));
    }

    bool isimporter(const std::string& file, const std::string &importer) const;

    bool isreflection(const std::string &token) const {
        return mReflection.find(token) != mReflection.end();
    }

    int reflectionArgument(const std::string &token) const {
        const std::map<std::string, int>::const_iterator it = mReflection.find(token);
        if (it != mReflection.end())
            return it->second;
        return -1;
    }

    std::set<std::string> returnuninitdata;

    struct UserDefinedValue {
        std::string name;
        std::string value;
        std::string type;
        bool isConst;
    };

    std::map<std::string, UserDefinedValue> defines; // to provide some library defines

    struct PodType {
        unsigned int   size;
        char           sign;
    };
    const struct PodType *podtype(const std::string &name) const {
        const std::map<std::string, struct PodType>::const_iterator it = mPodTypes.find(name);
        return (it != mPodTypes.end()) ? &(it->second) : nullptr;
    }

    struct PlatformType {
        PlatformType()
            : _signed(false)
            , _unsigned(false)
            , _long(false)
            , _pointer(false)
            , _ptr_ptr(false)
            , _const_ptr(false) {
        }
        bool operator == (const PlatformType & type) const {
            return (_signed == type._signed &&
                    _unsigned == type._unsigned &&
                    _long == type._long &&
                    _pointer == type._pointer &&
                    _ptr_ptr == type._ptr_ptr &&
                    _const_ptr == type._const_ptr &&
                    mType == type.mType);
        }
        bool operator != (const PlatformType & type) const {
            return !(*this == type);
        }
        std::string mType;
        bool _signed;
        bool _unsigned;
        bool _long;
        bool _pointer;
        bool _ptr_ptr;
        bool _const_ptr;
    };

    struct Platform {
        const PlatformType *platform_type(const std::string &name) const {
            const std::map<std::string, struct PlatformType>::const_iterator it = mPlatformTypes.find(name);
            return (it != mPlatformTypes.end()) ? &(it->second) : nullptr;
        }
        std::map<std::string, PlatformType> mPlatformTypes;
    };

    const PlatformType *platform_type(const std::string &name, const std::string & platform) const {
        const std::map<std::string, Platform>::const_iterator it = mPlatforms.find(platform);
        if (it != mPlatforms.end()) {
            const PlatformType * const type = it->second.platform_type(name);
            if (type)
                return type;
        }

        const std::map<std::string, PlatformType>::const_iterator it2 = mPlatformTypes.find(name);
        return (it2 != mPlatformTypes.end()) ? &(it2->second) : nullptr;
    }

    /**
     * Get function name for function call
     */
    std::string getFunctionName(const Token *ftok) const;

private:
    // load a <function> xml node
    Error loadFunction(const tinyxml2::XMLElement * const node, const std::string &name, std::set<std::string> &unknown_elements);

    class ExportedFunctions {
    public:
        void addPrefix(const std::string& prefix) {
            mPrefixes.insert(prefix);
        }
        void addSuffix(const std::string& suffix) {
            mSuffixes.insert(suffix);
        }
        bool isPrefix(const std::string& prefix) const {
            return (mPrefixes.find(prefix) != mPrefixes.end());
        }
        bool isSuffix(const std::string& suffix) const {
            return (mSuffixes.find(suffix) != mSuffixes.end());
        }

    private:
        std::set<std::string> mPrefixes;
        std::set<std::string> mSuffixes;
    };
    class CodeBlock {
    public:
        CodeBlock() : mOffset(0) {}

        void setStart(const char* s) {
            mStart = s;
        }
        void setEnd(const char* e) {
            mEnd = e;
        }
        void setOffset(const int o) {
            mOffset = o;
        }
        void addBlock(const char* blockName) {
            mBlocks.insert(blockName);
        }
        const std::string& start() const {
            return mStart;
        }
        const std::string& end() const {
            return mEnd;
        }
        int offset() const {
            return mOffset;
        }
        bool isBlock(const std::string& blockName) const {
            return mBlocks.find(blockName) != mBlocks.end();
        }

    private:
        std::string mStart;
        std::string mEnd;
        int mOffset;
        std::set<std::string> mBlocks;
    };
    int mAllocId;
    std::set<std::string> mFiles;
    std::map<std::string, AllocFunc> mAlloc; // allocation functions
    std::map<std::string, AllocFunc> mDealloc; // deallocation functions
    std::map<std::string, bool> mNoReturn; // is function noreturn?
    std::map<std::string, std::string> mReturnValue;
    std::map<std::string, std::string> mReturnValueType;
    std::map<std::string, bool> mReportErrors;
    std::map<std::string, bool> mProcessAfterCode;
    std::set<std::string> mMarkupExtensions; // file extensions of markup files
    std::map<std::string, std::set<std::string> > mKeywords; // keywords for code in the library
    std::map<std::string, CodeBlock> mExecutableBlocks; // keywords for blocks of executable code
    std::map<std::string, ExportedFunctions> mExporters; // keywords that export variables/functions to libraries (meta-code/macros)
    std::map<std::string, std::set<std::string> > mImporters; // keywords that import variables/functions
    std::map<std::string, int> mReflection; // invocation of reflection
    std::map<std::string, struct PodType> mPodTypes; // pod types
    std::map<std::string, PlatformType> mPlatformTypes; // platform independent typedefs
    std::map<std::string, Platform> mPlatforms; // platform dependent typedefs

    const ArgumentChecks * getarg(const Token *ftok, int argnr) const;

    std::string getFunctionName(const Token *ftok, bool *error) const;

    static const AllocFunc* getAllocDealloc(const std::map<std::string, AllocFunc> &data, const std::string &name) {
        const std::map<std::string, AllocFunc>::const_iterator it = data.find(name);
        return (it == data.end()) ? nullptr : &it->second;
    }
};

/// @}
//---------------------------------------------------------------------------
#endif // libraryH
