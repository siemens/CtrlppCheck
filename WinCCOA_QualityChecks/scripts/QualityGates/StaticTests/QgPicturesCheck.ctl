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
#uses "classes/QualityGates/QgStaticCheck/Pictures/PicturesDir"
#uses "classes/QualityGates/Qg"
#uses "classes/QualityGates/QgBase"
#uses "classes/Variables/Float"

//--------------------------------------------------------------------------------
// declare variables and constans


//--------------------------------------------------------------------------------
/**
  @brief QualityGate Check-pictures
  */
class QgStaticCheck_Pictures : QgBase
{
//--------------------------------------------------------------------------------
//@public members
//--------------------------------------------------------------------------------
  
  //------------------------------------------------------------------------------
  /** @brief Function setups pictures tests.
    @details Creates temp-source dir.
    @return 0 when successfull, otherwise -1.
  */
  public int setUp()
  {
    if ( QgBase::setUp() )
      return -1;
    
    _pictures.setDir(PROJ_PATH + PICTURES_REL_PATH);
    
    if ( !_pictures.exists() )
      setMinValidScore("QgStaticCheck_Pictures", "assert.missingPictures", "reason.missingPictures");
    
    return 0;
  }

  //------------------------------------------------------------------------------
  /** @brief Calculates pictures directory.
    @details Calculates pictures directory recursive.
    @return 0 when successfull, otherwise -1.
  */
  public int calculate()
  {
    if ( _pictures.exists() )
      return _pictures.calculate();
    
    return 0;
  }
  
  //------------------------------------------------------------------------------
  /** @brief Function validates calculated pictures dircetory.
    @warning Call function calculate() before. Otherwise validation does not work.
    @return 0 when successfull, otherwise -1.
  */
  public int validate()
  {
    return _pictures.validate();
  }

  //------------------------------------------------------------------------------
  /** @brief Teardown of QualityGate
    @details Functions cleanup temp-source dir.
    @return 0 when successfull, otherwise -1.
  */
  public int tearDown()
  {
    _result = _pictures.result;
    return QgBase::tearDown();
  }

//--------------------------------------------------------------------------------
//@private members
//--------------------------------------------------------------------------------
  
  PicturesDir _pictures = PicturesDir(); //!< Pictures directory
};

//--------------------------------------------------------------------------------
/** 
  @breif main rutine to start QualityGate QgStaticCheck-Pictures
*/
void main()
{
  Qg::setId("QgStaticCheck_Pictures");
  QgStaticCheck_Pictures qg = QgStaticCheck_Pictures();
  exit(qg.start());
}
