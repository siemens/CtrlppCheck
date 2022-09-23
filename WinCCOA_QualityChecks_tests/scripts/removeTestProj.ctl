//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2022 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//

#uses "CtrlPv2Admin"

main(string proj)
{  
  dyn_string ds, dsVer, dsPath;
  paGetProjs(ds, dsVer, dsPath);
  for(int i = 1; i <= dynlen(dsPath); i++)
  {
    string projName = baseName(dsPath[i]);
    
    if ( projName == proj )
      paDelProj(dsPath[i], FALSE); // deregister given project
    if ( !isdir(dsPath[i]) )
      paDelProj(dsPath[i], FALSE); // deregister not exisitng projects
  }
}
