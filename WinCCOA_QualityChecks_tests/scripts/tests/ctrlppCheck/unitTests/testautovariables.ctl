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


// #include "checkautovariables.h"
// #include "settings.h"
// #include "testsuite.h"
// #include "tokenize.h"


struct TestAutoVariables : TestFixture
{

  void check(const string &code, bool inconclusive = false)
  {
    settings.inconclusive = inconclusive;
    TestFixture::check(code);
  }
//     void check(const char code[], bool inconclusive = false, bool runSimpleChecks = true, const char* filename = "test.cpp") {
  // Clear the error buffer..
//         errout.str("");
//
//         settings.inconclusive = inconclusive;
//
  // Tokenize..
//         Tokenizer tokenizer(&settings, this);
//         std::istringstream istr(code);
//         tokenizer.tokenize(istr, filename);
//
//         CheckAutoVariables checkAutoVariables(&tokenizer, &settings, this);
//         checkAutoVariables.returnReference();
//         checkAutoVariables.assignFunctionArg();
//         checkAutoVariables.checkVarLifetime();
//
//         if (runSimpleChecks) {
//             tokenizer.simplifyTokenList2();
//
  // Check auto variables
//             checkAutoVariables.autoVariables();
//         }
//     }

  void run()
  {
    settings.addEnabled("warning");
    settings.addEnabled("style");

//         LOAD_LIB_2(settings.library, "std.cfg");

    TEST_CASE(testautovar10); // ticket #2930 - void f(char *p) { p = '\0'; }
    TEST_CASE(testautovar13); // ticket #5537 - crash
    TEST_CASE(testautovar15); // ticket #6538

    TEST_CASE(returnLocalVariable2);

    // return reference..
    TEST_CASE(returnReferenceLiteral);
    TEST_CASE(returnReferenceCalculation);

    // global namespace
    TEST_CASE(testglobalnamespace);

    TEST_CASE(testconstructor); // ticket #5478 - crash

    TEST_CASE(variableIsUsedInScope); // ticket #5599 crash in variableIsUsedInScope()
  }



  void testautovar1()
  {
    check("void func1(int **res)\n"
          "{\n"
          "    int num = 2;\n"
          "    *res = &num;\n"
          "}");
    ASSERT_EQUALS("[test.cpp:4]: (error) Address of local auto-variable assigned to a function parameter.\n", errout.str());

    check("void func1(int **res)\n"
          "{\n"
          "    int num = 2;\n"
          "    res = &num;\n"
          "}");
    ASSERT_EQUALS("[test.cpp:4]: (warning) Assignment of function parameter has no effect outside the function. Did you forget dereferencing it?\n" +
                  "[test.cpp:4]: (style) Variable 'res' is assigned a value that is never used.\n",
                  errout.str());

    check("void func1(int **res)\n"
          "{\n"
          "    int num = 2;\n"
          "    foo.res = &num;\n"
          "}");
    ASSERT_EQUALS("", errout.str());
  }

  void testautovar2()
  {
    check("class Fred {\n"
          "    void func1(int **res);\n"
          "}\n"
          "void Fred::func1(int **res)\n"
          "{\n"
          "    int num = 2;\n"
          "    *res = &num;\n"
          "}");
    ASSERT_EQUALS("[test.cpp:7]: (error) Address of local auto-variable assigned to a function parameter.\n" +
                  "[test.cpp:2]: (style) Unused private function: 'Fred::func1'\n",
                  errout.str());

    check("class Fred {\n"
          "    void func1(int **res);\n"
          "}\n"
          "void Fred::func1(int **res)\n"
          "{\n"
          "    int num = 2;\n"
          "    res = &num;\n"
          "}");
    ASSERT_EQUALS("[test.cpp:7]: (warning) Assignment of function parameter has no effect outside the function. Did you forget dereferencing it?\n" +
                  "[test.cpp:7]: (style) Variable 'res' is assigned a value that is never used.\n" +
                  "[test.cpp:2]: (style) Unused private function: 'Fred::func1'\n",
                  errout.str());

    check("class Fred {\n"
          "    void func1(int **res);\n"
          "}\n"
          "void Fred::func1(int **res)\n"
          "{\n"
          "    int num = 2;\n"
          "    foo.res = &num;\n"
          "}");
    ASSERT_EQUALS("[test.cpp:2]: (style) Unused private function: 'Fred::func1'\n", errout.str());
  }

  void testautovar3()   // ticket #2925
  {
    check("void foo(int **p)\n"
          "{\n"
          "    int x[100];\n"
          "    *p = x;\n"
          "}");
    ASSERT_EQUALS("[test.cpp:4]: (error) Address of local auto-variable assigned to a function parameter.\n", errout.str());
  }

  void testautovar4()   // ticket #2928
  {
    check("void foo(int **p)\n"
          "{\n"
          "    static int x[100];\n"
          "    *p = x;\n"
          "}");
    ASSERT_EQUALS("", errout.str());
  }

  void testautovar5()   // ticket #2926
  {
    check("void foo(struct AB *ab)\n"
          "{\n"
          "    char a;\n"
          "    ab->a = &a;\n"
          "}", false);
    ASSERT_EQUALS("", errout.str());

    check("void foo(struct AB *ab)\n"
          "{\n"
          "    char a;\n"
          "    ab->a = &a;\n"
          "}", true);
    ASSERT_EQUALS("[test.cpp:4]: (error, inconclusive) Address of local auto-variable assigned to a function parameter.\n", errout.str());
  }

  void testautovar6()   // ticket #2931
  {
    check("void foo(struct X *x)\n"
          "{\n"
          "    char a[10];\n"
          "    x->str = a;\n"
          "}", false);
    ASSERT_EQUALS("", errout.str());

    check("void foo(struct X *x)\n"
          "{\n"
          "    char a[10];\n"
          "    x->str = a;\n"
          "}", true);
    ASSERT_EQUALS("[test.cpp:4]: (error, inconclusive) Address of local auto-variable assigned to a function parameter.\n", errout.str());
  }

  void testautovar7()   // ticket #3066
  {
    check("struct txt_scrollpane_s * TXT_NewScrollPane(struct txt_widget_s * target)\n"
          "{\n"
          "    struct txt_scrollpane_s * scrollpane;\n"
          "    target->parent = &scrollpane->widget;\n"
          "    return scrollpane;\n"
          "}", false);
    ASSERT_EQUALS("", errout.str());
  }

  void testautovar8()
  {
    check("void foo(int*& p) {\n"
          "    int i = 0;\n"
          "    p = &i;\n"
          "}", false);
    ASSERT_EQUALS("[test.cpp:3]: (error) Address of local auto-variable assigned to a function parameter.\n", errout.str());

    check("void foo(std::string& s) {\n"
          "    s = foo;\n"
          "}", false);
    ASSERT_EQUALS("", errout.str());
  }

  void testautovar9()
  {
    check("struct FN {int i;};\n"
          "struct FP {FN* f};\n"
          "void foo(int*& p, FN* p_fp) {\n"
          "    FN fn;\n"
          "    FP fp;\n"
          "    p = &fn.i;\n"
          "    p = &p_fp->i;\n"
          "    p = &fp.f->i;\n"
          "}", false);
    ASSERT_EQUALS("[test.cpp:6]: (error) Address of local auto-variable assigned to a function parameter.\n", errout.str());
  }

  void testautovar10()   // #2930 - assignment of function parameter
  {
    check("void foo(int b) {\n"
          "    b = foo(b);\n"
          "}");
    ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) Assignment of function parameter 'b' has no effect outside the function.\n"
                  "[scripts/test.ctl:2]: (warning) Variable is assigned a value that is never used: 'b'\n",
                  errout.str());

    check("void foo(int b) {\n"
          "    b += 1;\n"
          "}");
    ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) Assignment of function parameter 'b' has no effect outside the function.\n"
                  "[scripts/test.ctl:2]: (warning) Variable is assigned a value that is never used: 'b'\n",
                  errout.str());

    check("void foo(string s) {\n"
          "    string b = \"\";\n"
          "    s = foo(b);\n"
          "}");
    ASSERT_EQUALS("[scripts/test.ctl:3]: (warning) Assignment of function parameter 's' has no effect outside the function.\n"
                  "[scripts/test.ctl:3]: (warning) Variable is assigned a value that is never used: 's'\n",
                  errout.str());

    check("void foo(shared_ptr<char> p) {\n" // don't warn for self assignment, there is another warning for this
          "  p = p;\n"
          "}");
    ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) Redundant assignment of 'p' to itself.\n"
                  "[scripts/test.ctl:2]: (warning) Variable is assigned a value that is never used: 'p'\n",
                  errout.str());

    check("void foo(shared_ptr<char> p, shared_ptr<char> buf) {\n"
          "    if (!p) p = buf;\n"
          "    p = 0;\n"
          "}");
    ASSERT_EQUALS("[scripts/test.ctl:3]: (warning) Assignment of function parameter 'p' has no effect outside the function.\n"
                  "[scripts/test.ctl:2] -> [scripts/test.ctl:3]: (style) Variable is reassigned a value before the old one has been used: 'p'\n"
                  "[scripts/test.ctl:3]: (warning) Variable is assigned a value that is never used: 'p'\n",
                  errout.str());

    check("void foo(shared_ptr<char> p, shared_ptr<char> buf) {\n"
          "    if (!p) p = buf;\n"
          "    do_something(p);\n"
          "}");
    ASSERT_EQUALS("", errout.str());

    check("void foo(shared_ptr<char> p, shared_ptr<char> buf) {\n"
          "    while (!p) p = buf;\n"
          "}");
    ASSERT_EQUALS("", errout.str());

    check("class Foo {};\n"
          "void foo(shared_ptr<Foo> p) {\n"
          "    p = 0;\n"
          "}");
    ASSERT_EQUALS("[scripts/test.ctl:2]: (style) Variable 'p' is assigned a value that is never used.\n", errout.str());

    check("class Foo {};\n"
          "void foo(Foo p) {\n"
          "    p = 0;\n"
          "}");
    ASSERT_EQUALS("[scripts/test.ctl:3]: (warning) Assignment of function parameter 'p' has no effect outside the function.\n"
                  "[scripts/test.ctl:3]: (warning) Variable is assigned a value that is never used: 'p'\n",
                  errout.str());

    check("class Foo {};\n"
          "void foo(Foo &p) {\n"
          "    p = 0;\n"
          "}");
    ASSERT_EQUALS("", errout.str());

    check("void foo(int& p) {\n"
          "    p = 0;\n"
          "}");
    ASSERT_EQUALS("", errout.str());

    check("double foo(double d) {\n" // #5005
          "    int i = d;\n"
          "    d = i;\n"
          "    return d;"
          "}", false, false);
    ASSERT_EQUALS("", errout.str());
  }

  void testautovar13()   // Ticket #5537
  {
    check("class FileManager {\n"
          "  FileManager() : UniqueRealDirs(*new UniqueDirContainer())\n"
          "  {}\n"
          "  ~FileManager() {\n"
          "    delete &UniqueRealDirs;\n"
          "   }\n"
          "};\n");
    //Missing ASSERT_EQUALS?
  }
  void testautovar15()   // Ticket #6538
  {
    check("const dyn_float darkOutline = makeDynFloat(0.05f, 0.05f, 0.05f, 0.95f);\n"
          "const float darkLuminosity = 0.05 +\n"
          "                             0.0722f * pow(darkOutline[2], 2.2);\n"
          "\n"
          "const dyn_float ChooseOutlineColor(const dyn_float& textColor, float something) {\n"
          "  const float lumdiff = something;\n"
          "  if (lumdiff > 5.0f)\n"
          "    return darkOutline;\n"
          "  return 0;\n"
          "}\n",
          false, false);
    ASSERT_EQUALS("[scripts/test.ctl:1]: (style) The name darkOutline does not match the following rule(s): const\n"
                  "[scripts/test.ctl:2]: (style) The name darkLuminosity does not match the following rule(s): const\n"
                  "[scripts/test.ctl:6]: (style) The name lumdiff does not match the following rule(s): const\n",
                  errout.str());
  }

  void returnLocalVariable2()
  {
    check("string foo()\n"
          "{\n"
          "  dyn_char str = makeDynChar();\n"
          "  return str;\n"
          "}\n");
    ASSERT_EQUALS("", errout.str());

    check("class Fred\n"
          "{\n"
          "  public string foo()\n"
          "  {\n"
          "    dyn_char str = makeDynChar();\n"
          "    return str;\n"
          "  }\n"
          "};\n");
    ASSERT_EQUALS("", errout.str());
  }

  void returnReferenceLiteral()
  {
    check("const string a() {\n"
          "  return \"foo\";\n"
          "}\n");
    ASSERT_EQUALS("", errout.str());
  }

  void returnReferenceCalculation()
  {
    check("const string a(const string& str) {\n"
          "    return \"foo\" + str;\n"
          "}");
    ASSERT_EQUALS("", errout.str());
  }

  void testglobalnamespace()
  {
    check("class SharedPtrHolder\n"
          "{\n"
          "  shared_ptr<int> pNum;\n"
          " public:\n"
          "  void SetNum(const shared_ptr<int>& apNum)\n"
          "  {\n"
          "    pNum = apNum;\n"
          "  }\n"
          "};\n");

    ASSERT_EQUALS("", errout.str());
  }

  void testconstructor()   // Ticket #5478 - crash while checking a constructor
  {
    check("class const_tree_iterator {\n"
          "  const_tree_iterator(bool (*_incream)(node_type*&)) {}\n"
          "  const_tree_iterator& parent() {\n"
          "    return const_tree_iterator(foo);\n"
          "  }\n"
          "};");
  }
  //Missing ASSERT_EQUALS?
  void variableIsUsedInScope()
  {
    check("void removed_cb (GList *uids) {\n"
          "  for (; uids; uids = uids->next) {\n"
          "  }\n"
          "}\n"
          "void opened_cb () {\n"
          "	g_signal_connect (G_CALLBACK (removed_cb));\n"
          "}");
    //Missing ASSERT_EQUALS?
  }
};

// REGISTER_TEST(TestAutoVariables)
void main()
{
  TestAutoVariables test;
  test.run();
}
