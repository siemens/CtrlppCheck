# Ctrlpp Naming Check

## Introduction

The naming check can be used to check the naming convention of variables. 

> e.g.: Hungarian notation [{prefix}] {datatype} {identifier}
>
```c
float fRadius;
bool bDoor;
global int g_iThreadId;
const int ICOUNT;
```

## Usage

By default, the rules provided by the add-on are used.
As example we added the naming convetions set within the ETM Style Guide. (For Details, please ask your ETM representative) 

But it's also possible change this rules or to create your own rules for this check. See "[Create your own rule file](#create-your-own-rule-file)".

## Rule defintion

A rule file consists of several rule tags. Each rule consists of a pattern and a message with id and summary.

``` XML
<rule version="1">
	<pattern>^[A-Z0-9_]+$</pattern>
	<message>
		<id>const</id>
		<summary>The varibale name can not consist of lowercase letters.</summary>
	</message>
</rule>
```

### pattern

Defines a regular expression which will be applied to the variable name. If the expression does not match, an error is returned.

**Example regular expression**
``` XML
	<pattern>^[A-Z0-9_]+$</pattern>
```

**Explanation**

Example regular expression (Source https://regexr.com/)

<div class=""><code class="token exp-anchor">^</code> <b>Beginning.</b> Matches the beginning of the string, or the beginning of a line if the multiline flag (<code>m</code>) is enabled.</div>
<div class="exp-group-set"><code class="token exp-set">[</code> <b>Character set.</b> Match any character in the set.
<div class=""><code class="token exp-set"><span class="exp-char">A</span>-<span class="exp-char">Z</span></code> <b>Range.</b> Matches a character in the range "A" to "Z" (char code 65 to 90). Case sensitive.</div>
<div class=""><code class="token exp-set"><span class="exp-char">0</span>-<span class="exp-char">9</span></code> <b>Range.</b> Matches a character in the range "0" to "9" (char code 48 to 57). Case sensitive.</div>
<div><code class="token exp-char">_</code> <b>Character.</b> Matches a "_" character (char code 95). </div>
<div class="close"><code class="token exp-set">]</code> &nbsp;</div></div>
<div class="applied"><code class="token exp-quant">+</code> <b>Quantifier.</b> Match 1 or more of the preceding token.</div><div><code class="token exp-anchor">$</code> <b>End.</b> Matches the end of the string, or the end of a line if the multiline flag (<code>m</code>) is enabled.</div></div>

**Result**

Pattern | Variable Type and Name | Check
--------|---------------|-----
^[A-Z0-9_]+$| const I_VAR | <span style="color:green">**OK**</span>
^[A-Z0-9_]+$| const i_VAR | <span style="color:red">**NOK**</span>
^[A-Z0-9_]+$| const I_Va | <span style="color:red">**NOK**</span>

**Special regular expression pattern**

With these special patterns you can check if special information are included in the variable name. More of them will come...
Pattern | Example
--------|---------------
%fileName% | b_aesAutoRestart
%fileName_allUpper% | b_AESAutoRestart
%fileName_allLower% | b_aesAutoRestart

### summary

Defines the message that will be used when the regualr expression does not apply.

### id

The id indicates to which variables the regular expression should be applied.
Each variable has a type and is available to a specific scope. 
In addition, it can be modified by specific keywords like *const*.

For the check process the categories of each variable will be automatically defined by the tool.

The id is structured in following categories:

``` XML
<modifier (const | nonconst)> <type (see list below)> <scope (local | argument | global)> 

e.g. <id>const int argument</id>
```

#### modifier*
Special keyword for detailed analytics
- const
- nonconst

#### type
See list of supportes types in the document below

#### scope*
Scope where variable is in use
- global - Script / library or manager global e.g. global or addGlobal();
- argument - Included in a function declaration e.g. funcA(string sVar1, string sVar2, ...)
- local - All others

*For **type** and **scope** only one value can be defined.

Either all three categories of values must be defined or only for one. 

## Check process

1. It will be searched through the rules if there is a perfect match for an **id** 

Example for an **const** **int** variable in an **global** scope if a specific rule is available:
```c
const global int g_IAES_VAR
```

``` XML
<rule version="1">
	<pattern> ... </pattern>
	<message>
		<id>const int global</id>
		<summary> ... </summary>
	</message>
</rule>
```
>This rule will be checked because all 3 categories are matching.

2. If there is no dedicated rule found the categories will be split and the rules found for each categoriy will be checked individually. If no rule for a category is found, it wont be checked.

Examples for an **const** **int** variable in an **global** scope if no specific rule is available:
```c
const global int g_IAES_VAR
```

``` XML
<rule version="1">
	<pattern> ... </pattern>
	<message>
		<id>const</id>
		<summary> ... </summary>
	</message>
</rule>
```

``` XML
<rule version="1">
	<pattern> ... </pattern>
	<message>
		<id>int</id>
		<summary> ... </summary>
	</message>
</rule>
```

``` XML
<!-- A rule for "global" couldn't be found. -->
```
>The two existing rules are applied.

> **Attention**
>
>Due to this logic, it is necessary that the single category rules can be combined with each other. Otherwise it would always come with certain combination errors. 

**Some examples:**

ID      | Pattern                  | const int local					 | const int local   | (nonconst) int global | (nonconst) int global | 
--------|--------------------------|-------------------------------------|-------------------|-----------------------|-----------------------|
Variable Name|                     | IAES_VAR    					 	 | iAES_VAR          | g_i_CountFlags		 | i_CountFlags
const   |^[A-Z0-9_]+$              | <span style="color:green">OK</span> | <span style="color:red">NOK</span> | ignored | ignored
int     |^(i\|I)(.*)               | <span style="color:green">OK</span> | <span style="color:green">OK</span> | <span style="color:green">OK</span> | <span style="color:green">OK</span>
float   |^(f\|F)(.*)               | ignored							 | ignored | ignored | ignored
local   |"(.*)(%fileName%_)(.*)"   | <span style="color:green">OK</span> | <span style="color:green">OK</span> | ignored | ignored
global  |^(g\|G)(.*)               | ignored							 | ignored | <span style="color:green">OK</span> | <span style="color:red">NOK</span>
Total   | 						   | <span style="color:green">**OK**</span> | <span style="color:red">**NOK**</span> | <span style="color:green">**OK**</span> | <span style="color:red">**NOK**</span>


## Create your own rule file

If a separate rule file is required, this can be specified with the command line option "--naming-rule-file =".

-------

The following values are currently available for each category:

**Types:**
- anytype
- atime
- bit32
- bit64
- blob
- bool
- char
- double
- errClass
- file
- float
- function_ptr
- int
- uint
- long
- ulong
- langString
- mixed
- mapping
- va_list
- string
- time
- unsigned
- dbRecordset
- dbConnection
- dbCommand
- shape
- idispatch

*dyn variables*

- dyn_anytype
- dyn_atime
- dyn_bit32
- dyn_bit64
- dyn_blob
- dyn_bool
- dyn_char
- dyn_errClass
- dyn_float
- dyn_int
- dyn_uint
- dyn_long
- dyn_ulong
- dyn_langString
- dyn_mapping
- dyn_string
- dyn_time
- dyn_shape

*dyn_dyn variables*

- dyn_dyn_anytype
- dyn_dyn_atime
- dyn_dyn_bit32
- dyn_dyn_bit64
- dyn_dyn_bool
- dyn_dyn_char
- dyn_dyn_errClass
- dyn_dyn_float
- dyn_dyn_int
- dyn_dyn_uint
- dyn_dyn_long
- dyn_dyn_ulong
- dyn_dyn_langString
- dyn_dyn_string
- dyn_dyn_time