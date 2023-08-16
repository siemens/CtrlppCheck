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
/*! @brief Float variable.
 *
 * @details Implements missing operations for float variable.
 * @author lschopp
 */
class Float
{
//--------------------------------------------------------------------------------
//@public members
//--------------------------------------------------------------------------------
  //------------------------------------------------------------------------------
  /** @brief Defualt c-tor
    @param f Value of float variable.
  */
  public Float(float f = 0)
  {
    _f = f;
  }

  //------------------------------------------------------------------------------
  /** @brief Function returns value of float variable.
    @return value of variable.
  */
  public float get()
  {
    return _f;
  }

  //------------------------------------------------------------------------------
  /** @brief Function sets new content to variable.
    @param f New float value.
  */
  public void set(float f = 0)
  {
    _f = f;
  }

  //------------------------------------------------------------------------------
  /** @brief Function rounds the float variable with given precision.
    @param precision Round precision.
    @return Rounded float variable.
  */
  public float round(int precision = 2)
  {
    if (precision < 0)
      precision = 0;

    float precisionFactor = pow(10.0, precision);
    return floor(_f * precisionFactor + 0.5) / precisionFactor;
  }

//--------------------------------------------------------------------------------
//@private members
//--------------------------------------------------------------------------------
  float _f;
};
