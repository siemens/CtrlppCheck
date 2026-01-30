//--------------------------------------------------------------------------------
/**
  @file $relPath
  @copyright Copyright 2026 SIEMENS AG
             SPDX-License-Identifier: GPL-3.0-only
*/

/*!
 * @brief Tests for class: QgResultPublisher
 */

//--------------------------------------------------------------------------------
// used libraries (#uses)
#uses "classes/QualityGates/QgResultPublisher" /*!< tested object */
#uses "classes/oaTest/OaTest"

//--------------------------------------------------------------------------------
class TstQgResultPublisher : OaTest
{
  public dyn_string getAllTestCaseIds()
  {
    return makeDynString(
      "QgResultState_enum_values",
      "QgResultPublisher_ctor",
      "QgResultPublisher_stateToString_success",
      "QgResultPublisher_stateToString_warning",
      "QgResultPublisher_stateToString_error",
      "QgResultPublisher_jsonFormat_default",
      "QgVersionResultJsonFormat_enum_values"
    );
  }

  protected int startTestCase(const string tcId)
  {
    switch (tcId)
    {
      case "QgResultState_enum_values":
      {
        // Test enum values exist and are distinct
        assertTrue((int)QgResultState::success != (int)QgResultState::warning);
        assertTrue((int)QgResultState::warning != (int)QgResultState::error);
        assertTrue((int)QgResultState::success != (int)QgResultState::error);
        return 0;
      }

      case "QgResultPublisher_ctor":
      {
        QgResultPublisher publisher;
        // Constructor should initialize fields
        assertEqual(dynlen(publisher.fields), 0);
        return 0;
      }

      case "QgResultPublisher_stateToString_success":
      {
        assertEqual(QgResultPublisher::stateToString(QgResultState::success), "success");
        return 0;
      }

      case "QgResultPublisher_stateToString_warning":
      {
        assertEqual(QgResultPublisher::stateToString(QgResultState::warning), "warning");
        return 0;
      }

      case "QgResultPublisher_stateToString_error":
      {
        assertEqual(QgResultPublisher::stateToString(QgResultState::error), "error");
        return 0;
      }

      case "QgResultPublisher_jsonFormat_default":
      {
        // Static member should be Compact by default
        assertEqual(QgResultPublisher::jsonFormat, QgVersionResultJsonFormat::Compact);
        return 0;
      }

      case "QgVersionResultJsonFormat_enum_values":
      {
        // Test both formats exist
        QgVersionResultJsonFormat fmt1 = QgVersionResultJsonFormat::Compact;
        QgVersionResultJsonFormat fmt2 = QgVersionResultJsonFormat::Indented;
        assertTrue((int)fmt1 != (int)fmt2);
        return 0;
      }
    }

    return -1;
  }
};

//--------------------------------------------------------------------------------
main()
{
  TstQgResultPublisher test;
  test.startAll();
  exit(0);
}
