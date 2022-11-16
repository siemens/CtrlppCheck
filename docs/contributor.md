# How to Build CtrlppCheck

We use connan to build.

## On first Usage

```bash
cd build
conan install .. -s compiler.runtime=MT -s compiler.toolset=v140
cmake .. -G "Visual Studio 16 2019" -A x64
cmake --build .  --config Release
```

## Debug

```bash 
conan install .. -s compiler.runtime=MTd -s compiler.toolset=v140 -s build_type=Debug --build=missing
cmake .. -G "Visual Studio 16 2019" -A x64
cmake --build .  --config Debug
```

## Rebuild

```bash
cd build
cmake --build .  --config Release
```

# How to Test CtrlPPcheck

## How to convert cppcheckTest

+ visit this site: <https://github.com/danmar/cppcheck/tree/master/test> and download some one test file.  (e.g. testautovariables.cpp)
+ open GEDI and create new ctrl script in the /scripts/tests/ctrlppCheck directory with the same name. ( testautovariables.ctl)
+ paste the content of cpp file into ctrl file
+ comment this block

```cpp
#include "checkautovariables.h"
#include "settings.h"
#include "testsuite.h"
#include "tokenize.h"
```

+ change the class to struct - so are the interface public for ctrl script

```cpp
cpp style
 class TestAutoVariables : public TestFixture {
 --> 
ctrl style
  struct estAutoVariables : TestFixture {
```

+ change the class to struct - so are the interface public for ctrl script

```cpp
cpp style
 class TestAutoVariables : public TestFixture {
 --> 
 ctrl style
  struct estAutoVariables : TestFixture {
```

+ remove the key `public:` and `private:`
+ remove the unecessary constructor and Settings member variable.

```cpp
TestAutoVariables() : TestFixture("TestAutoVariables") {

}
Settings settings; 
```

+ remove also function check(). It is defined in class TestFixture
+ remove OVERRIDE in the run() definition

```cpp
cpp style
void run() OVERRIDE { ... }
ctrl style
void run()  { ... }
```

+ replace `REGISTER_TEST` by `main()` function like following example

```cpp
// REGISTER_TEST(TestAutoVariables)
void main()
{
  TestAutoVariables test;
  test.run();
}
```

+ add uses  `#uses "classes/QualityGates/Tools/CppCheck/TestFixture"`
+ save the script and try it to run from oa-console.

 All assertions are thrown in log.

## How to make new test for CtrlPPcheck

+ Make a copy the file scripts/tests/ctrlppCheck/template.ctl
+ Open the file and edit the test.
+ save the script and try it to run from oa-console.

 All assertions are thrown in log.

**Don't forgot to document new testcases!**

# How To Test CtrlppCheck

## Tests: scripts and results

testscritpts are stored in:  
WinCCOA_QualityChecks_tests/scripts/tests/ctrlppCheck/testscripts

result files (.xml) are stored in:  
WinCCOA_QualityChecks_tests/data/ctrlPpCheck/testscripts

example:  
y2038.ctl  
und y2038.xml  

Each file in the scripts folder has a corresponding result file in the result files dir: same name, different extension
Subdirs are allowed to organize testases, must appera in bith directories; simply create new directories and throw your testcode, resp result files in there.  

Errors will be thrown, if for a .ctl file no .xml can be found.  
If there is an error is in the .ctl file and no corresponding <error> in the .xml file the test will pass.  
if  no error is in the .ctl file and an <error> is in the .xml file a warning will be issued.  

Testresults will be written to: "<project_dir>/log/ctrlPpCheck/testscripts/suspiciousSemicolon.xml"

## Tests: configs and rules

**config files** are loaded from 
<winccoa_install_path>/data/DevTools/Base/ctrl.xml // general  
WinCCOA_QualityChecks/data/ctrlPpCheck/cfg/__proj__.xml // proj specific  

configs define Ctrl Language specific stuff. Konstants, intrfaces of functions, ...

**rules** are loaded from
<winccoa_install_path>/data/DevTools/Base/ctrl.xml // general  
WinCCOA_QualityChecks/data/ctrlPpCheck/rule/__proj__.xml // proj specific  

Rules files define patterns
for namees (variables,files, ...), performance issues (do not use delay(), branding etc.
for a description of file format see: /Documentation/namingCheck.md
to create new rules and config files, see: /Documentation/namingCheck.md

examples:  

suspicious semicolon

testscript: D:/Repos/Ctrlppcheck_gulasch/WinCCOA_QualityChecks_tests/scripts/tests/ctrlppCheck/testscripts/suspiciousSemicolon.ctl  
(naming should be: test_suspiciousSemicolon.ctl)  
resultfile: D:/Repos/Ctrlppcheck_gulasch/WinCCOA_QualityChecks_tests/data/ctrlPpCheck/testscriptssuspiciousSemiclon.xml  
(naming should be: test_suspiciousSemicolon.ctl)  
the ceck itself:  
D:/Repos/Ctrlppcheck_gulasch/ctrlppcheck/lib/checks/checkother.h, checkother.cpp Zeile 141 ff  
Der Check selbst (was tut er, findet) ist beschrieben in:  keine Beschreibung gefunden, ausser im Kommentar im Code  
Das ist ein ziemlich kleiner check daher in check, daher auc nicht in eingenem file implementiert  

check 2038  
implemented in check2038.h ud 2038.cpp
