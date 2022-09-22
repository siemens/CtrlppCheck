#include "cppcheck.h"

#include "check.h"
#include "checkunusedfunctions.h"
#include "ctu.h"
#include "library.h"
#include "mathlib.h"
#include "path.h"
#include "platform.h"
#include "preprocessor.h" // Preprocessor
#include "suppressions.h"
#include "timer.h"
#include "token.h"
#include "tokenize.h" // Tokenizer
#include "tokenlist.h"
#include "version.h"

#include <simplecpp.h>
#include <tinyxml2.h>
#include <algorithm>
#include <cstring>
#include <new>
#include <set>
#include <stdexcept>
#include <vector>

#include <regex>

#include <iostream>
#include <cstdio>


static const char Version[] = FULL_VERSION_STRING;
static const char ExtraVersion[] = "";

static TimerResults S_timerResults;

// CWE ids used
static const CWE CWE398(398U);  // Indicator of Poor Code Quality

//----------------------------------------------------------------------------------------------------------------------------------------
static std::vector<std::string> split(const std::string &str, const std::string &sep)
{
    std::vector<std::string> ret;
    for (std::string::size_type defineStartPos = 0U; defineStartPos < str.size();) {
        const std::string::size_type defineEndPos = str.find(sep, defineStartPos);
        ret.push_back((defineEndPos == std::string::npos) ? str.substr(defineStartPos) : str.substr(defineStartPos, defineEndPos - defineStartPos));
        if (defineEndPos == std::string::npos)
            break;
        defineStartPos = defineEndPos + 1U;
    }
    return ret;
}

//----------------------------------------------------------------------------------------------------------------------------------------
CppCheck::CppCheck(ErrorLogger &errorLogger, bool useGlobalSuppressions)
    : mErrorLogger(errorLogger), mExitCode(0), mSuppressInternalErrorFound(false), mUseGlobalSuppressions(useGlobalSuppressions), mSimplify(true)
{
}

//----------------------------------------------------------------------------------------------------------------------------------------
CppCheck::~CppCheck()
{
    while (!mFileInfo.empty()) {
        delete mFileInfo.back();
        mFileInfo.pop_back();
    }
    S_timerResults.ShowResults(mSettings.showtime);
}

//----------------------------------------------------------------------------------------------------------------------------------------
const char * CppCheck::version()
{
    return Version;
}

//----------------------------------------------------------------------------------------------------------------------------------------
const char * CppCheck::extraVersion()
{
    return ExtraVersion;
}

//----------------------------------------------------------------------------------------------------------------------------------------
unsigned int CppCheck::check(const std::string &path)
{
    std::ifstream fin(path);
    return checkFile(Path::simplifyPath(path), emptyString, fin);
}

//----------------------------------------------------------------------------------------------------------------------------------------
unsigned int CppCheck::check(const std::string &path, const std::string &content)
{
    std::istringstream iss(content);
    return checkFile(Path::simplifyPath(path), emptyString, iss);
}

//----------------------------------------------------------------------------------------------------------------------------------------
unsigned int CppCheck::check(const ImportProject::FileSettings &fs)
{
    CppCheck temp(mErrorLogger, mUseGlobalSuppressions);
    temp.mSettings = mSettings;
    temp.mSettings.includePaths = fs.includePaths;
    if (fs.platformType != Settings::Unspecified) {
        temp.mSettings.platform(fs.platformType);
    }
    std::ifstream fin(fs.filename);
    return temp.checkFile(Path::simplifyPath(fs.filename), fs.cfg, fin);
}

//----------------------------------------------------------------------------------------------------------------------------------------
size_t CppCheck::getCountOfLines(std::istream &is)
{
    // skip when bad
    if (is.bad())
        return 0;
    // save state
    std::istream::iostate state_backup = is.rdstate();
    // clear state
    is.clear();
    std::streampos pos_backup = is.tellg();

    is.seekg(0);
    size_t line_cnt;
    size_t lf_cnt = std::count(std::istreambuf_iterator<char>(is), std::istreambuf_iterator<char>(), '\n');
    line_cnt = lf_cnt;
    // if the file is not end with '\n' , then line_cnt should plus 1  
    is.unget();
    if (is.get() != '\n') { ++line_cnt; }

    // recover state
    is.clear(); // previous reading may set eofbit
    is.seekg(pos_backup);
    is.setstate(state_backup);

    return line_cnt;
}

//----------------------------------------------------------------------------------------------------------------------------------------
unsigned int CppCheck::checkFile(const std::string& filename, const std::string &cfgname, std::istream& fileStream)
{   
    mExitCode = 0;
    mSuppressInternalErrorFound = false;

    // only show debug warnings for accepted C/C++ source files
    if (!Path::acceptFile(filename))
        mSettings.debugwarnings = false;

    if (mSettings.terminated())
        return mExitCode;

    if (!mSettings.quiet) {
        std::string fixedpath = Path::simplifyPath(filename);
        fixedpath = Path::toNativeSeparators(fixedpath);
        mErrorLogger.reportOut(std::string("Checking ") + fixedpath + " cfgname: " + cfgname + std::string("..."));

        if (mSettings.verbose) {
            std::string includePaths;
            for (const std::string &I : mSettings.includePaths)
                includePaths += " -I" + I;
            mErrorLogger.reportOut("Includes:" + includePaths);
            mErrorLogger.reportOut(std::string("Platform:") + mSettings.platformString());
        }
    }

    CheckUnusedFunctions checkUnusedFunctions(nullptr, nullptr, nullptr);

    bool internalErrorFound(false);
    try {
        Preprocessor preprocessor(mSettings, this);
        std::set<std::string> configurations;

        simplecpp::OutputList outputList;
        std::vector<std::string> files;
        simplecpp::TokenList tokens1(fileStream, files, filename, &outputList);

        // If there is a syntax error, report it and stop
        for (const simplecpp::Output &output : outputList) {
            bool err;
            switch (output.type) {
            case simplecpp::Output::ERROR:
            case simplecpp::Output::INCLUDE_NESTED_TOO_DEEPLY:
            case simplecpp::Output::SYNTAX_ERROR:
            case simplecpp::Output::UNHANDLED_CHAR_ERROR:
                err = true;
                break;
            case simplecpp::Output::WARNING:
            case simplecpp::Output::MISSING_HEADER:
            case simplecpp::Output::PORTABILITY_BACKSLASH:
                err = false;
                break;
            };

            if (err) {
                const ErrorLogger::ErrorMessage::FileLocation loc1(output.location.file(), output.location.line);
                std::list<ErrorLogger::ErrorMessage::FileLocation> callstack(1, loc1);

                ErrorLogger::ErrorMessage errmsg(callstack,
                                                 "",
                                                 Severity::error,
                                                 output.msg,
                                                 "syntaxError",
                                                 false);
                reportErr(errmsg);
                return mExitCode;
            }
        }

        preprocessor.loadFiles(tokens1, files);

        // write dump file xml prolog
        std::ofstream fdump;
        if (mSettings.dump) {
            const std::string dumpfile(mSettings.dumpFile.empty() ? (filename + ".dump") : mSettings.dumpFile);
            fdump.open(dumpfile);
            if (fdump.is_open()) {
                fdump << "<?xml version=\"1.0\"?>" << std::endl;
                fdump << "<dumps>" << std::endl;
                fdump << "  <platform"
                      << " name=\"" << mSettings.platformString() << '\"'
                      << " char_bit=\"" << mSettings.char_bit << '\"'
                      << " short_bit=\"" << mSettings.short_bit << '\"'
                      << " int_bit=\"" << mSettings.int_bit << '\"'
                      << " long_bit=\"" << mSettings.long_bit << '\"'
                      << " long_long_bit=\"" << mSettings.long_long_bit << '\"'
                      << " pointer_bit=\"" << (mSettings.sizeof_pointer * mSettings.char_bit) << '\"'
                      << "/>\n";
                fdump << "  <rawtokens>" << std::endl;
                for (unsigned int i = 0; i < files.size(); ++i)
                    fdump << "    <file index=\"" << i << "\" name=\"" << ErrorLogger::toxml(files[i]) << "\"/>" << std::endl;
                for (const simplecpp::Token *tok = tokens1.cfront(); tok; tok = tok->next) {
                    fdump << "    <tok "
                          << "fileIndex=\"" << tok->location.fileIndex << "\" "
                          << "linenr=\"" << tok->location.line << "\" "
                          << "str=\"" << ErrorLogger::toxml(tok->str()) << "\""
                          << "/>" << std::endl;
                }
                fdump << "  </rawtokens>" << std::endl;
            }
        }

        // Parse comments and then remove them
        preprocessor.inlineSuppressions(tokens1);
        if (mSettings.dump && fdump.is_open()) {
            mSettings.nomsg.dump(fdump);
        }
        tokens1.removeComments();
        preprocessor.removeComments();

        // Get directives
        preprocessor.setDirectives(tokens1);
        preprocessor.simplifyPragmaAsm(&tokens1);

        preprocessor.setPlatformInfo(&tokens1);

        Timer t("Preprocessor::getConfigs", mSettings.showtime, &S_timerResults);
        configurations = preprocessor.getConfigs(tokens1);

        if (mSettings.checkConfiguration) {
            for (const std::string &config : configurations)
                (void)preprocessor.getcode(tokens1, config, files, true);

            return 0;
        }

        // Run define rules on raw code
        for (const Settings::Rule &rule : mSettings.rules) {
            if (rule.tokenlist != "define")
                continue;

            std::string code;
            const std::list<Directive> &directives = preprocessor.getDirectives();
            for (const Directive &dir : directives) {
                if (dir.str.compare(0,8,"#define ") == 0)
                    code += "#line " + MathLib::toString(dir.linenr) + " \"" + dir.file + "\"\n" + dir.str + '\n';
            }
            Tokenizer tokenizer2(&mSettings, this);
            std::istringstream istr2(code);
            tokenizer2.list.createTokens(istr2);
            executeRules("define", tokenizer2);
            break;
        }

        std::set<unsigned long long> checksums;
        unsigned int checkCount = 0;
        bool hasValidConfig = false;
        std::list<std::string> configurationError;
        for (const std::string &currCfg : configurations) {
            // bail out if terminated
            if (mSettings.terminated())
                break;

            // Check only a few configurations (default 12), after that bail out.
            if (++checkCount > mSettings.maxConfigs)
                break;

            mCurrentConfig = currCfg;


            if (mSettings.preprocessOnly) {
                Timer t("Preprocessor::getcode", mSettings.showtime, &S_timerResults);
                std::string codeWithoutCfg = preprocessor.getcode(tokens1, mCurrentConfig, files, true);
                t.Stop();

                if (codeWithoutCfg.compare(0,5,"#file") == 0)
                    codeWithoutCfg.insert(0U, "//");
                std::string::size_type pos = 0;
                while ((pos = codeWithoutCfg.find("\n#file",pos)) != std::string::npos)
                    codeWithoutCfg.insert(pos+1U, "//");
                pos = 0;
                while ((pos = codeWithoutCfg.find("\n#endfile",pos)) != std::string::npos)
                    codeWithoutCfg.insert(pos+1U, "//");
                pos = 0;
                reportOut(codeWithoutCfg);
                continue;
            }

            Tokenizer mTokenizer(&mSettings, this);
            if (mSettings.showtime != SHOWTIME_NONE)
                mTokenizer.setTimerResults(&S_timerResults);

            try {
                bool result;

                // Create tokens, skip rest of iteration if failed
                Timer timer("Tokenizer::createTokens", mSettings.showtime, &S_timerResults);
                const simplecpp::TokenList &tokensP = preprocessor.preprocess(tokens1, mCurrentConfig, files, true);
                mTokenizer.createTokens(&tokensP);
                timer.Stop();
                hasValidConfig = true;

                // If only errors are printed, print filename after the check
                if (!mSettings.quiet && (!mCurrentConfig.empty() || checkCount > 1)) {
                    std::string fixedpath = Path::simplifyPath(filename);
                    fixedpath = Path::toNativeSeparators(fixedpath);
                    mErrorLogger.reportOut("Checking " + fixedpath + ": " + mCurrentConfig + "...");
                }

                if (tokensP.empty())
                    continue;

                // skip rest of iteration if just checking configuration
                if (mSettings.checkConfiguration)
                    continue;

                // Check raw tokens
                checkRawTokens(mTokenizer);

                // Simplify tokens into normal form, skip rest of iteration if failed
                Timer timer2("Tokenizer::simplifyTokens1", mSettings.showtime, &S_timerResults);
                result = mTokenizer.simplifyTokens1(mCurrentConfig);
                timer2.Stop();
                if (!result)
                    continue;

                // dump xml if --dump
                if (mSettings.dump && fdump.is_open()) {
                    fdump << "<dump cfg=\"" << ErrorLogger::toxml(mCurrentConfig) << "\">" << std::endl;
                    preprocessor.dump(fdump);
                    mTokenizer.dump(fdump);
                    fdump << "</dump>" << std::endl;
                }

                // Skip if we already met the same simplified token list
                if (mSettings.maxConfigs > 1) {
                    const unsigned long long checksum = mTokenizer.list.calculateChecksum();
                    if (checksums.find(checksum) != checksums.end()) {
                        if (mSettings.debugwarnings)
                            purgedConfigurationMessage(filename, mCurrentConfig);
                        continue;
                    }
                    checksums.insert(checksum);
                }

                // Check normal tokens
                checkNormalTokens(mTokenizer);

                // simplify more if required, skip rest of iteration if failed
                if (mSimplify) {
                    if (!mSettings.experimentalFast) {
                        // if further simplification fails then skip rest of iteration
                        Timer timer3("Tokenizer::simplifyTokenList2", mSettings.showtime, &S_timerResults);
                        result = mTokenizer.simplifyTokenList2();
                        timer3.Stop();
                        if (!result)
                            continue;
                    }

                    // Check simplified tokens
                    checkSimplifiedTokens(mTokenizer);
                }

            } catch (const simplecpp::Output &o) {
                // #error etc during preprocessing
                configurationError.push_back((mCurrentConfig.empty() ? "\'\'" : mCurrentConfig) + " : [" + o.location.file() + ':' + MathLib::toString(o.location.line) + "] " + o.msg);
                --checkCount; // don't count invalid configurations
                continue;

            } catch (const InternalError &e) {
                std::list<ErrorLogger::ErrorMessage::FileLocation> locationList;
                ErrorLogger::ErrorMessage::FileLocation loc;
                if (e.token) {
                    loc.line = e.token->linenr();
                    loc.col = e.token->col();
                    const std::string fixedpath = Path::toNativeSeparators(mTokenizer.list.file(e.token));
                    loc.setfile(fixedpath);
                } else {
                    ErrorLogger::ErrorMessage::FileLocation loc2;
                    loc2.setfile(Path::toNativeSeparators(filename));
                    locationList.push_back(loc2);
                    loc.setfile(mTokenizer.list.getSourceFilePath());
                }
                locationList.push_back(loc);
                ErrorLogger::ErrorMessage errmsg(locationList,
                                                 mTokenizer.list.getSourceFilePath(),
                                                 Severity::error,
                                                 e.errorMessage,
                                                 e.id,
                                                 false);

                if (errmsg._severity == Severity::error || mSettings.isEnabled(errmsg._severity)) {
                    reportErr(errmsg);
                    if (!mSuppressInternalErrorFound)
                        internalErrorFound = true;
                }
            } catch (const std::exception& e) { // reference to the base of a polymorphic object
                std::cout << "runCheckException: " << e.what() << std::endl; // information from length_error printed
            }
        }

        if (!hasValidConfig && configurations.size() > 1 && mSettings.isEnabled(Settings::INFORMATION)) {
            std::string msg;
            msg = "This file is not analyzed. Cppcheck failed to extract a valid configuration. Use -v for more details.";
            msg += "\nThis file is not analyzed. Cppcheck failed to extract a valid configuration. The tested configurations have these preprocessor errors:";
            for (const std::string &s : configurationError)
                msg += '\n' + s;

            std::list<ErrorLogger::ErrorMessage::FileLocation> locationList;
            ErrorLogger::ErrorMessage::FileLocation loc;
            loc.setfile(Path::toNativeSeparators(filename));
            locationList.push_back(loc);
            ErrorLogger::ErrorMessage errmsg(locationList,
                                             loc.getfile(),
                                             Severity::information,
                                             msg,
                                             "noValidConfiguration",
                                             false);
            reportErr(errmsg);
        }

        // dumped all configs, close root </dumps> element now
        if (mSettings.dump && fdump.is_open())
            fdump << "</dumps>" << std::endl;

    } catch (const std::runtime_error &e) {
        internalError(filename, e.what());
    } catch (const std::bad_alloc &e) {
        internalError(filename, e.what());
    } catch (const InternalError &e) {
        internalError(filename, e.errorMessage);
        mExitCode=1; // e.g. reflect a syntax error
    }

    // In jointSuppressionReport mode, unmatched suppressions are
    // collected after all files are processed
    if (!mSettings.jointSuppressionReport && (mSettings.isEnabled(Settings::INFORMATION) || mSettings.checkConfiguration)) {
        reportUnmatchedSuppressions(mSettings.nomsg.getUnmatchedLocalSuppressions(filename, isUnusedFunctionCheckEnabled()));
    }

    mErrorList.clear();
    if (internalErrorFound && (mExitCode==0)) {
        mExitCode = 1;
    }

    return mExitCode;
}

//----------------------------------------------------------------------------------------------------------------------------------------
void CppCheck::internalError(const std::string &filename, const std::string &msg)
{
    const std::string fixedpath = Path::toNativeSeparators(filename);
    const std::string fullmsg("Bailing out from checking " + fixedpath + " since there was an internal error: " + msg);

    if (mSettings.isEnabled(Settings::INFORMATION)) {
        const ErrorLogger::ErrorMessage::FileLocation loc1(filename, 0);
        std::list<ErrorLogger::ErrorMessage::FileLocation> callstack(1, loc1);

        ErrorLogger::ErrorMessage errmsg(callstack,
                                         emptyString,
                                         Severity::information,
                                         fullmsg,
                                         "internalError",
                                         false);

        mErrorLogger.reportErr(errmsg);
    } else {
        // Report on stdout
        mErrorLogger.reportOut(fullmsg);
    }
}

//---------------------------------------------------------------------------
// CppCheck - A function that checks a raw token list
//---------------------------------------------------------------------------
void CppCheck::checkRawTokens(const Tokenizer &tokenizer)
{
    // Execute rules for "raw" code
    executeRules("raw", tokenizer);
}

//---------------------------------------------------------------------------
// CppCheck - A function that checks a normal token list
//---------------------------------------------------------------------------

void CppCheck::checkNormalTokens(const Tokenizer &tokenizer)
{
    // call all "runChecks" in all registered Check classes
    for (Check *check : Check::instances()) {
        if (mSettings.terminated())
            return;

        if (tokenizer.isMaxTime())
            return;

        Timer timerRunChecks(check->name() + "::runChecks", mSettings.showtime, &S_timerResults);
        check->runChecks(&tokenizer, &mSettings, this);
    }

    // Analyse the tokens..

    CTU::FileInfo *fi1 = CTU::getFileInfo(&tokenizer);
    if (fi1) {
        mFileInfo.push_back(fi1);
    }

    for (const Check *check : Check::instances()) {
        Check::FileInfo *fi = check->getFileInfo(&tokenizer, &mSettings);
        if (fi != nullptr) {
            mFileInfo.push_back(fi);
        }
    }

    executeRules("normal", tokenizer);
}

//---------------------------------------------------------------------------
// CppCheck - A function that checks a simplified token list
//---------------------------------------------------------------------------

void CppCheck::checkSimplifiedTokens(const Tokenizer &tokenizer)
{
    // call all "runSimplifiedChecks" in all registered Check classes
    for (std::list<Check *>::const_iterator it = Check::instances().begin(); it != Check::instances().end(); ++it) {
        if (mSettings.terminated())
            return;

        if (tokenizer.isMaxTime())
            return;

        Timer timerSimpleChecks((*it)->name() + "::runSimplifiedChecks", mSettings.showtime, &S_timerResults);
        (*it)->runSimplifiedChecks(&tokenizer, &mSettings, this);
        timerSimpleChecks.Stop();
    }

    if (!mSettings.terminated())
        executeRules("simple", tokenizer);
}

//----------------------------------------------------------------------------------------------------------------------------------------
void CppCheck::executeRules(const std::string &tokenlist, const Tokenizer &tokenizer)
{
  (void)tokenlist;
  (void)tokenizer;

  // Step 1:
  // Are there rules to execute?
  
  // check if some rule exist, performance , otherwise is unnecessary genereated source from tokenize in next step.
  bool isrule = false;
  for (std::list<Settings::Rule>::const_iterator it = mSettings.rules.begin(); it != mSettings.rules.end(); ++it) {
    if (it->tokenlist == tokenlist)
    {
      isrule = true;
      break; // fine, just break here. Some one rules is founded
    }
  }


  if ( mSettings.verbose )
    mErrorLogger.reportOut("Tokenlist:" + tokenlist);

  // There is no rule to execute
  if (isrule == false)
  {
    const ErrorLogger::ErrorMessage errmsg(std::list<ErrorLogger::ErrorMessage::FileLocation>(),
        emptyString,
        Severity::error,
        "There is no rule to execute. Tokenlist: " + tokenlist,
        "ruleCheck_noRule",
        false);

    reportErr(errmsg);

    return;
  }

  // Step 2:
  // Write all tokens in a string that can be parsed by regExp
  std::ostringstream ostr;
  for (const Token *tok = tokenizer.tokens(); tok; tok = tok->next())
       ostr << " " << tok->str();
  std::string str(ostr.str());

  if (mSettings.verbose)
    mErrorLogger.reportOut("source:" + str);

  // Step 3:
  // check the source for rules
  for (std::list<Settings::Rule>::const_iterator it = mSettings.rules.begin(); it != mSettings.rules.end(); ++it)
  {
    const Settings::Rule &rule = *it;

    if ( (rule.severity == Severity::none) || (rule.tokenlist != tokenlist) )
      continue; // just ignore the rule
    
    // is the rule definition valid?
    if (rule.pattern.empty() || rule.id.empty())
    {
      const ErrorLogger::ErrorMessage errmsg(std::list<ErrorLogger::ErrorMessage::FileLocation>(),
        emptyString,
        Severity::error,
        std::string("The rule pattern or id is empty"),
        "ruleCheck",
        false);
    
      reportErr(errmsg);
      continue;
    }


    if ( mSettings.verbose )
        mErrorLogger.reportOut("pattern:" + rule.pattern);
    
    // Step 4:
    // do regExp
    try
    { 
      std::regex r(rule.pattern.c_str());
      std::smatch match;
    
      unsigned int lastPos = 0;
    
      while (regex_search(str, match, r))
      {
        /// @todo calculate correct line position
        const unsigned int pos1 = match.position(0) + 1;
        lastPos += pos1;
    
        // determine location..
        ErrorLogger::ErrorMessage::FileLocation loc;
        loc.setfile(tokenizer.list.getSourceFilePath());
        loc.line = 0;
    
        std::size_t len = 0;
        for (const Token *tok = tokenizer.tokens(); tok; tok = tok->next()) {
          len = len + 1U + tok->str().size();
          if (len > lastPos) {
            loc.setfile(tokenizer.list.getFiles().at(tok->fileIndex()));
            loc.line = tok->linenr();
            break;
          }
        }
    
        const std::list<ErrorLogger::ErrorMessage::FileLocation> callStack(1, loc);
    
        // Create error message
        std::string summary = "$symbol:" + match.str() + "\n";
        if (rule.summary.empty())
          summary += "found '" + std::string(match.str()) + "'";
        else
          summary += rule.summary;

        const ErrorLogger::ErrorMessage errmsg(callStack, tokenizer.list.getSourceFilePath(), rule.severity, summary, rule.id, false);
    
        // Report error
        reportErr(errmsg);
    
        // start again
        lastPos += match.length();
        str = match.suffix();  
      }
    }
    catch (const std::regex_error& e)
    {
        const ErrorLogger::ErrorMessage errmsg(std::list<ErrorLogger::ErrorMessage::FileLocation>(),
          emptyString,
          Severity::error,
          e.what() + std::string(" ID: ") + rule.id + std::string(", Pattern: ") + rule.pattern,
          "ruleCheck_regexError",
          false);
    
        reportErr(errmsg);
        continue;
    }
  }
}

//----------------------------------------------------------------------------------------------------------------------------------------
Settings &CppCheck::settings()
{
    return mSettings;
}

//----------------------------------------------------------------------------------------------------------------------------------------
void CppCheck::purgedConfigurationMessage(const std::string &file, const std::string& configuration)
{
    if (mSettings.isEnabled(Settings::INFORMATION) && file.empty())
        return;

    std::list<ErrorLogger::ErrorMessage::FileLocation> loclist;
    if (!file.empty()) {
        ErrorLogger::ErrorMessage::FileLocation location;
        location.setfile(file);
        loclist.push_back(location);
    }

    ErrorLogger::ErrorMessage errmsg(loclist,
                                     emptyString,
                                     Severity::information,
                                     "The configuration 'inconclusive" + configuration + "' was not checked because its code equals another one.",
                                     "purgedConfiguration",
                                     false);

    reportErr(errmsg);
}

//---------------------------------------------------------------------------

void CppCheck::reportErr(const ErrorLogger::ErrorMessage &msg)
{
    mSuppressInternalErrorFound = false;

    if (!mSettings.library.reportErrors(msg.file0))
        return;

    const std::string errmsg = msg.toString(mSettings.verbose);
    if (errmsg.empty())
        return;

  // make unique message, it works good wit verbose messages
  // otherwise are the regExp messages ignored
  std::string errmsgUnique = msg.toString(true);
  if (errmsgUnique == "")
    errmsgUnique = errmsg;

    // Alert only about unique errors
    if (std::find(mErrorList.begin(), mErrorList.end(), errmsgUnique) != mErrorList.end())
        return;

    const Suppressions::ErrorMessage errorMessage = msg.toSuppressionsErrorMessage();

    if (mUseGlobalSuppressions) {
        if (mSettings.nomsg.isSuppressed(errorMessage)) {
            mSuppressInternalErrorFound = true;
            return;
        }
    } else {
        if (mSettings.nomsg.isSuppressedLocal(errorMessage)) {
            mSuppressInternalErrorFound = true;
            return;
        }
    }

    if (!mSettings.nofail.isSuppressed(errorMessage) && (mUseGlobalSuppressions || !mSettings.nomsg.isSuppressed(errorMessage)))
        mExitCode = 1;

    mErrorList.push_back(errmsg);

    mErrorLogger.reportErr(msg);
}

void CppCheck::reportOut(const std::string &outmsg)
{
    mErrorLogger.reportOut(outmsg);
}

void CppCheck::reportProgress(const std::string &filename, const char stage[], const std::size_t value)
{
    mErrorLogger.reportProgress(filename, stage, value);
}

void CppCheck::reportInfo(const ErrorLogger::ErrorMessage &msg)
{
    const Suppressions::ErrorMessage &errorMessage = msg.toSuppressionsErrorMessage();
    if (!mSettings.nomsg.isSuppressed(errorMessage))
        mErrorLogger.reportInfo(msg);
}

void CppCheck::reportStatus(unsigned int /*fileindex*/, unsigned int /*filecount*/, std::size_t /*sizedone*/, std::size_t /*sizetotal*/)
{

}

void CppCheck::getErrorMessages()
{
    Settings s(mSettings);
    s.addEnabled("warning");
    s.addEnabled("style");
    s.addEnabled("portability");
    s.addEnabled("performance");
    s.addEnabled("information");

    purgedConfigurationMessage("","");

    // call all "getErrorMessages" in all registered Check classes
    for (std::list<Check *>::const_iterator it = Check::instances().begin(); it != Check::instances().end(); ++it)
        (*it)->getErrorMessages(this, &s);

    Preprocessor::getErrorMessages(this, &s);
}

bool CppCheck::analyseWholeProgram()
{
    bool errors = false;
    // Init CTU
    CTU::maxCtuDepth = mSettings.maxCtuDepth;
    // Analyse the tokens
    CTU::FileInfo ctu;
    for (const Check::FileInfo *fi : mFileInfo) {
        const CTU::FileInfo *fi2 = dynamic_cast<const CTU::FileInfo *>(fi);
        if (fi2) {
            ctu.functionCalls.insert(ctu.functionCalls.end(), fi2->functionCalls.begin(), fi2->functionCalls.end());
            ctu.nestedCalls.insert(ctu.nestedCalls.end(), fi2->nestedCalls.begin(), fi2->nestedCalls.end());
        }
    }
    for (Check *check : Check::instances())
        errors |= check->analyseWholeProgram(&ctu, mFileInfo, mSettings, *this);  // TODO: ctu
    return errors && (mExitCode > 0);
}

bool CppCheck::isUnusedFunctionCheckEnabled() const
{
    return mSettings.isEnabled(Settings::UNUSED_FUNCTION);
}
