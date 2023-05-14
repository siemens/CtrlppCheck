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
#uses "classes/QualityGates/QgStaticCheck/Panels/PanelsDir"
#uses "classes/QualityGates/Qg"
#uses "classes/QualityGates/QgBase"

//--------------------------------------------------------------------------------
// declare variables and constans


//--------------------------------------------------------------------------------
/**
  @brief QualityGate Check-panels
  */
class QgStaticPanelCheck : QgBase
{
//--------------------------------------------------------------------------------
//@public members
//--------------------------------------------------------------------------------
  public string checkedPath = PROJ_PATH;
  
  //------------------------------------------------------------------------------
  /** @brief Function setups panels tests.
    @details Creates temp-source dir.
    @return 0 when successfull, otherwise -1.
  */
  public int setUp()
  {
    if ( QgBase::setUp() )
      return -1;


    PanelCheck::setSourceDirPath(this.checkedPath);
    PanelFile::setSourceDirPath(this.checkedPath);
    _panels.setDir(this.checkedPath + PANELS_REL_PATH);
    
    if ( !_panels.exists() )
      setMinValidScore("QgStaticCheck_Panels", "assert.missingPanels", "reason.missingPanels");
    
    return 0;
  }

  //------------------------------------------------------------------------------
  /** @brief Calculates panels directory.
    @details Calculates panels directory recursive.
    @return 0 when successfull, otherwise -1.
  */
  public int calculate()
  {
    if ( _panels.exists() )
      return _panels.calculate();
    else
      return 0;
  }
  
  //------------------------------------------------------------------------------
  /** @brief Function validates calculated panels dircetory.
    @warning Call function calculate() before. Otherwise validation does not work.
    @return 0 when successfull, otherwise -1.
  */
  public int validate()
  {
    delay(1);
    return _panels.validate();
  }

  //------------------------------------------------------------------------------
  /** @brief Teardown of QualityGate
    @return 0 when successfull, otherwise -1.
  */
  public int tearDown()
  {
    _result = _panels.result;
    return QgBase::tearDown();
  }

//--------------------------------------------------------------------------------
//@private members
//--------------------------------------------------------------------------------
  
  PanelsDir _panels = PanelsDir(); //!< panels directory
};

//--------------------------------------------------------------------------------
/** 
  @breif main rutine to start QualityGate QgStaticCheck-panels
*/
void main(string path = PROJ_PATH)
{
  Qg::setId("QgStaticCheck_Panels");
  QgStaticPanelCheck qg = QgStaticPanelCheck();
  qg.checkedPath = path;
  exit(qg.start());
}

