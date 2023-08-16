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
/*!
 * @brief Implemented mathematic function missing in WinCC OA
 *
 * @author lschopp
 */
class Math
{
//--------------------------------------------------------------------------------
//@public members
//--------------------------------------------------------------------------------

  //------------------------------------------------------------------------------
  /**
   * @brief Default c-tor
   *
   *
   * @return intialized object of class Math
   */
  public Math()
  {
  }

  //------------------------------------------------------------------------------
  /** @brief Function calculated percentil from 2 float variables.
   * @param f1
   * @param f2
   * @return Percentil.
   */
  public static float getPercent(const float &f1, const float &f2)
  {
    if( (f1 == 0.0) || (f2 == 0) )
      return 0.0;

    float f = f1 / f2;

    f = (float)100 / f;

    return f;
  }

//--------------------------------------------------------------------------------
//@private members
//--------------------------------------------------------------------------------
};
