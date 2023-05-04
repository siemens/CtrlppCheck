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
#uses "classes/QualityGates/QgStaticCheck/CtrlCode/ScriptsDir"
#uses "classes/QualityGates/Qg"
#uses "classes/QualityGates/QgBase"

//--------------------------------------------------------------------------------
// declare variables and constans

//--------------------------------------------------------------------------------
/*!
  @brief Static code checker for ctrl scripts / libs

  @author lschopp
*/
class QgStaticCheck_Scripts : QgBase
{
  public string checkedPath = PROJ_PATH;
  public int setUp()
  {
    if ( QgBase::setUp() )
    {
      throwError(makeError("", PRIO_SEVERE, ERR_CONTROL, 0, "QgBase::setUp fails"));
      return -1;
    }
    
    if ( Qg::getId() == "QgStaticCheck_Scripts" )
    {
      throwError(makeError("", PRIO_INFO, ERR_CONTROL, 0, Qg::getId() + " will check " + this.checkedPath + SCRIPTS_REL_PATH));
      _scriptsData.setDir(this.checkedPath + SCRIPTS_REL_PATH);
      _scriptsData.setType(ScriptsDataType::scripts);
    }
    else if ( Qg::getId() == "QgStaticCheck_Libs" )
    {
      throwError(makeError("", PRIO_INFO, ERR_CONTROL, 0, Qg::getId() + " will check " + this.checkedPath + LIBS_REL_PATH));
      _scriptsData.setDir(this.checkedPath + LIBS_REL_PATH);
      _scriptsData.setType(ScriptsDataType::libs);
    }

    if ( !_scriptsData.exists() )
      setMinValidScore(Qg::getId(), "assert.missingScripts", "reason.missingScripts");
    
    return 0;
  }

  public int calculate()
  {
    if ( _scriptsData.exists() )
      return _scriptsData.calculate();
    else
      return 0;
  }
  
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
  

  public int tearDown()
  {
    _result = _scriptsData.result;
    return QgBase::tearDown();
  }
  
  ScriptsDir _scriptsData = ScriptsDir();
};

//---------------------------------------------------------------------------------------------------------------------------------------
/** 
  Main rutine to start QG-Static check of scripts
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
    DebugN("Unknown testType", testType);
    exit(-1);
  }
  
  QgStaticCheck_Scripts qg = QgStaticCheck_Scripts();
  qg.checkedPath = path;
  exit(qg.start());
}
