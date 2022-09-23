//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2022 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//

#uses "classes/QualityGates/QgApp"


class QgAddOnSourceDir
{
  public QgAddOnSourceDir()
  {
    QgApp app  = QgApp::getAppFromProjName(PROJ);
    _dirPath = makeNativePath(app.getSourcePath() + "appData/" + app.getExtendedTechnicalName() + "/");
  }
  
  public string getDirPath()
  {
    return makeNativePath(_dirPath);
  }
  
  public bool exists()
  {
    return (_dirPath != "" && isdir(_dirPath));
  }
    
  string _dirPath;
};
