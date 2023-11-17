//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2023 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//

#uses "classes/QualityGates/QgCtrlppCheck/QgCtrlppCheck"
#uses "classes/QualityGates/Tools/CtrlppCheck/CtrlppCheck"

/// useGediScope enable to use gedi scope. It is faster, but not so safe.
/// + faster execution
/// - gedi can stop working, because of fatal messages
/// - it can happend, that the engineering is not possible during of checks - cpu usage of process.
///
/// @todo make it configurable.
bool useGediScope = TRUE;

void makeGediToolbar()
{
  int action;
  int id;

//   id = moduleAddMenu(getCatStr("WinCCOA_gedi_ext", "tools") );
  id = moduleAddMenu(getCatStr("WinCCOA_gedi_ext", "tool_QualityGates"));
  moduleAddAction(getCatStr("WinCCOA_gedi_ext", "tool_QualityGates_QgStaticCheck_CtrlppCheck"), "", "", id, -1, "tool_QualityGates_QgStaticCheck_CtrlppCheck");

  // separator
  action = moduleAddAction("", "", "", id, -1, "");
  moduleSetAction(action, "separator", TRUE);

  moduleAddAction(getCatStr("WinCCOA_gedi_ext", "tool_QualityGates_QgStaticCheck_Panels"), "", "", id, -1, "tool_QualityGates_QgStaticCheck_Panels");
  moduleAddAction(getCatStr("WinCCOA_gedi_ext", "tool_QualityGates_QgStaticCheck_Scripts"), "", "", id, -1, "tool_QualityGates_QgStaticCheck_Scripts");
  moduleAddAction(getCatStr("WinCCOA_gedi_ext", "tool_QualityGates_QgStaticCheck_Libs"), "", "", id, -1, "tool_QualityGates_QgStaticCheck_Libs");
  moduleAddAction(getCatStr("WinCCOA_gedi_ext", "tool_QualityGates_QgStaticCheck_Pictures"), "", "", id, -1, "tool_QualityGates_QgStaticCheck_Pictures");
  moduleAddAction(getCatStr("WinCCOA_gedi_ext", "tool_QualityGates_QgOverloadedFilesCheck"), "", "", id, -1, "tool_QualityGates_QgOverloadedFilesCheck");

  // separator
  action = moduleAddAction("", "", "", id, -1, "");
  moduleSetAction(action, "separator", TRUE);

  moduleAddAction(getCatStr("WinCCOA_gedi_ext", "tool_QualityGates_QgSyntaxCheck"), "", "", id, -1, "tool_QualityGates_QgSyntaxCheck");

  // separator
  //   action = moduleAddAction("", "", "", id, -1, "");
  //   moduleSetAction(action, "separator", TRUE);

  //moduleAddAction(getCatStr("WinCCOA_gedi_ext", "tool_QualityGates_UnitTests"), "", "", id, -1, "tool_QualityGates_UnitTests");


  // separator
  action = moduleAddAction("", "", "", id, -1, "");
  moduleSetAction(action, "separator", TRUE);

  moduleAddAction(getCatStr("WinCCOA_gedi_ext", "open_Result"), "", "", id, -1, "tool_QualityGates_OpenResult");
  moduleAddAction(getCatStr("WinCCOA_gedi_ext", "open_Dock"), "", "", id, -1, "openDockModule");

  // separator
  action = moduleAddAction("", "", "", id, -1, "");
  moduleSetAction(action, "separator", TRUE);

  moduleAddAction(getCatStr("WinCCOA_gedi_ext", "open_Docu"), "", "", id, -1, "open_Docu");

  openDockModule();
}

private  int _startCtrlMan(string script, const string scriptOptions = "")
{
  string stdOut = "";
  string stdErr = "";
  string cmd;

  if (_WIN32)
    cmd = getPath(BIN_REL_PATH, getComponentName(CTRL_COMPONENT) + ".exe");
  else if (_UNIX)
    cmd = getPath(BIN_REL_PATH, getComponentName(CTRL_COMPONENT));
  else
  {
    stdErr = "this platform is not implemented";
    DebugFTN("qualityCheck_ext", __FUNCTION__, stdErr);
    return -1;
  }

  if (getPath(SCRIPTS_REL_PATH, script + ".ctl") != "")
    script += ".ctl";
  else if (getPath(SCRIPTS_REL_PATH, script + ".ctc") != "")
    script += ".ctc";
  else
    DebugFTN("qualityCheck_ext", "Sorry, the script " + script + " was not found.");

  cmd += " " + script + " " + scriptOptions + " -proj " + PROJ;// + " -log +stderr";

  int rc = system(cmd, stdOut, stdErr);

  if (rc)
  {
    DebugFTN("qualityCheck_ext", __FUNCTION__, "command exited with rc = " + rc, cmd, stdOut, stdErr);
    return rc;
  }

  return 0;
}

private tool_QualityGates_showBusy(const string &qgId)
{
  dpSet("_WinCCOA_qgCmd.Command", qgId + ":START");
}

void tool_QualityGates_QgStaticCheck_CtrlppCheck()
{
  tool_QualityGates_showBusy("QgCtrlppCheck");

  if (useGediScope)
    start_QgCtrlppCheck();
  else
    _startCtrlMan("QualityGates/StaticTests/QgCtrlppCheck");
}

void tool_QualityGates_QgStaticCheck_Panels()
{
  tool_QualityGates_showBusy("QgStaticCheck_Panels");
  _startCtrlMan("QualityGates/StaticTests/QgPanelsCheck");
}

void tool_QualityGates_QgStaticCheck_Scripts()
{
  tool_QualityGates_showBusy("QgStaticCheck_Scripts");
  _startCtrlMan("QualityGates/StaticTests/QgScriptsCheck", "scripts");
}

void tool_QualityGates_QgStaticCheck_Libs()
{
  tool_QualityGates_showBusy("QgStaticCheck_Libs");
  _startCtrlMan("QualityGates/StaticTests/QgScriptsCheck", "libs");
}

void tool_QualityGates_QgStaticCheck_Pictures()
{
  tool_QualityGates_showBusy("QgStaticCheck_Pictures");
  _startCtrlMan("QualityGates/StaticTests/QgPicturesCheck");
}

void tool_QualityGates_QgSyntaxCheck()
{
  tool_QualityGates_showBusy("QgSyntaxCheck");
  _startCtrlMan("QualityGates/BuildAddOn/QgSyntaxCheck");
}

void tool_QualityGates_QgOverloadedFilesCheck()
{
  tool_QualityGates_showBusy("QgStaticCheck_OverloadedFiles");
  _startCtrlMan("QualityGates/StaticTests/QgOverloadedFilesCheck");
}

void tool_QualityGates_UnitTests()
{
  tool_QualityGates_showBusy("UnitTests");
  _startCtrlMan("QualityGates/QgTestRunner");
}

void tool_QualityGates_OpenResult()
{
  ModuleOnWithPanel("QgResult-1", -1, -1, 400, 400, 1, 1, "Scale", "vision/QualityChecks/QG_Main.pnl", "QG Result Overview", makeDynString(""));
}

void open_Docu()
{
  openUrl("https://github.com/siemens/CtrlppCheck/blob/main/README.md");
}

void tool_QualityGates_BuildDocu()
{
  _startCtrlMan("QGatesMakeDocu");
}

void openDockModule()
{
  if (isModuleOpen("WinCCOA_QualityChecks"))
    moduleShow("WinCCOA_QualityChecks");
  else
    moduleAddDockModule("WinCCOA_QualityChecks", "vision/gedi/QualityChecks.pnl");
}
