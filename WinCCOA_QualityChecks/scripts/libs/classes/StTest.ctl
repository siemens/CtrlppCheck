//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2022 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//

//--------------------------------------------------------------------------------
// used libraries (#uses)
#uses "CtrlOaUnit"
#uses "classes/QualityGates/Qg"
#uses "classes/oaTest/OaTest"

//--------------------------------------------------------------------------------
// declare variables and constans

//--------------------------------------------------------------------------------
/*! @brief WinCCOA Test class.
 *
 * @details This class defined base test class. For each test you must create own
 *         derived class.
 *
 * @author lschopp
 */
class StTest : OaTest
{
//--------------------------------------------------------------------------------
//@public members
//--------------------------------------------------------------------------------

  //------------------------------------------------------------------------------
  /** @brief Function returns all test-cases defined for this test.
    @return List with all test-case IDs.
  */
  public dyn_string getTestCaseIds()
  {
    return _tcIds;
  }
  
  public string testSuiteId;
  public string _testCaseId; //!< Current test case ID.
  public dyn_string _tcIds;  //!< List with all test-case IDs. must be overloaded in derived class.
  public static const int RESULT_FILE_FORMAT = 2;  //!< Default result output.
  
//--------------------------------------------------------------------------------
//@private members
//--------------------------------------------------------------------------------
  int _errCount; // count of all NOT passed test-cases.
  
};


