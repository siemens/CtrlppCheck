//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2022 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//

#uses "classes/QualityGates/QgVersionResult"
#uses "classes/FileSys/QgFile"
#uses "classes/QualityGates/QgAddOnResultErr"
#uses "classes/QualityGates/QgStaticCheck/Panels/PanelFile/PanelFile"
#uses "classes/QualityGates/QgSettings"

//--------------------------------------------------------------------------------
/**
 * @brief Oa panel checker.
 * @author lschopp
 * @details Class is created to handle, checks oa-panels.
 */
class PanelCheck : QgFile
{
//--------------------------------------------------------------------------------
//@public members
//--------------------------------------------------------------------------------

  //------------------------------------------------------------------------------
  public shared_ptr <QgVersionResult> result; //!< Quality gate result

  //------------------------------------------------------------------------------
  /** Default c-tor
   * @param filePath Full path to the oa-panel.
   */
  public PanelCheck(const string &filePath)
  {
    setFilePath(filePath);
    // !! extension must be written lowercase, that NonCaseSensitive works
    _enabledExtensions = makeDynString("pnl", "xml", "");
  }

  //------------------------------------------------------------------------------
  /** @brief  Function sets path to the source directory.
   * @details this must be setted, otherwise can not be calculated relative path
   *         to panel.
   * @param path Full apth to the source directory.
   */
  static public void setSourceDirPath(const string &path)
  {
    _sourceDir = path;
  }

  //------------------------------------------------------------------------------
  /** Function returns relative path from panel.
   * @return relitive path.
   */
  public string getRelPath()
  {
    return substr(makeNativePath(getFilePath()), strlen(makeNativePath(_sourceDir + PANELS_REL_PATH)));
  }

  //------------------------------------------------------------------------------
  /** Function checks if the panel is calculated.
   * @return TRUE panel is cacluated, otherwise FALSE.
   */
  public bool isCalculated()
  {
    return _isCalculated;
  }

  //------------------------------------------------------------------------------
  /** @brief  Function returns CCN of panel inclusive shapes.
   * @details CCN - Cyclomatic complexity number.
   * @warning Panel must be calculated. Call function calculate() before this.
   * @return CCN
   */
  public int getCCN()
  {
    return _pnl.getCCN();
  }

  //------------------------------------------------------------------------------
  /** @brief  Function returns avreage CCN of panel inclusive shapes.
   * @details CCN - Cyclomatic complexity number.
   * @warning Panel must be calculated. Call function calculate() before this.
   * @return average CCN
   */
  public float getAvgCCN()
  {
    return _pnl.getAvgCCN();
  }

  //------------------------------------------------------------------------------
  /** @brief Function returns NLOC of panel inclusive shapes.
   * @details NLOC - number of lines of code.
   * @return NLOC
   */
  public int getNLOC()
  {
    return _pnl.getNLOC();
  }

  //------------------------------------------------------------------------------
  /** @brief Function returns average NLOC of panel inclusive shapes.
   * @details NLOC - number of lines of code.
   * @return average NLOC.
   */
  public float getAvgNLOC()
  {
    return _pnl.getAvgNLOC();
  }

  //------------------------------------------------------------------------------
  /**
   * @brief Function calculates the panel file.
   *
   * Calculate means, read all properties, shapes, shapes properties, scripts, shapes scripts.
   * Calculated data are stored in meber var _pnl.
   * @warning We supported only xml panel format, so we need to convert the panel in xml format.
   *          The new panel has unique name. This has some site-effects. For example error message
   *          with getName() returns this unique name and not the original name.
   *
   * @return Error code.
   * value | description
   * ------|------------
   * 0     | success
   * -1    | can not load panel.
   * -2    | can no find panel.
   *
   * @todo lschopp 12.06.2018: throw errors with throwError() to be visible in the log files.
   */
  public int calculate()
  {
    _pnl.setPath(getRelPath());
    _isBackUp = _pnl.isBackUp();

    if (_isBackUp)
    {
      return 0;
    }

    if (isExample())
    {
      // do not calculate example, improve performance
      return 0;
    }

    _extension = getExt(getFilePath());

    _pnl.read();

    if (_pnl.isCrypted())
    {
      return 0;
    }

    if (_pnl.isXmlFormat())
    {
      // xml panel can be directly loaded
      if (_pnl.load())
      {
        DebugFTN("PanelCheck", __FUNCTION__, "can not load XML panel", getRelPath());
        _pnl.strContent = "";
        return -1;
      }
    }
    else
    {
      // convert non xml format to a xml format, otherwise can not be loaded
//       _pnl.strContent = "";
      string oldRelPath = _pnl.getRelPath();
      const string originFullPath = _pnl.getFullPath();

      string uuId = createUuid();
      strreplace(uuId, "{", "");
      strreplace(uuId, "}", "");
      string newPath = uuId + ".xml";
      _pnl.setPath(newPath);

      newPath = PROJ_PATH + PANELS_REL_PATH + newPath;

      // copy file into current project and convert it into xml format
      // conversion works only with panels located in project
      copyFile(originFullPath, newPath);

      const string origSourcePath = _sourceDir;
      PanelFile::setSourceDirPath(PROJ_PATH);
      _pnl.toXml();
      _pnl.read();
      int rc = _pnl.load();
      PanelFile::setSourceDirPath(origSourcePath);

      if (rc)
      {
        remove(newPath);
        DebugFTN("PanelCheck", __FUNCTION__, "can not load PNL/XML panel", getRelPath());
        _pnl.strContent = "";
        return -1;
      }


      remove(newPath);

      if (isfile(newPath + ".bak"))
        remove(newPath + ".bak"); // on converting generate the ui back up panel, so delete the waste.

      _pnl.setPath(oldRelPath);
    }

    _pnl.calculate();

    _pnl.strContent = "";

    _isCalculated = TRUE;
    return 0;
  }

  //------------------------------------------------------------------------------
  /** Function checks if the panel is back up panel.
   *
   * @warning call function calculate() before this to get the value.
   * @return return TRUE when is back up panel, otherwise return FALSE.
   */
  public bool isBackUp()
  {
    return _isBackUp;
  }

  //------------------------------------------------------------------------------
  public int validate()
  {
    QgVersionResult::lastErr = "";
    result = new QgVersionResult();
    result.text = getName();

    if (validateIsExample() ||
        validateIsBackUp() ||
        validateExtension() ||
        validateIsCrypted() ||
        validateIsCalculated())
    {
      return 0;
    }

    validateCountOfProperties();
    validateCountOfEvents();
    validateCountOfShapes();
    validateCCN();
    validateAvgCCN();
    validateNLOC();
    validateAvgNLOC();
    validateEvents();
    validateShapes();
    validateProperties();
    return 0;
  }

//--------------------------------------------------------------------------------
//@protected members
//--------------------------------------------------------------------------------

  // check if the file is example.
  // ignore all example files, the example are terrible panels
  protected int validateIsExample()
  {
    shared_ptr<QgSettings> settings = new QgSettings("PanelCheck.panel.isExampleFile");

    if (settings.isEnabled())
    {
      shared_ptr <QgVersionResult> assertion = new QgVersionResult();
      assertion.setMsgCatName("QgStaticCheck_Panels");
      const mapping dollars = makeMapping("panel.name", getName());
      assertion.setAssertionText("assert.panel.isExampleFile", dollars);
      assertion.setReasonText("reason.panel.isExampleFile", dollars);
      assertion.allowNextErr(TRUE);

      if (!assertion.assertFalse(isExample(), settings.getScorePoints()))
      {
        result.addChild(assertion);
        return 1;
      }

      result.addChild(assertion);
    }

    return 0;
  }

  // is backup
  protected int validateIsBackUp()
  {
    shared_ptr<QgSettings> settings = new QgSettings("PanelCheck.panel.isBackUp");

    if (settings.isEnabled())
    {
      shared_ptr <QgVersionResult> assertion = new QgVersionResult();
      assertion.setMsgCatName("QgStaticCheck_Panels");
      const mapping dollars = makeMapping("panel.name", getName());
      assertion.setAssertionText("assert.panel.isBackUp", dollars);
      assertion.setReasonText("reason.panel.isBackUp", dollars);

      if (!assertion.assertFalse(isBackUp(), settings.getScorePoints()))
      {
        result.addChild(assertion);
        return 1;
      }

      result.addChild(assertion);
    }

    return 0;
  }

  // check for valid extensions
  protected int validateExtension()
  {
    shared_ptr<QgSettings> settings = new QgSettings("PanelCheck.panel.extension");

    if (settings.isEnabled())
    {
      shared_ptr <QgVersionResult> assertion = new QgVersionResult();
      assertion.setMsgCatName("QgStaticCheck_Panels");
      const mapping dollars = makeMapping("panel.name", getName(),
                                          "panel.extension", _extension);
      assertion.setAssertionText("assert.panel.extension", dollars);
      assertion.setReasonText("reason.panel.extension", dollars);

      if (!assertion.assertDynContains(settings.getReferenceValues(), strtolower(_extension), settings.getScorePoints()))
      {
        result.addChild(assertion);
        return 1;
      }

      result.addChild(assertion);
    }

    return 0;
  }

  // is crypted
  protected int validateIsCrypted()
  {
    shared_ptr<QgSettings> settings = new QgSettings("PanelCheck.panel.isCrypted");

    if (settings.isEnabled())
    {
      shared_ptr <QgVersionResult> assertion = new QgVersionResult();
      assertion.setMsgCatName("QgStaticCheck_Panels");
      const mapping dollars = makeMapping("panel.name", getName());
      assertion.setAssertionText("assert.panel.isCrypted", dollars);
      assertion.setReasonText("reason.panel.isCrypted", dollars);

      if (!assertion.assertFalse(_pnl.isCrypted(), settings.getScorePoints()))
      {
        result.addChild(assertion);
        return 1;
      }

      result.addChild(assertion);
    }

    return 0;
  }

  // is calculated
  protected int validateIsCalculated()
  {
    shared_ptr<QgSettings> settings = new QgSettings("PanelCheck.panel.isCalculated");

    if (settings.isEnabled())
    {
      shared_ptr <QgVersionResult> assertion = new QgVersionResult();
      assertion.setMsgCatName("QgStaticCheck_Panels");
      const mapping dollars = makeMapping("panel.name", getName());
      assertion.setAssertionText("assert.panel.isCalculated", dollars);
      assertion.setReasonText("reason.panel.isCalculated", dollars);

      if (!assertion.assertTrue(isCalculated(), settings.getScorePoints()))
      {
        result.addChild(assertion);
        return 1;
      }
    }

    return 0;
  }

  // countOfProperties
  protected validateCountOfProperties()
  {
    shared_ptr<QgSettings> settings = new QgSettings("PanelCheck.panel.countOfProperties");

    if (settings.isEnabled())
    {
      shared_ptr <QgVersionResult> assertion = new QgVersionResult();
      assertion.setMsgCatName("QgStaticCheck_Panels");
      const mapping dollars = makeMapping("panel.name", getName(),
                                          "panel.countOfProperties", _pnl.getCountOfProperties());
      assertion.setAssertionText("assert.panel.countOfProperties", dollars);
      assertion.setReasonText("reason.panel.countOfProperties", dollars);
      assertion.info(_pnl.getCountOfProperties(), settings.getScorePoints());
      result.addChild(assertion);
    }
  }

  // getCountOfEvents
  protected validateCountOfEvents()
  {
    shared_ptr<QgSettings> settings = new QgSettings("PanelCheck.panel.countOfEvents");

    if (settings.isEnabled())
    {
      shared_ptr <QgVersionResult> assertion = new QgVersionResult();
      assertion.setMsgCatName("QgStaticCheck_Panels");
      const mapping dollars = makeMapping("panel.name", getName(),
                                          "panel.countOfEvents", _pnl.getCountOfEvents());
      assertion.setAssertionText("assert.panel.countOfEvents", dollars);
      assertion.setReasonText("reason.panel.countOfEvents", dollars);
      assertion.assertLessEqual(_pnl.getCountOfEvents(), settings.getHighLimit(DEFAULT_EVENTCOUNT_HIGH), settings.getScorePoints());
      result.addChild(assertion);
    }
  }

  protected validateCountOfShapes()
  {
    shared_ptr<QgSettings> settings = new QgSettings("PanelCheck.panel.countOfShapes");

    if (settings.isEnabled())
    {
      shared_ptr <QgVersionResult> assertion = new QgVersionResult();
      assertion.setMsgCatName("QgStaticCheck_Panels");
      // countOfShapes
      const mapping dollars = makeMapping("panel.name", getName(),
                                          "panel.countOfShapes", _pnl.getCountOfShapes());
      assertion.setAssertionText("assert.panel.countOfShapes", dollars);
      assertion.setReasonText("reason.panel.countOfShapes", dollars);
      assertion.assertLessEqual(_pnl.getCountOfShapes(), settings.getHighLimit(DEFAULT_SHAPECOUNT_HIGH), settings.getScorePoints());
      result.addChild(assertion);
    }
  }


  //----------------------------------------------------------------------------
  protected validateCCN()
  {
    shared_ptr<QgSettings> settings = new QgSettings("PanelCheck.panel.panel.CCN");

    if (settings.isEnabled())
    {
      shared_ptr <QgVersionResult> assertion = new QgVersionResult();
      assertion.setMsgCatName("QgStaticCheck_Panels");
      // info only, dont check the values
      assertion.setAssertionText("assert.panel.CCN");
      assertion.setReasonText("reason.panel.CCN", makeMapping("panel.name", getName(),
                              "panel.CCN", getCCN()));
      assertion.info(getCCN(), settings.getScorePoints());
      result.addChild(assertion);
    }
  }

  protected validateAvgCCN()
  {
    shared_ptr<QgSettings> settings = new QgSettings("PanelCheck.panel.avgCCN");

    if (settings.isEnabled())
    {
      shared_ptr <QgVersionResult> assertion = new QgVersionResult();
      assertion.setMsgCatName("QgStaticCheck_Panels");
      assertion.setAssertionText("assert.panel.avgCCN");
      assertion.setReasonText("reason.panel.avgCCN", makeMapping("panel.name", getName(),
                              "panel.avgCCN", getAvgCCN()));
      assertion.info(getAvgCCN(), settings.getScorePoints());
      result.addChild(assertion);
    }
  }

  protected validateNLOC()
  {
    shared_ptr<QgSettings> settings = new QgSettings("PanelCheck.panel.NLOC");

    if (settings.isEnabled())
    {
      shared_ptr <QgVersionResult> assertion = new QgVersionResult();
      assertion.setMsgCatName("QgStaticCheck_Panels");
      assertion.setAssertionText("assert.panel.NLOC");
      assertion.setReasonText("reason.panel.NLOC", makeMapping("panel.name", getName(),
                              "panel.NLOC", getNLOC()));
      assertion.info(getNLOC(), settings.getScorePoints());
      result.addChild(assertion);
    }
  }

  protected validateAvgNLOC()
  {
    shared_ptr<QgSettings> settings = new QgSettings("PanelCheck.panel.avgNLOC");

    if (settings.isEnabled())
    {
      shared_ptr <QgVersionResult> assertion = new QgVersionResult();
      assertion.setMsgCatName("QgStaticCheck_Panels");
      assertion.setAssertionText("assert.panel.avgNLOC");
      assertion.setReasonText("reason.panel.avgNLOC", makeMapping("panel.name", getName(),
                              "panel.avgNLOC", getAvgNLOC()));
      assertion.info(getAvgNLOC(), settings.getScorePoints());
      result.addChild(assertion);
    }
  }

  protected validateEvents()
  {
    shared_ptr <QgVersionResult> assertion = new QgVersionResult();
    assertion.setMsgCatName("QgStaticCheck_Panels");

    //----------------------------------------------------------------------------
    // validate events
    if (_pnl.getCountOfEvents() > 0)
    {
      shared_ptr <QgVersionResult>  ev = new QgVersionResult();
      ev.setAssertionText("panel.events");

      while (_pnl.getCountOfEvents() > 0)
      {
        const anytype key = mappingGetKey(_pnl.events, 1);
        _pnl.events[key].validate();
        ev.addChild(_pnl.events[key].result);
        mappingRemove(_pnl.events, key);
      }

      result.addChild(ev);
    }
  }

  protected validateShapes()
  {
    //----------------------------------------------------------------------------
    // validate shapes
    if (_pnl.getCountOfShapes() > 0)
    {
      shared_ptr <QgVersionResult> sh = new QgVersionResult();
      sh.setAssertionText("panel.shapes");

      while (_pnl.getCountOfShapes() > 0)
      {
        _pnl.shapes[1].validate();
        sh.addChild(_pnl.shapes[1].result);
        dynRemove(_pnl.shapes, 1);
      }

      result.addChild(sh);
    }
  }

  //----------------------------------------------------------------------------
  // validate properties
  protected validateProperties()
  {
    shared_ptr <QgVersionResult> assertion = new QgVersionResult();
    assertion.setMsgCatName("QgStaticCheck_Panels");

    if (_pnl.getCountOfProperties() > 0)
    {
      shared_ptr <QgVersionResult>  prop = new QgVersionResult();
      prop.setAssertionText("panel.properties");

      while (_pnl.getCountOfProperties() > 0)
      {
        ///@todo probably place for checking properties
        string key = mappingGetKey(_pnl.properties, 1);
        shared_ptr <QgVersionResult>  property = new QgVersionResult();
        property.setAssertionText(key);
        property.info(_pnl.properties[key]);
        mappingRemove(_pnl.properties, key);
        prop.addChild(property);
      }

      result.addChild(prop);
    }
  }



//--------------------------------------------------------------------------------
//@private members
//--------------------------------------------------------------------------------

  static string _sourceDir = PROJ_PATH;
  static dyn_string _enabledExtensions = makeDynString();
  string _extension;

  PanelFile _pnl = PanelFile();

  bool _isBackUp;
//   string _filePath = "";

  bool _isCalculated;
  const int DEFAULT_EVENTCOUNT_HIGH = 100;
  const int DEFAULT_SHAPECOUNT_HIGH = 100;
};
