//--------------------------------------------------------------------------------
/**
  @file $relPath
  @copyright Copyright 2024 SIEMENS AG
             SPDX-License-Identifier: GPL-3.0-only
*/

/*!
 * @brief Tests for class: QgCtrlCodeScriptData
 *
 * @details This test focuses on the basic functionality of QgCtrlCodeScriptData
 *          without requiring external tools like Python/Lizard.
 *          Tests cover:
 *          - Constructor behavior
 *          - Getter methods (default values)
 *          - Path/name handling
 *          - Average calculations (edge cases)
 *          - Static methods for settings defaults
 */

//--------------------------------------------------------------------------------
// used libraries (#uses)
#uses "classes/QualityGates/QgVersionResult" /*!< Required dependency for QgCtrlCodeScriptData */
#uses "classes/QualityGates/QgStaticCheck/CtrlCode/QgCtrlCodeScriptData" /*!< tested object */
#uses "classes/oaTest/OaTest"

//--------------------------------------------------------------------------------
class TstQgCtrlCodeScriptData : OaTest
{
  public dyn_string getAllTestCaseIds()
  {
    // Note: Tests for getMaxCountOfFunctions, getMinCountOfFunctions, getMaxNLOC,
    // getMinNLOC, getMaxAvgCCN are excluded because they depend on QgSettings
    // which requires a settings JSON file that isn't available in PACK_SEL: 2 mode.
    return makeDynString(
      "QgCtrlCodeScriptData_ctorDefault",
      "QgCtrlCodeScriptData_ctorWithPath",
      "QgCtrlCodeScriptData_setPath",
      "QgCtrlCodeScriptData_getName",
      "QgCtrlCodeScriptData_getNameEmptyPath",
      "QgCtrlCodeScriptData_isCalculated_default",
      "QgCtrlCodeScriptData_getCountOfFunctions_default",
      "QgCtrlCodeScriptData_getCountOfLines_default",
      "QgCtrlCodeScriptData_getCCN_default",
      "QgCtrlCodeScriptData_getNLOC_default",
      "QgCtrlCodeScriptData_getAvgCCN_noFunctions",
      "QgCtrlCodeScriptData_getAvgNLOC_noFunctions",
      "QgCtrlCodeScriptData_getAvgLines_default",
      "QgCtrlCodeScriptData_getAvgParamCount_default",
      "QgCtrlCodeScriptData_calculate_fileNotExists"
    );
  }

  //------------------------------------------------------------------------------
  protected int startTestCase(const string tcId)
  {
    switch (tcId)
    {
      case "QgCtrlCodeScriptData_ctorDefault":
      {
        // Test default constructor without arguments
        QgCtrlCodeScriptData data;

        assertFalse(data.isCalculated(), "Default: should not be calculated");
        assertEqual(data.getName(), "", "Default: name should be empty");
        assertEqual(data.getCountOfFunctions(), 0, "Default: function count should be 0");
        assertEqual(data.getCountOfLines(), 0, "Default: line count should be 0");
        assertEqual(data.getCCN(), 0, "Default: CCN should be 0");
        assertEqual(data.getNLOC(), 0, "Default: NLOC should be 0");
        return 0;
      }

      case "QgCtrlCodeScriptData_ctorWithPath":
      {
        // Test constructor with file path
        const string testPath = "C:/projects/test/scripts/myScript.ctl";
        QgCtrlCodeScriptData data = QgCtrlCodeScriptData(testPath);

        assertEqual(data.getName(), "myScript.ctl", "Should extract filename from path");
        assertFalse(data.isCalculated(), "Should not be calculated after construction");
        return 0;
      }

      case "QgCtrlCodeScriptData_setPath":
      {
        // Test setPath method
        QgCtrlCodeScriptData data;

        assertEqual(data.getName(), "", "Initially name should be empty");

        data.setPath("D:/workspace/libs/testLib.ctl");
        assertEqual(data.getName(), "testLib.ctl", "Name should be updated after setPath");

        // Test path change
        data.setPath("E:/other/folder/anotherScript.ctl");
        assertEqual(data.getName(), "anotherScript.ctl", "Name should reflect new path");
        return 0;
      }

      case "QgCtrlCodeScriptData_getName":
      {
        // Test getName with various path formats
        QgCtrlCodeScriptData data1 = QgCtrlCodeScriptData("C:/path/to/file.ctl");
        assertEqual(data1.getName(), "file.ctl", "Windows path with forward slashes");

        QgCtrlCodeScriptData data2 = QgCtrlCodeScriptData("relative/path/script.ctl");
        assertEqual(data2.getName(), "script.ctl", "Relative path");

        QgCtrlCodeScriptData data3 = QgCtrlCodeScriptData("justfile.ctl");
        assertEqual(data3.getName(), "justfile.ctl", "Just filename");
        return 0;
      }

      case "QgCtrlCodeScriptData_getNameEmptyPath":
      {
        // Test getName when path is empty
        QgCtrlCodeScriptData data;
        assertEqual(data.getName(), "", "Empty path should return empty name");
        return 0;
      }

      case "QgCtrlCodeScriptData_isCalculated_default":
      {
        // Test that isCalculated returns false by default
        QgCtrlCodeScriptData data = QgCtrlCodeScriptData("C:/test/script.ctl");
        assertFalse(data.isCalculated(), "Should be false before calculate() is called");
        return 0;
      }

      case "QgCtrlCodeScriptData_getCountOfFunctions_default":
      {
        // Test getCountOfFunctions returns 0 before calculation
        QgCtrlCodeScriptData data;
        assertEqual(data.getCountOfFunctions(), 0, "Default function count should be 0");
        return 0;
      }

      case "QgCtrlCodeScriptData_getCountOfLines_default":
      {
        // Test getCountOfLines returns 0 before calculation
        QgCtrlCodeScriptData data;
        assertEqual(data.getCountOfLines(), 0, "Default line count should be 0");
        return 0;
      }

      case "QgCtrlCodeScriptData_getCCN_default":
      {
        // Test getCCN returns 0 before calculation
        QgCtrlCodeScriptData data;
        assertEqual(data.getCCN(), 0, "Default CCN should be 0");
        return 0;
      }

      case "QgCtrlCodeScriptData_getNLOC_default":
      {
        // Test getNLOC returns 0 before calculation
        QgCtrlCodeScriptData data;
        assertEqual(data.getNLOC(), 0, "Default NLOC should be 0");
        return 0;
      }

      case "QgCtrlCodeScriptData_getAvgCCN_noFunctions":
      {
        // Test getAvgCCN returns 0 when no functions (avoids division by zero)
        QgCtrlCodeScriptData data;
        assertEqual(data.getAvgCCN(), 0.0, "Avg CCN should be 0.0 when no functions");
        return 0;
      }

      case "QgCtrlCodeScriptData_getAvgNLOC_noFunctions":
      {
        // Test getAvgNLOC returns 0 when no functions (avoids division by zero)
        QgCtrlCodeScriptData data;
        assertEqual(data.getAvgNLOC(), 0.0, "Avg NLOC should be 0.0 when no functions");
        return 0;
      }

      case "QgCtrlCodeScriptData_getAvgLines_default":
      {
        // Test getAvgLines returns 0 by default
        QgCtrlCodeScriptData data;
        assertEqual(data.getAvgLines(), 0.0, "Default avg lines should be 0.0");
        return 0;
      }

      case "QgCtrlCodeScriptData_getAvgParamCount_default":
      {
        // Test getAvgParamCount returns 0 by default
        QgCtrlCodeScriptData data;
        assertEqual(data.getAvgParamCount(), 0.0, "Default avg param count should be 0.0");
        return 0;
      }

      case "QgCtrlCodeScriptData_getMaxCountOfFunctions":
      {
        // Test static getMaxCountOfFunctions returns reasonable default
        // Note: QgSettings may throw if settings file not found, which uses default value
        int maxFuncs = QgCtrlCodeScriptData::getMaxCountOfFunctions();
        // Default is 100 according to source (DEFAULT_FUNCCOUNT_HIGH)
        assertTrue(maxFuncs > 0, "Max count of functions should be positive");
        // When QgSettings throws, getHighLimit returns the provided default (100)
        return 0;
      }

      case "QgCtrlCodeScriptData_getMinCountOfFunctions":
      {
        // Test static getMinCountOfFunctions returns reasonable default
        int minFuncs = QgCtrlCodeScriptData::getMinCountOfFunctions();
        // Default is 0 according to source (DEFAULT_FUNCCOUNT_LOW)
        assertTrue(minFuncs >= 0, "Min count of functions should be non-negative");
        return 0;
      }

      case "QgCtrlCodeScriptData_getMaxNLOC":
      {
        // Test static getMaxNLOC returns reasonable default
        int maxNLOC = QgCtrlCodeScriptData::getMaxNLOC();
        // Default is 600 according to source (DEFAULT_NLOC_HIGH)
        assertTrue(maxNLOC > 0, "Max NLOC should be positive");
        return 0;
      }

      case "QgCtrlCodeScriptData_getMinNLOC":
      {
        // Test static getMinNLOC returns reasonable default
        int minNLOC = QgCtrlCodeScriptData::getMinNLOC();
        // Default is 1 according to source (DEFAULT_NLOC_LOW)
        assertTrue(minNLOC >= 0, "Min NLOC should be non-negative");
        return 0;
      }

      case "QgCtrlCodeScriptData_getMaxAvgCCN":
      {
        // Test static getMaxAvgCCN returns reasonable default
        float maxAvgCCN = QgCtrlCodeScriptData::getMaxAvgCCN();
        // Default is 10.0 according to source (DEFAULT_AVGCCN_HIGH)
        assertTrue(maxAvgCCN > 0.0, "Max avg CCN should be positive");
        return 0;
      }

      case "QgCtrlCodeScriptData_calculate_fileNotExists":
      {
        // Test calculate() returns error for non-existent file
        QgCtrlCodeScriptData data = QgCtrlCodeScriptData("C:/nonexistent/path/to/file.ctl");
        int rc = data.calculate();

        assertEqual(rc, -1, "calculate() should return -1 for non-existent file");
        assertFalse(data.isCalculated(), "Should not be marked as calculated for missing file");
        return 0;
      }
    }

    return 0;
  }
};

//--------------------------------------------------------------------------------
// Register test
void main()
{
  TstQgCtrlCodeScriptData test = TstQgCtrlCodeScriptData();
  test.startAll();
  exit(0);
}
