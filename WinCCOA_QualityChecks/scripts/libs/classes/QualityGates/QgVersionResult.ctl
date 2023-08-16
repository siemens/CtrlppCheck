//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2022 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//

#uses "csv"
#uses "classes/QualityGates/QgMsgCat"
#uses "classes/QualityGates/Qg"
#uses "classes/QualityGates/QgBaseError"
#uses "classes/QualityGates/QgTest"
#uses "classes/Variables/Float"

enum QgVersionResultType
{
  TableView,
  TreeView,
  SimpleTreeView
};

enum QgVersionResultJsonFormat
{
  Indented, //!< Defines human readable output
  Compact   //!< Defines a compact output. More performant for parsing.
};


QgVersionResultType qgVersionResultType = QgVersionResultType::TableView;

struct QgVersionResult
{

//--------------------------------------------------------------------------------
//@public members
//--------------------------------------------------------------------------------
  static bool showErrorsOnly = FALSE;
  string lowerBound;
  string upperBound;
  string value;
  string referenceValue;


  bool hasError;
  dyn_anytype children;
  static string lastErr;
  QgMsgCat msgCat = QgMsgCat();

  int totalPoints;
  int errorPoints;


  string text;
  string  assertKey;
  mapping assertDollars;

  string reason;
  string  reasonKey;
  mapping reasonDollars;
  string location;

  static const float NOT_VALID_SCORE = 0.0;
  float  score    = NOT_VALID_SCORE;

  static const float MIN_VALID_SCORE = 1.0;

  //------------------------------------------------------------------------------
  QgVersionResult()
  {
  }

  //------------------------------------------------------------------------------
  void setLocation(const string &location)
  {
    this.location = location;
  }

  //------------------------------------------------------------------------------
  string getLocation()
  {
    return this.location;
  }

  //------------------------------------------------------------------------------
  void setMsgCatName(const string &name)
  {
    msgCat.setName(name);
  }

  //------------------------------------------------------------------------------
  string getLastErr()
  {
    return lastErr;
  }

  //------------------------------------------------------------------------------
  public void setAssertionText(const string &key, const mapping dollars = makeMapping())
  {
    assertKey = key;
    assertDollars = dollars;
  }

  //------------------------------------------------------------------------------
  public void setReasonText(const string &key, const mapping dollars = makeMapping())
  {
    reasonKey = key;
    reasonDollars = dollars;
  }

  //------------------------------------------------------------------------------
  mapping sumToMap()
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

    switch (qgVersionResultType)
    {
      case QgVersionResultType::TableView:
      {
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
        break;
      }

      case QgVersionResultType::SimpleTreeView:
      case QgVersionResultType::TreeView:
      {
        map[KEY_SCORE_TOTAL_POINTS] = totalPoints;
        map[KEY_SCORE_ERROR_POINTS] = errorPoints;
        map[KEY_SCORE_PERCENT] = f.round(2);
        break;
      }

    }

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
  public int getTotalPoints()
  {
    return totalPoints;
  }

  //------------------------------------------------------------------------------
  /** @brief Function returns count of error points.
    @return Count of errors.
  */
  public int getErrorPoints()
  {
    return errorPoints;
  }

  //------------------------------------------------------------------------------
  public void toStdOut()
  {
    DebugN(__FUNCTION__, text, assertKey, dynlen(children), lowerBound, upperBound, referenceValue);
  }

  //------------------------------------------------------------------------------
  anytype toMap(const bool clearObjectOnReturn = TRUE)
  {
    mapping map;
    string goodRange;

    if ((lowerBound != "") || (upperBound != ""))
      goodRange = lowerBound + " " + _operand + " " + upperBound;
    else if (referenceValue != "")
      goodRange = referenceValue;

    switch (qgVersionResultType)
    {
      case QgVersionResultType::TableView:
      {
        if (text != "")
          map["text"] = text;
        else if (assertKey != "")
          map["text"] = msgCat.getText(assertKey, assertDollars);


        if (value != "")
          map["value"] = value;

        map["leaf"] = (dynlen(children) <= 0);

        if (goodRange != "")
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
          if (reason != "")
            map["reason"] = reason;
          else if (reasonKey != "")
            map["reason"] = msgCat.getText(reasonKey, reasonDollars);
        }

        break;
      }

      case QgVersionResultType::TreeView:
      {
        if (goodRange != "")
          map["goodRange"] = goodRange;

        if (value != "")
          map["value"] = value;

        if (errorPoints > 0)
        {
          map["totalPoints"] = totalPoints;
          map["errorPoints"] = errorPoints;
        }
        else if (totalPoints > 0)
          map["totalPoints"] = totalPoints;

        if (hasError && reason != "")
          map["reason"] = reason;


        if (dynlen(children))
        {
          for (int i = 1; i <= dynlen(children); i++)
          {
            map[children[i].text] = children[i].toMap();
          }
        }

        break;
      }

      case QgVersionResultType::SimpleTreeView:
      {
        dyn_string ret;

        if (goodRange != "")
          dynAppend(ret, "goodRange: " + goodRange);

        if (value != "")
          dynAppend(ret, "value: " + value);

        if (errorPoints > 0)
        {
          dynAppend(ret, "errorPoints: " + errorPoints);
        }

        if (hasError && reason != "")
          dynAppend(ret, "reason: " + reason);


        if (dynlen(children))
        {
          for (int i = 1; i <= dynlen(children); i++)
          {
            map[children[i].text] = children[i].toMap();
          }

          if (clearObjectOnReturn)
            clear();

          return map;
        }

        if (clearObjectOnReturn)
          clear();

        return strjoin(ret, ", ");
      }
    }

    if (clearObjectOnReturn)
      clear();

    return map;
  }


  //------------------------------------------------------------------------------
  bool assertGreatherEqual(const anytype &currentValue, const anytype &refValue, int points = 1)
  {
    value = (string)_castToString(currentValue);
    upperBound = (string)_castToString(refValue);

    _operand = ">=";
    hasError = !(currentValue >= refValue);
    _addScorePoints(points);
    return !hasError;
  }

  //------------------------------------------------------------------------------
  bool assertLessEqual(const anytype &currentValue, const anytype &refValue, int points = 1)
  {
    value = (string)_castToString(currentValue);
    upperBound = (string)_castToString(refValue);

    _operand = "<=";
    hasError = !(currentValue <= refValue);
    _addScorePoints(points);
    return !hasError;
  }

  //------------------------------------------------------------------------------
  bool assertEqual(const anytype &currentValue, const anytype &refValue, int points = 1)
  {
    value = (string)_castToString(currentValue);
    upperBound = (string)_castToString(refValue);

    _operand = "==";
    hasError = !(currentValue == refValue);
    _addScorePoints(points);
    return !hasError;
  }

  //------------------------------------------------------------------------------
  bool assertFalse(const bool condition, int points = 1)
  {
    value = (string)condition;
    referenceValue = (string)FALSE;
    hasError = condition;
    _addScorePoints(points);
    return !hasError;
  }

  //------------------------------------------------------------------------------
  bool assertTrue(const bool condition, int points = 1)
  {
    value = (string)condition;
    referenceValue = (string)TRUE;
    hasError = !condition;
    _addScorePoints(points);
    return !hasError;
  }

  //------------------------------------------------------------------------------
  bool assertDynContains(const dyn_anytype &list, const anytype &refValue, int points = 1)
  {
    value = (string)refValue;
    referenceValue = strjoin(list, ", ");
    hasError = (dynContains(list, refValue) <= 0);
    _addScorePoints(points);
    return !hasError;
  }

  //------------------------------------------------------------------------------
  bool assertBetween(const anytype &currentValue, const anytype &lowerLimit, const anytype &upperLimit, int points = 1)
  {
    value = (string)_castToString(currentValue);
    upperBound = (string)_castToString(upperLimit);
    lowerBound = (string)_castToString(lowerLimit);
    hasError = (lowerLimit > currentValue) || (upperLimit < currentValue);
    _addScorePoints(points);
    return !hasError;
  }

  //------------------------------------------------------------------------------
  bool info(const anytype &currentValue, int points = 1)
  {
    value = (string)_castToString(currentValue);
    referenceValue = (string)"-";

    _operand = "";
    hasError = FALSE;
    _addScorePoints(points);
    return !hasError;
  }

  //------------------------------------------------------------------------------
  public void setMinValidScore(const string &keyText, const string &keyReason,
                               const mapping dollars = makeMapping())
  {
    setAssertionText(keyText, dollars);
    setReasonText(keyReason, dollars);
    hasError = TRUE;

    _addScorePoints();

    // min valid score must probably set
    totalPoints = 100;
    errorPoints = 99;
  }

  //------------------------------------------------------------------------------
  public void setNotValidScore(const string &keyText, const string &keyReason,
                               const mapping dollars = makeMapping())
  {
    shared_ptr <QgVersionResult> child = new QgVersionResult();
    child.setAssertionText(keyText, dollars);
    child.setReasonText(keyReason, dollars);
    child.hasError = TRUE;
    child.totalPoints = 0;
    child.errorPoints = 0;
    addChild(child);

    hasError = TRUE;

    _addScorePoints();

    // non-valid (null) score must probably set
    totalPoints = 0;
    errorPoints = 0;
  }

  //------------------------------------------------------------------------------
  void addChild(shared_ptr<QgVersionResult> child, const int pos = -1)
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
  void clear()
  {
    lowerBound = "";
    upperBound = "";
    value = "";
    referenceValue = "";
    text = "";
    reason = "";
    hasError = FALSE;
    dynClear(children);
    _operand = "-";
    totalPoints = 0;
    errorPoints = 0;
  }

  //------------------------------------------------------------------------------
  void allowNextErr(bool flag)
  {
    _allowNextErr = flag;
  }


//--------------------------------------------------------------------------------
//@protected members
//--------------------------------------------------------------------------------
  //------------------------------------------------------------------------------
  protected bool _allowNextErr;

  //------------------------------------------------------------------------------
  protected _addScorePoints(int points = 1)
  {
    mapping userData;

    if (location != "")
    {
      assertDollars["location"] = location;
      userData["Location"] = location;
    }

    userData["Note"] = msgCat.getText(assertKey, assertDollars);

    userData["Method"] = assertKey;
    userData["ErrMsg"] = msgCat.getText(reasonKey, reasonDollars);
    userData["StackTrace"] = makeDynString();

    getKnownBugId(userData);

    totalPoints += points;

    if (hasError && !_allowNextErr)
    {
      errorPoints += points;
      lastErr = reason;

      if (_enableOaTestOutput())
        oaUnitFail(assertKey, userData);
      else
      {
        const int prio = mappingHasKey(userData, "KnownBug") ? PRIO_INFO : PRIO_WARNING;
        OaLogger logger = OaLogger("QgBase");

        if (mappingHasKey(userData, "KnownBug"))
          logger.info(QgBaseError::AssertionErrorAccepted, msgCat.getText(reasonKey, reasonDollars), userData["Note"]);
        else
          logger.warning(QgBaseError::AssertionError, msgCat.getText(reasonKey, reasonDollars), userData["Note"]);
      }
    }
    else
    {
      if (_enableOaTestOutput() || true)
        oaUnitPass(assertKey, userData);

      // else
      {
        OaLogger logger = OaLogger("QgBase");
        logger.info(QgBaseError::AssertionOK, assertKey, userData);
      }
    }

    _allowNextErr = FALSE;
  }

  //------------------------------------------------------------------------------
  /// @todo replace this code by OaTest-knownBug-handler
  protected getKnownBugId(mapping &userData)
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

      if ((tcId == assertKey) && patternMatch(pattern, msg))
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
  protected static bool _enableOaTestOutput()
  {
    return QgTest::isStartedByTestFramework();
  }

  //------------------------------------------------------------------------------
  protected static string _castToString(const anytype &expr)
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


//--------------------------------------------------------------------------------
//@private members
//--------------------------------------------------------------------------------
  private string _operand = "-";

  /// @todo mPunk 30.10.2018: remove this contans and replace it by msg-cat, there shall be obsolete
  static const string KEY_SCORE_REASON = "Reason";
  static const string KEY_SCORE_PERCENT = "%";
  static const string KEY_SCORE_TOTAL_POINTS = "Total points";
  static const string KEY_SCORE_ERROR_POINTS = "Error points";
  static dyn_dyn_string knownBugs;
};
