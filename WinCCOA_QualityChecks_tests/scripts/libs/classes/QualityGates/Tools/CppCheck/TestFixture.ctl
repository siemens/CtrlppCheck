//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2022 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//

#uses "CtrlOaUnit"
#uses "classes/QualityGates/Tools/CppCheck/CppCheck"

struct Errout
{
  string str() { return err; }
  string err;
  int rc;
};

class TestFixture
{
  public bool assertMatch = 0;
  public bool dbgPass = FALSE;
  public const int InternalError = -1;
  public Errout errout;
  protected CppCheckSettings settings;
  protected string script;

  protected void check(string code)
  {
    if ( getPath(SCRIPTS_REL_PATH, "tests/" + code) != "" )
    {
      fileToString(getPath(SCRIPTS_REL_PATH, "tests/" + code), code);
    }
    script = code;

    int errPos;
    bool isOaOk = checkScript(script, errPos);

    if ( !isOaOk || ( errPos > 0) )
    {
      errout.rc = "35889";
      errout.err = "WinCC OA syntax error at pos: " + errPos;
      return;
    }
    // Clear the error buffer..
    errout.err = "";
    errout.rc = 0;

    string testScript = PROJ_PATH + SCRIPTS_REL_PATH + "test.ctl";
    file f = fopen(testScript, "wb+");
    fputs(code, f);
    fflush(f);
    fclose(f);

    CppCheck checker;
    checker.settings = settings;
    checker.checkFiles(makeDynString(testScript));

    errout.rc = checker.rc;
    errout.err = checker.stdErr;


    remove(testScript);
  }

//   protected ASSERT_THROW(int val, int expVal)
//   {
//      string tcData = "\n" + "Script:\n" + script;
//     oaUnitAssertEqual(testCaseId, val, expVal, tcData);
//   }

  /// @todo No idea why it is used in test. Just print the values here and make the implementation later
  protected TODO_ASSERT_EQUALS(const string &s1, const string &s2, const string &error)
  {
    DebugN(__FUNCTION__, s1, s2, error);
  }

  protected ASSERT_EQUALS(string expVal, string val)
  {
    string totalPath = makeNativePath(PROJ_PATH + SCRIPTS_REL_PATH + "test.ctl");
    string relativePath = SCRIPTS_REL_PATH + "test.ctl";
    strreplace(expVal, "test.cpp", makeNativePath("scripts/test.ctl"));
    strreplace(expVal, "\r", "");

    strreplace(val, "(error) There is no rule to execute. Tokenlist: raw\n",    "");
    strreplace(val, "(error) There is no rule to execute. Tokenlist: normal\n", "");
    strreplace(val, "(error) There is no rule to execute. Tokenlist: simple\n", "");

    strreplace(val, "\r", "");
    strreplace(relativePath, "\\", "/");
    strreplace(val, totalPath, relativePath);



    string tcData = "\n" + "Script:\n" + script +
                    "\n" + "ExpMessage:\n" + expVal +
                    "\n" + "ErrMessage:\n" + val;
    if ( assertMatch && (expVal != "" ) && (strpos(val, expVal) >= 0) )
      oaUnitPass(testCaseId, dbgPass ? tcData : "");
    else if ( val == expVal )
      oaUnitPass(testCaseId, dbgPass ? tcData : "");
    else
    {
      oaUnitFail(testCaseId, tcData);
    }
  }

  protected TEST_CASE(const function_ptr func)
  {
    try
    {
      testCaseId = (string)func;
      oaUnitInfo(testCaseId, "Start test case");
      callFunction(func);
    }
    catch
    {
      throwError(getLastException());
    }
  }

  protected string testCaseId;
//   protected string testCaseIdPrefix;
};
