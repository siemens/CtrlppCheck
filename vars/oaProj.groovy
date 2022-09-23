//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2022 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//

package WinCCOA

//-------------------------------------------------------------------
def startCtrlManager(Map opts) {
  def platform = opts.platform;
  def projName = opts.projectName;
  def cfgPath = opts.configFile;
  def oaVerion = opts.version;
  def evCon = opts.evConnection;
  def cmd = opts.startOptions;
     
  if ( cfgPath && (cfgPath != '') )
    cmd += ' -config ' + cfgPath + ' -autofreg'
  else
    cmd += ' -proj ' + projName

  cmd += ' -log +stderr'

  if ( !evCon )
    cmd += ' -n'

  if ( platform.contains('windows') ) {
    bat 'cd E:\\Siemens\\Automation\\WinCC_OA\\'+ oaVerion + '\\bin \n'+
        'WCCOActrl.exe ' + cmd
  } else {
    sh 'cd /opt/WinCC_OA/'+ oaVerion + '/bin \n'+
       './WCCOActrl ' + cmd
  }
}

//-------------------------------------------------------------------
def registerProj(Map opts)
{
  // start ctrl manager with non-existing script and option -autofreg.
  // it register the project, but ctrl return exit code != 0
  try {
    startCtrlManager configFile: opts.destination + '/config/config',
                     version: opts.version,
                     startOptions: 'd.ctl',
                     platform: opts.platform
  }
  catch (exc)
  {
  }
}

//-------------------------------------------------------------------
def addOaSubProj(subProjPath, projDir)
{
  def cfgFile = projDir + '/config/config'
  def cfg = readFile(cfgFile)
  cfg = cfg.replaceFirst('proj_path = ', 'proj_path = \"' + subProjPath.replace("\\", "/") + '\"' + '\n' + 'proj_path = ')
  writeFile file: cfgFile, text: cfg
}

//-------------------------------------------------------------------
def create(Map opts)
{
  def name = opts.name;
  def destination = pwd() + '/' + name;
  def platform = 'centos';
  def langs = ["de_AT.utf8", "en_US.utf8", "ru_RU.utf8"]
  def version = '3.17'
  
  if ( opts.langs )
    langs = opts.langs;
  if ( opts.version )
    version = opts.version;
  if ( opts.platform )
    platform = opts.platform;
  
  def pvss_path = '/opt/WinCC_OA/'
  if ( platform == 'windows' )
    pvss_path = 'E:/Siemens/Automation/WinCC_OA/'
    
  pvss_path = pvss_path  + version
  
  
  dir(name + '/config'){
    // create config file
    def cfg = '[general]' + '\n' + 
              'pvss_path = \"' + pvss_path + '\"' + '\n';
              
             
    cfg = cfg + 'proj_path = \"' + destination + '\"' + '\n';
    cfg = cfg + 'proj_version = \"' + version + '\"' + '\n';
   
    for (int i = 0; i < langs.size() ; i++) {
      cfg = cfg + 'langs = \"' + langs[i] + '\"' + '\n';
    }
    
    cfg = cfg + 'mxProxy = \"none\"' + '\n'; // we don't need the proxy
    
    echo '*** cfg:\n' + cfg
    writeFile file: 'config', text: cfg
    
  // create dummy progs file
    def progs = 'version 1' + '\n' +
                'auth \"\" \"\"' + '\n' +
                '#Manager         | Start  | SecKill | Restart# | ResetMin | Options' + '\n' +
                'WCCILpmon        | manual |      30 |        3 |        1 |' + '\n' +
                'WCCILdata        | always |      30 |        3 |        1 |' + '\n' +
                'WCCILevent       | always |      30 |        3 |        1 |' + '\n' +
                'WCCOActrl        | always |      30 |        3 |        1 |-f pvss_scripts.lst' + '\n' +
                'WCCILsim         | always |      30 |        3 |        1 |' + '\n';
                
    echo '*** progs:\n' + progs
    writeFile file: 'progs', text: progs
  }
  
  // create log-dir, otherwise we lost logs.
  dir(name + '/log'){}
  
  /// @todo create DB
  
  
}
