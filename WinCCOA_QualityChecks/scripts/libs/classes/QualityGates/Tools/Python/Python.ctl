//--------------------------------------------------------------------------------
/**
  @file $relPath
  @copyright Copyright 2023 SIEMENS AG
             SPDX-License-Identifier: GPL-3.0-only
*/

#uses "CtrlPv2Admin"

//--------------------------------------------------------------------------------
/**
  Interface for python.

  We need python for ToolLizard.
  @warning It is external dependency.
*/
class Python
{
//@public
  //------------------------------------------------------------------------------
  public Python()
  {
  }

  //------------------------------------------------------------------------------
  public static bool isInstalled()
  {
    string execPath = getExecutable();

    if ((execPath != "") && isfile(execPath))
      return TRUE;

    string stdOut;

    if (_WIN32)
    {
      system("assoc .py", stdOut);
      strreplace(stdOut, "\n", "");
      strreplace(stdOut, "\r", "");
      return (stdOut == ".py=Python.File");
    }
    else
    {
      system("python --version", stdOut);
      return (strpos(stdOut, "Python ") == 0);
    }
  }

  //------------------------------------------------------------------------------
  public static synchronized string getExecutable()
  {
    return findExecutable("python");
  }

//@private
  //------------------------------------------------------------------------------
  static bool initialized;
  static string path;
};
