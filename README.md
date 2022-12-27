
WinCC OA CtrlppCheck
=====================

Introduction
------------

WinCC OA CtrlppCheck is a set of tools for static code analysis for SIMATIC WinCC OA's scripting language Ctrl and Ctrl++.  
They are easy to use and integrate perfectly with WinCC OA's script editor GEDI but may also be used in an CI toolchain for automated pull request builds etc.

Community Resources
-------------------

Project home:

- https://github.com/siemens/CtrlppCheck

Source code:

- https://github.com/siemens/CtrlppCheck.git
- git@github.com:siemens/CtrlppCheck.git

Continuous integration:

- https://github.com/siemens/CtrlppCheck/actions

- Status:
  - ![](https://github.com/siemens/CtrlppCheck/actions/workflows/main.yaml/badge.svg?branch=master) on master
  - ![](https://github.com/siemens/CtrlppCheck/actions/workflows/main.yaml/badge.svg?branch=next) on next

- For the Ctrl language definition the tool depends on an installed WinCC OA version. Released major versions of WinCC OA are marked in our source code history using tags with syntax WinCC_OA_X_Y, e.g. WinCC_OA_3_19.

user documentation:

- [Link](docs/user.md)

technical documentation:

- [Link](docs/contributor.md)

License
-------

The CtrlppCheck is primarily licensed under the terms of the GNU
General Public License version 3.

Each of its source code files contains a license declaration in its header.
Whenever a file is provided under an additional or different license than
GPLv3, this is stated in the file header. Any file that may lack such a
header has to be considered licensed under GPLv3 (default license).

If two licenses are specified in a file header, you are free to pick the one
that suits best your particular use case. You can also continue to use the
file under the dual license. When choosing only one, remove the reference to
the other from the file header.

Copyright declarations are stated in regarding files separately.

License Usage
-------------

The default license GPLv3 shall be used unless valid reasons are provided for a
deviation.

License Header Format
---------------------

Use the following template (replacing comment markers as required) when
creating a new file in the CtrlppCheck project:

```
/*
 * CtrlppCheck, a static code analyser for WinCC OA's Ctrl language
 *
 * Copyright (c) <COPYRIGHT HOLDER>, <YEAR>
 *
 * Authors:
 *  Your Name <your.email@host.dom>
 *
 * This work is licensed under the terms of the GNU GPL, version 3.  See
 * the COPYING file in the top-level directory.
 */
```

When applying a dual GPL/BSD license, append the following to the above:

```
 ...
 *
 * Alternatively, you can use or redistribute this file under the following
 * BSD license:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */
```
