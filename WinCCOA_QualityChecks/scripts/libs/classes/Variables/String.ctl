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
/*! @brief String variable.
 *
 * @details Implements missing operations for String variable.
 * @author mPunk
 */
class String
{
//--------------------------------------------------------------------------------
//@public members
//--------------------------------------------------------------------------------
  //------------------------------------------------------------------------------
  /** @brief Defualt c-tor
    @param f Value of string variable.
  */
  public String(string s = "")
  {
    _s = s;
  }
  
  //------------------------------------------------------------------------------
  /** @brief Function returns value of string variable.
    @return value of variable.
  */
  public string get()
  {
    return _s;
  }
  
  //------------------------------------------------------------------------------
  /** @brief Function sets new content to variable.
    @param f New string value.
  */
  public void set(string s = "")
  {
    _s = s;
  }
  
  public bool endsWith(const string &str)
  {
    return substr(_s, strlen(_s) - strlen(str)) == str;
  }
//--------------------------------------------------------------------------------
//@private members
//--------------------------------------------------------------------------------
  string _s;
};
