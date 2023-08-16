//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2022 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//


#uses "classes/FileSys/QgDir"
#uses "classes/QualityGates/AddOn/FileSys/QgAddOnSourceDir"
#uses "classes/QualityGates/Qg"
#uses "classes/QualityGates/QgApp"
#uses "classes/QualityGates/QgTest"

class QgAddOnResultsDir
{
  public QgAddOnResultsDir()
  {
    if (_buildNo <= 0)
      _buildNo = (long)getCurrentTime();

    if (Qg::getId() != "")
      setQgId(Qg::getId());
  }

  public void setQgId(const string &qgId)
  {
    _qgId = qgId;
  }

  public string getLastDirPath()
  {
    dyn_string histDirs = getHistoryDirs();

    if (dynlen(histDirs) > 0)
      return histDirs[1];
    else
      return "";
  }

  public static dyn_string getRunningQgs()
  {
    QgDir dir = QgDir(PROJ_PATH + DATA_REL_PATH + "QualityGates/");
    dyn_string subdirs = dir.getSubDirNames();

    for (int i = dynlen(subdirs); i >= 1; i--)
    {
      //settings folder must be ignored,
      //otherwise a tab will be created in the result panel
      if (subdirs[i] == "settings")
      {
        dynRemove(subdirs, i);
      }
    }

    return subdirs;
  }

  public dyn_string getHistoryDirs()
  {
    const string qgResDir = PROJ_PATH + DATA_REL_PATH + "QualityGates/" + _qgId + "/";
    QgDir dir = QgDir(qgResDir);
    dyn_string histDirs = dir.getSubDirNames();
    dynSort(histDirs, FALSE);

    for (int i = 1; i <= dynlen(histDirs); i++)
    {
      histDirs[i] = makeNativePath(qgResDir + histDirs[i] + "/");
    }

    return histDirs;
  }

  public bool exists()
  {
    return isdir(getDirPath());
  }

  public int create()
  {
    if (exists())
      cleanUp();

    QgDir dir = QgDir(getDirPath());

    if (dir.mk())
      return -2;

    return 0;
  }

  public int cleanUp()
  {
    if (exists())
      return rmdir(getDirPath(), TRUE);

    return 0;
  }

  public string getDirPath()
  {
    if (_resultDir == "")
    {
      if (!QgTest::isStartedByTestFramework())
      {
        // When you start some locale tests, proj path will be used
        _resultDir = makeNativePath(PROJ_PATH + DATA_REL_PATH + "QualityGates/" + _qgId + "/" + _buildNo + "/");
      }
      else
      {
        /// FIXME (mpokorny) currently I does not see any reason for this code.
        /// probably can be remove
        QgApp app  = QgApp::getAppFromProjName(PROJ);
        _resultDir = makeNativePath(app.getSourcePath() + "QgResult/" + Qg::getId() + "/");
      }
    }

    return _resultDir;
  }

  long _buildNo;
  string _resultDir;
  string _qgId;
};
