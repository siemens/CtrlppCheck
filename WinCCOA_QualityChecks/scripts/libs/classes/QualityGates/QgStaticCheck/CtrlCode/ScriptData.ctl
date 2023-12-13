//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2023 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//

#uses "csv"
#uses "classes/FileSys/QgFile"
#uses "classes/QualityGates/QgAddOnResultErr"
#uses "classes/QualityGates/QgStaticCheck/CtrlCode/FunctionData"
#uses "classes/QualityGates/QgResult"
#uses "classes/QualityGates/QgSettings"
#uses "classes/QualityGates/Tools/Lizard/ToolLizard"
#uses "classes/QualityGates/Tools/Python/Python"
#uses "classes/Variables/Float"

/**
  Checks for static script data.
  Script data are basic information about the script.
  - CCN
  - NLOC
  - Functions list
  - CountOfLines
  - Average values

 @note Call function calculate() before you want acces some file information.
       C-tor does not read the file to eliminate performacne.
*/
class ScriptData
{
//--------------------------------------------------------------------------------
//@public members
//--------------------------------------------------------------------------------

  //---------------------------------------------------------------------------
  public shared_ptr <QgResult> result; //!< Quality gate result

  //---------------------------------------------------------------------------
  /**
    Default c-tor
    @param filePath Full native path to file there shall be checked.
  */
  public ScriptData(const string filePath = "")
  {
    setPath(filePath);
  }

  //---------------------------------------------------------------------------
  /**
    Set path to the checked file.
    It shall be used before calculation.
    @param filePath Full native path to file there shall be checked.
  */
  public void setPath(const string &filePath)
  {
    _filePath = filePath;
  }

  //---------------------------------------------------------------------------
  /**
    Return the checked file name with extension.
    @note Not full path only the file name.
    */
  public string getName()
  {
    return baseName(_filePath);
  }

  //---------------------------------------------------------------------------
  /**
    Returns TRUE when file is calcualted, otherwise false.
    */
  public bool isCalculated()
  {
    return _isCalculated;
  }

  //---------------------------------------------------------------------------
  /**
    Returns count of function located in file.
    */
  public int getCountOfFunctions()
  {
    return dynlen(_functions);
  }

  //---------------------------------------------------------------------------
  /**
    Returns clount of lines in the script.
    Count of all lines (code, comments, empty ...)
    */
  public int getCountOfLines()
  {
    return _linesCount;
  }

  //---------------------------------------------------------------------------
  /**
    Returns CCN (cyclomatic complexicity) of script.
    CCN of all functions.
    */
  public int getCCN()
  {
    return _ccn;
  }

  //---------------------------------------------------------------------------
  /**
    Returns NLOC (NumberLinesOfCode) of script.
    Pure code lines count (without comments, empty lines)
    */
  public int getNLOC()
  {
    return _nloc;
  }

  //---------------------------------------------------------------------------
  /**
    Returns average CCN of script.
    Seel also function getCCN() .
    */
  public float getAvgCCN()
  {
    float count = getCountOfFunctions();

    if (count <= 0)
      return 0.0;

    Float f = Float((float)_ccn / count);
    return f.round();
  }

  //---------------------------------------------------------------------------
  /**
    Returns average NLOC of script.
    Seel also function getNLOC() .
    */
  public float getAvgNLOC()
  {
    float count = getCountOfFunctions();

    if (count <= 0)
      return 0.0;

    Float f = Float((float)_nloc / count);
    return f.round();
  }

  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  /**
    Returns average count of lines per function in script.
    */
  public float getAvgLines()
  {
    return _avgLines;
  }

  //---------------------------------------------------------------------------
  /**
    Returns maximum enabled count of functions, there can be located in script.
    Quality limit.
    */
  public static int getMaxCountOfFunctions()
  {
    shared_ptr<QgSettings> settings = new QgSettings("ScriptData.script.countOfFunctions");
    return (int)settings.getHighLimit(DEFAULT_FUNCCOUNT_HIGH);
  }

  //---------------------------------------------------------------------------
  /**
    Returns minimun enabled count of functions, there can be located in script.
    Quality limit.
    */
  public static int getMinCountOfFunctions()
  {
    shared_ptr<QgSettings> settings = new QgSettings("ScriptData.script.countOfFunctions");
    return (int)settings.getLowLimit(DEFAULT_FUNCCOUNT_LOW);
  }

  //---------------------------------------------------------------------------
  /**
    Returns maximum enabled NLOC.
    Quality limit.
    See also getNLOC().
    */
  public static int getMaxNLOC()
  {
    shared_ptr<QgSettings> settings = new QgSettings("ScriptData.script.NLOC");
    return (int)settings.getHighLimit(DEFAULT_NLOC_HIGH);
  }

  //---------------------------------------------------------------------------
  /**
    Returns minimum enabled NLOC.
    Quality limit.
    See also getNLOC().
    */
  public static int getMinNLOC()
  {
    shared_ptr<QgSettings> settings = new QgSettings("ScriptData.script.NLOC");
    return (int)settings.getLowLimit(DEFAULT_NLOC_LOW);
  }

  //---------------------------------------------------------------------------
  /**
    Returns maximum enabled average CCN.
    Quality limit.
    See also getCCN().
    */
  public static float getMaxAvgCCN()
  {
    shared_ptr<QgSettings> settings = new QgSettings("ScriptData.script.avgCCN");
    return (float)settings.getHighLimit(DEFAULT_AVGCCN_HIGH);
  }

  //---------------------------------------------------------------------------
  /**
    Returns everage count of functions parameters.
    Quality limit.
    */
  public float getAvgParamCount()
  {
    return _avgParamCount;
  }

  //---------------------------------------------------------------------------
  /**
    Function calculate script data.

    @details Script data are calculated by lizard. See also class ToolLizard.
    @note Lizard need python
    @warning We have modified the lizard, that the csv output returns also file summary.
             Default csv output returns only functions data.
             Summary is located in first line in format NLOC,CCN

    @return Return 0 when successfull.
  */
  public int calculate()
  {
    dynClear(_functions);
    _nloc = 0;
    _ccn = 0;
    _isCalculated = FALSE;

    if (!isfile(_filePath))
      return -1;

    string cmd;
    cmd = Python::getExecutable() + " " + ToolLizard::getBinDir() + "lizard.py --csv " + makeUnixPath(_filePath);
    string stdOut, stdErr;
    int rc = system(cmd, stdOut, stdErr);

    if (rc != 0)
    {
      DebugFTN("ScriptData", __FUNCTION__, "!!! check if lizard is installed", rc, cmd, stdErr);
      return -2;
    }

    dyn_dyn_string data;
    csvParseContent(stdOut, data, ",");
    stdOut = "";

    uint allFuncParams, allFuncLines;

    if (dynlen(data) > 0)
    {
      // the first line contains only summary for file in format: NLOC,CCN
      if (dynlen(data[1]) >= 2)
      {
        _nloc = data[1][1];
        _ccn  = data[1][2];
      }

      for (int i = 2; i <= dynlen(data); i++)
      {
        dyn_string line = data[i];

        FunctionData func = FunctionData();

        if (func.fillFromCsv(line))
        {
          continue; // shouldn't be possible / probably a fault in lizard format
        }

        allFuncParams += func.getParamCount();
        allFuncLines += func.getLinesCount();
        dynAppend(_functions, func);
      }
    }

    if (dynlen(data) > 0)
    {
      _avgLines = (float)allFuncLines / (float)dynlen(data);
      _avgParamCount = (float)allFuncParams / (float)dynlen(data);
    }

    {
      string str;
      fileToString(_filePath, str);

      if (str != "")
        str += " "; // otherwise last line could be ignored

      _linesCount = dynlen(strsplit(str, "\n"));
    }

    _isCalculated = TRUE;
    return 0;
  }

  //---------------------------------------------------------------------------
  /**
    Validate script data.
    @return Return 0 when successfull.
    */
  public int validate()
  {
    const mapping dollars = makeMapping("script.name", getName());
    result = new QgResult("QgStaticCheck_ScriptData", "function", dollars);

    if (!validateIsCalucalted())
      return 0;

    validateCountOfFunctions();
    validateAvgCCN();
    validateNLOC();
    validateAvgNLOC();
    validateFunctions();
    return 0;
  }

//--------------------------------------------------------------------------------
//@protected members
//--------------------------------------------------------------------------------

  //---------------------------------------------------------------------------
  protected dyn_anytype _functions;  //!< list with functions data.
  protected string _filePath = "";   //!< Full native path to the script.

  //---------------------------------------------------------------------------
  /**
    Validate calculation state of the script.
    @detals There are more reasons, why the script is not calculated
      - does not exist
      - can not be readed
      - crypted
      - ...
    @return Return 0 when successfull.
    */
  protected int validateIsCalucalted()
  {
    shared_ptr<QgSettings> settings = new QgSettings("ScriptData.script.isCalculated");

    if (settings.isEnabled())
    {
      // check if file is calculated.
      // ognore all not calculated files (crypted, empty files ...)
      const mapping dollars = makeMapping("script.name", getName());
      shared_ptr <QgResult> assertion = new QgResult("QgStaticCheck_ScriptData", "script.isCalculated", dollars);

      if (!assertion.assertTrue(isCalculated(), settings.getScorePoints()))
      {
        result.addChild(assertion);
        return 0;
      }
    }

//     result.addChild(assertion); // sonnst doppelt drinnen ist
    return 1;
  }

  //---------------------------------------------------------------------------
  /**
    Validate count of function in the script.
    Enabled count of functions are depend of the script type. For example scope of panel
    has other limits like a event click or some library.
    Per default does not need the script some functions. Only panel events.
    For more info see also
      + getCountOfFunctions()
      + getMinCountOfFunctions()
      + getMaxCountOfFunctions()
    @return Return 0 when successfull.
    */
  protected validateCountOfFunctions()
  {
    shared_ptr<QgSettings> settings = new QgSettings("ScriptData.script.countOfFunctions");

    if (settings.isEnabled())
    {
      // check count of functions.
      const mapping dollars = makeMapping("script.name", getName(), "script.countOfFunctions", getCountOfFunctions());
      shared_ptr <QgResult> assertion = new QgResult("QgStaticCheck_ScriptData", "script.countOfFunctions", dollars);
      assertion.assertBetween(getCountOfFunctions(), getMinCountOfFunctions(), getMaxCountOfFunctions(), settings.getScorePoints());
      result.addChild(assertion);
    }
  }

  //---------------------------------------------------------------------------
  /**
    Validate average CCN of the script.
    Check average CCN (Cyclomatic complexicity -McCabe)
    */
  protected validateAvgCCN()
  {
    if (getCountOfFunctions() > 1)   // only when has more then 1 function
    {
      shared_ptr<QgSettings> settings = new QgSettings("ScriptData.script.avgCCN");

      if (settings.isEnabled())
      {
        const mapping dollars = makeMapping("script.name", getName(), "script.avgCCN", getAvgCCN());
        shared_ptr <QgResult> assertion = new QgResult("QgStaticCheck_ScriptData", "script.avgCCN", dollars);
        assertion.assertLessEqual(getAvgCCN(), getMaxAvgCCN(), settings.getScorePoints());
        result.addChild(assertion);
      }
    }
  }

  //---------------------------------------------------------------------------
  /**
    Validate NLOC of the script.
    Check NLOC - Noumber Line Of Code
    */
  protected validateNLOC()
  {
    shared_ptr<QgSettings> settings = new QgSettings("ScriptData.script.NLOC");

    if (settings.isEnabled())
    {
      const mapping dollars = makeMapping("script.name", getName(), "script.NLOC", getNLOC());
      shared_ptr <QgResult> assertion = new QgResult("QgStaticCheck_ScriptData", "script.NLOC", dollars);
      assertion.assertBetween(getNLOC(), getMinNLOC(), getMaxNLOC(), settings.getScorePoints());
      result.addChild(assertion);
    }
  }

  //---------------------------------------------------------------------------
  /**
    Validate average NLOC.
    Check average NLOC - Noumber Line Of Code
  */
  protected validateAvgNLOC()
  {
    if (getCountOfFunctions() > 1)   // only when has more then 1 function
    {
      shared_ptr<QgSettings> settings = new QgSettings("ScriptData.script.avgNLOC");

      if (settings.isEnabled())
      {
        const mapping dollars = makeMapping("script.name", getName(), "script.avgNLOC", getAvgNLOC());
        shared_ptr <QgResult> assertion = new QgResult("QgStaticCheck_ScriptData", "script.avgNLOC", dollars);
        assertion.info(getAvgNLOC(), settings.getScorePoints()); // does not check it, only information character
        //     assertion.assertLessEqual(getAvgNLOC(), getMaxAvgCCN());
        result.addChild(assertion);
      }
    }
  }

  //---------------------------------------------------------------------------
  /**
    Validate functions in script.
    Check each functions located in script.
  */
  protected validateFunctions()
  {
    // check all functions too.
    if (getCountOfFunctions() > 0)
    {
      const mapping dollars = makeMapping("script.name", getName(), "functionsList", getAvgNLOC());
      shared_ptr <QgResult> assertion = new QgResult("QgStaticCheck_ScriptData", "functionsList", dollars);

      for (int i = 1; i <= dynlen(_functions); i++)
      {
        _functions[i].validate();
        functions.addChild(_functions[i].result);
      }

      result.addChild(functions);
    }
  }

//--------------------------------------------------------------------------------
//@private members
//--------------------------------------------------------------------------------
  int _nloc, _ccn, _linesCount;
  float _avgLines, _avgParamCount;
  bool _isCalculated;
  static const int DEFAULT_FUNCCOUNT_HIGH = 100;
  static const int DEFAULT_FUNCCOUNT_LOW  = 0;
  static const int DEFAULT_NLOC_HIGH      = 600;
  static const int DEFAULT_NLOC_LOW       = 1;
  static const float DEFAULT_AVGCCN_HIGH  = 10.0;
};
