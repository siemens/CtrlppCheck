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

//--------------------------------------------------------------------------------
// declare variables and constans

//--------------------------------------------------------------------------------
/*! @brief Quality-Gate base utils.
 *
 * @details Base utilitys to handle with QualityGate.
 * @author lschopp
 */
class Qg
{
//--------------------------------------------------------------------------------
//@public members
//--------------------------------------------------------------------------------
  //------------------------------------------------------------------------------
  /** @brief Function returns id for current QualityGate.
    @return ID of QualityGate.
    @exception 0 when ID are not setted.
  */
  public static string getId()
  {
    return _id;
  }
  
  //------------------------------------------------------------------------------
  /** @brief Function set id for current QualityGate.
    @details Each QualityGate must have unique ID to handle with result files.
    @warning This ID is loaded "global" in the manager. It can not be quered by other
             manager at the same time.
    @param id QualityGate ID.
  */
  public static void setId(const string id)
  {
    _id = id;
  }
  
  //------------------------------------------------------------------------------
  /** @brief Function returns all defined QualityGate IDs.
    @warning This list muss be equal to store-DB. in other case can not be results
             imported in store-DB.
    @return List with all QG-IDs.
  */
  public static dyn_string getAllIds()
  {
    return makeDynString("QgStaticCheck_Pictures", "QgStaticCheck_Scripts", "QgStaticCheck_Panels",
                         "QgSyntaxCheck", 
                         "QgStaticCheck_OverloadedFiles", "Documentation",
                         "UnitTests", "CtrlCoverage", "QgCtrlppCheck",
                         "QgStaticCheck_Libs");
  }
  
  //------------------------------------------------------------------------------
  /** @brief Function converts current QG-ID to int variable.
    @details We need this convert for backend (store).
    @return 
  */
  public static int idToNum()
  {
    return dynContains(getAllIds(), getId());
  }
  
  //------------------------------------------------------------------------------
  public static bool isRunningOnJenkins()
  {
    return (getenv("WORKSPACE") != "");
  }
  
//--------------------------------------------------------------------------------
//@private members
//--------------------------------------------------------------------------------
  static string _id; //!< Current QG Id.
};

