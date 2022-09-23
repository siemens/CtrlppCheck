//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2022 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//

#uses "classes/QualityGates/QgStaticCheck/Pictures/PicturesDir"
#uses "classes/QualityGates/Qg"
#uses "classes/QualityGates/QgBase"

class QgTemplate : QgBase
{

  public int setUp()
  {
    if ( QgBase::setUp() )
      return -1;

//     _sourceDir.create();
//     _pictures.setDir(_sourceDir.getDirPath + PICTURES_REL_PATH);
    _pictures.setDir(WINCCOA_PATH + PICTURES_REL_PATH);
    return 0;
  }

  public int calculate()
  {
    return _pictures.calculate();
  }

  public int validate()
  {
    return _pictures.validate();
  }

  public mapping getStoreResult()
  {
    return _pictures.result.toMap();
  }

  public int tearDown()
  {
    _sourceDir.cleanUp();
    return QgBase::tearDown();
  }

  PicturesDir _pictures = PicturesDir();
};

main()
{
  Qg::setId("QgTemplate");
  QgTemplate qg = QgTemplate();
  exit(qg.start());
}
