echo off
REM Execute all relevant tests

REM ---------------------------------------------------------------------------
cls

REM ---------------------------------------------------------------------------
REM default values
SET WINCC_OA_VERSION=3.19
set WINCC_OA_TEST_PATH=%cd%\..\WinCC_OA_Test\
set WINCC_OA_TEST_RUN_ID=Regression-tests

REM get input params
:loopStdIn
IF NOT "%1"=="" (

  REM print help
  IF "%1"=="-oaVersion" (
    SET WINCC_OA_VERSION=%2
    SHIFT
  )
  IF "%1"=="-oaTestPath" (
    SET WINCC_OA_TEST_PATH=%2
    SHIFT
  )
  IF "%1"=="-oaTestRunId" (
    SET WINCC_OA_TEST_RUN_ID=%2
    SHIFT
  )

  SHIFT
  GOTO :loopStdIn
)

call registerHelperProject.cmd

REM ---------------------------------------------------------------------------
REM execute tests
echo ****** Execute WinCC OA Tests : %WINCC_OA_TEST_RUN_ID%
call "%oaBinPath%WCCOActrl.exe" -config %WINCC_OA_TEST_PATH%Projects\TfCustomizedQG\config\config -n testRunner.ctl {'registerGlobalProject':true,'registerAllTools':true,'registerAllTemplates':true,'cleanOldResults':true,'cleanStoredProjects':true,'showLogViewer':true,'TfTestManager.checkForPossibleFreezeTests':true,'testRunId':'%WINCC_OA_TEST_RUN_ID%'} -log +stderr -lang en_US.utf8


REM ---------------------------------------------------------------------------
REM convert to jUnit
echo ****** Convert results into jUnit format
call "%oaBinPath%WCCOActrl.exe" -config %WINCC_OA_TEST_PATH%Projects\TfCustomizedQG\config\config -n oaTestParsers/jsonToJUnit.ctl -log +stderr -lang en_US.utf8


