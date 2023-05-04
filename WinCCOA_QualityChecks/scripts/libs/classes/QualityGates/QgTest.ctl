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
#uses "classes/QualityGates/QgMsgCat"
#uses "classes/Variables/Float"
#uses "classes/oaTest/OaTest"

//--------------------------------------------------------------------------------
// declare variables and constans

//--------------------------------------------------------------------------------
/*!
  @brief QualityGate test class.

  @details OaTest customized to handle with QualityGates.
  @author lschopp
*/
class QgTest : OaTest
{
//--------------------------------------------------------------------------------
//@public members
//--------------------------------------------------------------------------------
  
    
  //------------------------------------------------------------------------------
  /** @brief Function calculates QG score.
    @details Calculates the QG-score depended of test-results. Calculate how many 
             percente of test-cases are OK.
    @return Percentil of passed testcases.
    @todo lschopp 23.05.2018: return result in float format. But we need to check what
          happends on store side.
  */
  public float calculateScore()
  {
    float error = getErrorCount();
    float all   = getAllCount();
    float perc;
    
    if ( all != 0 )
      perc = (error / all) * 100.0;
    
    perc = 100.0 - perc;
    
    Float f = Float(perc);
    perc = f.round();
    
    mapping map = makeMapping("Total points", all,
                              "Error points", error,
                              "%", perc);
    addScoreDetail(map);

    return perc;
  }
  
  public void addScoreDetail(const mapping &info)
  {
    try
    {
      // try-catch necessary / no guarantee that variable "_data" exists
      if ( mappingHasKey(_data, "qgTestVersionResults") )
      {
        _data["qgTestVersionResults"]["Score"] = info;
      }
    }
    catch
    {
    }
    
    DebugTN("QgTest", __FUNCTION__, info);
  }
    
  //------------------------------------------------------------------------------
  /** @brief Function returns count of all errors.
    @details Returns count of all NOT passed test cases executed in this QG.
    @return Count of errors.
  */
  public int getErrorCount()
  {
    return _errCount;
  }
  
  //------------------------------------------------------------------------------
  /** @brief Function returns count of all testcases.
    @details Returns count of all test cases executed in this QG.
    @return Count of all testcases.
  */
  public int getAllCount()
  {
    return _all;
  }
  
  //------------------------------------------------------------------------------
  /** @brief Function returns error priority of current error.
    @return Error priority.
  */
  public int getErrPrio()
  {
    return _errPrio;
  }
  
  //------------------------------------------------------------------------------
  /** @brief Function returns error code of current error.
    @return Error code.
  */
  public int getErrCode()
  {
    return _errCode;
  }
  
  //------------------------------------------------------------------------------
  /** @brief Function returns error note of current error.
    @return Error note.
  */
  public string getErrNote()
  {
    return _errNote;
  }

  public int _errCount;  //!< Count of all NOT passed test cases.
  public int _all;  //!< Count of all test-cases.

//--------------------------------------------------------------------------------
//@protected members
//--------------------------------------------------------------------------------  
  protected string _errNote  = "";       //!< Error note.
  protected int _errPrio = PRIO_SEVERE;  //!< Error priority.
  protected int _errCode = 1;            //!< Error code.

//--------------------------------------------------------------------------------
//@private members
//--------------------------------------------------------------------------------  
  const int RESULT_FILE_FORMAT = 3;  //!< Default result output.
};
