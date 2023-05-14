# CtrlPPcheck

## Introduction

CtrlPPcheck is an analysis tool for ctrl/ctrl++ code. It based on cppCheck and developed for CTRL code. It detects the types of bugs that the WinCC OA syntax check normally fail to detect. The goal is no false positives. 

### Supported platforms:
+ CtrlPPcheck should work on any platform that has sufficient CPU and memory.

Please understand that there are limits of CtrlPPcheck. CtrlPPcheck is rarely wrong about reported errors. But there are many bugs that it doesn't detect. You will find more bugs in your software by testing your software carefully, than by using CtrlPPcheck. You will find more bugs in your software by dynamic tests, than by using CtrlPPcheck. But CtrlPPcheck can still detect some of the bugs that you miss when testing your software.

## Getting started (GUI)
 TBD
 
## Getting started (command line)

### Checkin one file
The first test is a simple code 
```
main()
{
  foo();
}

void foo()
{
  string str;
  isfile(a);
}
```

If you save that into <proj_path>\scripts\file1.ctl and execute: 

```
cppcheck <proj_path>\scripts\file1.ctl 
```
The output from CtrlPPcheck will then be: 

```
Checking <proj_path>\scripts\file1.ctl ...
debugMessage:: SymbolDatabase::isFunction found CTRL function 'main' without a return type.
[<proj_path>\scripts\file1.ctl:2]: (debug) SymbolDatabase::isFunction found CTRL function 'main' without a return type.
debugMessage:: SymbolDatabase::isFunction found CTRL function 'main' without a return type.
```

### Checking all files in a folder

Normally a program has many source files. And you want to check them all. CtrlPPcheck can check all source files in a directory: 

The following command can recursively check all scripts located in project directory. 
```
cppcheck <proj_path>\scripts\
```
And this will be the output
```
Checking path/file1.cpp... 1/2 files checked 50% done
Checking path/file2.cpp... 2/2 files checked 100% done
```

Check allfFiles in sub directory
```
cppcheck <proj_path>\scripts\libs\classes\
```

> It's not necessary to register the projects. That means you can also check source direct from a workspace.

### Recursive vs. manual check

Checking files manually gives you better control of the analysis.

We don't know which approach will give you the best results.
It is recommended that you try both. It is possible that you will get different results so that to find most bugs you need to use both approaches.

Later chapters will describe this in more detail.

### Excluding a file or folder from checking

To exclude a file or folder, there are two options.
The first option is to only provide the paths and files you want to check. 
```
cppcheck src/a src/b 
```
All files under *src/a* and *src/b* are then checked.

The second option is to use *-i*, with it you specify files/paths to ignore. With this command no files in *src/c* are checked: 
```
cppcheck -i src/c src
```
This option does not currently work with the *--project* option and is only valid when supplying an input directory. To ignore multiple directories supply the *-i* multiple times. The following command ignores both the *src/b* and *src/c* directories. 
```
cppcheck -i src/b -i src/c
```

### Severities
The possible severities for messages are: 
|Severity| Description |
|--|--|
|error | used when bugs are found |
|warning | suggestions about defensive programming to prevent bugs |
|style | stylistic issues related to code cleanup (unused functions redundant code, constness, and such) |
|performance | Suggestions for making the code faster. These suggestions are only based on common knowledge. It is not certain you'll get any measurable difference in speed by fixing these messages. |
|portability | portability warnings. 64-bit portability. The code might work differently on different compilers. etc. |
|information | Configuration problems. The recommendation is to only enable these during configuration.|

### Enable messages

By default, only *error* messages are shown. Through the *--enable* command more checks can be enabled.

 - enable warning messages 
 ```
 cppcheck --enable=warning file.c 
 ```
 - enable performance messages 
 ```
 cppcheck --enable=performance file.c 
 ```
 - enable information messages 
 ```
 cppcheck --enable=information file.c 4 Getting started (command line) 
 ```
 - For historical reasons, *--enable=style* enables warning, performance, portability and style messages.
   These are all reported as "style" when using the old xml format.  
 ```
 cppcheck --enable=style file.c 
 ```
 - enable warning and performance messages 
 ```
 cppcheck --enable=warning,performance file.c 
 ```
 - enable unusedFunction checking. This is not enabled by --enable=style because it doesn't work well on libraries.
 ```
 cppcheck --enable=unusedFunction file.c 
 ```
 - enable all messages
 ```
 cppcheck --enable=all
 ```
 
 ---
 Please note that *--enable=unusedFunction* should only be used when the whole program is scanned.
 Therefore, *--enable=all* should also only be used when the whole program is scanned. The reason is that the unused function
 check will warn if a function is not called. There will be noise if function calls are not seen.

### Saving results in a file
Many times you will want to save the results in a file. You can use the normal shell redirection for piping error output to a file. 
```
cppcheck file1.c 2> err.txt
```

## XML output
CtrlPPcheck can generate output in XML format.
Use --xml to enable this format. A sample command to check a file and output errors in the XML format: 
```
cppcheck --xml file1.cpp
```
Here is a sample report: 
```
<?xml version="1.0" encoding="UTF-8"?>
<results version="2">
  <cppcheck version="1.66">
  <errors>
  <error id="someError" severity="error" msg="short error text"
  verbose="long error text" inconclusive="true" cwe="312">
  <location file0="file.c" file="file.h" line="1"/>
  </error>
  </errors>
</results>
```
### The \<error\> element

Each error is reported in a \<error\> element. Attributes: 
| Attribute | Description |
|--|--|
| id | id of error. These are always valid symbolic names.  |
| severity | either: error, warning, style, performance, portability or information |
| msg | the error message in short format |
|verbose | the error message in long format. 
| inconclusive | This attribute is only used when the message is inconclusive. |
| cwe | CWE ID for the message. This attribute is only used when the CWE ID for the message is known. |

### The \<location>\ element

All locations related to an error is listed with \<location\> elements. The primary location is listed first. Attributes: 
| Attribute | Description |
|--|--|
| file | filename. Both relative and absolute paths are possible|
| file0 | name of the source file (optional)|
| line | a number|
 |info | short information message for each location (optional)|

## Suppressions
If you want to filter out certain errors you can suppress these.

### Plain text suppressions
You can suppress certain types of errors.
The format for such suppression is one of: 
```
[error id]:[filename]:[line]
[error id]:[filename2]
[error id] 
```
The error id is the id that you want to suppress. The easiest way to get it is to use the --xml command line flag. Copy and paste the id string from the XML output. This may be * to suppress all warnings (for a specified file or files). 

The filename may include the wildcard characters * or ?, which match any sequence of characters or any single character respectively. It is recommended that you use "/" as path separator on all operating systems.

### Command line suppression
The *--suppress=* command line option is used to specify suppressions on the command line. Example:
```
cppcheck --suppress=memleak:src/file1.cpp src/
```

### Listing suppressions in a file
You can create a suppressions file. Example: 
- suppress memleak and exceptNew errors in the file src/file1.cpp memleak:src/file1.cpp
```
exceptNew:src/file1.cpp
```
 - suppress all uninitvar errors in all files
```
 uninitvar
 ```
  Note that you may add empty lines and comments in the suppressions file. You can use the suppressions file like this:
  ```
  cppcheck --suppressions-list=suppressions.txt src/
  ```
### XML suppressions
You can specify suppressions in a XML file. Example file:
```
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
```
cppcheck --suppress-xml=suppressions.xml src/
```

### Inline suppressions

Suppressions can also be added directly in the code by adding comments that contain special keywords.
Before adding such comments, consider that the code readability is sacrificed a little.

This code will normally generate an error message:

```
void main()
{
  int i;
  
  if ( i > 0 )
  {
    i++;
  }
}
```

The output is:
```
[\scripts\file1.ctl:6]: (error) Uninitialized variable: i
```

To suppress the error message, a comment can be added:

```
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

Now the --inline-suppr flag can be used to suppress the warning. No error is reported when invoking cppcheck this way:

```
cppcheck --inline-suppr file1.ctl
```

you can specify that the inline suppression only applies to a specific symbol:

```
// ctrlppcheck-suppress uninitvar symbolName=arr
```

You can write comments for the suppress, however, is recommended to use ; or // to specify where they start:
```
// ctrlppcheck-suppress uninitvar ; some comment
// ctrlppcheck-suppress uninitvar // some comment
```

## Library configuration

TBD

## Rules

TBD
