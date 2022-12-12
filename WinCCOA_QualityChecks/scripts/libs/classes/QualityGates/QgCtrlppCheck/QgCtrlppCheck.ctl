//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2022 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//

#uses "classes/QualityGates/Tools/CppCheck/CppCheckError"
#uses "classes/FileSys/QgFile"
#uses "classes/QualityGates/Tools/CppCheck/CppCheck"
#uses "classes/QualityGates/Qg"
#uses "classes/QualityGates/QgBase"


//--------------------------------------------------------------------------------
/**
  @brief QualityCheck ctrlppcheck.

  @details Start ctrlppcheck.
*/
class QgCtrlppCheck : QgBase
{
//--------------------------------------------------------------------------------
//@public members
//--------------------------------------------------------------------------------

  //------------------------------------------------------------------------------
  /** @brief Calculates / start ctrlppcheck.
    @return 0 when successfull, otherwise -1.
    @attention override from QgBase::calculate()
  */
  public int calculate()
  {
    return checkDir(PROJ_PATH + SCRIPTS_REL_PATH);
  }

  //------------------------------------------------------------------------------
  /** @brief Function validates this quality-check.
    @details Validate all all errors from ctrlppcheck.
    Errors are filtered by function isErrorFiltered().

    @todo calculate somehow the score.
    @return 0 when successfull, otherwise -1.
    @attention override from QgBase::validate()
  */
  public int validate()
  {
    QgVersionResult::lastErr = "";
    _result = new QgVersionResult();

    _result.setMsgCatName("QgCtrlppCheck");
    _result.setAssertionText("checks");


    if ( dpExists("_CtrlppCheck") )
    {
      dpGet("_CtrlppCheck.filter.id", disabledIds,
            "_CtrlppCheck.filter.severity", disabledSeverities);
      includeFilesPattern = "*";
    }
    else if ( Qg::isRunningOnJenkins() )
    {
      disabledIds = makeDynString("debug", "unreadVariable",
                                  "checkLibraryFunction", "checkLibraryNoReturn",
                                  "unusedFunction");
      disabledSeverities = makeDynString("debug", "information");
      includeFilesPattern = makeUnixPath(dirPath + "*");
    }
    else
    {
      disabledIds = makeDynString("debug", "unreadVariable",
                                  "checkLibraryFunction", "checkLibraryNoReturn");
      disabledSeverities = makeDynString("debug", "information");
      includeFilesPattern = "*";
    }

    for(int i = 1; i <= dynlen(check.errList); i++)
    {
      CppCheckError error = check.errList[i];

      QgFile f = QgFile(error.path);
      if ( f.isExample() || f.isTest() || !f.isPatternMatch(includeFilesPattern) )
        continue;

      if ( isErrorFiltered(error) )
        continue;

      string relPath = f.getRelPath(SCRIPTS_REL_PATH);

      shared_ptr <QgVersionResult> assertion = new QgVersionResult();
      assertion.setMsgCatName("QgCtrlppCheck");
      assertion.setAssertionText(relPath + " : " + error.line);
      assertion.setReasonText(error.msg + " (" + error.id + ")");
      assertion.assertEqual(error.severity, "");
      _result.addChild(assertion);

    }
    return 0;
  }


//--------------------------------------------------------------------------------
//@protected members
//--------------------------------------------------------------------------------
  protected dyn_string disabledIds, disabledSeverities;
  protected string dirPath;
  protected string includeFilesPattern;

  //------------------------------------------------------------------------------
  /** Start ctrl pp check for given directory.
   @param path Path to directory, whit shall be checked.
   */
  protected int checkDir(const string &path)
  {
    dirPath = path;

    if ( dpExists("_CtrlppCheck") )
    {
      dpGet("_CtrlppCheck.settings.enableLibCheck", check.settings.enableLibCheck,
            //"_CtrlppCheck.settings.enableHeadersCheck", check.settings.enableHeadersCheck,  // currently disabled
            "_CtrlppCheck.settings.inconclusive", check.settings.inconclusive,
            "_CtrlppCheck.settings.includeSubProjects", check.settings.includeSubProjects,
            "_CtrlppCheck.settings.verbose", check.settings.verbose,
            "_CtrlppCheck.settings.inlineSuppressions", check.settings.inlineSuppressions);
    }
    else if ( Qg::isRunningOnJenkins() )
    {
      check.settings.enableLibCheck = FALSE;
      check.settings.enableHeadersCheck = TRUE;
      check.settings.includeSubProjects = TRUE;
      check.settings.inconclusive = FALSE;
      check.settings.verbose = FALSE;
      check.settings.inlineSuppressions = TRUE;
    }
    else
    {
      check.settings.enableCheckLibrary(FALSE);
    }

    // load configs
    check.settings.addLibraryFile(getPath(DATA_REL_PATH, "DevTools/Base/ctrl.xml")); // general
    check.settings.addLibraryFile(getPath(DATA_REL_PATH, "DevTools/Base/ctrl_" + VERSION + ".xml")); // version specific
    check.settings.addLibraryFile(getPath(DATA_REL_PATH, "ctrlPpCheck/cfg/__proj__.xml")); // proj specific

    // load rules
    check.settings.addRuleFile(getPath(DATA_REL_PATH, "ctrlPpCheck/rule/ctrl.xml")); // general
    check.settings.addRuleFile(getPath(DATA_REL_PATH, "ctrlPpCheck/rule/ctrl_" + VERSION + ".xml")); // version specific
    check.settings.addRuleFile(getPath(DATA_REL_PATH, "ctrlPpCheck/rule/__proj__.xml")); // proj specific

    check.settings.addEnabled("all");
    check.settings.enableXmlFormat(TRUE);

    check.checkFile(dirPath);

    return check.rc;
  }

  //------------------------------------------------------------------------------
  /// Checks if the error shall be filtered.
  protected bool isErrorFiltered(const CppCheckError &error)
  {
    if ( error.msg == "" )
      return TRUE;

    const string id = error.id;

    if ( dynContains(disabledIds, id)  )
      return TRUE;

    const string severity = error.severity;
    if ( dynContains(disabledSeverities, severity)  )
      return TRUE;

    return FALSE;
  }

//--------------------------------------------------------------------------------
//@private members
//--------------------------------------------------------------------------------

  CppCheck check;
};


/// Start Qg ctrlppcheck.
/// Simple old ctrl style.
public int start_QgCtrlppCheck()
{
  Qg::setId("QgCtrlppCheck");
  QgCtrlppCheck qg = QgCtrlppCheck();
  return qg.start();
}
