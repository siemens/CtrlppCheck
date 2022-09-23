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


// #include "check64bit.h"
// #include "settings.h"
// #include "testsuite.h"
// #include "tokenize.h"


struct Test64BitPortability : public TestFixture {
  public Test64BitPortability()
  {
//     testCaseIdPrefix = "Test64BitPortability";
  }

//     CppCheckSettings settings;

    public void run() /*OVERRIDE*/ {
        settings.addEnabled("portability");

        TEST_CASE(novardecl);
        TEST_CASE(functionpar);
        TEST_CASE(functionparWinCCOA);
        
        TEST_CASE(structmember);
        TEST_CASE(structmemberWinCC_OA);
        
        TEST_CASE(ptrcompare);
        TEST_CASE(ptrcompareWinCC_OA);
        
        TEST_CASE(ptrarithmetic);
        TEST_CASE(ptrarithmeticWinCC_OA);
        
        TEST_CASE(returnIssues);
        TEST_CASE(returnIssuesWinCC_OA);
    }

    void novardecl() {
        // if the variable declarations can't be seen then skip the warning
        check("void foo()\n"
              "{\n"
              "    a = p;\n"
              "}");
        ASSERT_EQUALS("", errout.str());
    }

    void functionpar() {
        check("int foo(int *p)\n"
              "{\n"
              "    int a = p;\n"
              "    return a + 4;\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 12", errout.str());

        check("int foo(int p[])\n"
              "{\n"
              "    int a = p;\n"
              "    return a + 4;\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 13", errout.str());

        check("int foo(int p[])\n"
              "{\n"
              "    int *a = p;\n"
              "    return a;\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 13", errout.str());

        check("void foo(int x)\n"
              "{\n"
              "    int *p = x;\n"
              "    *p = 0;\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 38", errout.str());

        check("int f(const char *p) {\n" // #4659
              "    return 6 + p[2] * 256;\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 17", errout.str());

        check("int foo(int *p) {\n" // #6096
              "    bool a = p;\n"
              "    return a;\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 12", errout.str());
    }

    void functionparWinCCOA() {
      //int, const int, address to int, const address to int
        check("int foo(int p)\n"
              "{\n"
              "    int a = p;\n"
              "    return a + 4;\n"
              "}");
        ASSERT_EQUALS("", errout.str());
        check("int foo(const int p)\n"
              "{\n"
              "    int a = p;\n"
              "    return a + 4;\n"
              "}");
        ASSERT_EQUALS("", errout.str());
        check("int foo(const int &p)\n"
              "{\n"
              "    int a = p;\n"
              "    return a + 4;\n"
              "}");
        ASSERT_EQUALS("", errout.str());
        check("int foo(int &p)\n"
              "{\n"
              "    int a = p;\n"
              "    return a + 4;\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        // char to int
        check("int f(const char p) {\n"
              "    return 6 + p[2] * 256;\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        // int to bool
        check("int foo(int p) {\n"
              "    bool a = p;\n"
              "    return a;\n"
              "}");
        ASSERT_EQUALS("", errout.str());
    }

    void structmember() {
        check("struct Foo { int *p; };\n"
              "void f(struct Foo *foo) {\n"
              "    int i = foo->p;\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 17", errout.str());
    }

    void structmemberWinCC_OA() {
        check("struct Foo { int p; };\n"
              "void f(Foo foo) {\n"
              "    int i = foo.p;\n"
              "}");
    }
        
    void ptrcompare() {
        // Ticket #2892
        check("void foo(int *p) {\n"
              "    int a = (p != NULL);\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 13", errout.str());
//         ASSERT_EQUALS("", errout.str());
    }

    void ptrcompareWinCC_OA() {
        check("void foo(int p) {\n"
              "    int a = (p != NULL);\n"
              "}");
        ASSERT_EQUALS("", errout.str());
        check("void foo(int p) {\n"
              "    int a = (p != nullptr);\n"
              "}");
        ASSERT_EQUALS("", errout.str());
    }

    void ptrarithmetic() {
        // #3073
        check("void foo(int *p) {\n"
              "    int x = 10;\n"
              "    int *a = p + x;\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 13", errout.str());
//         ASSERT_EQUALS("", errout.str());

        check("void foo(int *p) {\n"
              "    int x = 10;\n"
              "    int *a = x + p;\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 13", errout.str());
//         ASSERT_EQUALS("", errout.str());

        check("void foo(int *p) {\n"
              "    int x = 10;\n"
              "    int *a = x * x;\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 13", errout.str());
//         ASSERT_EQUALS("[test.cpp:3]: (portability) Assigning an integer to a pointer is not portable.\n", errout.str());

        check("void foo(int *start, int *end) {\n"
              "    int len;\n"
              "    int len = end + 10 - start;\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 13", errout.str());
//         ASSERT_EQUALS("", errout.str());
    }
    
    /// @todo ptr aritmetic for WinCC OA
    void ptrarithmeticWinCC_OA() {
    }

    void returnIssues() {
        check("void* foo(int i) {\n"
              "    return i;\n"
              "}");        
        ASSERT_EQUALS("WinCC OA syntax error at pos: 4", errout.str());
//         ASSERT_EQUALS("[test.cpp:2]: (portability) Returning an integer in a function with pointer return type is not portable.\n", errout.str());

        check("void* foo(int* i) {\n"
              "    return i;\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 4", errout.str());
//         ASSERT_EQUALS("", errout.str());

        check("void* foo() {\n"
              "    return 0;\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 4", errout.str());
//         ASSERT_EQUALS("", errout.str());

        check("int foo(int i) {\n"
              "    return i;\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("struct Foo {};\n"
              "\n"
              "int* dostuff(Foo foo) {\n"
              "  return foo;\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 19", errout.str());
//         ASSERT_EQUALS("", errout.str());

        check("int foo(char* c) {\n"
              "    return c;\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 12", errout.str());
//         ASSERT_EQUALS("[test.cpp:2]: (portability) Returning an address value in a function with integer return type is not portable.\n", errout.str());

        check("int foo(char* c) {\n"
              "    return 1+c;\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 12", errout.str());
//         ASSERT_EQUALS("[test.cpp:2]: (portability) Returning an address value in a function with integer return type is not portable.\n", errout.str());

        check("std::string foo(char* c) {\n"
              "    return c;\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 3", errout.str());
//         ASSERT_EQUALS("", errout.str());

        check("int foo(char *a, char *b) {\n" // #4486
              "    return a + 1 - b;\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 13", errout.str());
//         ASSERT_EQUALS("", errout.str());

        check("struct s {\n" // 4642
              "   int i;\n"
              "};\n"
              "int func(struct s *p) {\n"
              " return 1 + p->i;\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 33", errout.str());
//         ASSERT_EQUALS("", errout.str());

        check("static void __iomem *f(unsigned int port_no) {\n"
              "  void __iomem *mmio = hpriv->mmio;\n"
              "  return mmio + (port_no * 0x80);\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 20", errout.str());
//         ASSERT_EQUALS("", errout.str());

        // #7247: don't check return statements in nested functions..
        check("int foo() {\n"
              "  struct {\n"
              "    const char * name() { return \"abc\"; }\n"
              "  } table;\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 47", errout.str());
//         ASSERT_EQUALS("", errout.str());

        // #7451: Lambdas
        check("const int* test(std::vector<int> outputs, const std::string& text) {\n"
              "  auto it = std::find_if(outputs.begin(), outputs.end(), \n"
              "     [&](int ele) { return \"test\" == text; });\n"
              "  return nullptr;\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 9", errout.str());
//         ASSERT_EQUALS("", errout.str());
    }
    
    
    void returnIssuesWinCC_OA() {
        check("int foo() {\n"
              "    return 0;\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("int foo(int i) {\n"
              "    return i;\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("struct Foo {};\n"
              "\n"
              "int dostuff(Foo foo) {\n"
              "  return foo;\n"
              "}");
        ASSERT_EQUALS("", errout.str()); // this is something very bad ???

        check("int foo(char c) {\n"
              "    return c;\n"
              "}");
        ASSERT_EQUALS("", errout.str());
        check("int foo(char &c) {\n"
              "    return c;\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("int foo(char c) {\n"
              "    return 1+c;\n"
              "}");
        ASSERT_EQUALS("", errout.str());
        check("int foo(char &c) {\n"
              "    return 1+c;\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("string foo(char c) {\n"
              "    return c;\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("int foo(char a, char b) {\n" // #4486
              "    return a + 1 - b;\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("struct s {\n" // 4642
              "   int i;\n"
              "};\n"
              "int func(s &p) {\n"
              " return 1 + p.i;\n"
              "}");
        ASSERT_EQUALS("", errout.str());
    }
};

// REGISTER_TEST(Test64BitPortability)
void main()
{
  Test64BitPortability test;
  test.run();
}
