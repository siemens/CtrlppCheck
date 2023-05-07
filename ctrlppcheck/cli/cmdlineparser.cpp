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

#include "cmdlineparser.h"

#include "check.h"
#include "cppcheckexecutor.h"
#include "filelister.h"
#include "importproject.h"
#include "path.h"
#include "platform.h"
#include "settings.h"
#include "suppressions.h"
#include "timer.h"
#include "utils.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib> // EXIT_FAILURE
#include <cstring>
#include <iostream>
#include <list>
#include <set>



static void addFilesToList(const std::string& FileList, std::vector<std::string>& PathNames)
{
    // To keep things initially simple, if the file can't be opened, just be silent and move on.
    std::istream *Files;
    std::ifstream Infile;
    if (FileList == "-") { // read from stdin
        Files = &std::cin;
    } else {
        Infile.open(FileList);
        Files = &Infile;
    }
    if (Files && *Files) {
        std::string FileName;
        while (std::getline(*Files, FileName)) { // next line
            if (!FileName.empty()) {
                PathNames.push_back(FileName);
            }
        }
    }
}

static bool addIncludePathsToList(const std::string& FileList, std::list<std::string>* PathNames)
{
    std::ifstream Files(FileList);
    if (Files) {
        std::string PathName;
        while (std::getline(Files, PathName)) { // next line
            if (!PathName.empty()) {
                PathName = Path::removeQuotationMarks(PathName);
                PathName = Path::fromNativeSeparators(PathName);

                // If path doesn't end with / or \, add it
                if (!endsWith(PathName, '/'))
                    PathName += '/';

                PathNames->push_back(PathName);
            }
        }
        return true;
    }
    return false;
}

static bool addPathsToSet(const std::string& FileName, std::set<std::string>* set)
{
    std::list<std::string> templist;
    if (!addIncludePathsToList(FileName, &templist))
        return false;
    set->insert(templist.begin(), templist.end());
    return true;
}

CmdLineParser::CmdLineParser(Settings *settings)
    : mSettings(settings)
    , mShowHelp(false)
    , mShowVersion(false)
    , mShowErrorMessages(false)
    , mExitAfterPrint(false)
{
}

void CmdLineParser::printMessage(const std::string &message)
{
    std::cout << message << std::endl;
}

void CmdLineParser::printMessage(const char* message)
{
    std::cout << message << std::endl;
}

bool CmdLineParser::parseFromArgs(int argc, const char* const argv[])
{
    bool def = false;
    bool maxconfigs = false;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (std::strcmp(argv[i], "--version") == 0) {
                mShowVersion = true;
                mExitAfterPrint = true;
                return true;
            }

            else if (std::strncmp(argv[i], "--winccoa-projectName=", 22) == 0) {
                std::string str = argv[i] + 22;
                mSettings->projectName = str;
                if (mSettings->projectName.empty())
                {
                printMessage("No WinCC OA project name givent to '--winccoa-projectName' option.");
                return false;
                }
            }

            // Flag used for various purposes during debugging
            else if (std::strcmp(argv[i], "--debug-simplified") == 0)
                mSettings->debugSimplified = true;

            // Show --debug output after the first simplifications
            else if (std::strcmp(argv[i], "--debug") == 0 ||
                     std::strcmp(argv[i], "--debug-normal") == 0)
                mSettings->debugnormal = true;

            // Show debug warnings
            else if (std::strcmp(argv[i], "--debug-warnings") == 0)
                mSettings->debugwarnings = true;

            // Show template information
            else if (std::strcmp(argv[i], "--debug-template") == 0)
                mSettings->debugtemplate = true;

            // dump cppcheck data
            else if (std::strcmp(argv[i], "--dump") == 0)
                mSettings->dump = true;
            else if (std::strncmp(argv[i], "--dump-file=", 12) == 0)
            {
              mSettings->dumpFile = Path::simplifyPath(Path::fromNativeSeparators(argv[i] + 12));
              mSettings->dump = true;
            }
            // max ctu depth
            else if (std::strncmp(argv[i], "--max-ctu-depth=", 16) == 0)
                mSettings->maxCtuDepth = std::atoi(argv[i] + 16);

            else if (std::strcmp(argv[i], "--experimental-fast") == 0)
                // Skip slow simplifications and see how that affect the results, the
                // goal is to remove the simplifications.
                mSettings->experimentalFast = true;

            // (Experimental) exception handling inside cppcheck client
            else if (std::strcmp(argv[i], "--exception-handling") == 0)
                mSettings->exceptionHandling = true;
            else if (std::strncmp(argv[i], "--exception-handling=", 21) == 0) {
                mSettings->exceptionHandling = true;
                const std::string exceptionOutfilename = &(argv[i][21]);
                CppCheckExecutor::setExceptionOutput((exceptionOutfilename=="stderr") ? stderr : stdout);
            }

            // Inconclusive checking
            else if (std::strcmp(argv[i], "--inconclusive") == 0)
                mSettings->inconclusive = true;

            // Enforce language (--language=, -x)
            else if (std::strncmp(argv[i], "--language=", 11) == 0 || std::strcmp(argv[i], "-x") == 0) {
                std::string str;
                if (argv[i][2]) {
                    str = argv[i]+11;
                } else {
                    i++;
                    if (i >= argc || argv[i][0] == '-') {
                        printMessage("cppcheck: No language given to '-x' option.");
                        return false;
                    }
                    str = argv[i];
                }

                if (str == "ctrl")
                    mSettings->enforcedLang = Settings::CTRL;
                else {
                    printMessage("cppcheck: Unknown language '" + str + "' enforced.");
                    return false;
                }
            }

            // Filter errors
            else if (std::strncmp(argv[i], "--exitcode-suppressions=", 24) == 0) {
                // exitcode-suppressions=filename.txt
                std::string filename = 24 + argv[i];

                std::ifstream f(filename);
                if (!f.is_open()) {
                    printMessage("cppcheck: Couldn't open the file: \"" + filename + "\".");
                    return false;
                }
                const std::string errmsg(mSettings->nofail.parseFile(f));
                if (!errmsg.empty()) {
                    printMessage(errmsg);
                    return false;
                }
            }

            // Filter errors
            else if (std::strncmp(argv[i], "--suppressions-list=", 20) == 0) {
                std::string filename = argv[i]+20;
                std::ifstream f(filename);
                if (!f.is_open()) {
                    std::string message("cppcheck: Couldn't open the file: \"");
                    message += filename;
                    message += "\".";
                    if (std::count(filename.begin(), filename.end(), ',') > 0 ||
                        std::count(filename.begin(), filename.end(), '.') > 1) {
                        // If user tried to pass multiple files (we can only guess that)
                        // e.g. like this: --suppressions-list=a.txt,b.txt
                        // print more detailed error message to tell user how he can solve the problem
                        message += "\nIf you want to pass two files, you can do it e.g. like this:";
                        message += "\n    cppcheck --suppressions-list=a.txt --suppressions-list=b.txt file.cpp";
                    }

                    printMessage(message);
                    return false;
                }
                const std::string errmsg(mSettings->nomsg.parseFile(f));
                if (!errmsg.empty()) {
                    printMessage(errmsg);
                    return false;
                }
            }

            else if (std::strncmp(argv[i], "--suppress-xml=", 15) == 0) {
                const char * filename = argv[i] + 15;
                const std::string errmsg(mSettings->nomsg.parseXmlFile(filename));
                if (!errmsg.empty()) {
                    printMessage(errmsg);
                    return false;
                }
            }

            else if (std::strncmp(argv[i], "--suppress=", 11) == 0) {
                const std::string suppression = argv[i]+11;
                const std::string errmsg(mSettings->nomsg.addSuppressionLine(suppression));
                if (!errmsg.empty()) {
                    printMessage(errmsg);
                    return false;
                }
            }

            // Enables inline suppressions.
            else if (std::strcmp(argv[i], "--inline-suppr") == 0)
                mSettings->inlineSuppressions = true;

            // Verbose error messages (configuration info)
            else if (std::strcmp(argv[i], "-v") == 0 || std::strcmp(argv[i], "--verbose") == 0)
                mSettings->verbose = true;

            // Output relative paths
            else if (std::strcmp(argv[i], "-rp") == 0 || std::strcmp(argv[i], "--relative-paths") == 0)
                mSettings->relativePaths = true;
            else if (std::strncmp(argv[i], "-rp=", 4) == 0 || std::strncmp(argv[i], "--relative-paths=", 17) == 0) {
                mSettings->relativePaths = true;
                if (argv[i][argv[i][3]=='='?4:17] != 0) {
                    std::string paths = argv[i]+(argv[i][3]=='='?4:17);
                    for (;;) {
                        const std::string::size_type pos = paths.find(';');
                        if (pos == std::string::npos) {
                            mSettings->basePaths.push_back(Path::fromNativeSeparators(paths));
                            break;
                        }
                        mSettings->basePaths.push_back(Path::fromNativeSeparators(paths.substr(0, pos)));
                        paths.erase(0, pos + 1);
                    }
                } else {
                    printMessage("cppcheck: No paths specified for the '" + std::string(argv[i]) + "' option.");
                    return false;
                }
            }

            // Write results in file
            else if (std::strncmp(argv[i], "--output-file=", 14) == 0)
                mSettings->outputFile = Path::simplifyPath(Path::fromNativeSeparators(argv[i] + 14));

            else if (std::strncmp(argv[i], "--naming-rule-file=", 19) == 0)
                mSettings->namingRuleFile = Path::simplifyPath(Path::fromNativeSeparators(argv[i] + 19));

            // Write results in results.xml
            else if (std::strcmp(argv[i], "--xml") == 0)
                mSettings->xml = true;

            // Define the XML file version (and enable XML output)
            else if (std::strncmp(argv[i], "--xml-version=", 14) == 0) {
                const std::string numberString(argv[i]+14);

                std::istringstream iss(numberString);
                if (!(iss >> mSettings->xml_version)) {
                    printMessage("cppcheck: argument to '--xml-version' is not a number.");
                    return false;
                }

                if (mSettings->xml_version != 2) {
                    // We only have xml version 2
                    printMessage("cppcheck: '--xml-version' can only be 2.");
                    return false;
                }

                // Enable also XML if version is set
                mSettings->xml = true;
            }

            // Only print something when there are errors
            else if (std::strcmp(argv[i], "-q") == 0 || std::strcmp(argv[i], "--quiet") == 0)
                mSettings->quiet = true;

            // Check configuration
            else if (std::strcmp(argv[i], "--check-config") == 0) {
                mSettings->checkConfiguration = true;
            }

            // Check library definitions
            else if (std::strcmp(argv[i], "--check-library") == 0) {
                mSettings->checkLibrary = true;
            }

            else if (std::strncmp(argv[i], "--enable=", 9) == 0) {
                const std::string errmsg = mSettings->addEnabled(argv[i] + 9);
                if (!errmsg.empty()) {
                    printMessage(errmsg);
                    return false;
                }
                // when "style" is enabled, also enable "warning", "performance" and "portability"
                if (mSettings->isEnabled(Settings::STYLE)) {
                    mSettings->addEnabled("warning");
                    mSettings->addEnabled("performance");
                    mSettings->addEnabled("portability");
                }
            }

            // --error-exitcode=1
            else if (std::strncmp(argv[i], "--error-exitcode=", 17) == 0) {
                const std::string temp = argv[i]+17;
                std::istringstream iss(temp);
                if (!(iss >> mSettings->exitCode)) {
                    mSettings->exitCode = 0;
                    printMessage("cppcheck: Argument must be an integer. Try something like '--error-exitcode=1'.");
                    return false;
                }
            }

            // -E
            else if (std::strcmp(argv[i], "-E") == 0) {
                mSettings->preprocessOnly = true;
            }

            // Include paths
            else if (std::strncmp(argv[i], "-I", 2) == 0) {
                std::string path;

                // "-I path/"
                if (std::strcmp(argv[i], "-I") == 0) {
                    ++i;
                    if (i >= argc || argv[i][0] == '-') {
                        printMessage("cppcheck: argument to '-I' is missing.");
                        return false;
                    }
                    path = argv[i];
                }

                // "-Ipath/"
                else {
                    path = 2 + argv[i];
                }
                path = Path::removeQuotationMarks(path);
                path = Path::fromNativeSeparators(path);

                // If path doesn't end with / or \, add it
                if (!endsWith(path,'/'))
                    path += '/';

                mSettings->includePaths.push_back(path);
            } else if (std::strncmp(argv[i], "--include=", 10) == 0) {
                std::string path = argv[i] + 10;

                path = Path::fromNativeSeparators(path);

                mSettings->userIncludes.push_back(path);
            } else if (std::strncmp(argv[i], "--includes-file=", 16) == 0) {
                // open this file and read every input file (1 file name per line)
                const std::string includesFile(16 + argv[i]);
                if (!addIncludePathsToList(includesFile, &mSettings->includePaths)) {
                    printMessage("Cppcheck: unable to open includes file at '" + includesFile + "'");
                    return false;
                }
            } else if (std::strncmp(argv[i], "--config-exclude=",17) ==0) {
                std::string path = argv[i] + 17;
                path = Path::fromNativeSeparators(path);
                mSettings->configExcludePaths.insert(path);
            } else if (std::strncmp(argv[i], "--config-excludes-file=", 23) == 0) {
                // open this file and read every input file (1 file name per line)
                const std::string cfgExcludesFile(23 + argv[i]);
                if (!addPathsToSet(cfgExcludesFile, &mSettings->configExcludePaths)) {
                    printMessage("Cppcheck: unable to open config excludes file at '" + cfgExcludesFile + "'");
                    return false;
                }
            }

            // file list specified
            else if (std::strncmp(argv[i], "--file-list=", 12) == 0) {
                // open this file and read every input file (1 file name per line)
                addFilesToList(12 + argv[i], mPathNames);
            }

            // Ignored paths
            else if (std::strncmp(argv[i], "-i", 2) == 0) {
                std::string path;

                // "-i path/"
                if (std::strcmp(argv[i], "-i") == 0) {
                    ++i;
                    if (i >= argc || argv[i][0] == '-') {
                        printMessage("cppcheck: argument to '-i' is missing.");
                        return false;
                    }
                    path = argv[i];
                }

                // "-ipath/"
                else {
                    path = 2 + argv[i];
                }

                if (!path.empty()) {
                    path = Path::removeQuotationMarks(path);
                    path = Path::fromNativeSeparators(path);
                    path = Path::simplifyPath(path);

                    if (FileLister::isDirectory(path)) {
                        // If directory name doesn't end with / or \, add it
                        if (!endsWith(path, '/'))
                            path += '/';
                    }
                    mIgnoredPaths.push_back(path);
                }
            }

            // --library
            else if (std::strncmp(argv[i], "--library=", 10) == 0) {
                if (!CppCheckExecutor::tryLoadLibrary(mSettings->library, argv[0], argv[i]+10))
                    return false;
            }

            // Report progress
            else if (std::strcmp(argv[i], "--report-progress") == 0) {
                mSettings->reportProgress = true;
            }

            // Output formatter
            else if (std::strcmp(argv[i], "--template") == 0 ||
                     std::strncmp(argv[i], "--template=", 11) == 0) {
                // "--template format"
                if (argv[i][10] == '=')
                    mSettings->templateFormat = argv[i] + 11;
                else if ((i+1) < argc && argv[i+1][0] != '-') {
                    ++i;
                    mSettings->templateFormat = argv[i];
                } else {
                    printMessage("cppcheck: argument to '--template' is missing.");
                    return false;
                }

                if (mSettings->templateFormat == "gcc") {
                    //_settings->templateFormat = "{file}:{line}: {severity}: {message}";
                    mSettings->templateFormat = "{file}:{line}:{column}: warning: {message} [{id}]\\n{code}";
                    mSettings->templateLocation = "{file}:{line}:{column}: note: {info}\\n{code}";
                } else if (mSettings->templateFormat == "daca2") {
                    mSettings->templateFormat = "{file}:{line}:{column}: {severity}: {message} [{id}]";
                    mSettings->templateLocation = "{file}:{line}:{column}: note: {info}";
                } else if (mSettings->templateFormat == "vs")
                    mSettings->templateFormat = "{file}({line}): {severity}: {message}";
                else if (mSettings->templateFormat == "edit")
                    mSettings->templateFormat = "{file} +{line}: {severity}: {message}";
            }

            else if (std::strcmp(argv[i], "--template-location") == 0 ||
                     std::strncmp(argv[i], "--template-location=", 20) == 0) {
                // "--template-location format"
                if (argv[i][19] == '=')
                    mSettings->templateLocation = argv[i] + 20;
                else if ((i+1) < argc && argv[i+1][0] != '-') {
                    ++i;
                    mSettings->templateLocation = argv[i];
                } else {
                    printMessage("cppcheck: argument to '--template' is missing.");
                    return false;
                }
            }

            // print all possible error messages..
            else if (std::strcmp(argv[i], "--errorlist") == 0) {
                mShowErrorMessages = true;
                mSettings->xml = true;
                mExitAfterPrint = true;
            }

            // show timing information..
            else if (std::strncmp(argv[i], "--showtime=", 11) == 0) {
                const std::string showtimeMode = argv[i] + 11;
                if (showtimeMode == "file")
                    mSettings->showtime = SHOWTIME_FILE;
                else if (showtimeMode == "summary")
                    mSettings->showtime = SHOWTIME_SUMMARY;
                else if (showtimeMode == "top5")
                    mSettings->showtime = SHOWTIME_TOP5;
                else if (showtimeMode.empty())
                    mSettings->showtime = SHOWTIME_NONE;
                else {
                    std::string message("cppcheck: error: unrecognized showtime mode: \"");
                    message += showtimeMode;
                    message += "\". Supported modes: file, summary, top5.";
                    printMessage(message);
                    return false;
                }
            }

            // Rule file
            else if (std::strncmp(argv[i], "--rule-file=", 12) == 0) {
                tinyxml2::XMLDocument doc;
                if (doc.LoadFile(12+argv[i]) == tinyxml2::XML_SUCCESS) {
                    tinyxml2::XMLElement *node = doc.FirstChildElement();
                    for (; node && strcmp(node->Value(), "rule") == 0; node = node->NextSiblingElement()) {
                        Settings::Rule rule;

                        tinyxml2::XMLElement *tokenlist = node->FirstChildElement("tokenlist");
                        xmlGetText(tokenlist, rule.tokenlist);

                        tinyxml2::XMLElement *pattern = node->FirstChildElement("pattern");
                        rule.pattern = "";
                        xmlGetText(pattern, rule.pattern);

                        tinyxml2::XMLElement *message = node->FirstChildElement("message");
                        if (message) {
                            tinyxml2::XMLElement *severity = message->FirstChildElement("severity");
                            std::string str;
                            xmlGetText(severity, str);
                            rule.severity = Severity::fromString(str);

                            tinyxml2::XMLElement *id = message->FirstChildElement("id");
                            rule.id = "";
                            xmlGetText(id, rule.id);

                            tinyxml2::XMLElement *summary = message->FirstChildElement("summary");
                            xmlGetText(summary, rule.summary);
                        }

                        if (!rule.pattern.empty())
                            mSettings->rules.push_back(rule);
                    }
                } else {
                    printMessage("cppcheck: error: unable to load rule-file: " + std::string(12+argv[i]));
                    return false;
                }
            }

            // Specify platform
            else if (std::strncmp(argv[i], "--platform=", 11) == 0) {
                const std::string platform(11+argv[i]);

                if (platform == "win32A")
                    mSettings->platform(Settings::Win32A);
                else if (platform == "win32W")
                    mSettings->platform(Settings::Win32W);
                else if (platform == "win64")
                    mSettings->platform(Settings::Win64);
                else if (platform == "unix32")
                    mSettings->platform(Settings::Unix32);
                else if (platform == "unix64")
                    mSettings->platform(Settings::Unix64);
                else if (platform == "native")
                    mSettings->platform(Settings::Native);
                else if (platform == "unspecified")
                    mSettings->platform(Settings::Unspecified);
                else if (!mSettings->loadPlatformFile(argv[0], platform)) {
                    std::string message("cppcheck: error: unrecognized platform: \"");
                    message += platform;
                    message += "\".";
                    printMessage(message);
                    return false;
                }
            }

            // Print help
            else if (std::strcmp(argv[i], "-h") == 0 || std::strcmp(argv[i], "--help") == 0) {
                mPathNames.clear();
                mShowHelp = true;
                mExitAfterPrint = true;
                break;
            }

            else {
                std::string message("cppcheck: error: unrecognized command line option: \"");
                message += argv[i];
                message += "\".";
                printMessage(message);
                return false;
            }
        }

        else {
            std::string path = Path::removeQuotationMarks(argv[i]);
            path = Path::fromNativeSeparators(path);
            mPathNames.push_back(path);
        }
    }

    mSettings->project.ignorePaths(mIgnoredPaths);

    if (argc <= 1) {
        mShowHelp = true;
        mExitAfterPrint = true;
    }
  
  if (mShowHelp) {
    printHelp();
    return true;
  }

  if ( mSettings->projectName.empty() )
  {
    printMessage("Mandatory option missing:  --winccoa-projectName");
    mExitAfterPrint = true;
    return true;
  }

    // Print error only if we have "real" command and expect files
    if (!mExitAfterPrint && mPathNames.empty() && mSettings->project.fileSettings.empty()) {
        printMessage("cppcheck: No C or C++ source files found.");
        return false;
    }

    // Use paths _pathnames if no base paths for relative path output are given
    if (mSettings->basePaths.empty() && mSettings->relativePaths)
        mSettings->basePaths = mPathNames;

    return true;
}

void CmdLineParser::xmlGetText(tinyxml2::XMLElement *element, std::string &value)
{
    if (element)
    {
        const char* txt = element->GetText();
        if (txt)
            value = txt;
    }
}

void CmdLineParser::printHelp()
{
    std::cout << "Ctrlppcheck - A tool for static Ctrl/Ctrl++ code analysis\n"
              "\n"
              "Syntax:\n"
              "    cppcheck [OPTIONS] [files or paths]\n"
              "\n"
              "If a directory is given instead of a filename, *.ctl files are\n"
              " checked recursively from the given directory.\n\n"
              "Options:\n"
              "  Mandatory:\n"
              "    --winccoa-projectName=PROJ_NAME\n"
              "    --winCCOA-productCode=WinCCOA_PRODUCT_CODE\n"
              "\n"
              "  Optional:\n"
              "    --check-config       Check cppcheck configuration. The normal code\n"
              "                         analysis is disabled by this flag.\n"
              "    --check-library      Show information messages when library files have\n"
              "                         incomplete info.\n"
              "    --config-exclude=<dir>\n"
              "                         Path (prefix) to be excluded from configuration\n"
              "                         checking. Preprocessor configurations defined in\n"
              "                         headers (but not sources) matching the prefix will not\n"
              "                         be considered for evaluation.\n"
              "    --config-excludes-file=<file>\n"
              "                         A file that contains a list of config-excludes\n"
              "    --dump               Dump xml data for each translation unit. The dump\n"
              "                         files have the extension .dump and contain ast,\n"
              "                         tokenlist, symboldatabase, valueflow.\n"
              "    -E                   Print preprocessor output on stdout and don't do any\n"
              "                         further processing.\n"
              "    --enable=<id>        Enable additional checks. The available ids are:\n"
              "                          * all\n"
              "                                  Enable all checks. It is recommended to only\n"
              "                                  use --enable=all when the whole program is\n"
              "                                  scanned, because this enables unusedFunction.\n"
              "                          * warning\n"
              "                                  Enable warning messages\n"
              "                          * style\n"
              "                                  Enable all coding style checks. All messages\n"
              "                                  with the severities 'style', 'performance' and\n"
              "                                  'portability' are enabled.\n"
              "                          * performance\n"
              "                                  Enable performance messages\n"
              "                          * portability\n"
              "                                  Enable portability messages\n"
              "                          * information\n"
              "                                  Enable information messages\n"
              "                          * unusedFunction\n"
              "                                  Check for unused functions. It is recommend\n"
              "                                  to only enable this when the whole program is\n"
              "                                  scanned.\n"
              "                          * missingInclude\n"
              "                                  Warn if there are missing includes. For\n"
              "                                  detailed information, use '--check-config'.\n"
              "                         Several ids can be given if you separate them with\n"
              "                         commas. See also --std\n"
              "    --error-exitcode=<n> If errors are found, integer [n] is returned instead of\n"
              "                         the default '0'. '" << EXIT_FAILURE << "' is returned\n"
              "                         if arguments are not valid or if no input files are\n"
              "                         provided. Note that your operating system can modify\n"
              "                         this value, e.g. '256' can become '0'.\n"
              "    --errorlist          Print a list of all the error messages in XML format.\n"
              "    --exitcode-suppressions=<file>\n"
              "                         Used when certain messages should be displayed but\n"
              "                         should not cause a non-zero exitcode.\n"
              "    --file-list=<file>   Specify the files to check in a text file. Add one\n"
              "                         filename per line. When file is '-,' the file list will\n"
              "                         be read from standard input.\n"
              "    -h, --help           Print this help.\n"
              "    -I <dir>             Give path to search for include files. Give several -I\n"
              "                         parameters to give several paths. First given path is\n"
              "                         searched for contained header files first. If paths are\n"
              "                         relative to source files, this is not needed.\n"
              "    --includes-file=<file>\n"
              "                         Specify directory paths to search for included header\n"
              "                         files in a text file. Add one include path per line.\n"
              "                         First given path is searched for contained header\n"
              "                         files first. If paths are relative to source files,\n"
              "                         this is not needed.\n"
              "    --include=<file>\n"
              "                         Force inclusion of a file before the checked file. Can\n"
              "                         be used for example when checking the Linux kernel,\n"
              "                         where autoconf.h needs to be included for every file\n"
              "                         compiled. Works the same way as the GCC -include\n"
              "                         option.\n"
              "    -i <dir or file>     Give a source file or source file directory to exclude\n"
              "                         from the check. This applies only to source files so\n"
              "                         header files included by source files are not matched.\n"
              "                         Directory name is matched to all parts of the path.\n"
              "    --inconclusive       Allow that Cppcheck reports even though the analysis is\n"
              "                         inconclusive.\n"
              "                         There are false positives with this option. Each result\n"
              "                         must be carefully investigated before you know if it is\n"
              "                         good or bad.\n"
              "    --inline-suppr       Enable inline suppressions. Use them by placing one or\n"
              "                         more comments, like: '// ctrlppcheck-suppress warningId'\n"
              "                         on the lines before the warning to suppress.\n"
              "    --language=<language>, -x <language>\n"
              "                         Forces cppcheck to check all files as the given\n"
              "                         language. Valid values are: c, c++, crl\n"
              "    --library=<cfg>      Load file <cfg> that contains information about types\n"
              "                         and functions. With such information Cppcheck\n"
              "                         understands your code better and therefore you\n"
              "                         get better results. The std.cfg file that is\n"
              "                         distributed with Cppcheck is loaded automatically.\n"
              "                         For more information about library files, read the\n"
              "                         manual.\n"
              "    --max-ctu-depth=N    Max depth in whole program analysis. The default value\n"
              "                         is 2. A larger value will mean more errors can be found\n"
              "                         but also means the analysis will be slower.\n"
              "    --output-file=<file> Write results to file, rather than standard error.\n"
              "    --naming-rule-file=<file>   Use given naming rule file.\n"
              "                         The rules defined in the file are used to\n"
              "                         check the variable names.\n"
              "    --platform=<type>, --platform=<file>\n"
              "                         Specifies platform specific types and sizes. The\n"
              "                         available builtin platforms are:\n"
              "                          * unix32\n"
              "                                 32 bit unix variant\n"
              "                          * unix64\n"
              "                                 64 bit unix variant\n"
              "                          * win32A\n"
              "                                 32 bit Windows ASCII character encoding\n"
              "                          * win32W\n"
              "                                 32 bit Windows UNICODE character encoding\n"
              "                          * win64\n"
              "                                 64 bit Windows\n"
              "                          * avr8\n"
              "                                 8 bit AVR microcontrollers\n"
              "                          * native\n"
              "                                 Type sizes of host system are assumed, but no\n"
              "                                 further assumptions.\n"
              "                          * unspecified\n"
              "                                 Unknown type sizes\n"
              "    -q, --quiet          Do not show progress reports.\n"
              "    -rp, --relative-paths\n"
              "    -rp=<paths>, --relative-paths=<paths>\n"
              "                         Use relative paths in output. When given, <paths> are\n"
              "                         used as base. You can separate multiple paths by ';'.\n"
              "                         Otherwise path where source files are searched is used.\n"
              "                         We use string comparison to create relative paths, so\n"
              "                         using e.g. ~ for home folder does not work. It is\n"
              "                         currently only possible to apply the base paths to\n"
              "                         files that are on a lower level in the directory tree.\n"
              "    --remove-unused-templates\n"
              "                         Remove unused templates.\n"
              "    --remove-unused-included-templates\n"
              "                         Remove unused templates in included files.\n"
              "    --report-progress    Report progress messages while checking a file.\n"
              "    --rule-file=<file>   Use given rule file."
          /// @todo link to own docu
              "    --suppress=<spec>    Suppress warnings that match <spec>. The format of\n"
              "                         <spec> is:\n"
              "                         [error id]:[filename]:[line]\n"
              "                         The [filename] and [line] are optional. If [error id]\n"
              "                         is a wildcard '*', all error ids match.\n"
              "    --suppressions-list=<file>\n"
              "                         Suppress warnings listed in the file. Each suppression\n"
              "                         is in the same format as <spec> above.\n"
              "    --template='<text>'  Format the error messages. Available fields:\n"
              "                           {file}              file name\n"
              "                           {line}              line number\n"
              "                           {column}            column number\n"
              "                           {callstack}         show a callstack. Example:\n"
              "                                                 [file.c:1] -> [file.c:100]\n"
              "                           {inconlusive:text}  if warning is inconclusive, text\n"
              "                                               is written\n"
              "                           {severity}          severity\n"
              "                           {message}           warning message\n"
              "                           {id}                warning id\n"
              "                           {cwe}               CWE id (Common Weakness Enumeration)\n"
              "                           {code}              show the real code\n"
              "                           \\t                 insert tab\n"
              "                           \\n                 insert newline\n"
              "                           \\r                 insert carriage return\n"
              "                         Example formats:\n"
              "                         '{file}:{line},{severity},{id},{message}' or\n"
              "                         '{file}({line}):({severity}) {message}' or\n"
              "                         '{callstack} {message}'\n"
              "                         Pre-defined templates: gcc, vs, edit.\n"
              "    --template-location='<text>'\n"
              "                         Format error message location. If this is not provided\n"
              "                         then no extra location info is shown.\n"
              "                         Available fields:\n"
              "                           {file}      file name\n"
              "                           {line}      line number\n"
              "                           {column}    column number\n"
              "                           {info}      location info\n"
              "                           {code}      show the real code\n"
              "                           \\t         insert tab\n"
              "                           \\n         insert newline\n"
              "                           \\r         insert carriage return\n"
              "                         Example format (gcc-like):\n"
              "                         '{file}:{line}:{column}: note: {info}\\n{code}'\n"
              "    -v, --verbose        Output more detailed error information.\n"
              "    --version            Print out version number.\n"
              "    --xml                Write results in xml format to error stream (stderr).\n"
              "    --xml-version=<version>\n"
              "                         Select the XML file version. Currently only versions 2 is available."
              "\n"
              "Example usage:\n"
              "  # Recursively check the current folder. Print the progress on the screen and\n"
              "  # write errors to a file:\n"
              "  cppcheck . 2> err.txt\n"
              "\n"
              "  # Recursively check ../myproject/ and don't print progress:\n"
              "  cppcheck --quiet ../myproject/\n"
              "\n"
              "  # Check test.ctl, enable all checks:\n"
              "  cppcheck --enable=all --inconclusive test.ctl\n"
              "\n"
              "  # Check f.ctl and search include files from inc1/ and inc2/:\n"
              "  cppcheck -I inc1/ -I inc2/ f.ctl\n"
              "\n"
              "Many thanks to the 3rd party libraries we use:\n"
              " * tinyxml2 -- loading project/library/ctu files.\n";
}
