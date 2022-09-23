
//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2022 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//

/**
  This script pre-build WinCCOA_QualityChecks package
*/

def call(packageName) {
  try {  
    node('centos-8') {
      // buils workspace
      stage('get ws') {
        echo 'Pulling...' + env.BRANCH_NAME
        checkout scm
      }
      // build WinCCOA package
      stage('Build package') {
        dir('__PACKAGE__'){
          deleteDir()
          sh 'cp -rf $WORKSPACE/' + packageName + ' .'
          dir(packageName + "/bin/ctrlppcheck") {
            unstash "ctrlppcheck_win"
            // unstash "ctrlppcheck_centos"
          }
          sh 'if [ -d \"$WORKSPACE/' + packageName + '_tests\" ] \n  then \n cp -rf $WORKSPACE/' + packageName + '_tests . \n fi'
          
          stash name: "__PACKAGE__", includes: "**/*", fingerprint: true
        }
        archiveArtifacts artifacts: '__PACKAGE__/**/*'

        // add unix executables to separate artifacts
        // CentOS
        dir('ctrlppcheck_centos'){
          unstash "ctrlppcheck_centos"
        }
        archiveArtifacts artifacts: 'ctrlppcheck_centos/**/*'

        // Debian
        dir('ctrlppcheck_debian'){
          unstash "ctrlppcheck_debian"
        }
        archiveArtifacts artifacts: 'ctrlppcheck_debian/**/*'
      }
      deleteDir()
    }
  }
  
  catch (exc) {
    currentBuild.result = 'FAILURE'
    throw exc
  }
  finally {
  
  }
}