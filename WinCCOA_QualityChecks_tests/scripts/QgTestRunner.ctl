//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2022 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//

#uses "classes/Math/Math"
#uses "CtrlXml"
#uses "classes/FileSys/Dir"
#uses "classes/QualityGates/QgTest"
#uses "classes/QualityGates/AddOn/Output/QgAddOnResult"
#uses "classes/QualityGates/QgApp"



class QgTestSummary
{
  public int fromQuickResult()
  {
    string json;

    fileToString(PROJ_PATH + "quickResult.json", json);
    mapping map = jsonDecode(json);

    if( mappinglen(map) == 0 )
      return -1;

    if( mappingHasKey(map, "Aborted") )
      _aborted = map[ "Aborted" ];
    if( mappingHasKey(map, "Failed") )
      _failed = map[ "Failed" ];
    if( mappingHasKey(map, "Passed") )
      _passed = map[ "Passed" ];
    if( mappingHasKey(map, "KnownBugs") )
      _knownBugs = map[ "KnownBugs" ];

    return 0;
  }

  public mapping toMap()
  {
    return makeMapping("Aborted", _aborted,
                       "Failed", _failed,
                       "Passed", _passed,
                       "KnownBugs", _knownBugs);
  }

  public int getAll()
  {
    return _aborted + _failed + _passed + _knownBugs;
  }
  
  public int getAborted()
  {
    return _aborted;
  }

  public int getFailed()
  {
    return _failed;
  }

  public int getPassed()
  {
    return _passed;
  }

  public int getKnownBugs()
  {
    return _knownBugs;
  }

  public QgTestSummary opPlus(const QgTestSummary &summary)
  {
    _aborted += summary.getAborted();
    _failed += summary.getFailed();
    _passed += summary.getPassed();
    _knownBugs += summary.getKnownBugs();

    return this;
  }

  int _aborted, _failed, _passed, _knownBugs;
  
  public int exitCode;
};

class QgLibTest
{
  public QgLibTest(const string path = "")
  {
    _path = path;
  }

  public int test()
  {
    string path = makeUnixPath(_path);
    string relPath = substr(path, strlen(_baseDir) );

    strreplace(relPath, ".ctc", ".ctl"); // encrypted lib has decrypted test
    
    string testPath = getPath(SCRIPTS_REL_PATH, "tests/libs/" + relPath);

    if( testPath == "" )
    {
      DebugTN(__FUNCTION__, "For this test script does not exist unit-test", relPath);
      return 1;
    }

    string cmd;
    if( _WIN32 )
    {
      cmd = getPath(BIN_REL_PATH, getComponentName(CTRL_COMPONENT) + ".exe");
    }
    else if( _UNIX )
    {
      cmd = getPath(BIN_REL_PATH, getComponentName(CTRL_COMPONENT) );
    }
    else
    {
      DebugTN(__FUNCTION__, "this platform is not implemented");
      return -1;
    }

    if( cmd == "" )
    {
      DebugTN(__FUNCTION__, "missing ctrl manager");
      return -2;
    }

    Dir ctrlCovDir = Dir(dirName(getCtrlCovReportPath() ) );
    if( !ctrlCovDir.exists() )
      ctrlCovDir.mk();

    cmd += " " + makeUnixPath("tests/libs/" + relPath) + " -proj " + PROJ + " -n -log +stderr -dumpCoverageOnExit -coveragereportfile " + getCtrlCovReportPath(FALSE);

    fclose(fopen(PROJ_PATH + "quickResult.json", "wb+") );
    string stdOut, stdErr;
    _sum.exitCode = system(cmd, stdOut, stdErr);
    if( _sum.exitCode )
    {
      DebugTN(__FUNCTION__, "command exited with rc = " + _sum.exitCode, cmd, stdOut, stdErr);
      return -3;
    }
    
    _sum.fromQuickResult();
    
    remove(PROJ_PATH + "quickResult.json");
    return 0;
  }

  public QgTestSummary getSum()
  {
    return _sum;
  }

  public string getName()
  {
    return baseName(_path);
  }

  public mapping toMap()
  {
    mapping map;
    map = _sum.toMap();
    return map;
  }

  string getCtrlCovReportPath(const bool fullPath = TRUE)
  {
    string relPath = substr(_path, strlen(_baseDir) ) + ".xml";

    if( !fullPath )
      return relPath;

    return makeNativePath(PROJ_PATH + LOG_REL_PATH + relPath);
  }


  string _path;
  QgTestSummary _sum;

  string _baseDir = PROJ_PATH + LIBS_REL_PATH;
};

class QqTestDir
{
  public QqTestDir(const string path = "")
  {
    _path = path;
  }

  public int test()
  {
    _path = makeUnixPath(_path);
    if( _path == "" || !isdir(_path) )
    {
      DebugTN(__FUNCTION__, "Directory is does not exist", _path);
      return 0;
    }

    dyn_string libs = getFileNames(_path, "*");

    for( int i = 1; i <= dynlen(libs); i++ )
    {
      QgLibTest lib = QgLibTest(_path + libs[i]);
      int rc = lib.test();
      if ( rc == 0 )
        DebugN(__FUNCTION__, "Summary", lib.toMap(), _path + libs[i]);

      QgTestSummary sum = lib.getSum();
            
      if ( sum.exitCode )
      {
        countOfCrashes++;
      } 
      if ( sum.getAll() <= 0 )
      {
        countOfNotTestedObjects++;
      }
      else
      {
        _sum.opPlus(lib.getSum() );
        dynAppend(_libs, lib);
      }
    }

    dyn_string dirs = getFileNames(_path, "*", FILTER_DIRS);

    for( int i = 1; i <= dynlen(dirs); i++ )
    {
      if( dirs[ i ] == "" || dirs[ i ] == "." || dirs[ i ] == ".." )
        continue;

      QqTestDir dir = QqTestDir(_path + dirs[ i ] + "/");
      dir.test();
      _sum.opPlus(dir.getSum());
      countOfNotTestedObjects += dir.countOfNotTestedObjects;
      countOfCrashes += dir.countOfCrashes;
      dynAppend(_childs, dir);
    }

    return 0;
  }

  public mapping toMap()
  {
    mapping map;

    if( dynlen(_childs) > 0 )
    {
      map[ "@dirs" ] = makeMapping();

      for( int i = 1; i <= dynlen(_childs); i++ )
        map[ "@dirs" ][ _childs[ i ].getName() ] = _childs[ i ].toMap();
    }

    if( dynlen(_libs) > 0 )
    {
      map[ "@files" ] = makeMapping();

      for( int i = 1; i <= dynlen(_libs); i++ )
        map[ "@files" ][ _libs[ i ].getName() ] = _libs[ i ].toMap();
    }

    return map;
  }

  public string getName()
  {
    return baseName(_path);
  }

  public QgTestSummary getSum()
  {
    return _sum;
  }
  
  public int countOfNotTestedObjects;
  public int countOfCrashes;

  QgTestSummary _sum = QgTestSummary();
  dyn_anytype _libs;
  dyn_anytype _childs;

  string _path;
};

class QgTestRunner : QgTest
{
  public dyn_string _tcIds = makeDynString("UnitTests");

  QqTestDir _libDir = QqTestDir(PROJ_PATH + LIBS_REL_PATH);
  public int _startSingle()
  {
    switch( _testCaseId )
    {
      case "UnitTests":
      {
        return _libDir.test();
      }
    }
    return -1;
  }


  public int setUp()
  {
    if( QgTest::setUp() )
      return -1;

    _data["qgTestVersionResults"] = makeMapping();
    return 0;
  }
  
  public float getAllCount()
  {
    QgTestSummary sum = _libDir.getSum();
    return (float)sum.getAll();
  }
  
  public float getErrorCount()
  {
    QgTestSummary sum = _libDir.getSum();
    
    return (float)sum.getFailed() + (float)sum.getAborted() + _libDir.countOfNotTestedObjects + _libDir.countOfCrashes; 
  }
  

  public int tearDown()
  {
    _data["qgTestVersionResults"] = _libDir.toMap();

    QgTestSummary sum = _libDir.getSum();
    _data["qgSummary"] = sum.toMap();

    _data["score"] = calculateScore();
    return QgTest::tearDown();
  }
  
  public float calculateScore()
  {
    float perc = QgTest::calculateScore();
    QgTestSummary sum = _libDir.getSum();
    
    mapping map = _data["qgTestVersionResults"]["Score"];
    
    if ( _libDir.countOfCrashes > 0 )
      map[QgAddOnResult::KEY_SCORE_REASON] =  myQgMsgCat.getText("reasonCrashFound", makeMapping("count", _libDir.countOfCrashes));
    else if ( _libDir.countOfNotTestedObjects )
      map[QgAddOnResult::KEY_SCORE_REASON] =  myQgMsgCat.getText("reasonNotTestedObject", makeMapping("count", _libDir.countOfNotTestedObjects));
    else
      return perc;
    
    addScoreDetail(map);
    return perc;
  }

  string _baseDir = PROJ_PATH + LIBS_REL_PATH;


  public mapping _data = makeMapping();
};


class QgAddOnResultUnitTestRunner:
QgAddOnResult
{
  public int calculate()
  {
    setState(QgAddOnResultState::success);
    return 0;
  }
};

main()
{
  Qg::setId("UnitTests");
  QgAddOnResultUnitTestRunner result = QgAddOnResultUnitTestRunner();

  QgTestRunner test = QgTestRunner();
  if( test.start() )
    result.setErr(test.getErrPrio(), test.getErrCode(), test.getErrNote() );
  else
    result.setData(test._data);

  exit(result.publish() );
}
