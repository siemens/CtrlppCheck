//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2022 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//

#uses "classes/QualityGates/Tools/Lizard/ToolLizard"
#uses "classes/QualityGates/QgSettings"

class FunctionData
{
//--------------------------------------------------------------------------------
//@public members
//--------------------------------------------------------------------------------

  //------------------------------------------------------------------------------
  public shared_ptr <QgVersionResult> result; //!< Quality gate result

  //------------------------------------------------------------------------------
  public FunctionData()
  {
  }

  //------------------------------------------------------------------------------
  public int fillFromCsv(const dyn_string &line)
  {
    if ( dynlen(line) < (int)LizardCsvIndx::PARAMS )
    {
      return -1; // there might be an fault - lizard format not valid possibly
    }

    _nloc = (int)line[(int)LizardCsvIndx::NLOC];
    _ccn = (int)line[(int)LizardCsvIndx::CCN];
//       func["TOKENS"] = (int)line[(int)LizardCsvIndx::TOKEN_COUNT]; // doesn't work ??? Lizard error ???
    _paramCount = (int)line[(int)LizardCsvIndx::PARAM_COUNT];
    _linesCount = (int)line[(int)LizardCsvIndx::LINES];
    _synopsis = line[(int)LizardCsvIndx::PARAMS];
    _name = line[(int)LizardCsvIndx::FUNCTION_NAME];

    // check c-tor
    // trimm :: from function name like FunctionData::FunctionData()
    // and compare 1. with 3. item.
    _isCtor = FALSE;
    dyn_string items = strsplit(_name, "::");
    // strsplit returns 3 items "FunctionData", "", "FunctionData" --> (oa-Bug)
    if ( dynlen(items) == 3 )
    {
      _isCtor = (items[1] == items[3]);
    }

    return 0;
  }

  //------------------------------------------------------------------------------
  /// cyclomatic complex. number
  public int getCCN()
  {
    return _ccn;
  }

  //------------------------------------------------------------------------------
  // lines of comment
  public int getLOC()
  {
    return getLinesCount() - getNLOC();
  }

  //------------------------------------------------------------------------------
  // no. lines of code
  public int getNLOC()
  {
    return _nloc;
  }

  //------------------------------------------------------------------------------
  public int getParamCount()
  {
    return _paramCount;
  }

  //------------------------------------------------------------------------------
  public int getLinesCount()
  {
    return _linesCount;
  }

  //------------------------------------------------------------------------------
  public string getSynopsis()
  {
    return _synopsis;
  }

  //------------------------------------------------------------------------------
  public string getName()
  {
    return _name;
  }

  //------------------------------------------------------------------------------
  public string isCtor()
  {
    return _isCtor;
  }

  //------------------------------------------------------------------------------
  public int validate()
  {
    result = new QgVersionResult();
    result.text = getName();

    validateCCN();
    validateNLOC();
    validateParamCount();
    validateCountOfLines();
    return 0;
  }

//--------------------------------------------------------------------------------
//@protected members
//--------------------------------------------------------------------------------


  //------------------------------------------------------------------------------
  protected validateCCN()
  {
    shared_ptr<QgSettings> settings = new QgSettings("FunctionData.function.CCN");

    if ( settings.isEnabled() )
    {
      shared_ptr <QgVersionResult> assertion = new QgVersionResult();
      assertion.setMsgCatName("QgStaticCheck_FunctionData");

      const mapping dollars = makeMapping("function.name", getName(),
                                          "function.CCN", getCCN());
      assertion.setAssertionText("assert.function.CCN", dollars);
      assertion.setReasonText("reason.function.CCN", dollars);
      assertion.assertLessEqual(getCCN(),
          settings.getHighLimit(DEFAULT_CNN_HIGH),
          settings.getScorePoints());
      result.addChild(assertion);
    }
  }

  //------------------------------------------------------------------------------
  protected validateNLOC()
  {
    string path = isCtor() ? "FunctionData.function.NLOC.ctor" : "FunctionData.function.NLOC";
    shared_ptr<QgSettings> settings = new QgSettings(path);

    if ( settings.isEnabled() )
    {
      shared_ptr <QgVersionResult> assertion = new QgVersionResult();
      assertion.setMsgCatName("QgStaticCheck_FunctionData");

      const mapping dollars = makeMapping("function.name", getName(),
                                          "function.NLOC", getNLOC());
      assertion.setAssertionText("assert.function.NLOC", dollars);
      assertion.setReasonText("reason.function.NLOC", dollars);

      assertion.assertBetween(getNLOC(),
          settings.getLowLimit(DEFAULT_NLOC_LOW),
          settings.getHighLimit(DEFAULT_NLOC_HIGH),
          settings.getScorePoints());
      result.addChild(assertion);
    }
  }

  //------------------------------------------------------------------------------
  protected validateParamCount()
  {
    shared_ptr<QgSettings> settings = new QgSettings("FunctionData.function.paramCount");

    if ( settings.isEnabled() )
    {
      shared_ptr <QgVersionResult> assertion = new QgVersionResult();
      assertion.setMsgCatName("QgStaticCheck_FunctionData");

      const mapping dollars = makeMapping("function.name", getName(),
                                          "function.paramCount", getNLOC());
      assertion.setAssertionText("assert.function.paramCount", dollars);
      assertion.setReasonText("reason.function.paramCount", dollars);
      assertion.assertLessEqual(getParamCount(),
          settings.getHighLimit(10),
          settings.getScorePoints());
      result.addChild(assertion);
    }
  }

  //------------------------------------------------------------------------------
  protected validateCountOfLines()
  {
    shared_ptr<QgSettings> settings = new QgSettings("FunctionData.function.countOfLines");

    if ( settings.isEnabled() )
    {
      shared_ptr <QgVersionResult> assertion = new QgVersionResult();
      assertion.setMsgCatName("QgStaticCheck_FunctionData");

      const mapping dollars = makeMapping("function.name", getName(),
                                          "function.countOfLines", getLinesCount());
      assertion.setAssertionText("assert.function.countOfLines", dollars);
      assertion.setReasonText("reason.function.countOfLines", dollars);
      assertion.info(getLinesCount(), settings.getScorePoints());
      result.addChild(assertion);
    }
  }

//--------------------------------------------------------------------------------
//@private members
//--------------------------------------------------------------------------------

  int _nloc, _ccn, _paramCount, _linesCount;
  string _synopsis, _name;
  bool _isCtor;
  const int DEFAULT_CNN_HIGH  = 15;
  const int DEFAULT_NLOC_LOW  = 3;
  const int DEFAULT_NLOC_HIGH = 20;
  const int DEFAULT_PARAMCOUNT_HIGH  = 10;
};
