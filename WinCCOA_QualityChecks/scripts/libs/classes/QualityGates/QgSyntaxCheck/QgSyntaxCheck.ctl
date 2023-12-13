//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2023 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//

#uses "classes/FileSys/QgDir"
#uses "classes/QualityGates/Tools/OaSyntaxCheck/OaSyntaxCheck"
#uses "classes/QualityGates/Qg"
#uses "classes/QualityGates/QgBase"
#uses "classes/Variables/String"
#uses "classes/QualityGates/QgResult"
#uses "classes/QualityGates/QgSettings"

//--------------------------------------------------------------------------------
/**
  @brief QualityCheck OA-syntax check.

  @details Start classic WinCC OA syntax check.

  ## Debug flags
  + QgSyntaxCheck - enable all debugs specific to this quality check.
*/
class QgSyntaxCheck : QgBase
{
//--------------------------------------------------------------------------------
//@public members
//--------------------------------------------------------------------------------

  //------------------------------------------------------------------------------
  /**
    @brief Setup this quality-check.
    @details
    For this check are enabled errors only.
    When the project does not contains some panels or scripts (libs) is the score
    setted to min-valid (1).

    @warning Work only with the current project. If you want start it to other projec or sub project.
    Start the manager with option -proj <checkedProj>

    @attention override from QgBase::setUp()
  */
  public int setUp()
  {
    if (QgBase::setUp())
      return -1;

    QgResult::showErrorsOnly = TRUE;

    QgDir panelsDir = QgDir(PROJ_PATH + PANELS_REL_PATH);
    QgDir scriptsDir = QgDir(PROJ_PATH + SCRIPTS_REL_PATH);

    if (!panelsDir.exists() && !scriptsDir.exists())
      this.setMinValidScore("QgSyntaxCheck", "missingFiles");

    return 0;
  }

  //------------------------------------------------------------------------------
  /** @brief Calculates / start syntax check.
    @return 0 when successfull, otherwise -1.
    @attention override from QgBase::calculate()
  */
  public int calculate()
  {
    return _oaSyntaxCheck.checkAll();
  }

  //------------------------------------------------------------------------------
  /** @brief Function validates this quality-check.
    @details Validate all syntax error. The score the ration between all messages
    and error messages. Error messegase are all syntax earnigns, errors ... . All
    messages contains the valid syntax messages (info messages) and errors too.
    Valid syntax message is throwed only 1 times per checked file. But in a file
    can be more then 1 error. So it can happend, that you have 2 files. 1 is valid
    and other one has 1000 syntax errors. In that case are the score terrible (~0).

    @return 0 when successfull, otherwise -1.
    @attention override from QgBase::validate()

    @todo many code might be eliminated by using WinCC OA LogAnalyzer. But this feature
          exists in versions >= 3.19. That means after refactoring it will no more works with
          WinCC OA version <= 3.18
          The value is to eliminate code here.
           + is parsed in weir way and no body can grant, that it works "for ever"
           + performance
  */
  public int validate()
  {
    _result = new QgResult("QgSyntaxCheck", "syntaxMsgs");

    dyn_string msgs;
    _oaSyntaxCheck.stdErrToMessages(msgs);
    _oaSyntaxCheck.stdErr = ""; // free memory
    _oaSyntaxCheck.stdOut = ""; // free memory

    for (int i = 1; i <= dynlen(msgs); i++)
    {
      string msg = msgs[i];

      if (strpos(msg, "is Qt installed correctly") > 0)
      {
        continue;  // this message is not relevant
      }

      shared_ptr<QgSettings> settings = new QgSettings("SyntaxCheck.isSyntaxValid");

      if (settings.isEnabled())
      {
        DebugFTN("QgSyntaxCheck", __FUNCTION__, "check message", msg);
        dyn_string items = strsplit(msg, ",");

        if (dynlen(items) < 4)
          continue; // ignore something like 'This plugin does not support createPlatformOpenGLContext!'

        const string type    = strltrim(items[3], " ");
        const string prio    = strltrim(items[4], " ");
        string errCode;

        if (dynlen(items) >= 5)
          errCode = strltrim(items[5], " ");

        dynRemove(items, 1);
        dynRemove(items, 1);
        dynRemove(items, 1);
        msg = strjoin(items, ",");
        msg = makeUnixPath(msg);
        strreplace(msg, makeUnixPath(PROJ_PATH), "");

        shared_ptr <QgResult> assertion = new QgResult("QgSyntaxCheck", ("isSyntaxValid", makeMapping("msg", msg));

        if (prio == "INFO")
        {
          if (errCode == "0")
          {
            DebugFN("QgSyntaxCheck", __FUNCTION__, "this will works, syntax check does not returns errors");
            assertion.assertTrue(TRUE, settings.getScorePoints());
            assertion.referenceValue = msg;
            _result.addChild(assertion);
          }

          DebugFN("QgSyntaxCheck", __FUNCTION__, "Skip checks for info message");
          continue;  // ignore info messages
        }

        DebugFTN("QgSyntaxCheck", __FUNCTION__, "add message to bad-list");

        assertion.assertTrue(FALSE, settings.getScorePoints());
        _result.addChild(assertion);
      }
    }

    return 0;
  }


//--------------------------------------------------------------------------------
//@private members
//--------------------------------------------------------------------------------

  // oa syntax check
  OaSyntaxCheck _oaSyntaxCheck = OaSyntaxCheck();
};
