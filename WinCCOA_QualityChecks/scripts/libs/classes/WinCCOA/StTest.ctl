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
class StTest
{
//--------------------------------------------------------------------------------
//@public members
//--------------------------------------------------------------------------------
  
  //------------------------------------------------------------------------------
  public int startAll()
  {
    if ( dynlen(_tcIds) <= 0 )
      _tcIds = _getAllTcIds();
    
    return start();
  }
  //------------------------------------------------------------------------------
  /** @brief Function start tests.
   * @details Function starts all test-cases defined in _tcIds.
   * @return Error code.
   * value | description
   * ------|------------
   * 0     | Success.
   * -1    | List of test-cases is empty.
   * -2    | Setup does not works.
   * -3    | Tear-Down does not works.
   */
  public int start()
  {

    if ( dynlen(_tcIds) <= 0 )
      _tcIds = _getAllTcIds();
    
    if ( dynlen(_tcIds) <= 0 )
    {
      DebugFTN("StTest", __FUNCTION__, "has not found tc ids");
      return -1;
    }

    if ( setUp() )
    {
      DebugFTN("StTest", __FUNCTION__, "setUp returns some error");
      return -2;
    }

    for(int i = 1; i <= dynlen(_tcIds); i++)
    {
      oaUnitInfo(_tcIds[i], "start test case");
      if ( startSingle(_tcIds[i]) )
        _errCount++;
    }

    if ( tearDown() )
    {
      DebugFTN("StTest", __FUNCTION__, "tearDown returns some error");
      return -3;
    }

    return _errCount;
  }
  
  //------------------------------------------------------------------------------
  public dyn_string _getAllTcIds()
  {
    return makeDynString();
  }

  //------------------------------------------------------------------------------
  /** @brief Default tear-down of this test.
    @details Function is called when are tests finished to clear some test dependency.
             This is created for customizing the the tests.
    @return Error code. Returns 0 when successfull, otherwise -1.
  */
  public int tearDown()
  {
    return 0;
  }

  //------------------------------------------------------------------------------
  /** @brief Default setup of this test.
    @details Function is called before start (on setup) of test cases.
             This is created for customizing the the tests.
    @return Error code. Returns 0 when successful, otherwise -1.
  */
  public int setUp()
  {
    if ( !Qg::isRunningOnJenkins() )
      return 0; // init test dir only on jenkins
      
    string dir = getenv("WORKSPACE") + "/test/";
    if ( !isdir(dir) )
      mkdir(dir);
  
    string timeStamp = (int)getCurrentTime();
    
    dir += timeStamp + "/";   
    mkdir(dir);
     
    string fileName;
    
    if ( testSuiteId != "" )
      fileName = testSuiteId + ".json";
    else
    {
      fileName = getTypeName(this);
      int idx = strpos(fileName, ":");
      if ( idx > 0 )
        fileName = substr(fileName, 0, idx);
    }
    
    if ( fileName == "" )
      fileName = "result.json";
      
    if ( oaUnitSetup(dir + fileName, makeMapping("Format", RESULT_FILE_FORMAT)) )
      return -1;
    
    return 0;
  }

  //------------------------------------------------------------------------------
  /** @brief Function start single test case.
   * @details Test cases are defined in _tcIds.
   * @param testCaseId ID of the test case. Id must be unique in the test class.
   * @return Error code. Returns 0 when successfull. For more error codes see also
   *         oaUnit functions.
   */
  public int startSingle(const string &testCaseId)
  {
    _testCaseId = testCaseId;
    DebugFTN("StTest", __FUNCTION__, _testCaseId);
    try
    {
      int err = _startSingle();
      if ( err == 0 )
        return 0;

      if ( err < 0 )
        return oaUnitAbort(_testCaseId, "undefined test case");

      return err;
    }
    catch
    {
      throwError(getLastException());
      return oaUnitAbort(_testCaseId, "luaft in catch");
    }
    return 0;  // defensive
  }
  
  //------------------------------------------------------------------------------
  public string _getTcId()
  {
    return _testCaseId;
  }

  //------------------------------------------------------------------------------
  /** @brief Place holder for customized test cases.
    * @details Function is called to start customized test-cases. This must be 
    *          overloaded in  derived test class. Here is place to make the test-cases
    *          self.
    * @return Error code. 
    * value | description
    * ------|------------
    * 0     | Success.
    * -1    | Test-case is aborted or failed.
    */
  public int _startSingle()
  {
    return _startTest();
  }

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


