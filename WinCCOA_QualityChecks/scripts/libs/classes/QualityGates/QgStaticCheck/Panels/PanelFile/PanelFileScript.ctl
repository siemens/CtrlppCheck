//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2023 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//

#uses "classes/QualityGates/QgStaticCheck/CtrlCode/QgCtrlCodeScriptData"

class PanelFileScript : QgCtrlCodeScriptData
{
  public PanelFileScript(const string name = "")
  {
    _name = name;
  }

  public void setScript(const string &script)
  {
    _script = script;
    strreplace(_script, "&quot;", "\"");
    strreplace(_script, "&amp;", "&");
  }

  public string getName()
  {
    return _name;
  }

  public string getScript()
  {
    return _script;
  }

  public int getMaxCountOfFunctions()
  {
    if ("ScopeLib" == _name)
      return 30;

    return 5;
  }

  public int getMinCountOfFunctions()
  {
    if ("ScopeLib" == _name)
      return 0; // scope is allowed to be empty

    return 1;
  }

  public int getMinNLOC()
  {
    if ("ScopeLib" == _name)
      return 0; // scope is allowed to be empty

    return 4;
  }


  public float getMaxAvgCCN()
  {
    return 10;
  }

  public int calculate()
  {
    _filePath = tmpnam() + ".ctl";

    file f = fopen(_filePath, "wb+");
    fputs(getScript(), f);
    fclose(f);

    if (QgCtrlCodeScriptData::calculate())
      return -1;

    remove(_filePath);
    return 0;
  }


  string _name;
  string _script;

};
