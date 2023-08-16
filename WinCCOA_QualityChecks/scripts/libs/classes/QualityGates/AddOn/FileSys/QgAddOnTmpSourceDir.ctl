
//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2022 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//

#uses "classes/QualityGates/QgApp"
#uses "classes/QualityGates/AddOn/FileSys/QgAddOnSourceDir"

class QgAddOnTmpSourceDir
{
  public QgAddOnTmpSourceDir()
  {
    QgApp app  = QgApp::getAppFromProjName(PROJ);
    _tmpSourceDir = makeNativePath(app.getSourcePath() + "tmpSource/");
  }

  public bool exists()
  {
    return (_tmpSourceDir != "" && isdir(_tmpSourceDir));
  }

  public int create()
  {
    if (exists())
      cleanUp();

    QgAddOnSourceDir source = QgAddOnSourceDir();

    if (!source.exists())
    {
      DebugFTN("QgAddOnTmpSourceDir", "Could not find source packet", source.getDirPath());
      return -1;
    }

    if (!isdir(_tmpSourceDir))
      mkdir(_tmpSourceDir);

    copyAllFilesRecursive(source.getDirPath(), _tmpSourceDir);

    if (!exists())
      return -2;

    return 0;
  }

  public int cleanUp()
  {
    if (exists())
      return rmdir(_tmpSourceDir, TRUE);

    return 0;
  }

  public string getDirPath()
  {
    return makeNativePath(_tmpSourceDir);
  }

  public string trimPath(const string &fullPath)
  {
    return substr(fullPath, strlen(getDirPath()));
  }

  string _tmpSourceDir;
};
