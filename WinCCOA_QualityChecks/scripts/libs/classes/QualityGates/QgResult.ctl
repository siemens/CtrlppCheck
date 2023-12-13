//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2023 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//

#uses "classes/ErrorHdl/OaLogger"
#uses "classes/QualityGates/QgMsgCat"
#uses "classes/QualityGates/Qg"
#uses "classes/QualityGates/QgTest"
#uses "classes/Variables/Float"
#uses "csv"

enum QgResultJsonFormat
{
  Indented, //!< Defines human readable output
  Compact   //!< Defines a compact output. More performant for parsing.
};

enum QgResultError
{
  AssertionError = 1,
  AssertionOK,
  AssertionErrorAccepted
};

class QgResult
{

//--------------------------------------------------------------------------------
//@public members
//--------------------------------------------------------------------------------
  /// enable OaTest checks. It may be disabled for testing purpouse
  public static bool selfTest = false;
  public static bool showErrorsOnly = FALSE;
  public string lowerBound;
  public string upperBound;
  public string value;
  public string referenceValue;
  public bool hasError;

  /// getTotalPoints() shall be used
  public int totalPoints;
  /// getTotalPoints() shall be used
  public int errorPoints;

  //------------------------------------------------------------------------------
  public QgResult(const string &catalog, const string &key, const mapping &dollars)
  {
    this.setMsgCatName(catalog);
    this.setKey(key);
    this.setDollars(dollars);
  }

  //------------------------------------------------------------------------------
  public void setKey(const string &key)
  {
    this.key = key;
  }

  //------------------------------------------------------------------------------
  public void setDollars(const mapping &dollars)
  {
    this.dollars = dollars;
  }

  //------------------------------------------------------------------------------
  public void setLocation(const string &location)
  {
    this.location = location;
  }

  //------------------------------------------------------------------------------
  public string getLocation()
  {
    return this.location;
  }

  //------------------------------------------------------------------------------
  public void setMsgCatName(const string &name)
  {
    msgCat.setName(name);
  }

  //------------------------------------------------------------------------------
  public static dyn_string getLastErrors()
  {
    return lastErr;
  }

  //------------------------------------------------------------------------------
  public static void clearLastErr()
  {
    dynClear(lastErr);
  }

  //------------------------------------------------------------------------------
  public mapping sumToMap()
  {
    return makeMapping("totalPoints", totalPoints,
                       "errorPoints", errorPoints,
                       "hasError", hasError);
  }

  //------------------------------------------------------------------------------
  public mapping scoreToMap()
  {
    Float f = Float(score);
    mapping map;

    map["children"] = makeDynMapping();

    map["children"][1] = makeMapping("value", totalPoints,
                                     "text", KEY_SCORE_TOTAL_POINTS,
                                     "leaf", TRUE,
                                     "goodRange", "> 0");
    map["children"][2] = makeMapping("value", errorPoints,
                                     "text", KEY_SCORE_ERROR_POINTS,
                                     "leaf", TRUE,
                                     "goodRange", "0");
    map["children"][3] = makeMapping("value", f.round(),
                                     "text", KEY_SCORE_PERCENT,
                                     "leaf", TRUE,
                                     "goodRange", "1 - 100");

    map["leaf"] = FALSE;
    map["expanded"] = TRUE;
    map["text"] = "Score";

    return map;
  }

  //------------------------------------------------------------------------------
  /** @brief Function calculates QG score.
    @details Calculates the QG-score depended of results. Calculate how many
             percente of test-cases are OK.
    @return Percentil of passed testcases.
  */
  public float calculateScore()
  {
    float error = getErrorPoints();
    float all   = getTotalPoints();
    float perc;

    if (all < 0)
      return NOT_VALID_SCORE;
    else if (all != 0)
    {
      perc = (error / all) * 100.0;
      perc = 100.0 - perc;
    }

    return perc;
  }

  //------------------------------------------------------------------------------
  /** @brief Function returns count of all points.
    @return Count of all points.
  */
  private int getTotalPoints()
  {
    return totalPoints;
  }

  //------------------------------------------------------------------------------
  /** @brief Function returns count of error points.
    @return Count of errors.
  */
  private int getErrorPoints()
  {
    return errorPoints;
  }

  //------------------------------------------------------------------------------
  private void toStdOut()
  {
    DebugN(__FUNCTION__, text, this.key, dynlen(children), lowerBound, upperBound, referenceValue);
  }

  //------------------------------------------------------------------------------
  public mapping toMap()
  {
    mapping map;
    string goodRange;

    if ((lowerBound != "") || (upperBound != ""))
      goodRange = lowerBound + " " + _operand + " " + upperBound;
    else if (referenceValue != "")
      goodRange = referenceValue;

    if (!this.text.isEmpty())
      map["text"] = this.text;

    if (!this.value.isEmpty())
      map["value"] = value;

    map["leaf"] = (dynlen(children) <= 0);

    if (!this.goodRange.isEmpty())
      map["goodRange"] = goodRange;

    if (dynlen(children) > 0)
    {
      map["children"] = makeDynMapping();

      for (int i = 1; i <= dynlen(children); i++)
      {
        if (showErrorsOnly && !children[i].hasError)
          continue;

        dynAppend(map["children"], children[i].toMap());
      }
    }

    if (hasError)
      map["expanded"] = hasError;

    if (errorPoints > 0)
    {
      map["totalPoints"] = totalPoints;
      map["errorPoints"] = errorPoints;
    }
    else if (totalPoints > 0)
      map["totalPoints"] = totalPoints;

    if (hasError)
    {
      map["reason"] = this.reason;
    }

    return map;
  }

  //------------------------------------------------------------------------------
  public bool assertGreatherEqual(const anytype &currentValue, const anytype &refValue, int points = 1)
  {
    value = (string)_castToString(currentValue);
    upperBound = (string)_castToString(refValue);

    _operand = ">=";
    hasError = !(currentValue >= refValue);
    _addScorePoints(points);
    return !hasError;
  }

  //------------------------------------------------------------------------------
  public bool assertLessEqual(const anytype &currentValue, const anytype &refValue, int points = 1)
  {
    value = (string)_castToString(currentValue);
    upperBound = (string)_castToString(refValue);

    _operand = "<=";
    hasError = !(currentValue <= refValue);
    _addScorePoints(points);
    return !hasError;
  }

  //------------------------------------------------------------------------------
  public bool assertEqual(const anytype &currentValue, const anytype &refValue, int points = 1)
  {
    value = (string)_castToString(currentValue);
    upperBound = (string)_castToString(refValue);

    _operand = "==";
    hasError = !(currentValue == refValue);
    _addScorePoints(points);
    return !hasError;
  }

  //------------------------------------------------------------------------------
  public bool assertFalse(const bool condition, int points = 1)
  {
    value = (string)condition;
    referenceValue = (string)FALSE;
    hasError = condition;
    _addScorePoints(points);
    return !hasError;
  }

  //------------------------------------------------------------------------------
  public bool assertTrue(const bool condition, int points = 1)
  {
    value = (string)condition;
    referenceValue = (string)TRUE;
    hasError = !condition;
    _addScorePoints(points);
    return !hasError;
  }

  //------------------------------------------------------------------------------
  public bool assertDynContains(const dyn_anytype &list, const anytype &refValue, int points = 1)
  {
    value = (string)refValue;
    referenceValue = strjoin(list, ", ");
    hasError = (dynContains(list, refValue) <= 0);
    _addScorePoints(points);
    return !hasError;
  }

  //------------------------------------------------------------------------------
  public bool assertBetween(const anytype &currentValue, const anytype &lowerLimit, const anytype &upperLimit, int points = 1)
  {
    value = (string)_castToString(currentValue);
    upperBound = (string)_castToString(upperLimit);
    lowerBound = (string)_castToString(lowerLimit);
    hasError = (lowerLimit > currentValue) || (upperLimit < currentValue);
    _addScorePoints(points);
    return !hasError;
  }

  //------------------------------------------------------------------------------
  public bool info(const anytype &currentValue, int points = 1)
  {
    value = (string)_castToString(currentValue);
    referenceValue = (string)"-";

    _operand = "";
    hasError = FALSE;
    _addScorePoints(points);
    return !hasError;
  }

  //------------------------------------------------------------------------------
  public void setMinValidScore()
  {
    hasError = TRUE;

    _addScorePoints();

    // min valid score must probably set
    totalPoints = 100;
    errorPoints = 99;
  }

  //------------------------------------------------------------------------------
  public void addChild(shared_ptr<QgResult> child, const int pos = -1)
  {
    if (pos <= 0)
      dynAppend(children, child);
    else
      dynInsertAt(children, child, pos);

    if (child.hasError)
      hasError = TRUE;

    errorPoints += child.errorPoints;
    totalPoints += child.totalPoints;
  }

  //------------------------------------------------------------------------------
  private void clear()
  {
    this.lowerBound = "";
    this.upperBound = "";
    this.value = "";
    this.referenceValue = "";
    this.text = "";
    this.hasError = FALSE;
    dynClear(this.children);
    this._operand = "-";
    this.totalPoints = 0;
    this.errorPoints = 0;
  }

  //------------------------------------------------------------------------------
  public void allowNextErr(bool flag)
  {
    _allowNextErr = flag;
  }

//--------------------------------------------------------------------------------
//@protected members
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
//@private members
//--------------------------------------------------------------------------------

  private OaLogger logger = OaLogger("QgResult");
  private string key;
  private mapping dollars;

  // cached assertion text and reason
  private string text;
  private string reason;


  private dyn_anytype children;
  private static dyn_string lastErr;
  private QgMsgCat msgCat = QgMsgCat();

  private string location;

  private static const float NOT_VALID_SCORE = 0.0;
  private float  score    = NOT_VALID_SCORE;

  private static const float MIN_VALID_SCORE = 1.0;

  private string _operand = "-";

  /// @todo mPunk 30.10.2018: remove this contans and replace it by msg-cat, there shall be obsolete
  private static const string KEY_SCORE_PERCENT = "%";
  private static const string KEY_SCORE_TOTAL_POINTS = "Total points";
  private static const string KEY_SCORE_ERROR_POINTS = "Error points";
  private static dyn_dyn_string knownBugs;

  //------------------------------------------------------------------------------
  private bool _allowNextErr;

  //------------------------------------------------------------------------------
  private _addScorePoints(int points = 1)
  {
    mapping userData;

    if (location != "")
    {
      this.dollars["location"] = location;
      userData["Location"] = location;
    }

    this.text = msgCat.getText(this.getAssertKey(), this.dollars);
    this.reason = msgCat.getText(this.getReasonKey(), this.dollars);
    // clear dollars to free memory
    mappingClear(this.dollars);

    userData["Note"] = this.text;

    userData["Method"] = this.getAssertKey();
    userData["ErrMsg"] = this.reason;
    userData["StackTrace"] = makeDynString();

    getKnownBugId(userData);

    totalPoints += points;

    // store last errors (only for internal testing)
    if (hasError && selfTest)
      this.lastErr.append(this.key);

    if (hasError && !_allowNextErr)
    {
      errorPoints += points;

      if (_enableOaTestOutput())
        oaUnitFail(this.key, userData);
      else
      {
        const int prio = mappingHasKey(userData, "KnownBug") ? PRIO_INFO : PRIO_WARNING;

        if (mappingHasKey(userData, "KnownBug"))
          logger.info(QgResultError::AssertionErrorAccepted, this.reason, userData["Note"]);
        else
          logger.warning(QgResultError::AssertionError, this.reason, userData["Note"]);
      }
    }
    else
    {
      if (_enableOaTestOutput())
      {
        oaUnitPass(this.key, userData);
      }
      else
      {
        logger.info(QgResultError::AssertionOK, this.key, userData);
      }
    }

    _allowNextErr = FALSE;

  }

  //------------------------------------------------------------------------------
  private string getAssertKey()
  {
    if (this.key.isEmpty())
      return "";

    return "assert." + this.key;
  }


  //------------------------------------------------------------------------------
  private string getReasonKey()
  {
    if (this.key.isEmpty())
      return "";

    return "reason." + this.key;
  }

  //------------------------------------------------------------------------------
  /// @todo replace this code by OaTest-knownBug-handler
  private getKnownBugId(mapping &userData)
  {
    if (dynlen(knownBugs) <= 0)
      readKnownBugList();

    string msg = userData["ErrMsg"];

    for (int i = 2; i <= dynlen(knownBugs); i++)
    {
      if (dynlen(knownBugs[i]) < 2)
        continue;

      string bugId = knownBugs[i][1];
      string tcId = knownBugs[i][2];
      string pattern = knownBugs[i][3];

      if ((tcId == this.key) && patternMatch(pattern, msg))
      {
        userData["KnownBug"] = bugId;
        break;
      }
    }
  }

  //------------------------------------------------------------------------------
  private readKnownBugList()
  {
    string path = getPath(DATA_REL_PATH, "knownBugList.csv");

    if (path != "")
      csvFileRead(path, knownBugs, ";");
    else
      knownBugs[1] = makeDynString("BUG_ID", "TC_ID", "PATTERN", "COMMENT");
  }

  //------------------------------------------------------------------------------
  /** @brief enabled or disabled oaUnitResults
    @return TRUE when are oaUnit results enabled
  */
  private static bool _enableOaTestOutput()
  {
    return QgTest::isStartedByTestFramework() && !selfTest;
  }

  //------------------------------------------------------------------------------
  private static string _castToString(const anytype &expr)
  {
    string str;

    if (getType(expr) == FLOAT_VAR)
    {
      Float f = Float(expr);
      str = (string)f.round(2); // round float
    }
    else
    {
      str = (string)expr;
    }

    return str;
  }
};
