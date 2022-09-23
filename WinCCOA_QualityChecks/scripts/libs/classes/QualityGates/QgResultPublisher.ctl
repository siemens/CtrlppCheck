//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2022 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//

#uses "classes/QualityGates/Qg"
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
  public static string stateToString(const QgResultState &state)
  {
    switch(state)
    {
      case QgResultState::success:
        return "success";
      case QgResultState::warning:
        return "warning";
      case QgResultState::error:
        return "error";
    }

    DebugFTN("QgBase", __FUNCTION__, "internall error, this state does not exists", state);
    return "";
  }

  //------------------------------------------------------------------------------
  public int publish()
  {
    Float f = Float(result.calculateScore());
    result.score = f.round(2);
    QgAddOnResultsDir resDir = QgAddOnResultsDir();

    if ( !resDir.exists() )
      resDir.create();

    if ( _publishState(resDir) )
      return sendNotification(-1);
    
    if ( _publishSummary(resDir) )
      return sendNotification(-2);
    
    if ( _publishFull(resDir) )
      return sendNotification(-3);
    
    return sendNotification(0);
  }
  
//--------------------------------------------------------------------------------
//@protected members
//--------------------------------------------------------------------------------
  
  //------------------------------------------------------------------------------
  protected int sendNotification(const int errCode)
  {
    if (isEvConnOpen() && (  Qg::getId() != "" ) )
      dpSet("_WinCCOA_qgCmd.Command", Qg::getId() + ":DONE:" + errCode);
    
    return errCode;
  }

  //------------------------------------------------------------------------------
  protected int _publishState(const QgAddOnResultsDir &resDir)
  {
    string resPath;
    if ( Qg::isRunningOnJenkins() )
      resPath = resDir.getDirPath() + "_state";
    else
      resPath = resDir.getDirPath() + "State";
          
    file f = fopen(resPath, "wb+");  
    if ( ferror(f) )
      return -1;

    fputs(stateToString(state), f);
    fclose(f);
    return 0;
  }

  //------------------------------------------------------------------------------
  protected int _publishSummary(const QgAddOnResultsDir &resDir)
  {
    string resPath;
    if ( Qg::isRunningOnJenkins() )
      resPath = resDir.getDirPath() + "_data";
    else
      resPath = resDir.getDirPath() + "sum.json";

    file f = fopen(resPath, "wb+");
    if ( ferror(f) )
      return -1;

    fputs(jsonEncode(result.sumToMap(), jsonFormat == QgVersionResultJsonFormat::Compact), f);
    fclose(f);
    return 0;
  }

  //------------------------------------------------------------------------------
  protected int _publishFull(const QgAddOnResultsDir &resDir)
  {
    if ( Qg::isRunningOnJenkins() )
      return _publishFullOnJenkins(resDir);
    else
      return _publishFullLocale(resDir);
  }

  
//--------------------------------------------------------------------------------
//@private members
//--------------------------------------------------------------------------------
  
  //------------------------------------------------------------------------------
  private int _publishFullLocale(const QgAddOnResultsDir &resDir)
  {
    string path = resDir.getDirPath();

    file f = fopen(path + "Result", "wb+");
    if ( ferror(f) )
    {
      DebugFTN("QgBase", __FUNCTION__, "could not create file", path + "Result");
      return -3;
    }

    mapping map;
    map["fields"] = fields;
    map["root"]= makeMapping();
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

  //------------------------------------------------------------------------------
  private int _publishFullOnJenkins(const QgAddOnResultsDir &resDir)
  {
    string path = resDir.getDirPath() + "QgTestVersion/";

    mkdir(path);

    int testId = Qg::idToNum();

    if ( testId <= 0 )
    {
      DebugFTN("QgBase", __FUNCTION__, "could not calculate test id", Qg::getId(),  Qg::getAllIds());
      return -1;
    }

    file f = fopen(path + "_Id", "wb+");
    if ( ferror(f) )
    {
      DebugFTN("QgBase", __FUNCTION__, "could not create file", path + "_Id");
      return -2;
    }
    fputs((string)testId, f);
    fclose(f);

    DebugFN("QgBase", __FUNCTION__, path);
    f = fopen(path + "_Results", "wb+");
    if ( ferror(f) )
    {
      DebugFTN("QgBase", __FUNCTION__, "could not create file", path + "_Results");
      return -3;
    }

    mapping map;
    switch(qgVersionResultType)
    {
      case QgVersionResultType::TableView:
      {
        map["fields"] = fields;
        map["root"]= makeMapping();
        map["root"]["children"] = makeDynMapping();
        map["root"]["children"][1] = result.scoreToMap();
        map["root"]["children"][2] = result.toMap();
//         result.clear();
        break;
      }

      case QgVersionResultType::SimpleTreeView:
      case QgVersionResultType::TreeView:
      {
        map["score"] = result.scoreToMap();
        map["details"] = result.toMap();
//         result.clear();
        break;
      }
    }
//     [result.text] = result.toMap();
    DebugFTN("QgBase", __FUNCTION__, "write QgVersionResultJsonFormat to json file");
    fputs(jsonEncode(map, jsonFormat == QgVersionResultJsonFormat::Compact), f);
    fclose(f);

    DebugFTN("QgBase", __FUNCTION__, "write " + path + "_Score file" );
    f = fopen(path + "_Score", "wb+");
    fputs((string)result.score, f);
    fclose(f);
    return 0;
  }

};
