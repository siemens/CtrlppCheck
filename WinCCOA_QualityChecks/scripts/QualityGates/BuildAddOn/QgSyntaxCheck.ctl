//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2023 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//

//--------------------------------------------------------------------------------
// used libraries (#uses)
#uses "classes/QualityGates/QgSyntaxCheck/QgSyntaxCheck"

//--------------------------------------------------------------------------------
// declare variables and constans


//--------------------------------------------------------------------------------
/**
  @breif main rutine to start QualityGate QgStaticCheck-OASyntaxCheck
*/
void main()
{
  Qg::setId("QgSyntaxCheck");
  QgSyntaxCheck qg = QgSyntaxCheck();
  exit(qg.start());
}
