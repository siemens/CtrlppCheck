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
#uses "classes/QualityGates/Qg"
#uses "classes/QualityGates/QgBase"
#uses "classes/QualityGates/QgOverloadedFilesCheck/QgOverloadedFilesCheck"

//--------------------------------------------------------------------------------
// declare variables and constans

//--------------------------------------------------------------------------------
/**
  @brief QualityGate Check-pictures
  */
class QgStaticCheck_OverloadedFiles : QgBase
{
//--------------------------------------------------------------------------------
//@public members
//--------------------------------------------------------------------------------
  
  //------------------------------------------------------------------------------
  public int setUp()
  {
    if ( QgBase::setUp() )
      return -1;
    
    QgVersionResult::showErrorsOnly = TRUE;
    return 0;
  }
  
  //------------------------------------------------------------------------------
  /** @brief Calculates pictures directory.
    @details Calculates pictures directory recursive.
    @return 0 when successfull, otherwise -1.
  */
  public int calculate()
  {
    return _files.calculate();
  }
  
  //------------------------------------------------------------------------------
  /** @brief Function validates calculated pictures dircetory.
    @warning Call function calculate() before. Otherwise validation does not work.
    @return 0 when successfull, otherwise -1.
  */
  public int validate()
  {
    return _files.validate();
  }

  //------------------------------------------------------------------------------
  /** @brief Teardown of QualityGate
    @details Functions cleanup temp-source dir.
    @return 0 when successfull, otherwise -1.
  */
  public int tearDown()
  {
    _result = _files.result;
    return QgBase::tearDown();
  }

//--------------------------------------------------------------------------------
//@private members
//--------------------------------------------------------------------------------
  
  QgOverloadedFilesCheck _files = QgOverloadedFilesCheck(); //!< Pictures directory
};

//--------------------------------------------------------------------------------
/** 
  @breif main rutine to start QualityGate QgStaticCheck-Pictures
*/
void main()
{
  Qg::setId("QgStaticCheck_OverloadedFiles");
  QgStaticCheck_OverloadedFiles qg = QgStaticCheck_OverloadedFiles();
  exit(qg.start());
}
