//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2023 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//

#uses "classes/ErrorHdl/OaLogger"
#uses "classes/FileSys/QgDir"
#uses "classes/QualityGates/QgBase"
#uses "classes/QualityGates/QgResult"
#uses "classes/QualityGates/QgSettings"

class StaticDir : QgDir
{

  //---------------------------------------------------------------------------
  public setDir(string dirPath)
  {
    dynClear(_files);
    dynClear(_childs);

    dirPath = makeUnixPath(dirPath);

    if (!dirPath.isEmpty() && !dirPath.endsWith("/"))
      dirPath += "/"; // ensure trailing path delimiter

    QgDir::setDirPath(makeNativePath(dirPath));
  }

  //---------------------------------------------------------------------------
  /** @brief Function calculates statistic data from panels, scripts, libs dirs.
    @details It works for oa panels, scripts, libs directories.

    @warning Empty files or directories are ignored for average values.
    @warning Not calculated files or directories are ignored for average values.

    @return Error code.
     value | description
     ------|------------
     0     | Success
     -1    | Internal error. Directory does not exists.

  */
  public int calculate()
  {
    OaLogger logger;
    logger.info(0, Qg::getId(), "Check directory", getDirPath());

    dynClear(_files);
    dynClear(_childs);
    _allFilesCount = 0;

    if (!exists())
    {
      logger.warning(0, Qg::getId(), __FUNCTION__, "Directory does not exist: " + getDirPath());
      return -1;
    }

    float count = 0;

    // check all files
    dyn_string fileNames = getFileNames(getDirPath());

    for (int i = 1; i <= dynlen(fileNames); i++)
    {
      const string fullPath = makeNativePath(getDirPath() + fileNames[i]);
      logger.info(0, Qg::getId(), "Check file", fullPath);
      anytype checkFile = makeCheckFile(fullPath);

      _allFilesCount++;

      if (checkFile.calculate())
      {
        continue;
      }

      dynAppend(_files, checkFile);
    }

    // check all directories
    dyn_string childs = getSubDirNames();

    for (int i = 1; i <= dynlen(childs); i++)
    {
      const string subDirPath = makeNativePath(getDirPath() + childs[i] + "/");
      anytype child = makeCheckSubDir(subDirPath);

      if (child.calculate())
        continue; // only for safety (should never occur)

      _allFilesCount += child.getCountOfFilesRecursive();

      dynAppend(_childs, child);
    }

    return 0;
  }

  //---------------------------------------------------------------------------
  public int validate()
  {
    const int filesCount  = getCountOfFiles();
    const int subDirCount = getCountOfSubDirs();
    const bool isEmpty = (filesCount + subDirCount) == 0;

    const mapping dollars = makeMapping("dir.name", getName(),
                                        "dir.filesCountRecursive", getCountOfFilesRecursive(),
                                        "dir.isEmpty", isEmpty,
                                        "dir.subDirCount", subDirCount,
                                        "dir.filesCount", filesCount
                                       );

    result = new QgResult("QgStaticCheck_StaticDir", "directory", dollars);

    {
      shared_ptr<QgSettings> settings = new QgSettings(getSettingsRoot() + ".dir.isEmpty");

      if (settings.isEnabled())
      {
        shared_ptr <QgResult> assertion = new QgResult("QgStaticCheck_StaticDir", "dir.isEmpty", dollars);

        if (!assertion.assertFalse(isEmpty, settings.getScorePoints()))
        {
          result.addChild(assertion);
          return 1;
        }

        result.addChild(assertion);
      }
    }

    {
      shared_ptr<QgSettings> settings = new QgSettings(getSettingsRoot() + ".dir.hasFilesRecursive");

      if (settings.isEnabled())
      {
        shared_ptr <QgResult> assertion = new QgResult("QgStaticCheck_StaticDir", "dir.hasFilesRecursive", dollars);

        if (!assertion.assertGreatherEqual(getCountOfFilesRecursive(),
                                           settings.getLowLimit(DEFAULT_FILESREC_LOW),
                                           settings.getScorePoints()))
        {
          result.addChild(assertion);
          return 1;
        }

        result.addChild(assertion);
      }
    }

    {
      shared_ptr<QgSettings> settings = new QgSettings(getSettingsRoot() + ".dir.subDirCount");

      if (settings.isEnabled())
      {
        shared_ptr <QgResult> assertion = new QgResult("QgStaticCheck_StaticDir", "dir.subDirCount", dollars);
        assertion.assertLessEqual(subDirCount,
                                  settings.getHighLimit(DEFAULT_SUBDIRCOUNT_HIGH),
                                  settings.getScorePoints());
        result.addChild(assertion);
      }
    }


    {
      shared_ptr<QgSettings> settings = new QgSettings(getSettingsRoot() + ".dir.filesCount");

      if (settings.isEnabled())
      {
        shared_ptr <QgResult> assertion = new QgResult("QgStaticCheck_StaticDir", "dir.filesCount", dollars);
        DebugTN(__FUNCTION__, filesCount,
                                  settings.getHighLimit(DEFAULT_FILESCOUNT_HIGH),
                                  settings.getScorePoints(), dollars);
        assertion.assertLessEqual(filesCount,
                                  settings.getHighLimit(DEFAULT_FILESCOUNT_HIGH),
                                  settings.getScorePoints());
        result.addChild(assertion);
      }
    }

    return 0;
  }

  //---------------------------------------------------------------------------
  public int validateSubDirs()
  {
    if (dynlen(_childs) > 0)
    {
      const mapping dollars = makeMapping("dir.name", getName());
      shared_ptr <QgResult> subDirs = new QgResult("QgStaticCheck_StaticDir", "subDirsList", dollars);

      while (dynlen(_childs) > 0)
      {
        _childs[1].validate();
        subDirs.addChild(_childs[1].result);
        dynRemove(_childs, 1);
      }

      result.addChild(subDirs);
    }

    return 0;
  }

  //---------------------------------------------------------------------------
  public int validateFiles()
  {
    if (dynlen(_files) > 0)
    {
      const mapping dollars = makeMapping("dir.name", getName());
      shared_ptr <QgResult> files = new QgResult("QgStaticCheck_StaticDir", "filesList", dollars);

      while (dynlen(_files) > 0)
      {
        _files[1].validate();
        files.addChild(_files[1].result);
        dynRemove(_files, 1);
      }

      result.addChild(files);
    }

    return 0;
  }

  //---------------------------------------------------------------------------
  public dyn_anytype getSubDirs()
  {
    return _childs;
  }

  //---------------------------------------------------------------------------
  public dyn_anytype getFiles()
  {
    return _files;
  }

  //---------------------------------------------------------------------------
  public int getCountOfFiles()
  {
    return dynlen(_files);
  }

  //---------------------------------------------------------------------------
  public int getCountOfFilesRecursive()
  {
    return _allFilesCount;
  }

  //---------------------------------------------------------------------------
  public int getCountOfSubDirs()
  {
    return dynlen(_childs);
  }

  //---------------------------------------------------------------------------
  public void clear()
  {
    dynClear(_files);
    dynClear(_childs);
//     result = nullptr;
  }

  //---------------------------------------------------------------------------
  public string getSettingsRoot()
  {
    return "StaticDir";
  }

  //---------------------------------------------------------------------------
  public shared_ptr<QgResult> result;

//-----------------------------------------------------------------------------
//@protected members
//-----------------------------------------------------------------------------

  protected dyn_anytype _files;
  protected dyn_anytype _childs;

  protected int _allFilesCount;
  const int DEFAULT_FILESREC_LOW     = 1;
  const int DEFAULT_SUBDIRCOUNT_HIGH = 5;
  const int DEFAULT_FILESCOUNT_HIGH  = 10;
};
