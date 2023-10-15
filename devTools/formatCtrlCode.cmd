echo off
REM Formats ctrl code

REM ---------------------------------------------------------------------------
cls

REM ---------------------------------------------------------------------------
REM default values
SET WINCC_OA_INSTALL_PATH=C:\Siemens\Automation\WinCC_OA\
SET WINCC_OA_VERSION=3.19
set WINCC_OA_TEST_PATH=%cd%\..\WinCC_OA_Test\

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

  SHIFT
  GOTO :loopStdIn
)

call registerHelperProject.cmd

REM --------------------------------------------------------------------------
REM Format ctrl code
call %oaBinPath%WCCOActrl.exe -config %WINCC_OA_TEST_PATH%Projects\TfCustomizedQG\config\config -n astyle.ctl %WINCC_OA_TEST_PATH% -log +stderr -lang en_US.utf8
call %oaBinPath%WCCOActrl.exe -config %WINCC_OA_TEST_PATH%Projects\TfCustomizedQG\config\config -n astyle.ctl %WINCC_OA_TEST_PATH%..\WinCCOA_QualityChecks -log +stderr -lang en_US.utf8

