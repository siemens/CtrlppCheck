//--------------------------------------------------------------------------------
/**
  @file $relPath
  @copyright Copyright 2026 SIEMENS AG
             SPDX-License-Identifier: GPL-3.0-only
*/

/*!
 * @brief Tests for struct: CppCheckError
 */

//--------------------------------------------------------------------------------
// used libraries (#uses)
#uses "classes/QualityGates/Tools/CppCheck/CppCheckError" /*!< tested object */
#uses "classes/oaTest/OaTest"

//--------------------------------------------------------------------------------
class TstCppCheckError : OaTest
{
  public dyn_string getAllTestCaseIds()
  {
    return makeDynString(
      "CppCheckError_default_values",
      "CppCheckError_set_values",
      "CppCheckError_toStdErrString_minimal",
      "CppCheckError_toStdErrString_full"
    );
  }

  protected int startTestCase(const string tcId)
  {
    switch (tcId)
    {
      case "CppCheckError_default_values":
      {
        CppCheckError err;
        assertEqual(err.id, "");
        assertEqual(err.severity, "");
        assertEqual(err.msg, "");
        assertEqual(err.verbose, "");
        assertEqual(err.path, "");
        assertEqual(err.path0, "");
        assertEqual(err.line, 0);
        assertEqual(err.cwe, 0);
        assertEqual(err.knownBug, "");
        return 0;
      }

      case "CppCheckError_set_values":
      {
        CppCheckError err;
        err.id = "uninitvar";
        err.severity = "error";
        err.msg = "Uninitialized variable: x";
        err.verbose = "Uninitialized variable: x is used without being initialized";
        err.path = "/path/to/file.ctl";
        err.path0 = "/path/to/file.ctl";
        err.line = 42;
        err.cwe = 457;
        err.knownBug = "BUG-123";

        assertEqual(err.id, "uninitvar");
        assertEqual(err.severity, "error");
        assertEqual(err.msg, "Uninitialized variable: x");
        assertEqual(err.line, 42);
        assertEqual(err.cwe, 457);
        assertEqual(err.knownBug, "BUG-123");
        return 0;
      }

      case "CppCheckError_toStdErrString_minimal":
      {
        CppCheckError err;
        err.id = "testId";
        err.severity = "warning";
        err.line = 10;
        err.cwe = 100;

        string result = err.toStdErrString();

        assertTrue(strpos(result, "ID: testId") >= 0);
        assertTrue(strpos(result, "Severity: warning") >= 0);
        assertTrue(strpos(result, "Line: 10") >= 0);
        assertTrue(strpos(result, "CWE: 100") >= 0);
        return 0;
      }

      case "CppCheckError_toStdErrString_full":
      {
        CppCheckError err;
        err.id = "nullPointer";
        err.severity = "error";
        err.msg = "Null pointer dereference";
        err.verbose = "Dereferencing a null pointer leads to undefined behavior";
        err.path = "/scripts/libs/test.ctl";
        err.line = 25;
        err.cwe = 476;

        string result = err.toStdErrString();

        assertTrue(strpos(result, "ID: nullPointer") >= 0);
        assertTrue(strpos(result, "Severity: error") >= 0);
        assertTrue(strpos(result, "Msg: Null pointer dereference") >= 0);
        assertTrue(strpos(result, "Verbose: Dereferencing") >= 0);
        assertTrue(strpos(result, "Path: /scripts/libs/test.ctl") >= 0);
        assertTrue(strpos(result, "Line: 25") >= 0);
        assertTrue(strpos(result, "CWE: 476") >= 0);
        return 0;
      }
    }

    return -1;
  }
};

//--------------------------------------------------------------------------------
main()
{
  TstCppCheckError test;
  test.startAll();
  exit(0);
}
