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


enum QgAddOnResultErrPrio
{
  Info,
  Warning,
  Error
};

enum QgAddOnResultErrCode
{
  Unknown,
  DoesNotRunSuccessfull,
  CanNotCalculateScript,
  CanNotCalculatePanel
};

//--------------------------------------------------------------------------------
/*!
 * @brief Implemented error handling function missing in WinCC OA
 *
 * @author lschopp
 */
class QgAddOnResultErr
{
//--------------------------------------------------------------------------------
//@public members
//--------------------------------------------------------------------------------

  //------------------------------------------------------------------------------
  /**
   * @brief Default c-tor
   *
   * @return intialized object of class QgAddOnResultErr
   */
  public QgAddOnResultErr(int prio, QgAddOnResultErrCode errCode, const string node)
  {
    _err = makeError(_MSG_CAT, prio, ERR_CONTROL, (int)errCode, node);
  }

  //------------------------------------------------------------------------------
  /** @brief Function returns err text.
   * @return Error text.
   */
  public string getText()
  {
    return getErrorText(_err);
  }
  
  public int getPriority()
  {
    return getErrorPriority(_err);
  }
  
  public string getPriorityAsText()
  {
    switch(getPriority())
    {
      case PRIO_INFO:
      return "Info";
      case PRIO_WARNING:
      return "Warning";
      case PRIO_SEVERE:
      return "Error";
      default:
      return "Unkwon";
    }
  }
  
  public void fillMap(mapping &map)
  {
    map[getPriorityAsText()] = getText();
  }
  
  public mapping toMap()
  {
    mapping map;
    fillMap(map);
    return map;
  }
  
  

//--------------------------------------------------------------------------------
//@private members
//--------------------------------------------------------------------------------
  errClass _err;
  static const string _MSG_CAT = "QgAddOnResultErr";
};
