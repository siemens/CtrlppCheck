//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2022 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//


/**
  This scrip build the ctrlppcheck and archive build files (artifacts)
  
  @todo make linux happy
*/

def call(nodeLabel) {

  ///// @todo make linux happy
  //if ( nodeLabel != 'windows' ){
  //  echo 'This platform is not supported now: ' + nodeLabel
  //  return
  //}
      
  
  node(nodeLabel) {
    try {
    // Build in some dummy (tmp) directory.
    // otherwise create artifactory @2tmp directory direct in job-workspace directory, which one can not be removed
    // by function deleteDir(). Because it is out off job-workspace scope.
      dir('tmp'){
        def artifactory_name = "artifactory"
        def artifactory_repo = "conan-local"
      
        def server = Artifactory.server artifactory_name
        def client = Artifactory.newConanClient()
        def serverName = client.remote.add server: server, repo: artifactory_repo
        
        stage("Get project"){
          echo 'Pulling...' + env.BRANCH_NAME
          checkout scm
        }
      
        stage("Install required dependencies"){
          dir ('ctrlppcheck'){
            if ( nodeLabel.contains("windows") ) {
              def b = client.run command: "install . -s compiler.toolset=v140 -s compiler.runtime=MT"
            } 

            if ( nodeLabel.contains("centos") || nodeLabel.contains("debian") ){
              def b = client.run command: "install ."
            }
          }
        }
        
        stage("Create build file"){
          dir ('ctrlppcheck'){
            if ( nodeLabel.contains("windows") ){
              bat "cmake . -T v140 -G \"Visual Studio 16 2019\" -A x64"
            }

            if ( nodeLabel.contains("centos") || nodeLabel.contains("debian") ){
              sh "cmake ."
            }
          }
        }
        
        stage("Build project"){
          dir ('ctrlppcheck'){
            try{
              if ( nodeLabel.contains("windows") ){
                bat "cmake --build . --config Release > cmake_" + nodeLabel + ".log"
              }

              if ( nodeLabel.contains("centos") || nodeLabel.contains("debian") ){
                sh "cmake --build . --config Release > cmake_" + nodeLabel + ".log"
              }
            }
            catch(err){
              checkCmakeOut(nodeLabel);
              throw err;
            }
          }
        }    
      
        stage("Deploy"){
          dir ('ctrlppcheck/bin'){
            if ( nodeLabel.contains("windows") ){
              stash name: "ctrlppcheck_win", includes: "**/*"
            }

            if ( nodeLabel.contains("centos") ){
              stash name: "ctrlppcheck_centos", includes: "**/*"
            }

            if ( nodeLabel.contains("debian") ){
              stash name: "ctrlppcheck_debian", includes: "**/*"
            }
            
          }
          dir ('ctrlppcheck'){
            checkCmakeOut(nodeLabel)
          }
        }
      }
    }
  
    catch (exc) {
      currentBuild.result = 'FAILURE'
      throw exc
    }
    finally {
      deleteDir() // free space on node
    }
  }
}

def checkCmakeOut(nodeLabel){
  recordIssues tool: groovyScript(parserId: 'cmake', 
                                  id: nodeLabel + '_cmake',
                                  name: 'Compiler Warnings',
                                  pattern: 'cmake_' + nodeLabel + '.log')
  archiveArtifacts artifacts: 'cmake_' + nodeLabel + '.log'
}