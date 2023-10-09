//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2023 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//

#uses "classes/QualityGates/QgCtrlppCheck/QgCtrlppCheck"

void main(string path = PROJ_PATH)
{
  exit(start_QgCtrlppCheck(path));
}
