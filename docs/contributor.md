# How to Build CtrlppCheck

## On first Usage

```bash
cd build
cmake .. -G "Visual Studio 16 2019" -A x64
cmake --build .  --config Release
```

## Debug

```bash 
cmake .. -G "Visual Studio 16 2019" -A x64
cmake --build .  --config Debug
```

## Rebuild

```bash
cd build
cmake --build .  --config Release
```

# How to Test CtrlppCheck - Unit Tests

Currently we have no cpp unit tests.

# How To Test CtrlppCheck - using ctrl scripts

## Tests: scripts and results

Test scripts are stored in:  

WinCCOA_QualityChecks_tests/scripts/**tests**/CtrlppCheck/testscripts

result files (.xml) are stored in:  
WinCCOA_QualityChecks_tests/**data**/CtrlppCheck/testscripts

example:  
y2038.ctl  
und y2038.xml  

Each file in the scripts folder has a corresponding result file in the result files dir: same name, different extension
Sub directories are allowed to organize test cases, must appear in both directories; simply create new directories and throw your test-code, resp result files in there.  

Errors will be thrown, if for a .ctl file no .xml can be found.  
If there is an error is in the .ctl file and no corresponding <error> in the .xml file the test will pass.  
If no error is in the .ctl file and an error is in the .xml file a warning will be issued.  

Test results will be written to: "<project_dir>/log/CtrlppCheck/testscripts/suspiciousSemicolon.xml"

Please mind that well documented tests are the best examples. Therefore, document the test-script as well so that everybody can see and understand what the check does and what the (un)expected results are. Also if you find limitations, it is good practice to document them in the test-code as well. This is transparent for the public community and makes it easier for others to steps in and proceed.

## Tests: configs and rules

**config files** are loaded from 

<winccoa_install_path>/data/DevTools/Base/ctrl.xml // general  
WinCCOA_QualityChecks/data/CtrlppCheck/cfg/\_\_proj\_\_.xml // proj specific  

Configs define Ctrl language specific things: constants, interfaces of functions, ...

**rules** are loaded from
<winccoa_install_path>/data/DevTools/Base/ctrl.xml // general  
WinCCOA_QualityChecks/data/CtrlppCheck/rule/\_\_proj\_\_.xml // proj specific  

Rules files define patterns, performance issues, branding etc.  
for a description of file format see: /Documentation/namingCheck.md  
to create new rules and config files, see: /Documentation/namingCheck.md

Naming rules files define patterns for names (variables, files, functions, ...)

examples:  

suspicious semicolon

testscript: WinCCOA_QualityChecks_tests/scripts/tests/CtrlppCheck/testscripts/suspiciousSemicolon.ctl  
(naming should be: test_suspiciousSemicolon.ctl)  
resultfile: WinCCOA_QualityChecks_tests/data/CtrlppCheck/testscriptssuspiciousSemiclon.xml  
(naming should be: test_suspiciousSemicolon.xml)  

the check itself:  
CtrlppCheck/lib/checks/checkother.h, checkother.cpp Zeile 141 ff  

check 2038  
implemented in check2038.h and 2038.cpp
