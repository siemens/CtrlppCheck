//--------------------------------------------------------------------------------
/**
  @file $relPath
  @copyright Copyright 2026 SIEMENS AG
             SPDX-License-Identifier: GPL-3.0-only
*/

/*!
 * @brief Tests for class: QgTest
 */

//--------------------------------------------------------------------------------
// used libraries (#uses)
#uses "classes/QualityGates/QgTest" /*!< tested object */
#uses "classes/oaTest/OaTest"

//--------------------------------------------------------------------------------
class TstQgTest : OaTest
{
  public dyn_string getAllTestCaseIds()
  {
    return makeDynString(
      "QgTest_ctor",
      "QgTest_calculateScore_no_errors",
      "QgTest_calculateScore_with_errors",
      "QgTest_getErrorCount",
      "QgTest_getAllCount",
      "QgTest_getErrPrio_default",
      "QgTest_getErrCode_default",
      "QgTest_getErrNote_default"
    );
  }

  protected int startTestCase(const string tcId)
  {
    switch (tcId)
    {
      case "QgTest_ctor":
      {
        QgTest qgTest;

        // Default values
        assertEqual(qgTest._errCount, 0);
        assertEqual(qgTest._all, 0);
        return 0;
      }

      case "QgTest_calculateScore_no_errors":
      {
        QgTest qgTest;

        // Simulate 10 tests, no errors
        qgTest._all = 10;
        qgTest._errCount = 0;

        float score = qgTest.calculateScore();
        assertEqual(score, 100.0);
        return 0;
      }

      case "QgTest_calculateScore_with_errors":
      {
        QgTest qgTest;

        // Simulate 10 tests, 2 errors -> 80%
        qgTest._all = 10;
        qgTest._errCount = 2;

        float score = qgTest.calculateScore();
        assertEqual(score, 80.0);

        // Simulate 10 tests, 5 errors -> 50%
        qgTest._errCount = 5;
        score = qgTest.calculateScore();
        assertEqual(score, 50.0);

        // Simulate all errors -> 0%
        qgTest._errCount = 10;
        score = qgTest.calculateScore();
        assertEqual(score, 0.0);
        return 0;
      }

      case "QgTest_getErrorCount":
      {
        QgTest qgTest;

        assertEqual(qgTest.getErrorCount(), 0);

        qgTest._errCount = 5;
        assertEqual(qgTest.getErrorCount(), 5);
        return 0;
      }

      case "QgTest_getAllCount":
      {
        QgTest qgTest;

        assertEqual(qgTest.getAllCount(), 0);

        qgTest._all = 15;
        assertEqual(qgTest.getAllCount(), 15);
        return 0;
      }

      case "QgTest_getErrPrio_default":
      {
        QgTest qgTest;

        // Default error priority should be PRIO_SEVERE
        assertEqual(qgTest.getErrPrio(), PRIO_SEVERE);
        return 0;
      }

      case "QgTest_getErrCode_default":
      {
        QgTest qgTest;

        // Default error code is 1
        assertEqual(qgTest.getErrCode(), 1);
        return 0;
      }

      case "QgTest_getErrNote_default":
      {
        QgTest qgTest;

        // Default error note is empty
        assertEqual(qgTest.getErrNote(), "");
        return 0;
      }
    }

    return -1;
  }
};

//--------------------------------------------------------------------------------
main()
{
  TstQgTest test;
  test.startAll();
  exit(0);
}
