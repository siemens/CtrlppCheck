//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2022 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//

#uses "classes/QualityGates/QgBase"
#uses "classes/QualityGates/QgStaticCheck/CtrlCode/ScriptData"
#uses "classes/FileSys/QgFile"
#uses "classes/QualityGates/QgSettings"

class ScriptFile : QgFile
{
//--------------------------------------------------------------------------------
//@public members
//--------------------------------------------------------------------------------
  //------------------------------------------------------------------------------
  /** @brief Default c-tor.
    @param dirPath Full path to directory.
  */
  public ScriptFile(string filePath = "")
  {
    setFilePath(filePath);
    // !! extension must be written lowercase, that NonCaseSensitive works
    _enabledExtensions = makeDynString("ctl", "ctc");
  }

  //------------------------------------------------------------------------------
  public static bool isCrypted(const string &s)
  {
    return (strpos(s, "PVSS_CRYPTED_PANEL") == 0);
  }

  //------------------------------------------------------------------------------
  public bool isFileCrypted()
  {
    string s;
    fileToString(getFilePath(), s);

    if ((s == "") && (strpos(getFilePath(), ".ctc") > 0))
      return TRUE;

    return isCrypted(s);
  }

  public bool isCalculated()
  {
    return _isCalculated;
  }


  public int calculate()
  {
    _isCalculated = FALSE;
    _extension = getExt(getFilePath());

    if (!isfile(getFilePath()))
    {
      DebugFTN("ScriptFile", __FUNCTION__, "file does not exists", getFilePath());
      return -1;
    }

    if (isFileCrypted())
    {
      DebugFTN("ScriptFile", __FUNCTION__, "!!! file is encrypted", getFilePath());
      return 0;
    }

    if (this.isExample())
    {
      // do calculate example, aprove performance
      DebugFTN("ScriptFile", __FUNCTION__, "!!! Dont calculate example file", getFilePath());
      return 0;
    }

    _scriptData.setPath(getFilePath());

    if (_scriptData.calculate())
    {
      DebugFTN("ScriptFile", __FUNCTION__, "can not calculate script data");
      return -2;
    }

    _isCalculated = TRUE;

    return 0;
  }

  public int validate()
  {
    result = new QgVersionResult();
    result.text = getName();

    {
      shared_ptr<QgSettings> settings = new QgSettings("ScriptFile.file.isExampleFile");

      if (settings.isEnabled())
      {
        // check if the file is example.
        // ignore all example files, the example are terrible scripts
        shared_ptr <QgVersionResult> assertion = new QgVersionResult();
        assertion.setMsgCatName("QgStaticCheck_ScriptFile");
        assertion.setAssertionText("assert.file.isExampleFile");
        assertion.setReasonText("reason.file.isExampleFile", makeMapping("file.name", getName()));
        assertion.allowNextErr(TRUE);

        if (!assertion.assertFalse(this.isExample(), settings.getScorePoints()))
        {
          result.addChild(assertion);
          return 0;
        }

        result.addChild(assertion);
      }
    }

    {
      shared_ptr<QgSettings> settings = new QgSettings("ScriptFile.file.extension");

      if (settings.isEnabled())
      {
        // check for valid extensions
        shared_ptr <QgVersionResult> assertion = new QgVersionResult();
        assertion.setMsgCatName("QgStaticCheck_ScriptFile");
        assertion.setAssertionText("assert.file.extension");
        assertion.setReasonText("reason.file.extension", makeMapping("file.name", getName(),
                                "file.extension", _extension));

        if (!assertion.assertDynContains(settings.getReferenceValues(), strtolower(_extension), settings.getScorePoints()))
        {
          result.addChild(assertion);
          return 0;
        }

        result.addChild(assertion);
      }
    }

    {
      shared_ptr<QgSettings> settings = new QgSettings("ScriptFile.file.isCalculated");

      if (settings.isEnabled())
      {
        // check if file is calculated.
        // ognore all not calculated files (crypted, empty files ...)
        shared_ptr <QgVersionResult> assertion = new QgVersionResult();
        assertion.setMsgCatName("QgStaticCheck_ScriptFile");
        assertion.setAssertionText("assert.file.isCalculated");
        assertion.setReasonText("reason.file.isCalculated", makeMapping("file.name", getName()));

        if (!assertion.assertTrue(isCalculated(), settings.getScorePoints()))
        {
          result.addChild(assertion);
          return 0;
        }

        result.addChild(assertion);
      }
    }

    if (_scriptData.validate())
      return -1;

    result.addChild(_scriptData.result);

    return 0;
  }

  public int getCCN()
  {
    return _scriptData.getCCN();
  }

  public int getNLOC()
  {
    return _scriptData.getNLOC();
  }

  public float getAvgCCN()
  {
    return _scriptData.getAvgCCN();
  }

  public float getAvgNLOC()
  {
    return _scriptData.getAvgNLOC();
  }



  //------------------------------------------------------------------------------
  public shared_ptr <QgVersionResult> result; //!< Quality gate result

  protected ScriptData _scriptData = ScriptData();

  static dyn_string _enabledExtensions = makeDynString();
  string _extension;
  bool _isCalculated;
};
