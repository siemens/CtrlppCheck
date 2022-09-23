//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2022 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//

//--------------------------------------------------------------------------------
// used libraries (#uses)
#uses "classes/QualityGates/Tools/CppCheck/CppCheckError"
#uses "fileSys"
#uses "CtrlOaUnit"
#uses "classes/QualityGates/Tools/CppCheck/CppCheck"
#uses "classes/QualityGates/Qg" /*!< tested object */
#uses "classes/WinCCOA/StTest" /*!< oaTest basic class */

//--------------------------------------------------------------------------------
// declare variables and constans

//--------------------------------------------------------------------------------

class TstCppCheck : CppCheck
{
  public checkFile(const string &testFile)
  {
    string s;
    fileToString(testFile, s);
    s = substr(s, 0, strpos(s, "\n")); // get fist lines
    const string key = "// start options: ";
    int idx = strpos(s, key);
    if ( idx >= 0 )
      s = " " + substr(s, idx + strlen(key));
    else
      s = "";
    
    DebugN(__FUNCTION__, testFile, s);
    start(testFile + s);
    stdErrToErrList();
  }  
  
  public compare(const string &refFile)
  {
    const string tcId = "Ctrlppcheck." +  baseName(refFile);
    string str;
    fileToString(refFile, str);
    TstCppCheck reference;
    
    if ( str == "" )
    {
      oaUnitAbort(tcId, "reference file is empty");
      return;
    }
    reference.strToErrList(str);
    
//     if ( dynlen(errList) != dynlen(reference.errList) )
//     {
//       oaUnitFail(tcId, "Count of error does not match with reference file:\n" + refFile);
//       return;
//     } 
    
    dyn_string simpleErrStrings;
    for(int i = 1; i <= dynlen(reference.errList); i++)
    {
      reference.errList[i].path = "";
    }
    for(int i = 1; i <= dynlen(errList); i++)
    {
      errList[i].path = "";
      dynAppend(simpleErrStrings, errList[i].toStdErrString());
    }
    
    
    for(int i = 1; i <= dynlen(reference.errList); i++)
    {
      CppCheckError expErr = reference.errList[i];
      string expErrorStr = expErr.toStdErrString();
      mapping map = makeMapping("ErrMsg", "Ctrlppcheck can not found this error:" + 
                                          "\n  File:\n" + refFile +
                                          "\n  ExpectedValue:\n" + expErrorStr);
      if ( expErr.knownBug != "" )
        map["KnownBug"] = expErr.knownBug;
      // DebugN(__FUNCTION__, map, expErr);
      oaUnitAssertGreater(tcId, dynContains(simpleErrStrings, expErrorStr), 0, map);
    }
    
    
  }
};

class TstCtrlppcheck : StTest
{
  public dyn_string _getAllTcIds()
  {
    // list with our testcases
    return makeDynString("Ctrlppcheck");
  }

  public int _startTest()
  {
    const string tcId = _getTcId();

    switch( tcId )
    {
      case "Ctrlppcheck":
      {
        string path = getPath(SCRIPTS_REL_PATH, "tests/ctrlPpCheck/testscripts");
        dyn_string files = getFileNamesRecursive(path);
        
        for(int i = 1; i <= dynlen(files); i++)
        {
          string testFile = makeUnixPath(files[i]);
          TstCppCheck check;
          check.settings.enableXmlFormat(TRUE);
          
          check.settings.enableLibCheck = FALSE;
          check.settings.enableHeadersCheck = TRUE;
          check.settings.includeSubProjects = TRUE;
          check.settings.inconclusive = FALSE;
          check.settings.verbose = FALSE;
          check.settings.inlineSuppressions = FALSE;
          
           // load configs
          check.settings.addLibraryFile(getPath(DATA_REL_PATH, "ctrlPpCheck/cfg/ctrl.xml")); // general
          check.settings.addLibraryFile(getPath(DATA_REL_PATH, "ctrlPpCheck/cfg/ctrl_" + VERSION + ".xml")); // version specific
          check.settings.addLibraryFile(getPath(DATA_REL_PATH, "ctrlPpCheck/cfg/__proj__.xml")); // proj specific
    
          // load rules
          check.settings.addRuleFile(getPath(DATA_REL_PATH, "ctrlPpCheck/rule/ctrl.xml")); // general
          check.settings.addRuleFile(getPath(DATA_REL_PATH, "ctrlPpCheck/rule/ctrl_" + VERSION + ".xml")); // version specific
          check.settings.addRuleFile(getPath(DATA_REL_PATH, "ctrlPpCheck/rule/__proj__.xml")); // proj specific
    
          check.settings.addEnabled("all");
          check.settings.enableXmlFormat(TRUE);
    
          check.checkFile(testFile);
          
          string refFile = testFile;
          strreplace(refFile, SCRIPTS_REL_PATH + "tests/", DATA_REL_PATH);
          strreplace(refFile, ".ctl", ".xml");
                    
          check.compare(refFile);
          
          // make a copy for futher analysis
          string resDir = PROJ_PATH + LOG_REL_PATH + "ctrlPpCheck/testscripts/";
          if ( !isdir(resDir) )
            mkdir(resDir);
          
          moveFile(PROJ_PATH + LOG_REL_PATH + "cppcheck-result.xml",  resDir + baseName(refFile));

        }
        return 0;
      }
    }

    return -1;
  }
};

//--------------------------------------------------------------------------------
main()
{
  TstCtrlppcheck test = TstCtrlppcheck();

  test.startAll();

  exit(0);
}
