# Developer tools

Here are located few helper developer tools to make locale development a little bit easier.

Happy coding.

## executeTests.cmd

Helper to execute WinCC OA tests start the script *executeTests.cmd* from command line.
This script will prepare everything necessary to execute the test, executes the tests and convert the result to jUnit (might be used in CI/CD pipelines to show results)

See also [Execute tests](../WinCC_OA_Test/readme.md)

## formatCtrlCode.cmd

Helper to allow automatic formatting of ctrl code - inclusive ctrl tests.

Consistently using the same style throughout your code makes it easier to read. Code that is easy to read is easier to understand by you as well as by potential collaborators. Therefore, adhering to a coding style reduces the risk of mistakes and makes it easier to work together on software.

This stript format the ctrl code in unified way by preferred WinCC OA configuration.

Following options are possible:

+ -oaVersion ,defines the WinCC OA Version (default 3.19)

## changeCopyright.cmd

Helper to change copyright entries to correct year for all ctrl code - including ctrl tests.

As the license section contains year need to be changed ones per year.
To change it manually is a lot of work. Therefore you can start this script to change all the license section correctly.

Following options are supported:

+ -oaVersion ,defines the WinCC OA Version (default 3.19)
