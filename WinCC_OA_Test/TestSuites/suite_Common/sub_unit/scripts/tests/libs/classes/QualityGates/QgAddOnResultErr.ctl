//--------------------------------------------------------------------------------
/**
  @file $relPath
  @copyright Copyright 2026 SIEMENS AG
             SPDX-License-Identifier: GPL-3.0-only
*/

/*!
 * @brief Tests for class: QgAddOnResultErr
 */

//--------------------------------------------------------------------------------
// used libraries (#uses)
#uses "classes/QualityGates/QgAddOnResultErr" /*!< tested object */
#uses "classes/oaTest/OaTest"

//--------------------------------------------------------------------------------
class TstQgAddOnResultErr : OaTest
{
  public dyn_string getAllTestCaseIds()
  {
    return makeDynString(
      "QgAddOnResultErr_ctor",
      "QgAddOnResultErr_getPriority",
      "QgAddOnResultErr_getPriorityAsText",
      "QgAddOnResultErr_toMap"
    );
  }

  protected int startTestCase(const string tcId)
  {
    switch (tcId)
    {
      case "QgAddOnResultErr_ctor":
      {
        // Create error with different error codes
        QgAddOnResultErr errUnknown = QgAddOnResultErr(PRIO_WARNING, QgAddOnResultErrCode::Unknown, "TestNode");
        assertNotEqual(errUnknown.getText(), "");

        QgAddOnResultErr errDoesNotRun = QgAddOnResultErr(PRIO_SEVERE, QgAddOnResultErrCode::DoesNotRunSuccessfull, "TestScript.ctl");
        assertNotEqual(errDoesNotRun.getText(), "");
        return 0;
      }

      case "QgAddOnResultErr_getPriority":
      {
        QgAddOnResultErr errInfo = QgAddOnResultErr(PRIO_INFO, QgAddOnResultErrCode::Unknown, "Node1");
        assertEqual(errInfo.getPriority(), PRIO_INFO);

        QgAddOnResultErr errWarning = QgAddOnResultErr(PRIO_WARNING, QgAddOnResultErrCode::Unknown, "Node2");
        assertEqual(errWarning.getPriority(), PRIO_WARNING);

        QgAddOnResultErr errSevere = QgAddOnResultErr(PRIO_SEVERE, QgAddOnResultErrCode::Unknown, "Node3");
        assertEqual(errSevere.getPriority(), PRIO_SEVERE);
        return 0;
      }

      case "QgAddOnResultErr_getPriorityAsText":
      {
        QgAddOnResultErr errInfo = QgAddOnResultErr(PRIO_INFO, QgAddOnResultErrCode::Unknown, "Node");
        assertEqual(errInfo.getPriorityAsText(), "Info");

        QgAddOnResultErr errWarning = QgAddOnResultErr(PRIO_WARNING, QgAddOnResultErrCode::Unknown, "Node");
        assertEqual(errWarning.getPriorityAsText(), "Warning");

        QgAddOnResultErr errSevere = QgAddOnResultErr(PRIO_SEVERE, QgAddOnResultErrCode::Unknown, "Node");
        assertEqual(errSevere.getPriorityAsText(), "Error");
        return 0;
      }

      case "QgAddOnResultErr_toMap":
      {
        QgAddOnResultErr err = QgAddOnResultErr(PRIO_WARNING, QgAddOnResultErrCode::Unknown, "TestNode");
        mapping map = err.toMap();

        // Map should have priority as key
        assertTrue(mappingHasKey(map, "Warning"));
        assertNotEqual(map["Warning"], "");
        return 0;
      }
    }

    return -1;
  }
};

//--------------------------------------------------------------------------------
main()
{
  TstQgAddOnResultErr test;
  test.startAll();
  exit(0);
}
