//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2023 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//

#uses "classes/QualityGates/QgBase"
#uses "classes/QualityGates/QgStaticCheck/CtrlCode/ScriptFile"
#uses "classes/QualityGates/QgStaticCheck/StaticCodeDir"


enum ScriptsDataType
{
  none,
  scripts,
  libs
};

class ScriptsDir : StaticCodeDir
{
//--------------------------------------------------------------------------------
//@public members
//--------------------------------------------------------------------------------
  public ScriptsDir(const string dirPath = "")
  {
    setDir(dirPath);
  }


  public setType(const ScriptsDataType &type)
  {
    _type = type;

    if (_type == ScriptsDataType::scripts)
      setExcludePattern("libs");
  }

  public ScriptsDataType getType()
  {
    return _type;
  }

  //------------------------------------------------------------------------------
  public ScriptFile makeCheckFile(const string &fullPath)
  {
    ScriptFile p = ScriptFile(fullPath);
    return p;
  }

  public ScriptsDir makeCheckSubDir(const string &fullPath)
  {
    ScriptsDir dir = ScriptsDir(fullPath);
    return dir;
  }

  public int calculate()
  {
    if (this.getType() == ScriptsDataType::scripts)
    {
      if (strpos(makeUnixPath(getDirPath()), LIBS_REL_PATH) >= 0)
      {
        return 1;
      }
    }

    if (StaticCodeDir::calculate())
    {
      DebugFTN("ScriptsDir", __FUNCTION__, "Function StaticCodeDir::calculate return error");
      return -2;
    }

    return 0;
  }

//--------------------------------------------------------------------------------
//@private members
//--------------------------------------------------------------------------------
  private ScriptsDataType _type = ScriptsDataType::none;
};
