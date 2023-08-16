//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2022 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//

//--------------------------------------------------------------------------------
// used libraries (#uses)

//--------------------------------------------------------------------------------
// declare variables and constans

//--------------------------------------------------------------------------------
/*! @brief Handler with directories.
 *
 * @details Class to handle with directories.
 * @author lschopp
 */
class QgDir
{
//--------------------------------------------------------------------------------
//@public members
//--------------------------------------------------------------------------------
  //------------------------------------------------------------------------------
  /** @brief Default c-tor.
    @param dirPath Full path to directory.
  */
  public QgDir(string dirPath = "")
  {
    setDirPath(dirPath);
  }

  //------------------------------------------------------------------------------
  /** @brief Function set the directory path
    @param dirPath Full path to directory.
  */
  public void setDirPath(const string &dirPath)
  {
    _dirPath = dirPath;
  }

  //------------------------------------------------------------------------------
  /** @brief Function returns full native path to directory.
    @return Full native path to directory.
    @exception Empty string.
  */
  public string getDirPath()
  {
    return makeNativePath(_dirPath);
  }

  public string getName()
  {
    return baseName(_dirPath);
  }


  //------------------------------------------------------------------------------
  /** @brief Function creates new directory.
    @return Error code. Returns 0 when successfull created, otherwise -1.
  */
  public int mk()
  {
    if (exists())
      return 0;

    const string delim = makeNativePath("/");
    dyn_string items = strsplit(makeNativePath(_dirPath), delim);

    string dirPath = "";

    for (int i = 1; i <= dynlen(items); i++)
    {
      dirPath += items[i];

      if (!isdir(dirPath))
        mkdir(dirPath);

      dirPath += delim;
    }

    if (!exists())
      return -1;

    return 0;
  }

  //------------------------------------------------------------------------------
  /** @brief Function checks if directory exists.
    @return Returns TRUE when directory exists, otherwise FALSE.
    @warning Empty paht ("") is not acceptable.
  */
  public bool exists()
  {
    return (_dirPath != "" && isdir(_dirPath));
  }

  //------------------------------------------------------------------------------
  /** @brief Function removes directory.
    @details Function remove direcotry recursive.
    @return Error code. Returns 0 when successfull removed, otherwise -1.
    @warning Empty paht ("") is not acceptable.
  */
  public int rm()
  {
    if (_dirPath == "")
      return -1;

    if (rmdir(_dirPath, TRUE))
      return 0;

    return -1;
  }

  //------------------------------------------------------------------------------
  public dyn_string getSubDirNames()
  {
    if (!exists())
      return makeDynString();

    dyn_string dirs = getFileNames(getDirPath(), "*", FILTER_DIRS);

    for (int i = dynlen(dirs); i > 0; i--)
    {
      const string dir = dirs[i];
      const string exPattern = _excludePattern;

      if ((dir == "") || (dir == ".") ||
          (dir == "..") || patternMatch(exPattern, dir))
      {
        dynRemove(dirs, i);
      }
    }

    return dirs;
  }

  public void setExcludePattern(const string &pattern)
  {
    _excludePattern = pattern;
  }

//--------------------------------------------------------------------------------
//@private members
//--------------------------------------------------------------------------------
  string _dirPath;
  string _excludePattern;
};
