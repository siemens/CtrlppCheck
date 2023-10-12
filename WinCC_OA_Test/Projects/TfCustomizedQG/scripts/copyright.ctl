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
#uses "classes/file/File"

//--------------------------------------------------------------------------------
// Variables and Constants

//--------------------------------------------------------------------------------
/**
*/
main(string sourcePath)
{
  dyn_string files = getFileNamesRecursive(sourcePath, "*.ctl");
  dyn_string filesToCheck;

  for (int i = 1; i <= dynlen(files); i++)
  {
    const string path = makeUnixPath(files[i]);

    if (path.contains("suite_CtrlppCheck") && path.contains("testScripts"))
      continue; // here are located ctrlppcheck test-scripts. Changes will destroy bad cases

    dynAppend(filesToCheck, path);
  }

  uint now = year(getCurrentTime());
  const string correctCopyright = "Copyright " + now + " SIEMENS AG";
  const string oldCopyright = "Copyright " + (now - 1) + " SIEMENS AG";

  for (int i = 1; i <= dynlen(filesToCheck); i++)
  {
    const File ctrlFile = File(filesToCheck[i]);
    throwError(makeError("", PRIO_INFO, ERR_CONTROL, 0, "Check file", ctrlFile.getPath()));

    string result;

    if (ctrlFile.read(result))
      continue; // do not thrown error here, it is done in the function read() self

    bool changed = false;

    if (strreplace(result, oldCopyright, correctCopyright) > 0)
    {
      throwError(makeError("", PRIO_INFO, ERR_CONTROL, 0, "Old copyright changed", ctrlFile.getPath()));
      changed = true;
    }

    if (!result.contains(correctCopyright))
    {
      result = "//--------------------------------------------------------------------------------\n" +
               "/**\n  @file $relPath\n  @copyright " + correctCopyright +
               "\n             SPDX-License-Identifier: GPL-3.0-only\n*/\n\n" +
               result;
      throwError(makeError("", PRIO_INFO, ERR_CONTROL, 0, "New copyright added", ctrlFile.getPath()));
      changed = true;
    }

    if (changed && (ctrlFile.write(result) != 0))
      throwError(makeError("", PRIO_INFO, ERR_CONTROL, 0, "Cannot change file", ctrlFile.getPath()));
  }
}
