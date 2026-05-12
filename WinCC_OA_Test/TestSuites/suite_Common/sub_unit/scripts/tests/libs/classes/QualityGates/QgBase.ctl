//--------------------------------------------------------------------------------
/**
  @file $relPath
  @copyright Copyright 2026 SIEMENS AG
             SPDX-License-Identifier: GPL-3.0-only
*/

/*!
 * @brief Tests for class: QgBase (base class tests)
 */

//--------------------------------------------------------------------------------
// used libraries (#uses)
#uses "classes/QualityGates/QgBase" /*!< tested object */
#uses "classes/oaTest/OaTest"

//--------------------------------------------------------------------------------
class TstQgBase : OaTest
{
  public dyn_string getAllTestCaseIds()
  {
    return makeDynString(
      "QgBaseError_enum_values",
      "QgBase_getStoreFields",
      "QgBase_calculateState_no_error",
      "QgBase_calculateState_with_error"
    );
  }

  protected int startTestCase(const string tcId)
  {
    switch (tcId)
    {
      case "QgBaseError_enum_values":
      {
        // Test enum values are defined
        assertTrue((int)QgBaseError::Exception > 0);
        assertTrue((int)QgBaseError::NotImplemented > 0);
        assertTrue((int)QgBaseError::Start > 0);
        assertTrue((int)QgBaseError::Calculate > 0);
        assertTrue((int)QgBaseError::Validate > 0);
        assertTrue((int)QgBaseError::Done > 0);
        return 0;
      }

      case "QgBase_getStoreFields":
      {
        dyn_string fields = QgBase::getStoreFields();
        assertEqual(dynlen(fields), 6);
        assertTrue(dynContains(fields, "value") > 0);
        assertTrue(dynContains(fields, "descr") > 0);
        assertTrue(dynContains(fields, "goodRange") > 0);
        assertTrue(dynContains(fields, "totalPoints") > 0);
        assertTrue(dynContains(fields, "errorPoints") > 0);
        assertTrue(dynContains(fields, "reason") > 0);
        return 0;
      }

      case "QgBase_calculateState_no_error":
      {
        shared_ptr<QgVersionResult> result = new QgVersionResult();
        result.hasError = FALSE;
        QgResultState state = QgBase::calculateState(result);
        assertEqual(state, QgResultState::success);
        return 0;
      }

      case "QgBase_calculateState_with_error":
      {
        shared_ptr<QgVersionResult> result = new QgVersionResult();
        result.hasError = TRUE;
        QgResultState state = QgBase::calculateState(result);
        assertEqual(state, QgResultState::warning);
        return 0;
      }
    }

    return -1;
  }
};

//--------------------------------------------------------------------------------
main()
{
  TstQgBase test;
  test.startAll();
  exit(0);
}
