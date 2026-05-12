//--------------------------------------------------------------------------------
/**
  @file $relPath
  @copyright Copyright 2026 SIEMENS AG
             SPDX-License-Identifier: GPL-3.0-only
*/

/*!
 * @brief Tests for struct: QgVersionResult
 */

//--------------------------------------------------------------------------------
// used libraries (#uses)
#uses "classes/QualityGates/QgVersionResult" /*!< tested object */
#uses "classes/oaTest/OaTest"

//--------------------------------------------------------------------------------
class TstQgVersionResult : OaTest
{
  public dyn_string getAllTestCaseIds()
  {
    return makeDynString(
      "QgVersionResult_ctor",
      "QgVersionResult_setLocation",
      "QgVersionResult_setAssertionText",
      "QgVersionResult_setReasonText",
      "QgVersionResult_assertGreatherEqual",
      "QgVersionResult_assertLessEqual",
      "QgVersionResult_assertEqual",
      "QgVersionResult_assertTrue",
      "QgVersionResult_assertFalse",
      "QgVersionResult_assertBetween",
      "QgVersionResult_assertDynContains",
      "QgVersionResult_calculateScore",
      "QgVersionResult_info"
    );
  }

  protected int startTestCase(const string tcId)
  {
    switch (tcId)
    {
      case "QgVersionResult_ctor":
      {
        QgVersionResult result;

        assertEqual(result.hasError, FALSE);
        assertEqual(result.totalPoints, 0);
        assertEqual(result.errorPoints, 0);
        assertEqual(result.text, "");
        assertEqual(result.location, "");
        return 0;
      }

      case "QgVersionResult_setLocation":
      {
        QgVersionResult result;

        result.setLocation("/path/to/file.ctl:123");
        assertEqual(result.getLocation(), "/path/to/file.ctl:123");
        return 0;
      }

      case "QgVersionResult_setAssertionText":
      {
        QgVersionResult result;

        result.setAssertionText("testKey");
        assertEqual(result.assertKey, "testKey");

        mapping dollars = makeMapping("var1", "value1");
        result.setAssertionText("testKey2", dollars);
        assertEqual(result.assertKey, "testKey2");
        assertTrue(mappingHasKey(result.assertDollars, "var1"));
        return 0;
      }

      case "QgVersionResult_setReasonText":
      {
        QgVersionResult result;

        result.setReasonText("reasonKey");
        assertEqual(result.reasonKey, "reasonKey");

        mapping dollars = makeMapping("info", "detail");
        result.setReasonText("reasonKey2", dollars);
        assertEqual(result.reasonKey, "reasonKey2");
        assertTrue(mappingHasKey(result.reasonDollars, "info"));
        return 0;
      }

      case "QgVersionResult_assertGreatherEqual":
      {
        QgVersionResult result;

        // 10 >= 5 should pass
        bool passed = result.assertGreatherEqual(10, 5);
        assertTrue(passed);
        assertFalse(result.hasError);
        assertEqual(result.totalPoints, 1);
        assertEqual(result.errorPoints, 0);

        // 3 >= 5 should fail
        QgVersionResult result2;
        passed = result2.assertGreatherEqual(3, 5);
        assertFalse(passed);
        assertTrue(result2.hasError);
        assertEqual(result2.totalPoints, 1);
        assertEqual(result2.errorPoints, 1);

        // 5 >= 5 should pass (equal)
        QgVersionResult result3;
        passed = result3.assertGreatherEqual(5, 5);
        assertTrue(passed);
        assertFalse(result3.hasError);
        return 0;
      }

      case "QgVersionResult_assertLessEqual":
      {
        QgVersionResult result;

        // 3 <= 5 should pass
        bool passed = result.assertLessEqual(3, 5);
        assertTrue(passed);
        assertFalse(result.hasError);

        // 10 <= 5 should fail
        QgVersionResult result2;
        passed = result2.assertLessEqual(10, 5);
        assertFalse(passed);
        assertTrue(result2.hasError);

        // 5 <= 5 should pass (equal)
        QgVersionResult result3;
        passed = result3.assertLessEqual(5, 5);
        assertTrue(passed);
        return 0;
      }

      case "QgVersionResult_assertEqual":
      {
        QgVersionResult result;

        // 5 == 5 should pass
        bool passed = result.assertEqual(5, 5);
        assertTrue(passed);
        assertFalse(result.hasError);

        // 5 == 10 should fail
        QgVersionResult result2;
        passed = result2.assertEqual(5, 10);
        assertFalse(passed);
        assertTrue(result2.hasError);

        // String equality
        QgVersionResult result3;
        passed = result3.assertEqual("test", "test");
        assertTrue(passed);
        return 0;
      }

      case "QgVersionResult_assertTrue":
      {
        QgVersionResult result;

        // TRUE should pass
        bool passed = result.assertTrue(TRUE);
        assertTrue(passed);
        assertFalse(result.hasError);

        // FALSE should fail
        QgVersionResult result2;
        passed = result2.assertTrue(FALSE);
        assertFalse(passed);
        assertTrue(result2.hasError);
        return 0;
      }

      case "QgVersionResult_assertFalse":
      {
        QgVersionResult result;

        // FALSE should pass
        bool passed = result.assertFalse(FALSE);
        assertTrue(passed);
        assertFalse(result.hasError);

        // TRUE should fail
        QgVersionResult result2;
        passed = result2.assertFalse(TRUE);
        assertFalse(passed);
        assertTrue(result2.hasError);
        return 0;
      }

      case "QgVersionResult_assertBetween":
      {
        QgVersionResult result;

        // 5 between 1 and 10 should pass
        bool passed = result.assertBetween(5, 1, 10);
        assertTrue(passed);
        assertFalse(result.hasError);

        // 0 between 1 and 10 should fail (below lower)
        QgVersionResult result2;
        passed = result2.assertBetween(0, 1, 10);
        assertFalse(passed);
        assertTrue(result2.hasError);

        // 15 between 1 and 10 should fail (above upper)
        QgVersionResult result3;
        passed = result3.assertBetween(15, 1, 10);
        assertFalse(passed);
        assertTrue(result3.hasError);

        // Edge case: value equals lower bound
        QgVersionResult result4;
        passed = result4.assertBetween(1, 1, 10);
        assertTrue(passed);

        // Edge case: value equals upper bound
        QgVersionResult result5;
        passed = result5.assertBetween(10, 1, 10);
        assertTrue(passed);
        return 0;
      }

      case "QgVersionResult_assertDynContains":
      {
        QgVersionResult result;
        dyn_string list = makeDynString("a", "b", "c");

        // "b" in list should pass
        bool passed = result.assertDynContains(list, "b");
        assertTrue(passed);
        assertFalse(result.hasError);

        // "x" not in list should fail
        QgVersionResult result2;
        passed = result2.assertDynContains(list, "x");
        assertFalse(passed);
        assertTrue(result2.hasError);
        return 0;
      }

      case "QgVersionResult_calculateScore":
      {
        QgVersionResult result;

        // No points yet
        result.totalPoints = 0;
        result.errorPoints = 0;
        float score = result.calculateScore();
        assertEqual(score, 0.0);

        // 10 total, 0 errors -> 100%
        result.totalPoints = 10;
        result.errorPoints = 0;
        score = result.calculateScore();
        assertEqual(score, 100.0);

        // 10 total, 2 errors -> 80%
        result.totalPoints = 10;
        result.errorPoints = 2;
        score = result.calculateScore();
        assertEqual(score, 80.0);

        // 10 total, 10 errors -> 0%
        result.totalPoints = 10;
        result.errorPoints = 10;
        score = result.calculateScore();
        assertEqual(score, 0.0);
        return 0;
      }

      case "QgVersionResult_info":
      {
        QgVersionResult result;

        // info() should not set error
        bool passed = result.info(42, 2);
        assertTrue(passed);
        assertFalse(result.hasError);
        assertEqual(result.value, "42");
        assertEqual(result.totalPoints, 2);
        assertEqual(result.errorPoints, 0);
        return 0;
      }
    }

    return -1;
  }
};

//--------------------------------------------------------------------------------
main()
{
  TstQgVersionResult test;
  test.startAll();
  exit(0);
}
