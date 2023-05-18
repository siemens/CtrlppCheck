
//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2022 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//

//---------------------------------------------------------------------------------------------------------------------------------------
/**
  @brief Function returns list with all files recursive, relative to dir.
  @details Files are absolute and native pathes.

  @param dir Directory path
  @param pattern Pattern for filter. See also getFileNames()
  @param filter Filter. See also getFileNames()
  @return List with paths to filtered files.
*/
dyn_string getFileNamesRecursive(string dir, string pattern = "*", int filter = FILTER_FILES)
{
  if ((dir == "") || !isdir(dir))
    return makeDynString();

  dyn_string dirs     = getFileNames(dir, "*", FILTER_DIRS);
  dyn_string filtered = getFileNames(dir, pattern, filter);
  dyn_string result;

  int len = dynlen(filtered);

  for (int i = 1; i <= len; i++)
  {
    string subDir = filtered[i];

    if (!_isValidDirName(subDir))
      continue;

    dynAppend(result, makeNativePath(dir + "/" + subDir));
  }

  len = dynlen(dirs);

  for (int i = 1; i <= len; i++)
  {
    string subDir = dirs[i];

    if (!_isValidDirName(subDir))
      continue;

    dynAppend(result, getFileNamesRecursive(makeNativePath(dir +  "/" + subDir), pattern, filter));
  }

  string delims = makeNativePath("//");
  string delim = makeNativePath("/");

  for (int i = 1; i <= dynlen(result); i++)
  {
    strreplace(result[i], delims, delim);
  }

  dynUnique(result);
  dynSort(result);

  return result;
}

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

//---------------------------------------------------------------------------------------------------------------------------------------
private bool  _isValidDirName(const string name)
{
  if ((name == "..") || (name == "") || (name == "."))
    return FALSE;

  return TRUE;
}
