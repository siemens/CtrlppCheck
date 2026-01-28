
//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2026 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//


//---------------------------------------------------------------------------------------------------------------------------------------
dyn_string getSubProjPathes()
{
  dyn_string pathes;

  for (int i = 2; i < SEARCH_PATH_LEN; i++)
  {
    dynAppend(pathes, getPath("", "", -1, i));
  }

  return pathes;
}
