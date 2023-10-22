//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2023 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//

#uses "classes/json/JsonFile"
#uses "classes/QualityGates/Qg"
#uses "classes/QualityGates/QgTest"
#uses "classes/QualityGates/AddOn/FileSys/QgAddOnResultsDir"
#uses "classes/QualityGates/QgResult"

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
  public shared_ptr <QgResult> result;
  public dyn_string fields;
  public static QgResultJsonFormat jsonFormat = QgResultJsonFormat::Compact;

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

//--------------------------------------------------------------------------------
//@private members
//--------------------------------------------------------------------------------

  //------------------------------------------------------------------------------
  private int sendNotification(const int errCode)
  {
    if (isEvConnOpen() && (Qg::getId() != ""))
      dpSet("_WinCCOA_qgCmd.Command", Qg::getId() + ":DONE:" + errCode);

    return errCode;
  }

  //------------------------------------------------------------------------------
  private int _publishSummary(const QgAddOnResultsDir &resDir)
  {
    JsonFile jsonFile = JsonFile(resDir.getDirPath() + "sum.json");
    jsonFile.create();
    return jsonFile.write(result.sumToMap(), jsonFormat == QgResultJsonFormat::Compact);
  }

  //------------------------------------------------------------------------------
  private int _publishFull(const QgAddOnResultsDir &resDir)
  {
    if (!QgTest::isStartedByTestFramework())
      return _publishFullLocale(resDir);

    return 0;
  }

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

    fputs(jsonEncode(map, jsonFormat == QgResultJsonFormat::Compact), f);
    fclose(f);

    f = fopen(path + "Score", "wb+");
    fputs((string)result.score, f);
    fclose(f);
    return 0;
  }
};
