# WinCC OA tests for QualityChecks

Here are located all WinCC OA tests based on WinCC CtrlTestFramework to validate QualityChecks.

Execute the test run "Regression-tests" to validate all regressions before you commit some changes.

## Execute tests

To execute WinCC OA tests start the script *executeTests.cmd* form command line.
This script will prepare everything necessary to execute the test, executes the tests and convert the result to jUnit (might be used in CI/CD pipelines to show results)

``` bat
cd WinCC_OA_Test
executeTests.cmd
```

Following options are possible:
-OaVersion ,defines the WinCC OA Version (default 3.20)
-OaInstallPath ,define the installation path of WinCC OA (default C:\Siemens\Automation\WinCC_OA\)
-OaTestPath ,define the test path (default *thisWorkspace*\WinCC_OA_Test\)
-OaTestRunId , defines test-run ID to be executed (default Regression-tests )

``` bat
executeTests.cmd -OaVersion 3.19 -OaInstallPath C:\Siemens\Automation\WinCC_OA\ -OaTestPath C:\ws\Siemens\CtrlppCheck\WinCC_OA_Test\ -OaTestRunId Regression-tests
```
