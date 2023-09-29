echo off
REM Execute all relevant tests

REM ---------------------------------------------------------------------------
cls

REM ---------------------------------------------------------------------------
REM default values
SET WINCC_OA_INSTALL_PATH=C:\Siemens\Automation\WinCC_OA\
SET WINCC_OA_VERSION=3.20
set WINCC_OA_TEST_PATH=%cd%\
set WINCC_OA_TEST_RUN_ID=Regressions-tests

REM get input params
:loopStdIn
IF NOT "%1"=="" (

  REM print help
  IF "%1"=="-OaVersion" (
    SET WINCC_OA_VERSION=%2
    SHIFT
  )
  IF "%1"=="-OaInstallPath" (
    SET WINCC_OA_INSTALL_PATH=%2
    SHIFT
  )
  IF "%1"=="-OaTestPath" (
    SET WINCC_OA_TEST_PATH=%2
    SHIFT
  )
  IF "%1"=="-OaTestRunId" (
    SET WINCC_OA_TEST_RUN_ID=%2
    SHIFT
  )

  SHIFT
  GOTO :loopStdIn
)

set WINCC_OA_INSTALL_PATH=%WINCC_OA_INSTALL_PATH%%WINCC_OA_VERSION%

REM ---------------------------------------------------------------------------
REM register CtrlTestFramework

set oaBinPath=%WINCC_OA_INSTALL_PATH%\bin\
    
echo Register TestFramework-customized project
set _cfgPath=%WINCC_OA_TEST_PATH%\Projects\TfCustomizedQG\config\config
  
echo [general]  > %_cfgPath%
echo #WinCC OA path  >> %_cfgPath%
echo pvss_path = "%WINCC_OA_INSTALL_PATH%"  >> %_cfgPath%

echo #TestFramework self  >> %_cfgPath%
REM echo proj_path = "%WINCC_OA_INSTALL_PATH%\TestFramework_%WINCC_OA_VERSION%"  >> %_cfgPath%
echo proj_path = "C:\ws\WinCCOA\develop_3.x\Subprojects\TestFramework"  >> %_cfgPath%

echo #global test project  >> %_cfgPath%
echo proj_path = "%WINCC_OA_TEST_PATH%Projects\Global"  >> %_cfgPath%
echo #customized testFramework project >> %_cfgPath%
echo proj_path = "%WINCC_OA_TEST_PATH%Projects\TfCustomizedQG"  >> %_cfgPath%
  
echo proj_version = "%WINCC_OA_VERSION%"  >> %_cfgPath%
echo #default languages  >> %_cfgPath%
echo langs = "de_AT.utf8"  >> %_cfgPath%
echo langs = "en_US.utf8"  >> %_cfgPath%
echo langs = "ru_RU.utf8"  >> %_cfgPath%
  
echo pmonPort = 5999  >> %_cfgPath%

echo [testFramework]  >> %_cfgPath%
echo #path with tests  >> %_cfgPath%
echo installPath = "%WINCC_OA_TEST_PATH%"  >> %_cfgPath%

REM re-register project
call %oaBinPath%WCCILpmon.exe -config %WINCC_OA_TEST_PATH%Projects\TfCustomizedQG\config\config -n -autofreg -status -log +stderr
IF %ERRORLEVEL% NEQ 0 IF %ERRORLEVEL% NEQ 3 (
  REM ERRORLEVEL == 0 - fisrt registration
  REM ERRORLEVEL == 3 - re-registration
  echo ERRORLEVEL: %ERRORLEVEL%
  exit 0
)

REM ---------------------------------------------------------------------------
REM execute tests
echo ****** Execute WinCC OA Tests : %WINCC_OA_TEST_RUN_ID%
call %oaBinPath%WCCOActrl.exe -config %WINCC_OA_TEST_PATH%Projects\TfCustomizedQG\config\config -n testRunner.ctl {'registerGlobalProject':true,'registerAllTools':true,'registerAllTemplates':true,'cleanOldResults':true,'cleanStoredProjects':true,'showLogViewer':true,'TfTestManager.checkForPossibleFreezeTests':true,'testRunId':'%WINCC_OA_TEST_RUN_ID%'} -log +stderr -lang en_US.utf8


REM ---------------------------------------------------------------------------
REM convert to jUnit
echo ****** Convert results into jUnit format
call %oaBinPath%WCCOActrl.exe -config %WINCC_OA_TEST_PATH%Projects\TfCustomizedQG\config\config -n oaTestParsers/jsonToJUnit.ctl -log +stderr -lang en_US.utf8


