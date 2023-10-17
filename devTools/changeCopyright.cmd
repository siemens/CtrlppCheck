echo off
REM Change copyright sections

REM ---------------------------------------------------------------------------
cls

REM ---------------------------------------------------------------------------
REM default values
SET WINCC_OA_VERSION=3.19
set WINCC_OA_TEST_PATH=%cd%\..\WinCC_OA_Test\

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

  SHIFT
  GOTO :loopStdIn
)


call registerHelperProject.cmd

REM --------------------------------------------------------------------------
REM Change copyright information
call %oaBinPath%WCCOActrl.exe -config %WINCC_OA_TEST_PATH%Projects\TfCustomizedQG\config\config -n copyright.ctl %WINCC_OA_TEST_PATH% -log +stderr -lang en_US.utf8
call %oaBinPath%WCCOActrl.exe -config %WINCC_OA_TEST_PATH%Projects\TfCustomizedQG\config\config -n copyright.ctl %WINCC_OA_TEST_PATH%..\WinCCOA_QualityChecks -log +stderr -lang en_US.utf8
