//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2022 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//

#uses "classes/QualityGates/QgBase"
#uses "classes/QualityGates/QgStaticCheck/StaticDir"
#uses "classes/QualityGates/QgSettings"

class StaticCodeDir :StaticDir
{
  public StaticCodeDir(const string dirPath = "")
  {
    setDir(dirPath);
  }

  public anytype makeChekFile(const string &fullPath)
  {
    return NULL;
  }

  public anytype makeCheckSubDir(const string &fullPath)
  {
    return NULL;
  }

  //------------------------------------------------------------------------------
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

    dynClear(_files);
    dynClear(_childs);

    if ( !exists() )
      return -1;

    float count = 0;

    // check all files
    dyn_string fileNames = getFileNames(getDirPath());
    for(int i = 1; i <= dynlen(fileNames); i++)
    {
      string fullPath = makeNativePath(getDirPath() + fileNames[i]);

      anytype checkFile = makeChekFile(fullPath);

      _allFilesCount++;

      checkFile.calculate();

      if ( checkFile.isCalculated() )
      {
        // it is not possible to calculate file (non panel/script file, crypted ...), so dont updatet the statistic data.
        _ccn  += checkFile.getCCN();
        _nloc += checkFile.getNLOC();

        _avgCcn  += checkFile.getAvgCCN();
        _avgNloc += checkFile.getAvgNLOC();
        if ( checkFile.getAvgCCN() > 0.99 )
        {
          // average CCN can no be < 1. It looks like empty file
          count ++;
        }
      }

      dynAppend(_files, checkFile);
    }

    // check all directories
    dyn_string childs = getSubDirNames();
    for(int i = 1; i <= dynlen(childs); i++)
    {
      anytype child = makeCheckSubDir(getDirPath() + childs[i] + "/");
      child.calculate();

      _nloc += child.getNLOC();
      _ccn  += child.getCCN();
      _allFilesCount += child.getCountOfFilesRecursive();

      _avgCcn  += child.getAvgCCN();
      _avgNloc += child.getAvgNLOC();
      if ( child.getAvgCCN() > 0.99 )
      {
        // average CCN can no be < 1. It looks like empty dir
        count ++;
      }

      dynAppend(_childs, child);
    }

    // average per dir ??? not sure if is correct so
    if ( count > 0 )
    {
      _avgCcn = _avgCcn / count;
      _avgNloc = _avgNloc / count;
    }

    return 0;
  }


  public int getCCN()
  {
    return _ccn;
  }

  public float getAvgCCN()
  {
    return _avgCcn;
  }

  public int getNLOC()
  {
    return _nloc;
  }

  public float getAvgNLOC()
  {
    return _avgNloc;
  }


  public int validate()
  {
    int rc = StaticDir::validate();
    if ( rc == 0 )
    {
      {
        shared_ptr<QgSettings> settings = new QgSettings("StaticCodeDir.dir.NLOC");

        if ( settings.isEnabled() )
        {
          shared_ptr <QgVersionResult> assertion = new QgVersionResult();
          assertion.setMsgCatName("QgStaticCheck_StaticCodeDir");

          // NLOC in dir
          assertion.setAssertionText("assert.dir.NLOC");
          assertion.setReasonText("reason.dir.NLOC", makeMapping("dir.name", getName(),
                                                                 "dir.NLOC", getNLOC()));
          assertion.info(getNLOC(), settings.getScorePoints());
          result.addChild(assertion);
        }
      }
      // average NLOC in dir
      if ( getAvgNLOC() > 0 )
      {

        shared_ptr<QgSettings> settings = new QgSettings("StaticCodeDir.dir.avgNLOC");

        if ( settings.isEnabled() )
        {
          shared_ptr <QgVersionResult> assertion = new QgVersionResult();
          assertion.setMsgCatName("QgStaticCheck_StaticCodeDir");

          assertion.setAssertionText("assert.dir.avgNLOC");
          assertion.setReasonText("reason.dir.avgNLOC", makeMapping("dir.name", getName(),
                                                                    "dir.avgNLOC", getAvgNLOC()));
          assertion.info(getAvgNLOC(), settings.getScorePoints());
          result.addChild(assertion);
        }
      }

      // CCN in dir
      {
        shared_ptr<QgSettings> settings = new QgSettings("StaticCodeDir.dir.CC");

        if ( settings.isEnabled() )
        {
          shared_ptr <QgVersionResult> assertion = new QgVersionResult();
          assertion.setMsgCatName("QgStaticCheck_StaticCodeDir");

          assertion.setAssertionText("assert.dir.CCN");
          assertion.setReasonText("reason.dir.CCN", makeMapping("dir.name", getName(),
                                                                "dir.CCN", getCCN()));
          assertion.info(getCCN(), settings.getScorePoints());
          result.addChild(assertion);
        }
      }
      // average CCN in dir
      if ( getAvgCCN() > 0 )
      {
        shared_ptr<QgSettings> settings = new QgSettings("StaticCodeDir.dir.avgCCN");

        if ( settings.isEnabled() )
        {
          shared_ptr <QgVersionResult> assertion = new QgVersionResult();
          assertion.setMsgCatName("QgStaticCheck_StaticCodeDir");

          assertion.setAssertionText("assert.dir.avgCCN");
          assertion.setReasonText("reason.dir.avgCCN", makeMapping("dir.name", getName(),
                                                                   "dir.CCN", getAvgCCN()));
          assertion.info(getAvgCCN(), settings.getScorePoints());
          result.addChild(assertion);
        }
      }

    }

    // validate subdirs and files
    StaticDir::validateSubDirs();
    StaticDir::validateFiles();

    return 0;
  }

//--------------------------------------------------------------------------------
//@private members
//--------------------------------------------------------------------------------

  protected int _nloc, _ccn;
  protected float _avgCcn, _avgNloc;
};
