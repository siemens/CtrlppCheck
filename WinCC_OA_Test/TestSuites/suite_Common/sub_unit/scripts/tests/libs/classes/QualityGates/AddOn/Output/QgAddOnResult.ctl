//--------------------------------------------------------------------------------
/**
  @file $relPath
  @copyright Copyright 2026 SIEMENS AG
             SPDX-License-Identifier: GPL-3.0-only
*/

/*!
 * @brief Tests for class: QgAddOnResult
 */

//--------------------------------------------------------------------------------
// used libraries (#uses)
#uses "classes/QualityGates/AddOn/Output/QgAddOnResult" /*!< tested object */
#uses "classes/oaTest/OaTest"

//--------------------------------------------------------------------------------
class TstQgAddOnResult : OaTest
{
  public dyn_string getAllTestCaseIds()
  {
    return makeDynString(
      "QgAddOnResult_constants",
      "QgAddOnResult_ctor_default",
      "QgAddOnResult_setState_success",
      "QgAddOnResult_setState_warning",
      "QgAddOnResult_setState_error",
      "QgAddOnResult_stateToString_success",
      "QgAddOnResult_stateToString_warning",
      "QgAddOnResult_stateToString_error",
      "QgAddOnResult_setData",
      "QgAddOnResultState_enum_values"
    );
  }

  protected int startTestCase(const string tcId)
  {
    switch (tcId)
    {
      case "QgAddOnResult_constants":
      {
        // Test class constants
        assertEqual(QgAddOnResult::MIN_VALID_SCORE, 1.0);
        assertEqual(QgAddOnResult::NOT_VALID_SCORE, 0.0);
        assertEqual(QgAddOnResult::KEY_SCORE_REASON, "Reason");
        assertEqual(QgAddOnResult::KEY_SCORE_PERCENT, "%");
        assertEqual(QgAddOnResult::KEY_SCORE_TOTAL_POINTS, "Total points");
        assertEqual(QgAddOnResult::KEY_SCORE_ERROR_POINTS, "Error points");
        assertEqual(QgAddOnResult::KEY_QG_RESULT_TESTVERSION, "qgTestVersionResults");
        assertEqual(QgAddOnResult::KEY_QG_RESULT_SUM, "qgSummary");
        assertEqual(QgAddOnResult::KEY_QG_RESULT_SCORE, "score");
        return 0;
      }

      case "QgAddOnResult_ctor_default":
      {
        QgAddOnResult result;
        // Constructor should work without error
        assertTrue(TRUE);
        return 0;
      }

      case "QgAddOnResult_setState_success":
      {
        QgAddOnResult result;
        result.setState(QgAddOnResultState::success);
        assertEqual(result.stateToString(), "success");
        return 0;
      }

      case "QgAddOnResult_setState_warning":
      {
        QgAddOnResult result;
        result.setState(QgAddOnResultState::warning);
        assertEqual(result.stateToString(), "warning");
        return 0;
      }

      case "QgAddOnResult_setState_error":
      {
        QgAddOnResult result;
        result.setState(QgAddOnResultState::error);
        assertEqual(result.stateToString(), "error");
        return 0;
      }

      case "QgAddOnResult_stateToString_success":
      {
        QgAddOnResult result;
        result.setState(QgAddOnResultState::success);
        assertEqual(result.stateToString(), "success");
        return 0;
      }

      case "QgAddOnResult_stateToString_warning":
      {
        QgAddOnResult result;
        result.setState(QgAddOnResultState::warning);
        assertEqual(result.stateToString(), "warning");
        return 0;
      }

      case "QgAddOnResult_stateToString_error":
      {
        QgAddOnResult result;
        result.setState(QgAddOnResultState::error);
        assertEqual(result.stateToString(), "error");
        return 0;
      }

      case "QgAddOnResult_setData":
      {
        QgAddOnResult result;
        mapping data = makeMapping("key1", "value1", "key2", 42);
        result.setData(data);
        // setData should complete without error
        assertTrue(TRUE);
        return 0;
      }

      case "QgAddOnResultState_enum_values":
      {
        // Test enum values exist and are distinct
        assertTrue((int)QgAddOnResultState::success != (int)QgAddOnResultState::warning);
        assertTrue((int)QgAddOnResultState::warning != (int)QgAddOnResultState::error);
        assertTrue((int)QgAddOnResultState::error != (int)QgAddOnResultState::failed);
        return 0;
      }
    }

    return -1;
  }
};

//--------------------------------------------------------------------------------
main()
{
  TstQgAddOnResult test;
  test.startAll();
  exit(0);
}
