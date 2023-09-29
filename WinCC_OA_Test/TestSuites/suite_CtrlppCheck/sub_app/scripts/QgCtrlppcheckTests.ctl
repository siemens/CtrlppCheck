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
#uses "classes/QualityGates/Tools/CppCheck/CppCheck"
#uses "classes/QualityGates/Qg" /*!< tested object */
#uses "classes/oaTest/OaTest"

//--------------------------------------------------------------------------------

class MockCppCheck : CppCheck
{
  public void checkFile(const string &testFile)
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

  public void compare(const string &refFile)
  {
    DebugTN(__FUNCTION__, refFile);
    const string tcId = "Ctrlppcheck." +  baseName(refFile);
    string str;
    bool hasFailedRead = fileToString(refFile, str, "UTF8");
    str = str.trim();
    oaUnitAssertTrue(tcId, hasFailedRead, "Read reference file: " + refFile);
    MockCppCheck reference;

    if (str.isEmpty())
    {
      oaUnitAbort(tcId, "Reference file " + refFile + " is empty");
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

class TstCtrlppcheck : OaTest
{
  public string testScriptPath;

  public dyn_string getAllTestCaseIds()
  {
    // list with our testcases
    return makeDynString("Ctrlppcheck");
  }

  protected int startTestCase(const string tcId)
  {
    switch( tcId )
    {
      case "Ctrlppcheck":
      {
        const string path = getPath(SCRIPTS_REL_PATH, "testScripts/" + testScriptPath);
        if (path.isEmpty())
        {
          return abort("Test script does not exists: " + SCRIPTS_REL_PATH + "testScripts/" + testScriptPath);
        }
        this.info("check file: " + path);

        string refFile = testScriptPath;
        strreplace(refFile, ".ctl", ".xml");
        refFile = getPath(DATA_REL_PATH, "references/" + refFile);

        if (refFile.isEmpty() || !isfile(refFile))
        {
          return abort("Reference file does not exists.");
        }

        const string testFile = makeUnixPath(path);
        MockCppCheck check;
        check.settings.enableXmlFormat(TRUE);

        check.settings.enableLibCheck = FALSE;
        // check.settings.enableHeadersCheck = TRUE;
        check.settings.includeSubProjects = TRUE; 	
        check.settings.inconclusive = FALSE;
        check.settings.verbose = FALSE;
        check.settings.inlineSuppressions = FALSE;

        // load configs
        check.settings.addLibraryFile(getPath(DATA_REL_PATH, "DevTools/Base/ctrl.xml")); // general
        check.settings.addLibraryFile(getPath(DATA_REL_PATH, "DevTools/Base/ctrl_" + VERSION + ".xml")); // version specific
        check.settings.addLibraryFile(getPath(DATA_REL_PATH, "ctrlPpCheck/cfg/__proj__.xml")); // proj specific

        // load rules
        check.settings.addRuleFile(getPath(DATA_REL_PATH, "ctrlPpCheck/rule/ctrl.xml")); // general
        check.settings.addRuleFile(getPath(DATA_REL_PATH, "ctrlPpCheck/rule/ctrl_" + VERSION + ".xml")); // version specific
        check.settings.addRuleFile(getPath(DATA_REL_PATH, "ctrlPpCheck/rule/__proj__.xml")); // proj specific

        check.settings.addEnabled("all");
        check.settings.enableXmlFormat(TRUE);

        check.checkFile(testFile);

        check.compare(refFile);

        // make a copy for futher analysis
        string resDir = PROJ_PATH + LOG_REL_PATH + "ctrlPpCheck/";
        if ( !isdir(resDir) )
          mkdir(resDir);

        moveFile(PROJ_PATH + LOG_REL_PATH + "cppcheck-result.xml",  resDir + baseName(refFile));
        return 0;
      }
    }

    return -1;
  }
};

//--------------------------------------------------------------------------------
main(const string testScriptPath)
{
  TstCtrlppcheck test;
  test.testScriptPath = testScriptPath;
  test.startAll();
}
