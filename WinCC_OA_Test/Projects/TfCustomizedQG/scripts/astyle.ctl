// $License: NOLICENSE
//--------------------------------------------------------------------------------
/**
  @file $relPath
  @copyright Copyright 2023 SIEMENS AG
             SPDX-License-Identifier: GPL-3.0-only
  @author mPokorny
*/

//--------------------------------------------------------------------------------
// Libraries used (#uses)

//--------------------------------------------------------------------------------
// Variables and Constants

//--------------------------------------------------------------------------------
/**
*/
main(string sourcePath)
{
  dyn_string args;

  args[1] = getPath(BIN_REL_PATH, _WIN32 ? "astyle.exe" : "astyle");
  args[2] = "--options=" + makeNativePath(getPath(CONFIG_REL_PATH, "astyle.config"));

  dyn_string files = getFileNamesRecursive(sourcePath, "*.ctl");
  dyn_string filesToCheck;

  for (int i = 1; i <= dynlen(files); i++)
  {
    const string path = makeUnixPath(files[i]);

    if (path.contains("suite_CtrlppCheck") && path.contains("testScripts"))
      continue; // here are located ctrlppcheck test-scripts. Formatting will destroy bad cases

    dynAppend(filesToCheck, makeNativePath(path));
  }

  dynAppend(args, filesToCheck);

  string stdErr, stdOut;

  DebugTN("Start", args);
  int rc = system(args, stdOut, stdErr);

  strreplace(stdOut, "\r", "");
  DebugTN("fine", rc, stdErr, strsplit(stdOut, "\n"));

  // remove the backups
  for (int i = 1; i <= dynlen(files); i++)
  {
    if (isfile(files[i] + ".orig"))
      remove(files[i] + ".orig");
  }
}
