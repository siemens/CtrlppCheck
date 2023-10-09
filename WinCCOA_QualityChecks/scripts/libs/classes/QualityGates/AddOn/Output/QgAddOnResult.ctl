
//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2023 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//

#uses "classes/QualityGates/Qg"
#uses "classes/QualityGates/AddOn/Output/QgAddOnScore"
#uses "classes/QualityGates/AddOn/FileSys/QgAddOnResultsDir"

enum QgAddOnResultState
{
  success,
  warning,
  failed,
  error
};


class QgAddOnResult
{
  //--------------------------------------------------------------------------------
//@public members
//--------------------------------------------------------------------------------
  public static const float MIN_VALID_SCORE = 1.0;
  public static const float NOT_VALID_SCORE = 0.0;
  public static const string KEY_SCORE_REASON = "Reason";
  public static const string KEY_SCORE_PERCENT = "%";
  public static const string KEY_SCORE_TOTAL_POINTS = "Total points";
  public static const string KEY_SCORE_ERROR_POINTS = "Error points";

  public static const string KEY_QG_RESULT_TESTVERSION = "qgTestVersionResults";
  public static const string KEY_QG_RESULT_SUM = "qgSummary";
  public static const string KEY_QG_RESULT_SCORE = "score";


  public QgAddOnResult()
  {
  }

  public void setData(const mapping &data)
  {
    _data = data;
  }

  public void setErr(int prio, int code, string note = "")
  {
    if (prio == PRIO_FATAL)
      prio = PRIO_SEVERE;

    errClass err;

    if (note == "")
      err = makeError("QgAddOnResultErr", prio, ERR_CONTROL, code, Qg::getId());
    else
      err = makeError("QgAddOnResultErr", prio, ERR_CONTROL, code, Qg::getId(), note);

    throwError(err);
    DebugFTN("QgAddOnResult", getStackTrace());

    switch (prio)
    {
      case PRIO_INFO:
        setState(QgAddOnResultState::success);
        break;

      case PRIO_WARNING:
        setState(QgAddOnResultState::warning);
        break;

      case PRIO_SEVERE:
        setState(QgAddOnResultState::failed);
        break;

      default:
        setState(QgAddOnResultState::error);
        break;
    }

    _hasErr = TRUE;
  }

  public int calculate()
  {
    DebugFTN("QgAddOnResult", __FUNCTION__);
    setState(QgAddOnResultState::success);
    return 0;
  }

  public void addScore(int score)
  {

    if (mappingHasKey(_data, "score"))
      score = _data["score"];

    QgAddOnScore scoreFile = QgAddOnScore();
    scoreFile.addScore(score);
    _score = score;
  }

  public void setState(QgAddOnResultState state)
  {
    _state = state;
  }

  public string stateToString()
  {
    switch (_state)
    {
      case QgAddOnResultState::success:
        return "success";

      case QgAddOnResultState::warning:
        return "warning";

      case QgAddOnResultState::error:
        return "error";

      case QgAddOnResultState::failed:
      {
        ///@todo 05.06.2018 lschopp: remove the option QgAddOnResultState::failed
        DebugFTN("QgAddOnResult", __FUNCTION__, "obsolete option");
        return "error";
      }
    }

    DebugFTN("QgAddOnResult", __FUNCTION__, "internall error, this state does not exists", _state);
    return "";
  }

  public int publish()
  {
    if (!_resDir.exists())
      _resDir.create();

    if (!_hasErr && calculate())
    {
      DebugFTN("QgAddOnResult", __FUNCTION__, "calculate does not work");
      return -3;
    }

    file f = fopen(_resDir.getDirPath() + "_state", "wb+");

    if (ferror(f))
    {
      return -2;
    }

    fputs(stateToString(), f);
    fclose(f);

    f = fopen(_resDir.getDirPath() + "_data", "wb+");

    string json;

    if (mappingHasKey(_data, "qgSummary"))
      json = jsonEncode(_data["qgSummary"]);
    else
      json = jsonEncode(_data);

//     strreplace(json, "&", "");
    fputs(json, f);
    fclose(f);

    // due to the current state, the score and exitCode are updated
    int exitCode;

    switch (_state)
    {
      case QgAddOnResultState::success:
        addScore(2);
        exitCode = 0;
        break;

      case QgAddOnResultState::warning:
        addScore(1);
        exitCode = 0;
        break;

      case QgAddOnResultState::failed:
        addScore(-1);
        exitCode = 0;
        break;

      case QgAddOnResultState::error:
        addScore(-1);
        exitCode = 0;
        break;

      default:
        DebugFTN("QgAddOnResult", __FUNCTION__, "internall error, this state does not exists", _state);
        addScore(0);
        exitCode = 0;
    }

    if (mappingHasKey(_data, "qgTestVersionResults"))
    {
      string path = _resDir.getDirPath() + "QgTestVersion/";

      mkdir(path);

      int testId = Qg::idToNum();

      if (testId <= 0)
      {
        DebugFTN("QgAddOnResult", __FUNCTION__, "could not calculate test id", Qg::getId(),  Qg::getAllIds());
        return -4;
      }

      f = fopen(path + "_Id", "wb+");
      fputs((string)testId, f);
      fclose(f);

      f = fopen(path + "_Results", "wb+");
      json = jsonEncode(_data["qgTestVersionResults"]);

//       strreplace(json, "&", "");

      fputs(json, f);
      fclose(f);

      f = fopen(path + "_Score", "wb+");
      fputs((string)_score, f);
      fclose(f);
    }



    return exitCode;
  }

  public string getResultDirPath()
  {
    return _resDir.getDirPath();
  }

  QgAddOnResultsDir _resDir = QgAddOnResultsDir();
  protected mapping _data;
  QgAddOnResultState _state;
  bool _hasErr;
  int _score;
};
