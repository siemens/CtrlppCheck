//---------------------------------------------------------------------------
#ifndef cppcheckH
#define cppcheckH
//---------------------------------------------------------------------------


#include "check.h"
#include "config.h"
#include "errorlogger.h"
#include "importproject.h"
#include "settings.h"

#include <cstddef>
#include <istream>
#include <list>
#include <map>
#include <string>

class Tokenizer;

/// @addtogroup Core
/// @{

/**
 * @brief This is the base class which will use other classes to do
 * static code analysis for C and C++ code to find possible
 * errors or places that could be improved.
 * Usage: See check() for more info.
 */
class CPPCHECKLIB CppCheck : ErrorLogger {
public:
    /**
     * @brief Constructor.
     */
    CppCheck(ErrorLogger &errorLogger, bool useGlobalSuppressions);

    /**
     * @brief Destructor.
     */
    virtual ~CppCheck();

    /**
     * @brief This starts the actual checking. Note that you must call
     * parseFromArgs() or settings() and addFile() before calling this.
     * @return amount of errors found or 0 if none were found.
     */

    /**
      * @brief Check the file.
      * This function checks one given file for errors.
      * @param path Path to the file to check.
      * @return amount of errors found or 0 if none were found.
      * @note You must set settings before calling this function (by calling
      *  settings()).
      */
    unsigned int check(const std::string &path);
    unsigned int check(const ImportProject::FileSettings &fs);

    /**
      * @brief Check the file.
      * This function checks one "virtual" file. The file is not read from
      * the disk but the content is given in @p content. In errors the @p path
      * is used as a filename.
      * @param path Path to the file to check.
      * @param content File content as a string.
      * @return amount of errors found or 0 if none were found.
      * @note You must set settings before calling this function (by calling
      *  settings()).
      */
    unsigned int check(const std::string &path, const std::string &content);

    /**
     * @brief Get reference to current settings.
     * @return a reference to current settings
     */
    Settings &settings();

    /**
     * @brief Returns current version number as a string.
     * @return version, e.g. "1.38"
     */
    static const char * version();

    /**
     * @brief Returns extra version info as a string.
     * This is for returning extra version info, like Git commit id, build
     * time/date etc.
     * @return extra version info, e.g. "04d42151" (Git commit id).
     */
    static const char * extraVersion();

    virtual void reportStatus(unsigned int fileindex, unsigned int filecount, std::size_t sizedone, std::size_t sizetotal);

    /**
     * @brief Terminate checking. The checking will be terminated as soon as possible.
     */
    void terminate() {
        mSettings.terminate();
    }

    /**
     * @brief Call all "getErrorMessages" in all registered Check classes.
     * Also print out XML header and footer.
     */
    void getErrorMessages();

    void tooManyConfigsError(const std::string &file, const std::size_t numberOfConfigurations);
    void purgedConfigurationMessage(const std::string &file, const std::string& configuration);

    void dontSimplify() {
        mSimplify = false;
    }

    /** Analyse whole program, run this after all TUs has been scanned.
     * This is deprecated and the plan is to remove this when
     * .analyzeinfo is good enough.
     * Return true if an error is reported.
     */
    bool analyseWholeProgram();

    /** Check if the user wants to check for unused functions
     * and if it's possible at all */
    bool isUnusedFunctionCheckEnabled() const;

private:

    size_t getCountOfLines(std::istream &is);

    /** @brief There has been an internal error => Report information message */
    void internalError(const std::string &filename, const std::string &msg);

    /**
     * @brief Check a file using stream
     * @param filename file name
     * @param cfgname  cfg name
     * @param fileStream stream the file content can be read from
     * @return number of errors found
     */
    unsigned int checkFile(const std::string& filename, const std::string &cfgname, std::istream& fileStream);

    /**
     * @brief Check raw tokens
     * @param tokenizer tokenizer instance
     */
    void checkRawTokens(const Tokenizer &tokenizer);

    /**
     * @brief Check normal tokens
     * @param tokenizer tokenizer instance
     */
    void checkNormalTokens(const Tokenizer &tokenizer);

    /**
     * @brief Check simplified tokens
     * @param tokenizer tokenizer instance
     */
    void checkSimplifiedTokens(const Tokenizer &tokenizer);

    /**
     * @brief Execute rules, if any
     * @param tokenlist token list to use (normal / simple)
     * @param tokenizer tokenizer
     */
    void executeRules(const std::string &tokenlist, const Tokenizer &tokenizer);

    /**
     * @brief Errors and warnings are directed here.
     *
     * @param msg Errors messages are normally in format
     * "[filepath:line number] Message", e.g.
     * "[main.cpp:4] Uninitialized member variable"
     */
    virtual void reportErr(const ErrorLogger::ErrorMessage &msg) OVERRIDE;

    /**
     * @brief Information about progress is directed here.
     *
     * @param outmsg Message to show, e.g. "Checking main.cpp..."
     */
    virtual void reportOut(const std::string &outmsg) OVERRIDE;

    std::list<std::string> mErrorList;
    Settings mSettings;

    void reportProgress(const std::string &filename, const char stage[], const std::size_t value) OVERRIDE;

    /**
     * Output information messages.
     */
    virtual void reportInfo(const ErrorLogger::ErrorMessage &msg) OVERRIDE;

    ErrorLogger &mErrorLogger;

    /** @brief Current preprocessor configuration */
    std::string mCurrentConfig;

    unsigned int mExitCode;

    bool mSuppressInternalErrorFound;

    bool mUseGlobalSuppressions;

    /** Simplify code? true by default */
    bool mSimplify;

    /** File info used for whole program analysis */
    std::list<Check::FileInfo*> mFileInfo;
};

/// @}
//---------------------------------------------------------------------------
#endif // cppcheckH
