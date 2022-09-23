#! groovy

//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2022 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//

@Library('WinCC_OA_Library_Test_Jenkins')WinCC_OA_Library_Test_Jenkins
@Library('WinCC_OA_Library_Test@develop_3.x')WinCC_OA_Library_Test


import jenkins.model.Jenkins
import org.jenkins.CurrentBuild
import org.jenkins.TestNode

CurrentBuild.pl = this;
CurrentBuild.versionInfo = evaluate(readTrusted('Test/Jenkins/versionInfo.groovy'));
TestNode.pl = this;

String label = '';

architecture = 'amd64';
dockerPlatform = '--platform linux/' + architecture + ' ';
exceptions = [];

// get first free docker node
node('dockerBuild') {
  label = env.NODE_NAME;
}

lockResources([label]) { // lock build node
  def testNode = new TestNode(label);
  try {
    testNode.executeOnNode(['timeOut' : 10]) {
      def ret;
      stage('Prepare Docker'){
        testNode.sysCmd('sudo systemctl start docker');

        testNode.sysCmd('sudo groupadd docker'         + ' & ' +
                        'sudo gpasswd -a ' + env['USER'] + ' docker' + ' & ' +
                        'newgrp docker & exit 0');

        // Debian nodes needs the installation of the processor installation to run for example arm.
        // This fails on OS like Oracle, it is only yet known to work on Debian VMs.
        if (architecture == 'arm64') {
          testNode.sysCmd('sudo apt-get -y install qemu-user-static binfmt-support');
        }

        ret = testNode.sysCmd('sudo docker system prune --force --all --volumes', [returnStdOut: true, returnStatus: true]);
      }

      if (ret != 0)
      {
        stage('reboot')
        {
          testNode.reboot('system prune failed due to missing permission, reboot for permission update');
          ret = testNode.sysCmd('sudo docker system prune --force --all --volumes', [returnStdOut: true, returnStatus: true]);
          if(ret != 0 )
            throw new Exception('system prune has no permission, check group permissions in stage Prepare Docker');
        }
      }

      stage('Prepare ws') {
        prepareWorkspaces(testNode);
      }

      def image;
      stage('Build image') {
        image = buildTestDockerImage(testNode);
      }

      stage('set permissions') {  
              // add the jenkinststssh and oatstusr user to the same group, afterwards set the access control list (setfacl) for the user and group.
              // -Rdm: R is recursive. d is default, which means for all future items created under that directory have these rules applied by default. m is needed to add/modify rules.
              testNode.sysCmd("echo here are the permissions");
              testNode.sysCmd('sudo usermod -aG users jenkinststssh' + ' & ' +
                              'sudo usermod -aG users oatstusr' + ' & ' +
                              'sudo setfacl -Rdm g:users:rwx /home/jenkinststssh@ETM/jenkins/' + ' & ' +
                              'sudo setfacl -Rdm u:oatstusr:rwx /home/jenkinststssh@ETM/jenkins/' + ' & ' +
                              'sudo chown jenkinststssh:users -R /home/jenkinststssh@ETM/' + ' & ' + 
                              'sudo chmod 777 -R /home/jenkinststssh@ETM/ ');
      }
      stage('build ctrlppchceck') {
        String dockerParameters = initDockerParameters(testNode);
        image.inside(dockerParameters){
          sh 'sudo apt -y install cmake g++ make'
          sh 'cd /opt/ws/WinCCOA/Subprojects/QualityCheck/ctrlppcheck' + '\n'+
              'mkdir build' + '\n'+
            //   'cd build' + '\n'+
              'cmake .' + '\n'+
              'cmake --build .  --config Release' + '\n'+
              'pwd \n ls -l -r' + '\n'
        }
      }
      stage('Deploy') {
        testNode.sysCmd 'sudo rm -rf ' + env.WORKSPACE + '/bin\n' 
        testNode.sysCmd 'pwd \n ls -l -r'  + '\n'+
                        'sudo mv -f /opt/ws/WinCCOA/Subprojects/QualityCheck/ctrlppcheck/bin ' + env.WORKSPACE + '\n'
        archiveArtifacts  artifacts: '**/*', fingerprint: false, allowEmptyArchive: false // , excludes: '**/*/db*/wincc_oa/**/*,**/*/db/wincc_oa/**/*'
        testNode.sysCmd 'sudo rm -rf ' + env.WORKSPACE + '/bin\n' 
      }
    }
  } catch (error) {
    exceptions.push(error);
  } finally {
    
    if ( exceptions.size() > 0 ) {
      makeSummary(exceptions);
    }
    cleanUpNode(testNode);
  }
}


//------------------------------------------------------------------------------
def getVersionWithoutDots(){
  return CurrentBuild.versionInfo.version().replaceAll('\\.', '');
}

//------------------------------------------------------------------------------
def initDockerParameters(TestNode testNode){

  //if variable is not defined in testSuite.conf, add empty string
  def dockerPorts = '';
  def dockerEnvVariables = '';
  def dockerVolumes = '';

  // overwrite the usage of the jenkinststssh user with the oatstusr user, which is automatically given by jenkins
  def userId = '2033639';
  def groupId = '2000513';
  def containerCredentials = '-u ' + userId + ':' + groupId;

  //add standard Values
  dockerPorts += ' -p 8080:8080';
  //We are connected via SSH, the Display does not exist here. WE have to say the jenkins where the DISPLAY is.
  dockerEnvVariables += ' -e DISPLAY=:0'; //This is the Display variable only for Centos!! If We need docker for other Unix system this has to be adapted;
  dockerVolumes += ' -v /tmp/.X11-unix:/tmp/.X11-unix -v /opt/ws/:/opt/ws/ -v /etc/localtime:/etc/localtime';

  String dockerParameters = containerCredentials + ' ' + dockerPlatform + '-it ' + dockerPorts + ' ' + dockerEnvVariables + ' ' + dockerVolumes;

  return dockerParameters;

}


//------------------------------------------------------------------------------
def prepareWorkspaces(TestNode testNode) {

  testNode.executeOnNode(){

    if ( testNode.isWindows() ){
      throw new Exception('This is script is implemented only for UNIX nodes');
    }

    //------------------------------------------------------------------------------

    // prepareNode, checkout workspaces
    Map opts = [:];
    opts.WinCCOA = [
      version: CurrentBuild.versionInfo.version(),
      needReboot : false,
      doInstall : false
    ];

    opts.enableCppCoverage = false;
    opts.repo = [ WinCC_OA_Library_Test_Jenkins : [SHA1 : 'master'],
                  WinCC_OA_Library_Test : [SHA1 : getSetupBranch()],
                  WinCCOA : [SHA1 : getOaBranch()] ];

    testNode = prepareNode(testNode, opts);
  }
}


//------------------------------------------------------------------------------
/**
 * Returns the the Branch of the current Version.
 *
 * @return String branch name
*/
def getSetupBranch()
{
  // return env.BRANCH_NAME;
  return 'develop_3.x';
}

//------------------------------------------------------------------------------
def getOaBranch(){

//   if ( params.BUILD_SHA1 != "" )
//     return params.BUILD_SHA1.trim();

  return env.BRANCH_NAME;
}

//------------------------------------------------------------------------------
def buildTestDockerImage(TestNode testNode){

  def dockerImage = null;

  testNode.executeOnNode(){

    if ( testNode.isWindows() ){
      throw new Exception('This script is only implemented for UNIX nodes');
    }

    def user = getTstCredentials(testNode);

    //------------------------------------------------------------------------------
    // Build docker image
    dir('/tmp/WinCC_OA_setup'){
      docker.withRegistry(getDockerRegistryURL(), 'oatstdcpush') {
        sh 'docker pull ' + dockerPlatform + 'devops-nexus.etm.at:5000/debian_oabase_test:buster';
        def args = dockerPlatform +
                   '--build-arg BASE_IMAGE=devops-nexus.etm.at:5000/debian_oabase_test:buster ' +
                   '--build-arg USER_ID=$(id -u ' + user.testUserName + ') ' +
                   '--build-arg USER_GID=$(id -g ' + user.testUserName + ') ' +
                   '--build-arg USER=' + user.testUserName + ' ' +
                   // "--build-arg USER=" + tstUserCredentials.name + " " +
                   // "-t winccoa" + getVersionWithoutDots() + " " +
                   // "--build-arg USER_PASSWORD=" + tstUserCredentials.pw + " " +
                   ".";

        dockerImage = docker.build("winccoa" + getVersionWithoutDots(), args);
      }
    }
  }

  return dockerImage;
}

//------------------------------------------------------------------------------
String getDockerRegistryURL(){
//  return 'https://artifactory.etm.at:8445';
  return 'https://devops-nexus.etm.at:5000';
}