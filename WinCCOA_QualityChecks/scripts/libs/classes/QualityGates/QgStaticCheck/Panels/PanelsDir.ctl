//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2022 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//

#uses "classes/QualityGates/QgBase"
#uses "classes/QualityGates/QgStaticCheck/Panels/PanelCheck"
#uses "classes/QualityGates/QgStaticCheck/StaticCodeDir"


class PanelsDir : StaticCodeDir
{
//--------------------------------------------------------------------------------
//@public members
//--------------------------------------------------------------------------------
  public PanelsDir(const string dirPath = "")
  {
    setDir(dirPath);
  }
    
  //------------------------------------------------------------------------------
  public PanelCheck makeChekFile(const string &fullPath)
  {
    PanelCheck p = PanelCheck(fullPath);
    return p;
  }

  //------------------------------------------------------------------------------
  public static PanelsDir makeCheckSubDir(const string &fullPath)
  {
    PanelsDir dir = PanelsDir(fullPath);
    return dir;
  }
  
  
//--------------------------------------------------------------------------------
//@private members
//--------------------------------------------------------------------------------
};
