//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2022 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//

//-----------------------------------------------------------------------------
// used libraries (#uses)
#uses "classes/ErrorHdl/OaLogger"
#uses "classes/QualityGates/QgStaticCheck/CtrlCode/ScriptsDir"
#uses "classes/QualityGates/Qg"
#uses "classes/QualityGates/QgBase"

//-----------------------------------------------------------------------------
// declare variables and constans

//-----------------------------------------------------------------------------
/** Static code checker for ctrl WinCC OA scripts / libs

  @author lschopp
*/
class QgStaticCheck_Scripts : QgBase
{
//-----------------------------------------------------------------------------
//@public members
//-----------------------------------------------------------------------------
  public string checkedPath = PROJ_PATH;

  //---------------------------------------------------------------------------
  public int setUp()
  {
    if ( QgBase::setUp() )
    {
      throwError(makeError("", PRIO_SEVERE, ERR_CONTROL, 0, "QgBase::setUp fails"));
      return -1;
    }

    if ( Qg::getId() == "QgStaticCheck_Scripts" )
    {
      _scriptsData.setDir(this.checkedPath + SCRIPTS_REL_PATH);
      _scriptsData.setType(ScriptsDataType::scripts);
    }
    else if ( Qg::getId() == "QgStaticCheck_Libs" )
    {
      _scriptsData.setDir(this.checkedPath + LIBS_REL_PATH);
      _scriptsData.setType(ScriptsDataType::libs);
    }

    if ( !_scriptsData.exists() )
      setMinValidScore(Qg::getId(), "assert.missingScripts", "reason.missingScripts");

    return 0;
  }

  //---------------------------------------------------------------------------
  public int calculate()
  {
    if ( _scriptsData.exists() )
      return _scriptsData.calculate();
    else
      return 0;
  }

  //---------------------------------------------------------------------------
  public int validate()
  {
    if ( (Qg::getId() == "QgStaticCheck_Scripts") && (_scriptsData.getCountOfFilesRecursive() <= 0) &&
          isdir(this.checkedPath + LIBS_REL_PATH) && (_scriptsData.getCountOfSubDirs() <= 0) )
    {
      // there are no scripts. Libs only and libs are checked in QgStaticCheck_Libs
      setMinValidScore("QgStaticCheck_Scripts", "assert.missingScripts", "reason.missingScripts");
      return 0;
    }

    return _scriptsData.validate();
  }

  //---------------------------------------------------------------------------
  public int tearDown()
  {
    _result = _scriptsData.result;
    return QgBase::tearDown();
  }

//-----------------------------------------------------------------------------
//@protected members
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//@private members
//-----------------------------------------------------------------------------
  ScriptsDir _scriptsData = ScriptsDir();
};

//-----------------------------------------------------------------------------
/**
  Main rutine to start QG-Static check of WinCC OA scripts, libs directories.
  @param testType Checks WinCC OA scripts or libs directory
  @param path Path to WinCCOA project to be checked. Per default this project.
*/
void main(string testType, string path = PROJ_PATH)
{
  if ( testType == "scripts" )
  {
    Qg::setId("QgStaticCheck_Scripts");
  }
  else if ( testType == "libs" )
  {
    Qg::setId("QgStaticCheck_Libs");
  }
  else
  {
    OaLogger logger;
    // 00051,Parameter incorrect
    logger.fatal(51, "testType: " + testType, "Allowed values are 'scripts', 'libs'");
    // defensive code, shall never happens
    exit(-1);
  }

  QgStaticCheck_Scripts qg = QgStaticCheck_Scripts();
  qg.checkedPath = path;
  exit(qg.start());
}
