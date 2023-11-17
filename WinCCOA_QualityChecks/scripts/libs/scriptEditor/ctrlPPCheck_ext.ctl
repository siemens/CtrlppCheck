//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2023 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//

/**
  @file scripts/scriptEditor/ctrlPPCheck_ext.ctl
  @brief Extension for static tests in ctrl editor
  @details Extended static code check in ctrl editor
  + ctrlPP check
  + CCN
  + count of funcions
  + No. of lines

  */

//---------------------------------------------------------------------------------------------------------------------------------------
#uses "classes/QualityGates/Qg"
#uses "classes/QualityGates/QgStaticCheck/CtrlCode/ScriptData"
#uses "classes/QualityGates/Tools/CtrlppCheck/CtrlppCheck"
#uses "panel"

//---------------------------------------------------------------------------------------------------------------------------------------
int actionId;

//---------------------------------------------------------------------------------------------------------------------------------------
void makeScriptEditorToolbar()
{
  int tbID = moduleAddToolBar("CtrlppCheck");
  /// @todo translate label by msgCat
  actionId = moduleAddAction("CtrlppCheck", "", "", -1, tbID, "ctrlPPCheck");

  while (!dpExists("_CtrlCommandInterface_StaticTests"))
  {
    // ctrlppcheck-suppress badPerformanceInLoops // wait till created by update script
    delay(1);
  }

  dpConnect("update_cb", "_CtrlCommandInterface_StaticTests.Command");
}

//---------------------------------------------------------------------------------------------------------------------------------------
void ctrlPPCheck()
{

  // disable scriptEditor button
  moduleSetAction(actionId, "enabled", FALSE);

  string path; // script path;
  mapping map; // mapping with checked data

  // get current script path;
  if (isFunctionDefined("seGetFileName"))
    path = seGetFileName();

  // store path;
  map["path"] = path;

  bool tmpFileUsed = FALSE;

  if (path == "")   // panel or version < 3.16
  {
    /*
      When we want to check panel, we need to skip the script delimiter.
      The delimiter is ASCII-char 226 (something like --). This is of cores wrong code (parse problem)
      So it is fine when we commented the lines out. In that case can ctrlppcheck parse the code
      again. I want to delete the lines, but there are some helpfully informations, there
      can be used in ctrlppcheck in future.

      That means we need to read the code here now, and comment out all bad lines.
    */
    path = tmpnam() + ".ctl";
    file f = fopen(path, "wb+");

    string script = getScript();
    dyn_string lines = strsplit(script, "\n");

    for (int i = 1; i <= dynlen(lines) ; i++)
    {
      if (lines[i] == "")
        continue;

      char firstChar = lines[i][0];
      bool isDelim = firstChar == (char)226;

      if (isDelim)
        lines[i] = "//" + lines[i];

    }

    script = strjoin(lines, "\n");
    fputs(script, f);
    fclose(f);
    tmpFileUsed = TRUE;
  }

  {
    // in new scope to eliminate memory usage
    ScriptData script;
    script.setPath(path);
    script.calculate();
//     script.validate();
    mapping res;

    res["isCalculated"] = script.isCalculated();

    res["countOfFunctions"] = script.getCountOfFunctions();
    res["countOfFunctions.min"] = script.getMinCountOfFunctions();
    res["countOfFunctions.max"] = script.getMaxCountOfFunctions();

    res["countOfLines"] = script.getCountOfLines();
    res["countOfLines.avg"] = script.getAvgLines();

    res["CCN"] = script.getCCN();
    res["CCN.avg"] = script.getAvgCCN();
    res["CCN.avg.max"] = script.getMaxAvgCCN();

    res["NLOC"] = script.getNLOC();
    res["NLOC.max"] = script.getMaxNLOC();
    res["NLOC.min"] = script.getMinNLOC();
    res["NLOC.avg"] = script.getAvgNLOC();

    res["countOfParams.avg"] = script.getAvgParamCount();


    map["script"] = res;//script.result.toMap();
  }


  {
    // in new scope to eliminate memory usage
    CtrlppCheck ctrlPpCheck;

    dpGet("_CtrlppCheck.settings.enableLibCheck", ctrlPpCheck.settings.enableLibCheck,
          //"_CtrlppCheck.settings.enableHeadersCheck", ctrlPpCheck.settings.enableHeadersCheck,  // currently disabled
          "_CtrlppCheck.settings.inconclusive", ctrlPpCheck.settings.inconclusive,
          "_CtrlppCheck.settings.includeSubProjects", ctrlPpCheck.settings.includeSubProjects,
          "_CtrlppCheck.settings.verbose", ctrlPpCheck.settings.verbose,
          "_CtrlppCheck.settings.inlineSuppressions", ctrlPpCheck.settings.inlineSuppressions);

    // load configs
    ctrlPpCheck.settings.addLibraryFile(getPath(DATA_REL_PATH, "DevTools/Base/ctrl.xml")); // general
    ctrlPpCheck.settings.addLibraryFile(getPath(DATA_REL_PATH, "DevTools/Base/ctrl_" + VERSION + ".xml")); // version specific
    ctrlPpCheck.settings.addLibraryFile(getPath(DATA_REL_PATH, "ctrlPpCheck/cfg/__proj__.xml")); // proj specific

    // load rules
    ctrlPpCheck.settings.addRuleFile(getPath(DATA_REL_PATH, "ctrlPpCheck/rule/ctrl.xml")); // general
    ctrlPpCheck.settings.addRuleFile(getPath(DATA_REL_PATH, "ctrlPpCheck/rule/ctrl_" + VERSION + ".xml")); // version specific
    ctrlPpCheck.settings.addRuleFile(getPath(DATA_REL_PATH, "ctrlPpCheck/rule/__proj__.xml")); // proj specific

    ctrlPpCheck.settings.addEnabled("all");
    ctrlPpCheck.settings.enableXmlFormat(TRUE);
    ctrlPpCheck.checkFile(path);

    map["ctrlPpCheck"] = ctrlPpCheck.errList;
  }

  if (tmpFileUsed)
    remove(path); // remove temp file


  // open result panel
  showResult(map);


  // anable scriptEditor button
  moduleSetAction(actionId, "enabled", TRUE);
}

//---------------------------------------------------------------------------------------------------------------------------------------
/// @todo show result in dockModule, but this works since 3.17 only
/// @warning Here is a little hack.
///          The WinCC OA function restorePanel() does not work. So we close the just opened result module
///          and open it agin. It is terrible solution, but works.
/// @warning The panel functions needs active connection to event. Dont try it to start with -n option.
void showResult(const mapping &result)
{
  if (isModuleOpen("CtrlppCheck"))
    moduleOff("CtrlppCheck");

  while (isModuleOpen("CtrlppCheck"))
  {
    // ctrlppcheck-suppress badPerformanceInLoops
    delay(0, 20);
  }

  // open module and wait till is opened
  ModuleOnWithPanel("CtrlppCheck", -2, -2, 100, 200, 1, 1, "", "vision/scriptEditor/staticTests.pnl", "staticTests",
                    makeDynString());

  while (!isPanelOpen("staticTests", "CtrlppCheck"))
  {
    // ctrlppcheck-suppress badPerformanceInLoops
    delay(0, 100);
  }

  // send result to panel via ctrlCommandInterface
  delay(0, 100); // wait till is panel connected
  dpSet("_CtrlCommandInterface_StaticTests.Command", jsonEncode(result, TRUE));
}

//---------------------------------------------------------------------------------------------------------------------------------------
/// @todo make some identifier, that can be this action simple used for every lib
void update_cb(const string dpe, const string cmd)
{
  if (strpos(cmd, "line:") != 0)
    return;

  string line = substr(cmd, strlen("line:"));
  seSetCursorPos((int)line - 1, 0);
}


