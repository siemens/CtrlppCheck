
//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2022 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//

library "WinCCOA-shared-lib-317@$BRANCH_NAME" _

properties([buildDiscarder(logRotator(artifactDaysToKeepStr: '', artifactNumToKeepStr: '', daysToKeepStr: '', numToKeepStr: '10'))
           ])

def WinCCOA_version = '3.17';

def packageName = 'WinCCOA_QualityChecks'
def testPackageName = 'WinCCOA_QualityChecks_1'

def platforms = ["windows", "centos-8", "debian"]

// define stages to build executables
def stage_executables = [:]
for (int i = 0; i < platforms.size() ; i++) {
  def pl = platforms[i];

  stage_executables["Build executables on " + pl] = {
    node(pl){
      buildCtrlppcheck pl
      // here can be added other executables, like lizard
    }
  }
}
// define stages to build executables
def stage_cleanUp = [:]
for (int i = 0; i < platforms.size() ; i++) {
  def pl = platforms[i];

  stage_cleanUp["Clean up on: " + pl] = {
    node(pl){

      if ( !pl.contains('debian') ){
        /// @todo de-register projects
        oaProj.startCtrlManager projectName: testPackageName,
                                version: WinCCOA_version,
                                startOptions: 'removeTestProj.ctl ' + testPackageName,
                                platform: pl

        def archiveName = '__TESTPROJ__' + pl + '__'
        if ( pl.contains('windows') ){
          bat('MOVE __TESTPROJ__ ' + archiveName)
        }
        else{
          sh('mv __TESTPROJ__ ' + archiveName)
        }

        archiveArtifacts artifacts: archiveName + '/**/*'
      }
      // free disk space
      deleteDir()

    }
  }
}


// dynamic tests
def stage_dynamicTest = [:]
for (int i = 0; i < platforms.size() ; i++) {
  def pl = platforms[i];

  if ( pl.contains('debian') ){
    continue;
  }

  stage_dynamicTest["Dynamic test on: " + pl] = {
    node(pl){
      // start unit tests for ctrlppcheck
      step_qCheck(packageName: testPackageName,
                  version: WinCCOA_version,
                  id: 'QgCtrlppcheckTests',
                  startOptions: 'QgCtrlppcheckTests.ctl -dbg ctrlppcheck,ctrlppcheck_dtl',
                  platform: pl
                  )


      dir('test'){
        stash name: "results_" + pl, includes: "**"
      }
      /// @todo de-register projects
      oaProj.startCtrlManager projectName: testPackageName,
                              version: WinCCOA_version,
                              startOptions: 'removeTestProj.ctl ' + testPackageName,
                              platform: pl
    }
  }
}




// deploys
def stage_deploy = [:]
for (int i = 0; i < platforms.size() ; i++) {

  def pl = platforms[i];

  if ( pl.contains('debian') ){
    continue;
  }

  stage_deploy["Deploy on: " + pl] = {
    node(pl){
      dir('test'){
        unstash 'results_' + pl
      }

      def ext = '.ctc';
      if ( pl.contains('windows') ){
        ext = '.ctl' // for performace reason we encrypt it only on unix
      }
      // publish test results
      oaProj.startCtrlManager projectName: testPackageName,
                              version: WinCCOA_version,
                              startOptions: 'QualityGates/QGatesConvertOaTestToJUnitXml' + ext,
                              platform: pl

      junit testResults: 'test/*.xml'
    }
  }
}

// define stages to create test projects
def stage_makeProjs = [:]
for (int i = 0; i < platforms.size() ; i++) {
  def platform = platforms[i];

  if ( platform.contains('debian') ){
    continue;
  }

  stage_makeProjs["Make test project on " + platform] = {
    node(platform){
      def configFilePath;

      deleteDir()

      def WinCCOASubProjsDir;
      dir('WinCCOA'){
        WinCCOASubProjsDir = pwd() + '/';

        unstash 'WinCCOA_QGates'
        unstash 'SLT_Installation_Layer'

      }
      dir('__TESTPROJ__'){
        unstash "__PACKAGE__"

        // rename package for testing
        if ( platform.contains('windows') )
          bat('MOVE ' + packageName + ' ' + testPackageName)
        else
          sh('mv ' + packageName + ' ' + testPackageName)

        def destination = pwd() + '/' + testPackageName
        configFilePath = destination + '/config/config'
        oaProj.create name: testPackageName,
                      version:WinCCOA_version,
                      platform: platform

        // register project
        oaProj.registerProj(destination: destination, version: WinCCOA_version, platform: platform)

        oaProj.addOaSubProj(pwd() + '/' + packageName + '_tests', destination);
        oaProj.addOaSubProj(WinCCOASubProjsDir + 'WinCCOATools_QGates', destination);
        oaProj.addOaSubProj(WinCCOASubProjsDir + 'SLT_Installation_Layer', destination);
        // oaProj.addOaSubProj(WinCCOASubProjsDir + 'WinCCOA_InternallChecks', destination);
      }
    }
  }
}

def step_qCheck(Map opts)
{
  echo 'Start quality check: ' + opts.id
  oaProj.startCtrlManager projectName: opts.packageName,
                          version: opts.version,
                          startOptions: opts.startOptions,
                          platform: opts.platform
}

lock(label: 'test-farm') {
try {
  // pre-build package for testing
  stage('Build executables') {
    // build executables
    parallel stage_executables
  }

  stage('Build Package') {
    preBuildPackage(packageName)
  }


  // make test project
  stage('Download SW') {
    node('centos-8'){
      downloadQGates()
      downloadInstallationLayer()
    }
  }

  // this shall works parallel for multiple platforms
  // make test project

  stage('Make test project'){
    parallel stage_makeProjs
  }

  node('centos-8'){
    // Static code analyses
    stage('Static code analyses') {
      def checks = [// QgStaticCheck_Scripts
                     ["id":"QgStaticCheck_Scripts",  "startOptions":"QualityGates/StaticTests/QgScriptsCheck.ctl scripts"],
                    // QgStaticCheck_Libs
                     ["id":"QgStaticCheck_Libs",  "startOptions":"QualityGates/StaticTests/QgScriptsCheck.ctl libs"],
                    // QgStaticCheck_Panels
                     ["id":"QgStaticCheck_Panels",  "startOptions":"QualityGates/StaticTests/QgPanelsCheck.ctl"],
                    // QgStaticCheck_Pictures
                     ["id":"QgStaticCheck_Pictures", "startOptions":"QualityGates/StaticTests/QgPicturesCheck.ctl"]

      /// @todo add more analyses here
                   ]
      checks.each {
         def check = it
         check['version'] = WinCCOA_version;
         check['packageName'] = testPackageName;
         check['platform'] = 'centos-8';
         step_qCheck(check);
      }
    }

    // Build finally package
    stage('Build finally test project') {
      def checks = [// QgSyntaxCheck
                    ["id":"QgSyntaxCheck", "startOptions":"QualityGates/BuildAddOn/QgSyntaxCheck.ctl"],
                    // QgEncryptSource
                    ["id":"QgEncryptSource", "startOptions":"QualityGates/BuildAddOn/QgEncryptSource.ctl"]
                   ]
      checks.each {
         def check = it
         check['version'] = WinCCOA_version;
         check['packageName'] = testPackageName;
         check['platform'] = 'centos-8';
         step_qCheck(check);
      }
    }
  }

  // start dynamic tests for finally package
  stage('Dynamic Tests') {
    parallel stage_dynamicTest
  }

  node('centos-8'){
    // self test - some paranoid checks (crypt, license check)
    // !!! newer trust somebody. Trust is fine, control is better.
    stage('Self test') {
    }
  }

  // deploy
  stage('Deploy') {
    parallel stage_deploy
  }


  // ever think is fine (0 false positive) we can publish package

}  catch (exc) {
  currentBuild.result = 'FAILURE'
  throw exc
}  finally {
  // deleteDir()
  stage('Clean up'){
    parallel stage_cleanUp
  }
} // end of try-catch-finally
} // end of lock

