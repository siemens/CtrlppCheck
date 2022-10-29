# Introduction to WinCC OA Qualitycheck

The tool is implemented as a WinCC OA Subproject and is devided into two parts:

- Quality Checks
- Ctrlppcheck

## Quality Checks

Quality checks check panels and ctrl code based on the following software metrics and WinCC OA specific checks:

- Mc Cabe Complexity
- Code lines per script / Libs
- Code lines per function
- Number of function per script / libs
- Number of functions per panel
- Number of parameters per function
- Number of properties per panel
- Number of files per folder
- Pictures used in the project
- Overloaded files from the version, for maintenance purposes
- and other

## CtrlPPCheck

CtrlppCheck provides static code analysis for the WinCC OA Ctrl/Ctrl++ language.  
Included are, among many others, these checks:

- Undefined variables
- Unused variables
- unused functions
- Dead code
- Comparison is always true / false
- Return value of a specific function is not used e.g. return from dpExists ()

Please understand that there are limits of Ctrlppcheck. Ctrlppcheck is rarely wrong about reported errors. But there are many bugs that it doesn't detect. You will find more bugs in your software by testing your software carefully.

## GEDI Integration

Both types of checks can be used in WinCC OA's GEDI editor via. They are started from the IDE and results are displayed in the IDE:

- Drop-Down menu in the GEDI menu bar
- Dock-Module for the GEDI with results
- Script check via button in the script editor

## Execution From Command Line

Both types of checks may also be executed from the command line to facilitate integration in automated build environemnts for continuous integration. Results are provided via result files for further processing.  

# System Requirements And Installation

Hardware requirements
---------------------

see WinCC OA online documentation (<https://www.winccoa.com/documentation/WinCCOA/3.18/en_US/GettingStarted/GettingStarted-13_2.html>)

Software requirements
---------------------

for basic requirements see WinCC OA online documentation (<https://www.winccoa.com/documentation/WinCCOA/3.18/en_US/GettingStarted/GettingStarted-13_2.html>)

python must be installed; min. V3.6

## Installation

**TO BE DEFINED ONCE WE KNOW HOW IT SHOULD BE DONE: #150074**

# How To use

## Usage in WinCC OA project

1. Create new WinCC OA Project (With DB)
2. Add following sub-project

- Subprojects\QualityCheck\SLT_QualityChecks

3. Adapt following script to find python executable

Subprojects\QualityCheck\SLT_QualityChecks\scripts\libs\classes\QualityGates\Tools\Python\Python.ctl

``` cpp
public static synchronized string getExecutable()
{
  return findExecutable("python");
}
```

4. Build ctrlppcheck and copy it from Subprojects\QualityCheck\ctrlppcheck\build\bin\Release\ctrlppcheck.exe to Subprojects\QualityCheck\SLT_QualityChecks\ctrlppcheck
5. Import Dp-List Subprojects\QualityCheck\SLT_QualityChecks\dplist\SLT_QualityChecks.dpl
6. Restart your gedi

# Quality Checks

Each of the quality checks documented below) can return a score between 0 and 100. The sum of all performed checks gives the quality score. To avoid problems with problems that e.g. do not contain panels, a score of 1 will be returned for the related exam(s).

## Available Checks

### General static check

These checks give an initial assessment of the entire folder and its files and are therefore carried out for all folder checks.

| Check per folder | Good range | Reason |
|----- | ------|-----|
| is Empty - without folders & files | FALSE | An empty folder makes the overview more difficult |
| Count of sub directories - Number of subfolders | <= 5 | More than 5 subfolders make finding it difficult |
| Count of files recursive - Number of files recursively over all subfolders | > = 0 | in empty folder structures make the clarity |
| Count of files - number of files in this folder | <= 10 | More than 10 files make finding it difficult |

| Additional Information |
|---|
| Average CCN (Cyclomatic Complexity No. McCabe-Metric) - Average complexity |
| Average NLOC (No. Lines Of Code) Average number of lines of code |
| CCN (Cyclomatic Complexity No. McCabe-Metric) - Sum of the complexity of all files in this folder |
| NLOC (No. Lines Of Code) - Sum of all lines of code |

### Images Check - static (QgStaticCheck_Pictures)

| Check per | file Good range |
|---|---|
| Size - File size | 1 MB |
| Extention file type | bmp, xpm, jpg, png, svg, jpeg, gif, mng, ico, wmf |

### Script Check - static (QgStaticCheck_Scripts)

| Check per script | Good range | Reason |
| Is an example file - Ex. File | FALSE | Ex. Scripts (under the folder Examples) are not included in the calculation |
| Is calculated - is calculated | TRUE | It can not be calculated because it eg. is encrypted. |
| NLOC code lines | 4-600 | The entire script is difficult to analyze over 600 lines |

| Check per function | Good range | Reason |
| NLOC (No. Lines Of Code) - Code Lines | 4-80 | More than 80 lines of code are difficult to understand in one function |
| Count of Parameter - Transfer parameter of function | <= 10 | More than 10 parameters make a function difficult to read |
| Count of lines - total lines | NA | This number compared to the lines of code suggests the comment style |
| CCN (Cyclomatic Complexity No. McCabe-Metric) | <= 15 | Complexity greater than 15 makes a function difficult to analyze later |

### Library check - static (QgStaticCheck_Libs)

This check is similar to the script check, but refers to the library folder.

#### Panels Check - static (QgStaticCheck_Panels)

| Check per panel | Good range | Reason |
|---|---|---|
| Is an example file - Ex. File | FALSE | Ex. Panels (under the folder examples) are not included in the calculation |
| Is calculated - is calculated | TRUE | It can not be calculated because it eg. is encrypted. |
| Is encrypted - encrypted | FALSE | Encrypted panels can not be calculated and therefore no quality statement can be made |
| Backup panel (.bak) - Backup panel available | FALSE | Backup Panel should be deleted before delivery |
| Properties NA Important properties of a panel are stored in order to provide a later function (comparison with old version) |
| Count of Shapes - Number of Graphic Elements | <= 100 | More than 100 elements complicate an analysis of the panel |
| Count of Properties - Number of Properties | NA | - |
| Count of Events - Number of Events | <= 100 | More than 100 events complicate an analysis of the panel |

| Check per event | Good range | Reason |
|---|---|---|
| Count of functions - Number of functions | 1-5 | More than 5 functions in one event are difficult to analyze |
| NLOC (No. Lines Of Code) - Code Lines | 4-600 | More than 600 lines of code are difficult to survey in an event |

| Check per function | Good range | Reason |
|---|---|---|
| NLOC (No. Lines Of Code) - Code Lines | 4-80 | More than 80 lines of code are difficult to understand in one function |
| Count of Parameter - Transfer parameter of function | <= 10 | More than 10 parameters make a function difficult to read |
| Count of lines - total lines | NA | This number compared to the lines of code suggests the comment style |
| CCN (Cyclomatic Complexity No. McCabe-Metric) | <= 15 | Complexity greater than 15 makes a function difficult to analyze later |


#### Overloaded Files Check - static (QgStaticCheck_OverloadedFiles)

The Check "Overloaded Files Check" checks if files from the product have been overwriten in the project. This overloading of files is generally possible, but carries risks when upgrading the product and should therefore be carefully considered. However, there are some files that may or may be overloaded in certain cases. These exceptions were provided in the check and are therefore allowed.  
Allowed files are:

```text
CONFIG_REL_PATH + "powerconfig"
DATA_REL_PATH + "RDBSetup / ora / RDB_config_template.sql"
SCRIPTS_REL_PATH + "userDrivers.ctl"
SCRIPTS_REL_PATH + "userPara.ctl"
LIBS_REL_PATH + "aesuser.ctl"
LIBS_REL_PATH + "asModifyDisplay.ctl"
LIBS_REL_PATH + "driverSettings_HOOK.ctl"
PANELS_REL_PATH + "vision / aes / _AS_propFilterExtended.pnl"
PANELS_REL_PATH + "vision / aes / _ES_propFilterExtended.pnl"
```

| Check | Good range |
|---|---|
|Is file overloaded - Overloaded File | FALSE <> |

SloopTools Internal Check - statisch (QgStaticCheck_Internal)
This check checks whether certain files, for SloopTools internal functions, eg. the installation completion of the add-on, are present.

| Check | Good range |
| File exists - File exists | TRUE |

### License Activation (QgApplyLicense)

This check verifies that the license check has been included in the code. The provider must use the functions from the SLTLicenseCtrlExt independently and sensibly in his code. For licensing details for the add-on, please refer to License Information for Providers. Gladly we take the time to think together in which places which license function should be used.

If no license check has been installed in an add-on, this check is set to 0 and the add-on is locked before release. This is to prevent that by a mistake of the provider his add-ons without license check in the store is available. Exceptions are the following:

Add-ons without panels, scripts and libraries (eg .: EWOs, API Manager, Picture / Graphic libraries)
Add-ons of the category "Demo"
Check per file	Good range
Count of implemented license check - number of license checks	> = 1
Syntax Check (QgSyntaxCheck)
In the check syntax, all syntax errors of the files are listed in the add-on. Only the files with syntax errors are listed.

Check per file Good range
Syntax is valid - without syntax error TRUE

## Use From Within GEDI

TDB

# Ctrlppcheck

## Available Checks

### Severities

The possible severities for messages are:  

| Severity | Description |
|---|---|
| error | used when bugs are found; Code that will lead to errors. |
| warning | suggestions about defensive programming to prevent bugs |
| style | stylistic issues related to code cleanup (unused functions redundant code, constness, and such); code is difficult to read |
| performance | Suggestions for making the code faster. These suggestions are only based on common knowledge. It is not certain you'll get any measurable difference in speed by fixing these messages. |
| portability | portability warnings. 64-bit portability. The code might work differently on different compilers. etc. |
| information | Configuration problems. The recommendation is to only enable these during configuration. |

### Suppression of Messages

If the reported problem is not a problem or is accepted, the message can be suppressed. Activate this functionality via settings - Inline Suppression.

You can specify that the inline suppression only applies to a specific symbol:

```cpp
// ctrlppcheck-suppress undefinedVariable
unknownVar = 3;
```

You can describe the reason for the suppression with a commentary. For this a semicolon (;) or two slashes (//) can be used for the separation.

```cpp
// ctrlppcheck-suppress undefinedVariable ; some comment
// ctrlppcheck-suppress undefinedVariable // some comment
```

## Use From The Command Line

Syntax:
    ctrlppcheck [OPTIONS] [files or paths]

If a directory is given instead of a filename, *.ctl files are
 checked recursively from the given directory.
 
For all options start without parameters or pass -h, --help.

Example usage to check whole WinCC OA project:
```shell
ctrlppcheck --enable=all --quiet --rule-file=rule/ctrl.xml --naming-rule-file=rule/ctrl.xml --library=cfg/ctrl.xml --suppressions-list=custom/warnings.txt --winccoa-projectName=XYZ <path-to-XYZ>\scripts
```

### Check one file

The first example is simple code in \scripts\file1.ctl

```cpp
main()
{
  functionA();
}

void functionA()
{
  string str;
  isfile(a);
}
```

Execute:

```bash
ctrlppcheck <proj_path>\scripts\file1.ctl
```

The output from CtrlPPcheck will then be:

```text
Checking <proj_path>\scripts\file1.ctl ...
debugMessage:: SymbolDatabase::isFunction found CTRL function 'main' without a return type.
[<proj_path>\scripts\file1.ctl:2]: (debug) SymbolDatabase::isFunction found CTRL function 'main' without a return type.
debugMessage:: SymbolDatabase::isFunction found CTRL function 'main' without a return type.
```

### Check all files in a folder

Normally a project has many source files and all of them should be checked. CtrlppCheck can check all source files in a directory:

```bash
ctrlppcheck <proj_path>\scripts\
```

And this will be the output

```text
Checking path/file1.cpp... 1/2 files checked 50% done
Checking path/file2.cpp... 2/2 files checked 100% done
```

### Check allfFiles in sub directory

```bash
ctrlppcheck <proj_path>\scripts\libs\classes\
```

It's not necessary to register the projects. That means you can also check source direct from a workspace.

### Excluding A File Or Folder From Checking

There are two options to exclude fiels or folders from checking:  
The **first option** is to only provide the paths and files you want to check.

```bash
ctrlppcheck src/a src/b
```

All files under src/a and src/b are then checked.  

The **second option** is to use -i, with it you specify files/paths to ignore. With this command no files in src/c are checked:

```bash
ctrlppcheck -i src/c src
```

This option does not work with the --project option and is only valid when supplying an input directory. To ignore multiple directories supply the -i switch multiple times. The following command ignores both the src/b and src/c directories.

```bash
ctrlppcheck -i src/b -i src/c
```

### Enable Messages

By default, only error messages are shown. Through the --enable switch more checks can be enabled.

enable warning messages

```bash
ctrlppcheck --enable=warning file.c
```

enable performance messages

```bash
ctrlppcheck --enable=performance file.c
```

enable information messages

```bash
ctrlppcheck --enable=information file.c 4 Getting started (command line)
```

For historical reasons, --enable=style enables warning, performance, portability and style messages. These are all reported as "style" when using the old xml format.

```bash
ctrlppcheck --enable=style file.c
```

enable warning and performance messages

```bash
ctrlppcheck --enable=warning,performance file.c
```

enable unusedFunction checking. This is not enabled by --enable=style because it doesn't work well on libraries.

```bash
ctrlppcheck --enable=unusedFunction file.c
````

enable all messages

```bash
ctrlppcheck --enable=all
```

**Please note** that --enable=unusedFunction should only be used when the whole project is scanned. Therefore, --enable=all should also only be used when the whole project is scanned. Otherwise you might get a lot of flase positives of "unused function" if all the calls are out of scope.

### Saving results in a file

**Redirection:** simply use shell redirection for piping output to a file.

```bash
ctrlppcheck file1.c 2> err.txt
```

**XML output:** Ctrlppcheck can generate output in XML format. Use --xml to enable this format. A sample command to check a file and output errors in the XML format:

```bash
ctrlppcheck --xml file1.cpp
```

Here is a sample report:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<results version="2">
  <ctrlppcheck version="1.66">
  <errors>
    <error id="someError" severity="error" msg="short error text" verbose="long error text" inconclusive="true" cwe="312">
      <location file0="file.c" file="file.h" line="1"/>
    </error>
  </errors>
</results>
```

The `<error>` element  
Each error is reported in an `<error>` element. Its attributes are: 
| Attribute | Description |
|--|--|
| id | id of error. These are always valid symbolic names. |
| severity | either: error, warning, style, performance, portability or information |
| msg | the error message in short format |
| verbose | the error message in long format. |
| inconclusive | This attribute is only used when the message is inconclusive. |
| cwe | CWE ID for the message. This attribute is only used when the CWE ID for the message is known. |

The `<location>` element  
All error locations are reported as location elemnts on the error element. The primary location is listed first. Its attributes are:  
| Attribute | Description |
|--|--|
| file | filename. Both relative and absolute paths are possible |
| file0 | name of the source file (optional) |
| line | line number in file file |
| info | short information message for each location (optional) |

## Suppress errors

### Plain text suppressions
You can suppress certain types of errors. The format for such suppression is one of:

[error id]:[filename]:[line]
[error id]:[filename2]
[error id]
The error id is the id that you want to suppress. The easiest way to get it is to use the --xml command line flag. Copy and paste the id string from the XML output. This may be * to suppress all warnings (for a specified file or files).

The filename may include the wildcard characters * or ?, which match any sequence of characters or any single character respectively. It is recommended that you use "/" as path separator on all operating systems.

### Command line suppression
The --suppress= command line option is used to specify suppressions on the command line. Example:

```bash
ctrlppcheck --suppress=memleak:src/file1.cpp src/
```

### Define suppressions in a file

Create a suppressions file.  
  
Example:  
Suppress memleak and exceptNew errors in the file src/file1.cpp  

```bash
memleak:src/file1.cpp
exceptNew:src/file1.cpp
```

suppress all uninitvar errors in all files

```bash
uninitvar
```

Note that you may add empty lines and comments in the suppressions file.  
  
Use the suppressions file.

ctrlppcheck --suppressions-list=suppressions.txt src/

XML suppressions
Suppressions may also be spcified in a XML file.  
Example file:

```xml
<?xml version="1.0"?>
<suppressions>
  <suppress>
    <id>uninitvar</id>
    <fileName>src/file1.c</fileName>
    <lineNumber>10</lineNumber>
    <symbolName>var</symbolName>
  </suppress>
</suppressions>
```

You can use the suppressions file like this:

```bash
ctrlppcheck --suppress-xml=suppressions.xml src/
```

### In code suppressions
Suppressions can also be added directly in the code by adding comments that contain special keywords. Before adding such comments, consider that the code readability is sacrificed a little.

The code 

```cpp
void main()
{
  int i;

  if ( i > 0 )
  {
    i++;
  }
}
```

will result in an error:

```text
[\scripts\file1.ctl:6]: (error) Uninitialized variable: i
```

To suppress the error message:

```cpp
void main()
{
  int i;
  // ctrlppcheck-suppress uninitvar
  if ( i > 0 )
  {
    i++;
  }
}
```

Now the --inline-suppr flag can be used to suppress the warning. No error is reported when invoking ctrlppcheck this way:

```bash
ctrlppcheck --inline-suppr file1.ctl
```

A suppression may be limited to a certain symbol. In the example below the uninitvar error is limited only to arr. Alll other findings for this error will not be suppressed

```cpp
// ctrlppcheck-suppress uninitvar symbolName=arr
```

Comments may be combined with in code suppressions.  Itis recommended to use ";" or "//" to specify where they start:

```cpp
// ctrlppcheck-suppress uninitvar ; some comment
// ctrlppcheck-suppress uninitvar // some comment
```

### Define Naming rules
**Option 1**
Run the ctrlppcheck from /bin directory.

**Option 2**
Run ctrlppcheck with option --rule-file to include rules files for naming conventions. These rules files define naming conventions for classes, functions, variables etc.. Examples can be found under the pathr: SLT_QualityChecks\data\ctrlPpCheck\rule\

use this option with the full path to the rule file:  
--rule-file=/full/path/to/rules.xml

Note: Each rule must be specified and used individually. Example:

```bash
--rule-file=/full/path/to/SLT_QualityChecks/data/ctrlPpCheck/rule/crl.xml --rule-file=/full/path/to/SLT_QualityChecks/data/ctrlPpCheck/rule/functionNaming.xml
```

See namingCheck.md for detailed explanation of this check and documentation of the rule files.

### Include lib for your project

--library=/full/path/to/SLT_QualityChecks/data/ctrlPpCheck/cfg/__proj__.xml
Include libs for your WinCC OA version

--library=/full/path/to/SLT_QualityChecks/data/ctrlPpCheck/cfg/ctrl_3.16.xml
--library=/full/path/to/SLT_QualityChecks/data/ctrlPpCheck/cfg/ctrl.xml
Note: The xml file must be adapted to the WinCC OA version used. This configuration was implemented for WinCC OA 3.16.

### Helpful options
**--platform** can be used to test for a specific operating system. Is helpful if the check is only carried out on one operating system.

Possible parameter options:

--platform=win64
--platform=unix64
--platform=unix32
--platform=native

**--inconclusive** allows Ctrlppcheck to report results even though the analysis is inconclusive. With this option there can be false positive cases. Each result must be carefully examined before you know whether it is actually a error.  
Use this if no or no more errors can be found without this option.
