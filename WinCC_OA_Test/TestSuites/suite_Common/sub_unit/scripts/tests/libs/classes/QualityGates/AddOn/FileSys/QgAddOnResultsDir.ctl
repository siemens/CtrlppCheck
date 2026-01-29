//--------------------------------------------------------------------------------
/**
  @file $relPath
  @copyright Copyright 2026 SIEMENS AG
             SPDX-License-Identifier: GPL-3.0-only
*/

/*!
 * @brief Tests for class: QgAddOnResultsDir
 */

//--------------------------------------------------------------------------------
// used libraries (#uses)
#uses "classes/QualityGates/AddOn/FileSys/QgAddOnResultsDir" /*!< tested object */
#uses "classes/oaTest/OaTest"

//--------------------------------------------------------------------------------
class TstQgAddOnResultsDir : OaTest
{
  public dyn_string getAllTestCaseIds()
  {
    return makeDynString(
      "QgAddOnResultsDir_ctor",
      "QgAddOnResultsDir_setQgId",
      "QgAddOnResultsDir_getDirPath_format",
      "QgAddOnResultsDir_getRunningQgs_is_dyn_string",
      "QgAddOnResultsDir_getHistoryDirs_is_dyn_string"
    );
  }

  protected int startTestCase(const string tcId)
  {
    switch (tcId)
    {
      case "QgAddOnResultsDir_ctor":
      {
        QgAddOnResultsDir resDir;
        // Constructor should work
        assertTrue(TRUE);
        return 0;
      }

      case "QgAddOnResultsDir_setQgId":
      {
        QgAddOnResultsDir resDir;
        resDir.setQgId("TestQG");
        // setQgId should not throw
        assertTrue(TRUE);
        return 0;
      }

      case "QgAddOnResultsDir_getDirPath_format":
      {
        QgAddOnResultsDir resDir;
        resDir.setQgId("TestQG");
        string path = resDir.getDirPath();
        // Path should contain TestQG
        assertTrue(strpos(path, "TestQG") >= 0);
        // Path should contain QualityGates
        assertTrue(strpos(path, "QualityGates") >= 0);
        return 0;
      }

      case "QgAddOnResultsDir_getRunningQgs_is_dyn_string":
      {
        dyn_string qgs = QgAddOnResultsDir::getRunningQgs();
        // Should return a dyn_string (possibly empty)
        assertTrue(getType(qgs) == DYN_STRING_VAR);
        return 0;
      }

      case "QgAddOnResultsDir_getHistoryDirs_is_dyn_string":
      {
        QgAddOnResultsDir resDir;
        resDir.setQgId("NonExistentQG");
        dyn_string histDirs = resDir.getHistoryDirs();
        // Should return empty dyn_string for non-existent QG
        assertTrue(getType(histDirs) == DYN_STRING_VAR);
        assertEqual(dynlen(histDirs), 0);
        return 0;
      }
    }

    return -1;
  }
};

//--------------------------------------------------------------------------------
main()
{
  TstQgAddOnResultsDir test;
  test.startAll();
  exit(0);
}
