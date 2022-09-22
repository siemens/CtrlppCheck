# CtrlPPcheck tests

## How to convert cppcheckTest

+ visit this side:
https://github.com/danmar/cppcheck/tree/master/test
and download some one test file.  (ex. testautovariables.cpp)
+ open gedi and create new ctrl script in the /scripts/tests/cppCheck directory with the same name. ( testautovariables.ctl)
+ paste the content of cpp file in to ctrl file
+ comment this block
```
#include "checkautovariables.h"
#include "settings.h"
#include "testsuite.h"
#include "tokenize.h"
```
+ change the class to struct - so are the interface public for ctrl script
 ```
cpp style
 class TestAutoVariables : public TestFixture {
 --> 
 ctrl style
  struct estAutoVariables : TestFixture {
```

+ change the class to struct - so are the interface public for ctrl script
 ```
cpp style
 class TestAutoVariables : public TestFixture {
 --> 
 ctrl style
  struct estAutoVariables : TestFixture {
```
+ remove the key `public:` and `private:`
+ remove the unecessary constructor and Settings member variable.
 ```
TestAutoVariables() : TestFixture("TestAutoVariables") {

}
Settings settings; 

```
+ remove also function check(). It is defined in class TestFixture
+ remove OVERRIDE in the run() definition
```
cpp style
void run() OVERRIDE { ... }
ctrl style
void run()  { ... }

```
+ replace `REGISTER_TEST` by `main()` function like following example
```
// REGISTER_TEST(TestAutoVariables)
void main()
{
  TestAutoVariables test;
  test.run();
}
```
+ add uses  `#uses "classes/QualityGates/Tools/CppCheck/TestFixture"`
+ save the script and try it to run from oa-console.

 All assertions are throwed in log.

## How to make new test for CtrlPPcheck

+ Make a copy the file scripts/tests/cppCheck/template.ctl
+ Open the file and edit the test. 
+ save the script and try it to run from oa-console.

 All assertions are throwed in log.
### Dotn forgot to document new testcases