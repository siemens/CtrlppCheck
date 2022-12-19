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

# How to Extend CtrlppCheck

# How to Test CtrlppCheck - Unit Tests

## How to add new tests for CtrlppCheck from scratch

+ copy the Project "WinCCOA_QualityChecks_tests" from the repository to your projects and add it as a subproject to your current project.
+ visit this site: <https://github.com/danmar/cppcheck/tree/master/test> and download a test file (e.g. testautovariables.cpp).
+ open GEDI and create new ctrl script in the /scripts/tests/CtrlppCheck directory of the "WinCCOA_QualityChecks_tests" subproject with the same name. (testautovariables.ctl)
+ paste the content of cpp file into the ctrl file
+ comment this block out

```cpp
#include "checkautovariables.h"
#include "settings.h"
#include "testsuite.h"
#include "tokenize.h"
```

+ change the class to struct - so the interfaces are public for ctrl script

```cpp
cpp style
 class TestAutoVariables : public TestFixture {

--> 

ctrl style
  struct estAutoVariables : TestFixture {
```

+ remove the keywords `public:` and `private:`
+ remove the unecessary constructor and Settings member variable.

```cpp
TestAutoVariables() : TestFixture("TestAutoVariables") {

}
Settings settings; 
```

+ also remove the function check(). It is defined in class TestFixture
+ remove OVERRIDE in the run() definition

```cpp
cpp style
void run() OVERRIDE { ... }

-->

ctrl style
void run()  { ... }
```

+ replace `REGISTER_TEST` by `main()` function like following example

```cpp
cpp style
REGISTER_TEST(TestAutoVariables)

-->

ctrl style
void main()
{
  TestAutoVariables test;
  test.run();
}
```

+ add add  `#uses "classes/QualityGates/Tools/CppCheck/TestFixture"`
+ save the script and try to it run from OA-console with a Ctrl Manager - All assertions and results are written to log.

### How to add new tests for CtrlppCheck - the easy way

The steps described in the chapter above have already been made and a template is available:

+ Make a copy the file scripts/tests/CtrlppCheck/template.ctl
+ Open the file and edit the test.
+ save the script and try it to run from oa-console with a Ctrl-Manager - All assertions and results are written to log.

**Don't forgot to document new testcases!**

# How To Test CtrlppCheck - using ctrl scripts

## Tests: scripts and results

testscritpts are stored in:  

WinCCOA_QualityChecks_tests/scripts/**tests**/CtrlppCheck/testscripts

result files (.xml) are stored in:  
WinCCOA_QualityChecks_tests/**data**/CtrlppCheck/testscripts

example:  
y2038.ctl  
und y2038.xml  

Each file in the scripts folder has a corresponding result file in the result files dir: same name, different extension
Subdirs are allowed to organize testases, must appera in bith directories; simply create new directories and throw your testcode, resp result files in there.  

Errors will be thrown, if for a .ctl file no .xml can be found.  
If there is an error is in the .ctl file and no corresponding <error> in the .xml file the test will pass.  
If no error is in the .ctl file and an error is in the .xml file a warning will be issued.  

Testresults will be written to: "<project_dir>/log/CtrlppCheck/testscripts/suspiciousSemicolon.xml"

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
