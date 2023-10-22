//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2023 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//

//--------------------------------------------------------------------------------
/*! Quality-Gate base utils.
 *
 * @details Base utilities to handle with QualityGate.
 * @author lschopp
 */
class Qg
{
//--------------------------------------------------------------------------------
//@public members
//--------------------------------------------------------------------------------
  //------------------------------------------------------------------------------
  /** Function returns id for current QualityGate.
    @return ID of QualityGate.
    @exception 0 when ID are not setted.
  */
  public static string getId()
  {
    return _id;
  }

  //------------------------------------------------------------------------------
  /** Function set id for current QualityGate.
    @details Each QualityGate must have unique ID to handle with result files.
    @warning This ID is loaded "global" in the manager. It can not be quered by other
             manager at the same time.
    @param id QualityGate ID.
  */
  public static void setId(const string &id)
  {
    _id = id;
  }

//--------------------------------------------------------------------------------
//@private members
//--------------------------------------------------------------------------------
  static string _id; //!< Current QG Id.
};

