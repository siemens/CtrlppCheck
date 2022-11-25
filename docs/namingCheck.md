# Ctrlpp Naming Check

## Introduction

The naming check can be used to check the naming rules of variables.  

e.g.: Hungarian notation [{prefix}] {datatype} {identifier}

```c
float fRadius;
bool bDoor;
global int g_iThreadId;
const int ICOUNT;
```

## Usage

By default, the rules provided by the add-on are used.

But it's also possible change these rules or to create your own set of rules for this check. See "[Create your own rule file](#create-your-own-rule-file)".

## Rule defintion

A rule file consists of several rule tags. Each rule consists of a pattern and a message with id and summary.

```XML
<rule version="1">
  <pattern>^[A-Z0-9_]+$</pattern>
  <message>
    <id>const</id>
    <summary>The varibale name can not consist of lowercase letters.</summary>
  </message>
</rule>
```

### tag pattern

Defines a regular expression which will be applied to the variable name. If the expression does not match, an error is returned.

| Character | Explanation (Source <https://regexr.com/>) |
|---|---|
| ^ | Beginning. Matches the beginning of the string, or the beginning of a line if the multiline flag (m) is enabled.|
| [ | Character set. Match any character in the set.|
| A - Z | Range. Matches a character in the range "A" to "Z" (char code 65 to 90). Case sensitive. |
| 0 - 9 | Range. Matches a character in the range "0" to "9" (char code 48 to 57). Case sensitive. |
| _ | Character.  Matches a "_" character (char code 95). |
| ] | End of Character set |
| + | Quantifier. Match 1 or more of the preceding token. |
| $ | End. Matches the end of the string, or the end of a line if the multiline flag (m) is enabled. |

**Examples:**
Pattern | Variable Type and Name | Result of Check
--------|---------------|-----
^[A-Z0-9_]+$| const I_VAR | <span style="color:green">**OK**</span>
^[A-Z0-9_]+$| const i_VAR | <span style="color:red">**NOK**</span>
^[A-Z0-9_]+$| const I_Va | <span style="color:red">**NOK**</span>

With these **special patterns** you can check if special information are included in the variable name. More of them will come...
Pattern | Example
--------|---------------
%fileName% | b_aesAutoRestart
%fileName_allUpper% | b_AESAutoRestart
%fileName_allLower% | b_aesAutoRestart

### tag summary

Defines the message that will be used when the variable name does not match the pattern.

### tag id

The id indicates to which variables the regular expression should be applied. Each variable has a type, is available within a specific scope and it can be modified by specific keywords like *const*.
For the check process the categories of each variable will be automatically defined by the tool.
The id is structured in following categories:

```XML
<modifier (const | nonconst)> <type (see list below)> <scope (local | argument | global)> 
```

**Example**: <id>const int argument</id>

#### category modifier

Special keyword for detailed analytics:

- const - only apply to constant variables
- nonconst - apply to all variables

#### category type

See list of supportes types in the document below

#### category scope

Scope where variable is in use

- global - Script / library or manager global e.g. global or addGlobal();
- argument - Included in a function declaration e.g. funcA(string sVar1, string sVar2, ...)
- local - All others

For categories **type** and **scope** only one value can be defined.
Either all three categories must be defined or only one.  

## Check process

1. All rules will be searched if theere is a perfect match between an variable and a certain id:  

**Example**:
Variable:

```c
const global int g_IAES_VAR
```

for a **const int** variable in a **global** scope if a specific rule is available:

```XML
<rule version="1">
  <pattern> ... </pattern>
  <message>
    <id>const int global</id>
    <summary> ... </summary>
  </message>
</rule>
```

So this rule will be applied because all 3 categories are matching.

2. If there is no rule that perfectly matches the variable declaration the defined rules for each category will be applied separately. If no rule for a certain category is found, the category will go unchecked.

**Example**
Variable

```c
const global int g_IAES_VAR
```

For a **const int** variable in **global** no specific rule is available. but there are rules for **const** and **int** variables:

```XML
<rule version="1">
  <pattern> ... </pattern>
  <message>
    <id>const</id>
    <summary> ... </summary>
  </message>
</rule>
```

```XML
<rule version="1">
 <pattern> ... </pattern>
  <message>
    <id>int</id>
    <summary> ... </summary>
 </message>
</rule>
```

So the rules for **const** and **int** will be applied to *g_IAES_VAR*.

**Attention:**  
Due to this logic, it is necessary that the single category rules can be combined with each other. Otherwise it would always come with certain combination errors.

**Some examples:**

id      | pattern                  | const int local IAES_VAR      | const int local iAES_VAR  | (nonconst) int global g_i_CountFlags | (nonconst) int global i_CountFlags |
--------|--------------------------|-------------------------------------|-------------------|-----------------------|-----------------------|
const   |^[A-Z0-9_]+$              | <span style="color:green">OK</span> | <span style="color:red">NOK</span> | ignored | ignored
int     |^(i\|I)(.*)               | <span style="color:green">OK</span> | <span style="color:green">OK</span> | <span style="color:green">OK</span> | <span style="color:green">OK</span>
float   |^(f\|F)(.*)               | ignored        | ignored | ignored | ignored
local   |"(.*)(%fileName%_)(.*)"   | <span style="color:green">OK</span> | <span style="color:green">OK</span> | ignored | ignored
global  |^(g\|G)(.*)               | ignored        | ignored | <span style="color:green">OK</span> | <span style="color:red">NOK</span>
Total check result  |          | <span style="color:green">**OK**</span> | <span style="color:red">**NOK**</span> | <span style="color:green">**OK**</span> | <span style="color:red">**NOK**</span>


## Create your own rule file

If a separate rule file is required, this can be specified with the command line option "--naming-rule-file =".

-------

## Allowed values for category type

The following values are currently available as *type* in **id**:

*variables*  | *dyn variables* | *dyn_dyn variables* | *functions* | *class names |
|--|--|--|--|--|
| anytype        | dyn_anytype  | dyn_dyn_anytype | Function_Static |Class_Name |
| atime          | dyn_atime   | dyn_dyn_atime | Function_Inline |           |
| bit32          | dyn_bit32   | dyn_dyn_bit32 | Function_Default|         |
| bit64          | dyn_bit64   | dyn_dyn_bit64 |              |           |
| blob           | dyn_blob   | dyn_dyn_bool  |             |               |
| bool           | dyn_bool   | dyn_dyn_char  |             |               |
| char           | dyn_char   | dyn_dyn_errClass |          |               |
| double         | dyn_errClass  | dyn_dyn_float |              |           |
| errClass       | dyn_float   | dyn_dyn_int  |                 |           |
| file           | dyn_int   | dyn_dyn_uint  |             |               |
| float          | dyn_uint   | dyn_dyn_long  |             |               |
| function_ptr   | dyn_long   | dyn_dyn_ulong |              |           |
| int            | dyn_ulong   | dyn_dyn_langString |           |           |
| uint           | dyn_langString |  dyn_dyn_string  |         |               |
| long           | dyn_mapping  | dyn_dyn_time  |              |               |
| ulong          | dyn_string  |               |            |               |
| langString     | dyn_time   |                  |            |               |
| mixed          | dyn_shape   |                  |            |               |
| mapping        |         |                  |               |            |
| va_list        |         |                  |               |            |
| string         |         |                  |               |            |
| time           |         |                  |               |            |
| unsigned       |         |                  |               |            |
| dbRecordset    |         |                  |               |            |
| dbConnection   |         |                  |               |            |
| dbCommand   |               |                  |               |            |
| shape    |                |                  |               |            |
| idispatch   |               |                  |               |            |
