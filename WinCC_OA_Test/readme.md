# WinCC OA tests for QualityChecks

Here are located all WinCC OA tests based on WinCC CtrlTestFramework to validate QualityChecks.

Execute the test run "Regression-tests" to validate all regressions before you commit some changes.

## Execute tests

To execute WinCC OA tests start the script *executeTests.cmd* from command line.
This script will prepare everything necessary to execute the test, executes the tests and convert the result to jUnit (might be used in CI/CD pipelines to show results)

``` bat
cd devTools
executeTests.cmd
```

Following options are possible:

+ -oaVersion ,defines the WinCC OA Version (default 3.19)
+ -oaTestPath ,define the test path (default *thisWorkspace*/WinCC_OA_Test/)
+ -oaTestRunId , defines test-run ID to be executed (default Regression-tests)

``` bat
executeTests.cmd -oaVersion 3.19 -oaTestPath C:\ws\Siemens\CtrlppCheck\WinCC_OA_Test\ -oaTestRunId Regression-tests
```
