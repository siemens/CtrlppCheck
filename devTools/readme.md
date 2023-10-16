# Developer tools

Here are located few helper developer tools to make locale development a little bit easier.

Happy coding.

## executeTests.cmd

To execute WinCC OA tests start *executeTests.cmd* from command line.
This script prepares everything necessary to execute the tests, executes them and converts the results to jUnit (can be used in CI/CD pipelines).

See also [Execute tests](../WinCC_OA_Test/readme.md)

## formatCtrlCode.cmd

Applies automatic formatting of all ctrl code - including ctrl tests.

Consistently using the same style throughout your code makes it easier to read. Code that is easy to read is easier to understand by you as well as by potential collaborators. Therefore, adhering to a coding style reduces the risk of mistakes and makes it easier to work together on software.

This strict formats the ctrl code in a uniform way by preferred WinCC OA configuration.

Following options are supported:

+ -oaVersion, defines the WinCC OA Version (default 3.19)

## changeCopyright.cmd

Changes copyright entries to current year for all ctrl code - including ctrl tests.
The license section contains a year and therefore must be adapted accordingly.
This script adapts all the license section for the current year.
Following options are supported:

+ -oaVersion ,defines the WinCC OA Version (default 3.19)
