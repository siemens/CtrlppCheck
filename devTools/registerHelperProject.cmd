echo off
REM Register WinCC OA helper project

REM ---------------------------------------------------------------------------


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
echo proj_path = "%WINCC_OA_INSTALL_PATH%\TestFramework_%WINCC_OA_VERSION%"  >> %_cfgPath%

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
  REM ERRORLEVEL == 0 - first registration
  REM ERRORLEVEL == 3 - re-registration
  echo ERRORLEVEL: %ERRORLEVEL%
  exit 1
)


