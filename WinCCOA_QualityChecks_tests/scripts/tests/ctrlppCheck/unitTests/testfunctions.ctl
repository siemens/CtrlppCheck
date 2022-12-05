#uses "classes/QualityGates/Tools/CppCheck/TestFixture"

/*
 * Cppcheck - A tool for static C/C++ code analysis
 * Copyright (C) 2007-2019 Cppcheck team.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// #include <tinyxml2.h>

// #include "checkfunctions.h"
// #include "library.h"
// #include "settings.h"
// #include "standards.h"
// #include "testsuite.h"
// #include "tokenize.h"


struct TestFunctions : public TestFixture
{
// public:
//     TestFunctions() : TestFixture("TestFunctions") {
//     }

// private:
//     Settings settings;

  void run()
  {
    settings.addEnabled("style");
    settings.addEnabled("warning");
    settings.addEnabled("portability");
//         settings.standards.posix = true;
//         settings.standards.c = Standards::C11;
//         settings.standards.cpp = Standards::CPP11;
//         settings.addLibraryFile("std.cfg");
//         LOAD_LIB_2(settings.library, "std.cfg");
//         settings.addLibraryFile("posix.cfg");
//         LOAD_LIB_2(settings.library, "posix.cfg");

    // Prohibited functions
    TEST_CASE(prohibitedFunctions_posix);
    TEST_CASE(prohibitedFunctions_posix_WinCC_OA);

    TEST_CASE(prohibitedFunctions_index);
    TEST_CASE(prohibitedFunctions_qt_index); // FP when using the Qt function 'index'?
    TEST_CASE(prohibitedFunctions_rindex);
    TEST_CASE(prohibitedFunctions_var); // no false positives for variables
    TEST_CASE(prohibitedFunctions_gets); // dangerous function
    TEST_CASE(prohibitedFunctions_alloca);
    TEST_CASE(prohibitedFunctions_declaredFunction); // declared function ticket #3121
    TEST_CASE(prohibitedFunctions_std_gets); // test std::gets

    TEST_CASE(prohibitedFunctions_multiple); // multiple use of obsolete functions

    TEST_CASE(prohibitedFunctions_c_declaration); // c declared function
    TEST_CASE(prohibitedFunctions_functionWithBody); // function with body
    TEST_CASE(prohibitedFunctions_crypt); // Non-reentrant function
    TEST_CASE(prohibitedFunctions_crypt_WinCC_OA); // Non-reentrant function

    TEST_CASE(prohibitedFunctions_namespaceHandling);
/// @todo clearify followin functions
    // Invalid function usage
    TEST_CASE(invalidFunctionUsageStrings);

    // Math function usage
    TEST_CASE(mathfunctionCall_fmod);
    TEST_CASE(mathfunctionCall_sqrt);
    TEST_CASE(mathfunctionCall_log);
    TEST_CASE(mathfunctionCall_acos);
    TEST_CASE(mathfunctionCall_asin);
    TEST_CASE(mathfunctionCall_pow);
    TEST_CASE(mathfunctionCall_atan2);
    TEST_CASE(mathfunctionCall_precision);

    // Ignored return value
    TEST_CASE(checkIgnoredReturnValue);

    // memset..
    TEST_CASE(memsetZeroBytes);
    TEST_CASE(memsetInvalid2ndParam);
  }

//     void check(const char code[], const char filename[]="test.cpp", const Settings* settings_=nullptr) {
  // Clear the error buffer..
//         errout.str("");
//
//         if (!settings_)
//             settings_ = &settings;
//
  // Tokenize..
//         Tokenizer tokenizer(settings_, this);
//         std::istringstream istr(code);
//         tokenizer.tokenize(istr, filename);
//
//         CheckFunctions checkFunctions(&tokenizer, settings_, this);
//         checkFunctions.runChecks(&tokenizer, settings_, this);
//
  // Simplify...
//         tokenizer.simplifyTokenList2();
//
  // Check...
//         checkFunctions.runSimplifiedChecks(&tokenizer, settings_, this);
//     }

  void prohibitedFunctions_posix()
  {
    check("void f()\n"
          "{\n"
          "    bsd_signal(SIGABRT, SIG_IGN);\n"
          "}");
    ASSERT_EQUALS("[scripts/test.ctl:3]: (warning) Undefined variable: SIGABRT\n"
                  "[scripts/test.ctl:3]: (warning) Undefined variable: SIG_IGN\n",
                  errout.str());

    check("int f()\n"
          "{\n"
          "    int bsd_signal(0);\n"
          "    return bsd_signal;\n"
          "}");
    ASSERT_EQUALS("WinCC OA syntax error at pos: 28", errout.str());

    check("void f()\n"
          "{\n"
          "    struct hostent *hp;\n"
          "    if(!hp = gethostbyname(\"127.0.0.1\")) {\n"
          "        exit(1);\n"
          "    }\n"
          "}");
    ASSERT_EQUALS("WinCC OA syntax error at pos: 15", errout.str());
//         ASSERT_EQUALS("[test.cpp:4]: (style) Obsolescent function 'gethostbyname' called. It is recommended to use 'getaddrinfo' instead.\n", errout.str());

    check("void f()\n"
          "{\n"
          "    long addr;\n"
          "    addr = inet_addr(\"127.0.0.1\");\n"
          "    if(!hp = gethostbyaddr((char *) &addr, sizeof(addr), AF_INET)) {\n"
          "        exit(1);\n"
          "    }\n"
          "}");
    ASSERT_EQUALS("WinCC OA syntax error at pos: 116", errout.str());
//         ASSERT_EQUALS("[test.cpp:5]: (style) Obsolescent function 'gethostbyaddr' called. It is recommended to use 'getnameinfo' instead.\n", errout.str());

    check("void f()\n"
          "{\n"
          "    usleep( 1000 );\n"
          "}");
    ASSERT_EQUALS("", errout.str());
  }

  void prohibitedFunctions_posix_WinCC_OA()
  {

    const string oaRule = getPath(DATA_REL_PATH, "ctrlPpCheck/rule/prohibitedFunctions.xml");
    settings.addRuleFile(oaRule);


    check("int main(int p)\n"
          "{\n"
          "  bool b = isMotif();\n"
          "  DebugN(b);\n"
          "}");
    ASSERT_EQUALS("[scripts/test.ctl:3]: (style) Obsolescent function 'isMotif' called. It is recommended to use '' instead.\n", errout.str());

    check("int f()\n"
          "{\n"
          "    bool isMotif = false;\n"
          "    return isMotif;\n"
          "}");
    ASSERT_EQUALS("", errout.str());

    check("void f()\n"
          "{\n"
          "    if( \"\" == gethostbyname(\"127.0.0.1\")) {\n"
          "        exit(1);\n"
          "    }\n"
          "}");
    ASSERT_EQUALS("", errout.str());
    //settings.unloadLibrary(oaLib);

    check("void f()\n"
          "{\n"
          "    if( \"\" == getHostByName(\"127.0.0.1\")) {\n"
          "        exit(1);\n"
          "    }\n"
          "}");
    ASSERT_EQUALS("", errout.str());
    //settings.unloadLibrary(oaLib);
  }

  void prohibitedFunctions_index()
  {
    check("namespace n1 {\n"
          "    int index(){};\n"
          "}\n"
          "int main()\n"
          "{\n"
          "    n1::index();\n"
          "}");
    ASSERT_EQUALS("WinCC OA syntax error at pos: 0", errout.str());
//         ASSERT_EQUALS("", errout.str());

    check("std::size_t f()\n"
          "{\n"
          "    std::size_t index(0);\n"
          "    index++;\n"
          "    return index;\n"
          "}");
    ASSERT_EQUALS("WinCC OA syntax error at pos: 3", errout.str());
//         ASSERT_EQUALS("", errout.str());

    check("int f()\n"
          "{\n"
          "    return this->index();\n"
          "}");
    ASSERT_EQUALS("WinCC OA syntax error at pos: 25", errout.str());
//         ASSERT_EQUALS("", errout.str());

    check("void f()\n"
          "{\n"
          "    int index( 0 );\n"
          "}");
    ASSERT_EQUALS("WinCC OA syntax error at pos: 24", errout.str());
//         ASSERT_EQUALS("", errout.str());

    check("const char f()\n"
          "{\n"
          "    const char var[6] = \"index\";\n"
          "    const char i = index(var, 0);\n"
          "    return i;\n"
          "}");
    ASSERT_EQUALS("WinCC OA syntax error at pos: 35", errout.str());
//         ASSERT_EQUALS("[test.cpp:4]: (style) Obsolescent function 'index' called. It is recommended to use 'strchr' instead.\n",
//                       errout.str());
  }

  void prohibitedFunctions_qt_index()
  {
    check("void TDataModel::forceRowRefresh(int row) {\n"
          "    emit dataChanged(index(row, 0), index(row, columnCount() - 1));\n"
          "}");
    ASSERT_EQUALS("WinCC OA syntax error at pos: 15", errout.str());
//         ASSERT_EQUALS("[test.cpp:2]: (style) Obsolescent function 'index' called. It is recommended to use 'strchr' instead.\n", errout.str());
  }

  void prohibitedFunctions_rindex()
  {
    check("void f()\n"
          "{\n"
          "    int rindex( 0 );\n"
          "}");
    ASSERT_EQUALS("WinCC OA syntax error at pos: 25", errout.str());
//         ASSERT_EQUALS("", errout.str());

    check("void f()\n"
          "{\n"
          "    const char var[7] = \"rindex\";\n"
          "    print(rindex(var, 0));\n"
          "}");
    ASSERT_EQUALS("WinCC OA syntax error at pos: 29", errout.str());
//         ASSERT_EQUALS("[test.cpp:4]: (style) Obsolescent function 'rindex' called. It is recommended to use 'strrchr' instead.\n", errout.str());
  }


  void prohibitedFunctions_var()
  {
    check("class Fred {\n"
          "public:\n"
          "    Fred() : index(0) { }\n"
          "    int index;\n"
          "};");
    ASSERT_EQUALS("WinCC OA syntax error at pos: 19", errout.str());
//         ASSERT_EQUALS("", errout.str());
  }

  void prohibitedFunctions_gets()
  {
    check("void f()\n"
          "{\n"
          "    char *x = gets(a);\n"
          "}");
    ASSERT_EQUALS("WinCC OA syntax error at pos: 20", errout.str());
//         ASSERT_EQUALS("[test.cpp:3]: (warning) Obsolete function 'gets' called. It is recommended to use 'fgets' or 'gets_s' instead.\n", errout.str());

    check("void f()\n"
          "{\n"
          "    foo(x, gets(a));\n"
          "}");
    ASSERT_EQUALS("[scripts/test.ctl:3]: (warning) Undefined variable: x\n"
                  "[scripts/test.ctl:3]: (warning) Undefined variable: a\n",
                  errout.str());
  }

  void prohibitedFunctions_alloca()
  {
    check("void f()\n"
          "{\n"
          "    char *x = alloca(10);\n"
          "}", "test.cpp");  // #4382 - there are no VLAs in C++
    ASSERT_EQUALS("WinCC OA syntax error at pos: 20", errout.str());
//         ASSERT_EQUALS("[test.cpp:3]: (warning) Obsolete function 'alloca' called.\n", errout.str());

    check("void f()\n"
          "{\n"
          "    char *x = alloca(10);\n"
          "}", "test.c");
    ASSERT_EQUALS("WinCC OA syntax error at pos: 20", errout.str());
//         ASSERT_EQUALS("[test.c:3]: (warning) Obsolete function 'alloca' called. In C99 and later it is recommended to use a variable length array instead.\n", errout.str());

//         settings.standards.c = Standards::C89;
//         settings.standards.cpp = Standards::CPP03;
    check("void f()\n"
          "{\n"
          "    char *x = alloca(10);\n"
          "}", "test.cpp");  // #4382 - there are no VLAs in C++
    ASSERT_EQUALS("WinCC OA syntax error at pos: 20", errout.str());
//         ASSERT_EQUALS("", errout.str());

    check("void f()\n"
          "{\n"
          "    char *x = alloca(10);\n"
          "}", "test.c"); // #7558 - no alternative to alloca in C89
    ASSERT_EQUALS("WinCC OA syntax error at pos: 20", errout.str());
//         ASSERT_EQUALS("", errout.str());

    check("void f()\n"
          "{\n"
          "    char *x = alloca(10);\n"
          "}", "test.c");
    ASSERT_EQUALS("WinCC OA syntax error at pos: 20", errout.str());
//         ASSERT_EQUALS("", errout.str());
//         settings.standards.c = Standards::C11;
//         settings.standards.cpp = Standards::CPP11;
  }

  // ticket #3121
  void prohibitedFunctions_declaredFunction()
  {
    check("int ftime ( int a )\n"
          "{\n"
          "    return a;\n"
          "}\n"
          "int main ()\n"
          "{\n"
          "    int b ; b = ftime ( 1 ) ;\n"
          "    return 0 ;\n"
          "}");
    ASSERT_EQUALS("[scripts/test.ctl:7]: (warning) Variable is assigned a value that is never used: 'b'\n", errout.str());
  }

  // test std::gets
  void prohibitedFunctions_std_gets()
  {
    check("void f(char * str)\n"
          "{\n"
          "    char *x = std::gets(str);\n"
          "    char *y = gets(str);\n"
          "}");
    ASSERT_EQUALS("WinCC OA syntax error at pos: 12", errout.str());
//         ASSERT_EQUALS("[test.cpp:3]: (warning) Obsolete function 'gets' called. It is recommended to use 'fgets' or 'gets_s' instead.\n"
//                       "[test.cpp:4]: (warning) Obsolete function 'gets' called. It is recommended to use 'fgets' or 'gets_s' instead.\n", errout.str());
  }

  // multiple use
  void prohibitedFunctions_multiple()
  {
    check("void f(char * str)\n"
          "{\n"
          "    char *x = std::gets(str);\n"
          "    usleep( 1000 );\n"
          "}");
    ASSERT_EQUALS("WinCC OA syntax error at pos: 12", errout.str());
//         ASSERT_EQUALS("[test.cpp:3]: (warning) Obsolete function 'gets' called. It is recommended to use 'fgets' or 'gets_s' instead.\n"
//                       "[test.cpp:4]: (style) Obsolescent function 'usleep' called. It is recommended to use 'nanosleep' or 'setitimer' instead.\n", errout.str());
  }

  void prohibitedFunctions_c_declaration()
  {
    check("char * gets ( char * c ) ;\n"
          "int main ()\n"
          "{\n"
          "    char s [ 10 ] ;\n"
          "    gets ( s ) ;\n"
          "}");
    ASSERT_EQUALS("WinCC OA syntax error at pos: 5", errout.str());
//         ASSERT_EQUALS("[test.cpp:5]: (warning) Obsolete function 'gets' called. It is recommended to use 'fgets' or 'gets_s' instead.\n", errout.str());

    check("int getcontext(ucontext_t *ucp);\n"
          "int f (ucontext_t *ucp)\n"
          "{\n"
          "    getcontext ( ucp ) ;\n"
          "}");
    ASSERT_EQUALS("WinCC OA syntax error at pos: 15", errout.str());
//         ASSERT_EQUALS("[test.cpp:4]: (portability) Obsolescent function 'getcontext' called. Applications are recommended to be rewritten to use POSIX threads.\n", errout.str());
  }

  void prohibitedFunctions_functionWithBody()
  {
    check("char * gets ( char * c ) { return c; }\n"
          "int main ()\n"
          "{\n"
          "    char s [ 10 ] ;\n"
          "    gets ( s ) ;\n"
          "}");
    ASSERT_EQUALS("WinCC OA syntax error at pos: 5", errout.str());
//         ASSERT_EQUALS("", errout.str());
  }

  void prohibitedFunctions_crypt()
  {
    check("void f(char *pwd)\n"
          "{\n"
          "    char *cpwd;"
          "    crypt(pwd, cpwd);\n"
          "}");
    ASSERT_EQUALS("WinCC OA syntax error at pos: 12", errout.str());
//         ASSERT_EQUALS("[test.cpp:3]: (warning) Return value of function crypt() is not used.\n"
//                       "[test.cpp:3]: (portability) Non reentrant function 'crypt' called. For threadsafe applications it is recommended to use the reentrant replacement function 'crypt_r'.\n", errout.str());

    check("void f()\n"
          "{\n"
          "    char *pwd = getpass(\"Password:\");"
          "    char *cpwd;"
          "    crypt(pwd, cpwd);\n"
          "}");
    ASSERT_EQUALS("WinCC OA syntax error at pos: 57", errout.str());
//         ASSERT_EQUALS("[test.cpp:3]: (warning) Return value of function crypt() is not used.\n"
//                       "[test.cpp:3]: (portability) Non reentrant function 'crypt' called. For threadsafe applications it is recommended to use the reentrant replacement function 'crypt_r'.\n", errout.str());

    check("int f()\n"
          "{\n"
          "    int crypt = 0;"
          "    return crypt;\n"
          "}");
    ASSERT_EQUALS("", errout.str());
  }

  // @todo crypt is laos wincc oa  function and this can not match
  void prohibitedFunctions_crypt_WinCC_OA()
  {
    check("void f(char pwd)\n"
          "{\n"
          "    char cpwd = crypt(pwd);\n"
          "}");
    ASSERT_EQUALS("[scripts/test.ctl:3]: (warning) Variable is assigned a value that is never used: 'cpwd'\n", errout.str());
  }

  void prohibitedFunctions_namespaceHandling()
  {
    check("int f()\n"
          "{\n"
          "    time_t t = 0;"
          "    std::localtime(&t);\n"
          "}");
    ASSERT_EQUALS("WinCC OA syntax error at pos: 36", errout.str());
//         ASSERT_EQUALS("[test.cpp:3]: (portability) Non reentrant function 'localtime' called. For threadsafe applications it is recommended to use the reentrant replacement function 'localtime_r'.\n", errout.str());

    // Pass return value
    check("int f()\n"
          "{\n"
          "    time_t t = 0;"
          "    struct tm *foo = localtime(&t);\n"
          "}");
    ASSERT_EQUALS("WinCC OA syntax error at pos: 46", errout.str());
//         ASSERT_EQUALS("[test.cpp:3]: (portability) Non reentrant function 'localtime' called. For threadsafe applications it is recommended to use the reentrant replacement function 'localtime_r'.\n", errout.str());

    // Access via global namespace
    check("int f()\n"
          "{\n"
          "    ::getpwent();\n"
          "}");
    ASSERT_EQUALS("WinCC OA syntax error at pos: 14", errout.str());
//         ASSERT_EQUALS("[test.cpp:3]: (warning) Return value of function getpwent() is not used.\n"
//                       "[test.cpp:3]: (portability) Non reentrant function 'getpwent' called. For threadsafe applications it is recommended to use the reentrant replacement function 'getpwent_r'.\n", errout.str());

    // Be quiet on function definitions
    check("int getpwent()\n"
          "{\n"
          "    return 123;\n"
          "}");
    ASSERT_EQUALS("", errout.str());

    // Be quiet on other namespaces
    check("int f()\n"
          "{\n"
          "    foobar::getpwent();\n"
          "}");
    ASSERT_EQUALS("WinCC OA syntax error at pos: 22", errout.str());
//         ASSERT_EQUALS("", errout.str());

    check("struct foobar{static void getpwent(){}};\n"
          "int f()\n"
          "{\n"
          "    foobar::getpwent();\n"
          "}");
    ASSERT_EQUALS("[scripts/test.ctl:1]: (warning, inconclusive) The scope is empty: 'getpwent'\n", errout.str());

    // Be quiet on class member functions
    check("struct Foobar{static void getpwent(){}};\n"
          "Foobar foobar;\n"
          "int f()\n"
          "{\n"
          "    foobar.getpwent();\n"
          "}");
    ASSERT_EQUALS("[scripts/test.ctl:1]: (warning, inconclusive) The scope is empty: 'getpwent'\n", errout.str());
  }

  void mathfunctionCall_sqrt()
  {
    // sqrt
    check("void foo()\n"
          "{\n"
          "      DebugN(sqrt(-1));\n"
          "}");
    ASSERT_EQUALS("[scripts/test.ctl:3]: (error) Invalid sqrt() argument nr 1. The value is -1 but the valid values are '0.0:'.\n", errout.str());

    // implementation-defined behaviour for "finite values of x<0" only:
    check("void foo()\n"
          "{\n"
          "    DebugN(sqrt(-0.));\n"
          "}");
    ASSERT_EQUALS("", errout.str());

    check("void foo()\n"
          "{\n"
          "      DebugN(sqrt(1));\n"
          "}");
    ASSERT_EQUALS("", errout.str());
  }

  void mathfunctionCall_log()
  {
    // log,log10
    check("void foo()\n"
          "{\n"
          "      DebugN(log(-2));\n"
          "      DebugN(log10(-2));\n"
          "}");
    ASSERT_EQUALS("[scripts/test.ctl:3]: (warning) Passing value -2 to log() leads to implementation-defined result.\n"
                  "[scripts/test.ctl:4]: (warning) Passing value -2 to log10() leads to implementation-defined result.\n",
                  errout.str());

    check("void foo()\n"
          "{\n"
          "      DebugN(log(-1));\n"
          "      DebugN(log10(-1));\n"
          "}");
    ASSERT_EQUALS("[scripts/test.ctl:3]: (warning) Passing value -1 to log() leads to implementation-defined result.\n"
                  "[scripts/test.ctl:4]: (warning) Passing value -1 to log10() leads to implementation-defined result.\n",
                  errout.str());

    check("void foo()\n"
          "{\n"
          "      DebugN(log(-1.0));\n"
          "      DebugN(log10(-1.0));\n"
          "}");
    ASSERT_EQUALS("[scripts/test.ctl:3]: (warning) Passing value -1.0 to log() leads to implementation-defined result.\n"
                  "[scripts/test.ctl:4]: (warning) Passing value -1.0 to log10() leads to implementation-defined result.\n",
                  errout.str());

    check("void foo()\n"
          "{\n"
          "      DebugN(log(-0.1));\n"
          "      DebugN(log10(-0.1));\n"
          "}");
    ASSERT_EQUALS("[scripts/test.ctl:3]: (warning) Passing value -0.1 to log() leads to implementation-defined result.\n"
                  "[scripts/test.ctl:4]: (warning) Passing value -0.1 to log10() leads to implementation-defined result.\n",
                  errout.str());

    check("void foo()\n"
          "{\n"
          "      DebugN(log(0));\n"
          "      DebugN(log10(0.0));\n"
          "}");
    ASSERT_EQUALS("[scripts/test.ctl:3]: (warning) Passing value 0 to log() leads to implementation-defined result.\n"
                  "[scripts/test.ctl:4]: (warning) Passing value 0.0 to log10() leads to implementation-defined result.\n",
                  errout.str());

    check("void foo()\n"
          "{\n"
          "    DebugN(log(1E-3));\n"
          "    DebugN(log10(1E-3));\n"
          "    DebugN(log(1.0E-3));\n"
          "    DebugN(log10(1.0E-3));\n"
          "    DebugN(log(1.0E+3));\n"
          "    DebugN(log10(1.0E+3));\n"
          "    DebugN(log(2.0));\n"
          "    DebugN(log10(2.0));\n"
          "}");
    ASSERT_EQUALS("", errout.str());
  }

  void mathfunctionCall_acos()
  {
    // acos
    check("float foo()\n"
          "{\n"
          " return acos(-1)      \n"
          "    + acos(0.1)       \n"
          "    + acos(0.0001)    \n"
          "    + acos(0.01)      \n"
          "    + acos(1.0E-1)    \n"
          "    + acos(-1.0E-1)   \n"
          "    + acos(+1.0E-1)   \n"
          "    + acos(0.1E-1)    \n"
          "    + acos(+0.1E-1)   \n"
          "    + acos(-0.1E-1);   \n"
          "}");
    ASSERT_EQUALS("", errout.str());

    check("void foo()\n"
          "{\n"
          "    DebugN(acos(1.1));\n"
          "}");
    ASSERT_EQUALS("[scripts/test.ctl:3]: (error) Invalid acos() argument nr 1. The value is 1.1 but the valid values are '-1.0:1.0'.\n", errout.str());

    check("void foo()\n"
          "{\n"
          "    DebugN(acos(-1.1));\n"
          "}");
    ASSERT_EQUALS("[scripts/test.cptl:3]: (error) Invalid acos() argument nr 1. The value is -1.1 but the valid values are '-1.0:1.0'.\n", errout.str());
  }

  void mathfunctionCall_asin()
  {
    // asin
    check("float foo()\n"
          "{\n"
          " return asin(1)       \n"
          "    + asin(-1)        \n"
          "    + asin(0.1)       \n"
          "    + asin(0.0001)    \n"
          "    + asin(0.01)      \n"
          "    + asin(1.0E-1)    \n"
          "    + asin(-1.0E-1)   \n"
          "    + asin(+1.0E-1)   \n"
          "    + asin(0.1E-1)    \n"
          "    + asin(+0.1E-1)   \n"
          "    + asin(-0.1E-1);   \n"
          "}");
    ASSERT_EQUALS("", errout.str());

    check("void foo()\n"
          "{\n"
          "    DebugN(asin(1.1));\n"
          "}");
    ASSERT_EQUALS("[scripts/test.cptl:3]: (error) Invalid asin() argument nr 1. The value is 1.1 but the valid values are '-1.0:1.0'.\n", errout.str());

    check("void foo()\n"
          "{\n"
          "    DebugN(asin(-1.1));\n"
          "}");
    ASSERT_EQUALS("[scripts/test.ctl:3]: (error) Invalid asin() argument nr 1. The value is -1.1 but the valid values are '-1.0:1.0'.\n", errout.str());
  }

  void mathfunctionCall_pow()
  {
    // pow
    check("void foo()\n"
          "{\n"
          "    DebugN(pow(0,-10));\n"
          "}");
    ASSERT_EQUALS("[scripts/test.ctl:3]: (warning) Passing values 0 and -10 to pow() leads to implementation-defined result.\n", errout.str());

    check("void foo()\n"
          "{\n"
          "    DebugN(pow(0,10));\n"
          "}");
    ASSERT_EQUALS("", errout.str());
  }

  void mathfunctionCall_atan2()
  {
    // atan2
    check("float foo()\n"
          "{\n"
          "    DebugN(atan2(1,1))         ;\n"
          "    DebugN(atan2(-1,-1))       ;\n"
          "    DebugN(atan2(0.1,1))       ;\n"
          "    DebugN(atan2(0.0001,100))  ;\n"
          "    DebugN(atan2(0.0,1e-1))    ;\n"
          "    DebugN(atan2(1.0E-1,-3))   ;\n"
          "    DebugN(atan2(-1.0E-1,+2))  ;\n"
          "    DebugN(atan2(+1.0E-1,0))   ;\n"
          "    DebugN(atan2(0.1E-1,3))    ;\n"
          "    DebugN(atan2(+0.1E-1,1))   ;\n"
          "    DebugN(atan2(-0.1E-1,8))   ;\n"
          "}");
    ASSERT_EQUALS("", errout.str());

    check("void foo()\n"
          "{\n"
          "       DebugN(atan2(0,0));\n"
          "}");
    ASSERT_EQUALS("[scripts/test.ctl:3]: (warning) Passing values 0 and 0 to atan2() leads to implementation-defined result.\n", errout.str());
  }

  void mathfunctionCall_fmod()
  {
    // fmod
    check("void foo()\n"
          "{\n"
          "      DebugN(fmod(1.0,0));\n"
          "}");
    ASSERT_EQUALS("[scripts/test.ctl:3]: (warning) Passing values 1.0 and 0 to fmod() leads to implementation-defined result.\n", errout.str());

    check("void foo()\n"
          "{\n"
          "      DebugN(fmod(1.0,1));\n"
          "}");
    ASSERT_EQUALS("", errout.str());
  }
};

// REGISTER_TEST(TestFunctions)

void main()
{
  TestFunctions test;
  test.run();
}
