//--------------------------------------------------------------------------------
/**
  @file $relPath
  @copyright Copyright 2023 SIEMENS AG
             SPDX-License-Identifier: GPL-3.0-only
*/


//-----------------------------------------------------------------------------
struct CppCheckError
{
  string id;
  string severity;
  string msg;
  string verbose;
  string path;
  string path0;
  int line;
  int cwe;
  string knownBug; // for internal use (tests)

  //---------------------------------------------------------------------------
  string toStdErrString()
  {
    string s =  "ID: " + id + "\n" +
                "Severity: " + severity + "\n";

    if (msg != "")
      s += "Msg: " + msg + "\n";

    if (verbose != "")
      s += "Verbose: " + verbose + "\n";

    if (path != "")
      s += "Path: " + path + "\n";


    s += "Line: " + line + "\n" +
         "CWE: " + cwe;

    return s;
  }

  //---------------------------------------------------------------------------
  public mapping toMapping()
  {
    return makeMapping(
      "id", id,
      "severity", severity,
      "msg", msg,
      "verbose", verbose,
      "path", path,
      "path0", path0,
      "line", line,
      "cwe", cwe
    );
  }
};
