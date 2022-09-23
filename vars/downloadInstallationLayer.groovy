//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2022 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//


/**
  This script download InstallationLayer
*/

def call() {
  dir('tmp'){
    git branch: 'master',
        credentialsId: '8ff214bd-451e-499d-b346-0d8bbf9ec968',
        url: 'https://github.com/LukasSchopp/SLT_Installation_Layer.git'
    stash name: "SLT_Installation_Layer", includes: "**/*"
    deleteDir()
  }
}
