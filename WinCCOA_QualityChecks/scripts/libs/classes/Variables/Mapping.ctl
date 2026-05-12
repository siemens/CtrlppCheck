//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2023 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//

//--------------------------------------------------------------------------------
// used libraries (#uses)
#uses "std"

//--------------------------------------------------------------------------------
// declare variables and constans

//--------------------------------------------------------------------------------
/*! @brief Float variable.
 *
 * @details Implements missing operations for mapping variable.
 * @author lschopp
 */
class Mapping
{
//--------------------------------------------------------------------------------
//@public members
//--------------------------------------------------------------------------------
  //------------------------------------------------------------------------------
  /** @brief Defualt c-tor
   * @param map Value of mapping variable.
   * @warning The c-tor make a copy of variable and that cost some memory-usage..
   *          In case of big mappings it is better to use function set(), instead of c-tor.
   */
  public Mapping(mapping map = makeMapping())
  {
    _var = map;
  }

  //------------------------------------------------------------------------------
  /** @brief Function returns value of mapping variable.
   * @return value of variable.
   */
  public mapping get()
  {
    return _var;
  }

  //------------------------------------------------------------------------------
  /** @brief Function sets new content to variable.
   * @param map New mapping value.
   */
  public void set(const mapping &map)
  {
    _var = map;
  }

  //------------------------------------------------------------------------------
  /** @brief Function returns value of mapping variable at given key.
   * @return Value of variable. In case of missing key returns NULL.
   */
  public anytype getAt(const anytype &key, const anytype def = NULL)
  {
    if (mappingHasKey(_var, key))
      return _var[ key ];

    return def;
  }

  //------------------------------------------------------------------------------
  /** @brief Overloaded plus operand.
    @details Function add content of booth mapping variables together.
    @param var Mapping to be added.
  */
  public opPlus(const Mapping &var)
  {
    //
    mapping map = var.get();

    for (int i = 1; i <= mappinglen(map); i++)
    {
      const anytype key = mappingGetKey(map, i);
      anytype value = map[key];

      _var[key] = value;
    }
  }


//--------------------------------------------------------------------------------
//@private members
//--------------------------------------------------------------------------------
  mapping _var;
};
