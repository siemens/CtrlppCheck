//--------------------------------------------------------------------------------
/**
  @file $relPath
  @copyright Copyright 2023 SIEMENS AG
             SPDX-License-Identifier: GPL-3.0-only
*/

#uses "fileSys"

//--------------------------------------------------------------------------------
class CtrlppCheckSettings
{
//--------------------------------------------------------------------------------
//@public
//--------------------------------------------------------------------------------

  /// option --check-headers=no
  /// Turn off checking of included files, to make the analysis faster
  //public bool enableHeadersCheck = FALSE; (!!There is no such option!!)

  /// option  --check-library
  /// Show information messages when library files have incomplete info
  public bool enableLibCheck = FALSE;
  /// option --inconclusive
  /// Allow that Ctrlppcheck reports even though the analysis is inconclusive.
  /// There are false positives with this option. Each result must be carefully
  /// investigated before you know if it is good or bad.
  public bool inconclusive = TRUE;
  /// include subProjects and WinCC OA directory for depth check
  /// Useful to use with enableHeadersCheck = TRUE
  public bool includeSubProjects = TRUE;
  /// Output more detailed error information.
  /// For details for rules check
  public bool verbose = FALSE;
  /// Print a list of all the error messages
  public bool errorList = FALSE;
  /// Enable in-line suppressions. Use them by placing one or
  /// more comments, like: '// ctrlppcheck-suppress warningId'
  /// on the lines before the warning to suppress.
  public bool inlineSuppressions = FALSE;


  /// option --winccoa-projectName
  public string winccoaProjectName = PROJ;

  //------------------------------------------------------------------------------
  public addEnabled(const string &str)
  {
    if (enabled != "")
      enabled += "," + str;
    else
      enabled = str;
  }

  //------------------------------------------------------------------------------
  public bool enableCheckLibrary(bool enable)
  {
    enableLibCheck = enable;
  }

  //------------------------------------------------------------------------------
  public enableXmlFormat(bool enable)
  {
    if (enable)
      xml = "--xml";
    else
      xml = "";
  }

  //------------------------------------------------------------------------------
  public bool isXmlOutEnabled()
  {
    return (xml != "");
  }

  //------------------------------------------------------------------------------
  public addRuleFile(const string &path)
  {
    if (path == "")
      return;

    dynAppend(ruleFiles, path);
    dynUnique(ruleFiles);
  }

  //------------------------------------------------------------------------------
  /// Function unload rule file from list.
// It is never used, but it is prepared for settings panel
// ctrlppcheck-suppress unusedFunction
  public unloadRule(const string &path)
  {
    int idx = dynContains(ruleFiles, path);

    if (idx <= 0)
      return;

    dynRemove(ruleFiles, idx);
  }

  //------------------------------------------------------------------------------
  /// Function add include directory to includedSubProjDirs list.
// It is never used, but it is prepared for settings panel
// ctrlppcheck-suppress unusedFunction
  public addIncludeDir(const string &path)
  {
    if (path == "")
      return;

    dynAppend(includedSubProjDirs, path);
    dynUnique(includedSubProjDirs);
  }

  //------------------------------------------------------------------------------
  /// Function unload include directory from includedSubProjDirs list.
// It is never used, but it is prepared for settings panel
// ctrlppcheck-suppress unusedFunction
  public unloadIncludeDir(const string &path)
  {
    int idx = dynContains(includedSubProjDirs, path);

    if (idx <= 0)
      return;

    dynRemove(includedSubProjDirs, idx);
  }

  //------------------------------------------------------------------------------
  public addLibraryFile(const string &path)
  {
    if (path == "")
      return;

    dynAppend(libraryFiles, path);
    dynUnique(libraryFiles);
  }

  //------------------------------------------------------------------------------
  /// Function unload library file from list.
// It is never used, but it is prepared for settings panel
// ctrlppcheck-suppress unusedFunction
  public unloadLibrary(const string &path)
  {
    int idx = dynContains(libraryFiles, path);

    if (idx <= 0)
      return;

    dynRemove(libraryFiles, idx);
  }

  //------------------------------------------------------------------------------
  public string toCmdLine()
  {
    string opts;

    if (enabled != "")
      opts += " --enable=" + enabled;

    if (xml != "")
      opts += " " + xml;

    if (inconclusive)
      opts += " --inconclusive";


    for (int i = 1; i <= dynlen(ruleFiles); i++)
      opts += " --rule-file=" + makeNativePath(ruleFiles[i]);

    if (enableLibCheck)
      opts += " --check-library";

    // disable header check for perfomance reason
    //if ( !enableHeadersCheck )
    //  opts += " --check-headers=no";

    if (includeSubProjects)
    {
      // add all subproject to check includes via option -I
      if (dynlen(includedSubProjDirs) <= 0)
      {
        includedSubProjDirs = getSubProjPathes();
        dynAppend(includedSubProjDirs, WINCCOA_PATH);
      }

      for (int i = 1; i <= dynlen(includedSubProjDirs); i++)
        opts += " -I " + includedSubProjDirs[i];
    }

    for (int i = 1; i <= dynlen(libraryFiles); i++)
      opts += " --library=" + makeNativePath(libraryFiles[i]);

    opts += " --winccoa-projectName=" + winccoaProjectName;

    if (verbose)
      opts += " -v";

    if (inlineSuppressions)
      opts += " --inline-suppr";

    if (errorList)
      opts += " --errorlist";

    return opts;
  }

//--------------------------------------------------------------------------------
//@private
//--------------------------------------------------------------------------------
  dyn_string libraryFiles;
  dyn_string ruleFiles;
  dyn_string includedSubProjDirs;
  string enabled;
  string xml;
};
