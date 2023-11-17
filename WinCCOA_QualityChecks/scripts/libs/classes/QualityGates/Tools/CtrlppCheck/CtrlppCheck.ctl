//--------------------------------------------------------------------------------
/**
  @file $relPath
  @copyright Copyright 2023 SIEMENS AG
             SPDX-License-Identifier: GPL-3.0-only
*/

#uses "panel"
#uses "CtrlXml"
#uses "CtrlPv2Admin"
#uses "classes/QualityGates/Tools/CtrlppCheck/CtrlppCheckError"
#uses "classes/QualityGates/Tools/CtrlppCheck/CtrlppCheckSettings"

//--------------------------------------------------------------------------------
class CtrlppCheck
{
//--------------------------------------------------------------------------------
//@public
//--------------------------------------------------------------------------------

  public CtrlppCheckSettings settings;

  public dyn_anytype errList;

  public int rc;
  public string stdErr;
  public string stdOut;



  //------------------------------------------------------------------------------
  public static synchronized string getExecutable()
  {
    if (!initialized)
    {
      paCfgReadValue(getPath(CONFIG_REL_PATH, "config"), "qualityChecks", "ctrlppcheckPath", path);

      if (path == "")
      {
        if (_UNIX)
          path = getPath(BIN_REL_PATH, "ctrlppcheck/ctrlppcheck");
        else if (_WIN32)
          path = getPath(BIN_REL_PATH, "ctrlppcheck/ctrlppcheck.exe");
      }

      initialized = TRUE;

      if (path != "")
        path = "\"" + path + "\"";
    }

    return path;
  }

  //------------------------------------------------------------------------------
  public int start(const string addOptions = "")
  {
    string cmd = getExecutable();

    if (cmd == "")
    {
      stdErr = __FUNCTION__ + " can not find executable";
      rc = -1;
      return rc;
    }

    cmd += " " + addOptions + " " + settings.toCmdLine();

    DebugFTN("ctrlppcheck",  __FUNCTION__, cmd);
    rc = system(cmd, stdOut, stdErr);
    DebugFN("ctrlppcheck_dtl",  __FUNCTION__, stdOut, stdErr);

    if (settings.verbose)
      DebugN(stdOut);

    return rc;
  }


  //------------------------------------------------------------------------------
  /// @obsolete
  // ctrlppcheck-suppress unusedFunction
  public int checkFiles(dyn_string pathes)
  {
    for (int i = 1; i <= dynlen(pathes); i++)
      checkFile(pathes[i]);

    return 0;
  }

  //------------------------------------------------------------------------------
  public int checkFile(const string &path)
  {
    start(path);
    stdErrToErrList();
  }

  //------------------------------------------------------------------------------
  /// used in panel to get list of all possible ctrlppcheck errors.
// ctrlppcheck-suppress unusedFunction
  public dyn_anytype getAllPossibleErrors()
  {
    string cmd = getExecutable();

    if (cmd == "")
    {
      stdErr = __FUNCTION__ + " can not find executable";
      return makeDynAnytype();
    }

    cmd += " --errorlist --winccoa-projectName=" + PROJ;

    DebugFTN("ctrlppcheck",  __FUNCTION__, cmd);
    rc = system(cmd, stdOut, stdErr);


    // make a copy of result in log
    file f = fopen(PROJ_PATH + LOG_REL_PATH + "cppcheck-all-errors.xml", "wb+");
    fputs(stdOut, f);
    fclose(f);

    strToErrList(stdOut);
    return errList;
  }


//--------------------------------------------------------------------------------
//@protected
//--------------------------------------------------------------------------------

  //------------------------------------------------------------------------------
  protected strToErrList(string &str)
  {
    dynClear(errList);
    string errMsg, errLine;
    int errColumn;
    docNum = xmlDocumentFromString(str, errMsg, errLine, errColumn);

    DebugFTN("ctrlppcheck", __FUNCTION__,
             "docNum", docNum,
             "errMsg", errMsg,
             "errLine", errLine,
             "errColumn", errColumn);

    if (docNum < 0)
    {
      DebugFTN("ctrlppcheck", __FUNCTION__, str);
      return;
    }

    xmlRec(xmlFirstChild(docNum));
    str = "";
  }

  //------------------------------------------------------------------------------
  protected stdErrToErrList()
  {
    if (!settings.isXmlOutEnabled())
      return;

    // make a copy of result in log
    file f = fopen(PROJ_PATH + LOG_REL_PATH + "cppcheck-result.xml", "wb+");
    fputs(stdErr, f);
    fclose(f);

    strToErrList(stdErr);
  }

//--------------------------------------------------------------------------------
//@private
//--------------------------------------------------------------------------------

  int docNum;
  dyn_anytype syntaxErrors;

  static bool initialized;
  static string path;

  /// Form Feed. See also https://en.wikipedia.org/wiki/Page_break
  const string FF = "\\012";

  //------------------------------------------------------------------------------
  void xmlRec(int node)
  {
    if (node < 0)
      return;

    if (xmlNodeType(docNum, node) == XML_ELEMENT_NODE)
    {
      string nodeName = xmlNodeName(docNum, node);

      if (nodeName == "errors")
      {
        dyn_uint nodes;
        xmlChildNodes(docNum, node, nodes);

        for (int i = 1; i <= dynlen(nodes); i++)
        {
          CtrlppCheckError err;
          xnmlNextErr(nodes[i], err);
          dynAppend(errList, err);
        }

        return;
      }
    }

    xmlRec(xmlFirstChild(docNum, node));
    xmlRec(xmlNextSibling(docNum, node));
  }

  //------------------------------------------------------------------------------
  void xnmlNextErr(uint node, CtrlppCheckError &err)
  {
    if ((xmlNodeType(docNum, node) == XML_ELEMENT_NODE) && (xmlNodeName(docNum, node) == "error"))
    {
      mapping map = xmlElementAttributes(docNum, node);
      err.id = map["id"];
      err.severity = map["severity"];
      err.msg = map["msg"];

      if (mappingHasKey(map, "verbose"))
      {
        string msg = map["verbose"];
        strreplace(msg, FF, "\n");
        err.verbose = msg;
      }

      if (mappingHasKey(map, "cwe"))
        err.cwe = map["cwe"];

      if (mappingHasKey(map, "knownBug"))
        err.knownBug = map["knownBug"];

      dyn_uint nodes;
      xmlChildNodes(docNum, node, nodes);

      for (int i = 1; i <= dynlen(nodes); i++)
      {
        if (!xmlErrLocation(nodes[i], err))
          break;
      }
    }
  }

  //------------------------------------------------------------------------------
  int xmlErrLocation(uint node, CtrlppCheckError &err)
  {
    if ((xmlNodeType(docNum, node) == XML_ELEMENT_NODE) && (xmlNodeName(docNum, node) == "location"))
    {
      mapping map = xmlElementAttributes(docNum, node);
      err.line = (int)map["line"];
      err.path = map["file"];

      if (mappingHasKey(map, "file0"))
        err.path0 = map["file0"];

      return 0;
    }

    return -1;
  }
};
