//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2022 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//

#uses "classes/QualityGates/Qg"
#uses "classes/QualityGates/QgTest"
#uses "classes/QualityGates/AddOn/FileSys/QgAddOnResultsDir"
#uses "classes/QualityGates/QgVersionResult"

//--------------------------------------------------------------------------------
enum QgResultState
{
  success,
  warning,
  error
};

class QgResultPublisher
{
//--------------------------------------------------------------------------------
//@public members
//--------------------------------------------------------------------------------

  //------------------------------------------------------------------------------
  public QgResultState state;
  public shared_ptr <QgVersionResult> result;
  public dyn_string fields;
  public static QgVersionResultJsonFormat jsonFormat = QgVersionResultJsonFormat::Compact;

  //------------------------------------------------------------------------------
  public QgResultPublisher()
  {
  }

  //------------------------------------------------------------------------------
  public int publish()
  {
    Float f = Float(result.calculateScore());
    result.score = f.round(2);
    QgAddOnResultsDir resDir = QgAddOnResultsDir();

    if (!resDir.exists())
      resDir.create();

    if (_publishSummary(resDir))
      return sendNotification(-2);

    if (_publishFull(resDir))
      return sendNotification(-3);

    return sendNotification(0);
  }

//--------------------------------------------------------------------------------
//@protected members
//--------------------------------------------------------------------------------

  //------------------------------------------------------------------------------
  protected int sendNotification(const int errCode)
  {
    if (isEvConnOpen() && (Qg::getId() != ""))
      dpSet("_WinCCOA_qgCmd.Command", Qg::getId() + ":DONE:" + errCode);

    return errCode;
  }

  //------------------------------------------------------------------------------
  protected int _publishSummary(const QgAddOnResultsDir &resDir)
  {
    const string resPath = resDir.getDirPath() + "sum.json";
    file f = fopen(resPath, "wb+");

    if (ferror(f))
      return -1;

    fputs(jsonEncode(result.sumToMap(), jsonFormat == QgVersionResultJsonFormat::Compact), f);
    fclose(f);
    return 0;
  }

  //------------------------------------------------------------------------------
  protected int _publishFull(const QgAddOnResultsDir &resDir)
  {
    if (!QgTest::isStartedByTestFramework())
      return _publishFullLocale(resDir);

    return 0;
  }

//--------------------------------------------------------------------------------
//@private members
//--------------------------------------------------------------------------------

  //------------------------------------------------------------------------------
  private int _publishFullLocale(const QgAddOnResultsDir &resDir)
  {
    string path = resDir.getDirPath();

    file f = fopen(path + "Result", "wb+");

    if (ferror(f))
    {
      DebugFTN("QgBase", __FUNCTION__, "could not create file", path + "Result");
      return -3;
    }

    mapping map;
    map["fields"] = fields;
    map["root"] = makeMapping();
    map["root"]["children"] = makeDynMapping();
    map["root"]["children"][1] = result.scoreToMap();
    map["root"]["children"][2] = result.toMap();
//     result.clear();

    fputs(jsonEncode(map, jsonFormat == QgVersionResultJsonFormat::Compact), f);
    fclose(f);

    f = fopen(path + "Score", "wb+");
    fputs((string)result.score, f);
    fclose(f);
    return 0;
  }
};
