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

// #include "checkassert.h"
// #include "settings.h"
// #include "testsuite.h"
// #include "tokenize.h"


struct TestAssert : public TestFixture
{

//     Settings settings;



  void run()
  {
    settings.addEnabled("warning");
    settings.inconclusive = false;

    TEST_CASE(assignmentInAssert);
    TEST_CASE(functionCallInAssert);
    TEST_CASE(memberFunctionCallInAssert);
    TEST_CASE(safeFunctionCallInAssert);
  }


  void safeFunctionCallInAssert()
  {
    check(
      "int a;\n"
      "bool b = false;\n"
      "int foo() {\n"
      "   if (b) { a = 1+2; }\n"
      "   return a;\n"
      "}\n"
      "void main(){ assert(foo() == 3); }\n"
    );
    ASSERT_EQUALS("", errout.str());

    check(
      "int foo(int a) {\n"
      "    int b=a+1;\n"
      "    return b;\n"
      "}\n"
      "void main(){ assert(foo(1) == 2); }\n"
    );
    ASSERT_EQUALS("", errout.str());
  }

  void functionCallInAssert()
  {
    /* knownBug TFS(#158013)
    check(
      "int a;\n"
      "int foo() {\n"
      "    a = 1+2;\n"
      "    return a;\n"
      "}\n"
      "void main(){assert(foo() == 3);}\n"
    );
    ASSERT_EQUALS("[test.cpp:6]: (warning) Assert statement calls a function which may have desired side effects: 'foo'.\n", errout.str());
    */
    //  Ticket #4937 "false positive: Assert calls a function which may have desired side effects"
    check("struct Square {\n"
          "  static void dummy(){1+1;}\n"
          "};\n"
          "\n"
          "struct SquarePack {\n"
          "   static bool isRank1Or8( Square sq ) {\n"
          "      sq &= 0x38;\n"
          "      return sq == 0 || sq == 0x38;\n"
          "    }\n"
          "};\n"
          "void foo() {\n"
          "   Square push2;\n"
          "   assert( !SquarePack::isRank1Or8(push2) );\n"
          "}\n");
    ASSERT_EQUALS("", errout.str());
    /* knownBug TFS(#158013)
    check("struct Square {\n"
          "  static void dummy(){1+1;}\n"
          "};\n"
          "\n"
          "struct SquarePack {\n"
          "   static bool isRank1Or8( Square &sq ) {\n"
          "      sq &= 0x38;\n"
          "      return sq == 0 || sq == 0x38;\n"
          "    }\n"
          "};\n"
          "void foo() {\n"
          "   Square push2;\n"
          "   assert( !SquarePack::isRank1Or8(push2) );\n"
          "}\n");
    ASSERT_EQUALS("[test.cpp:8]: (warning) Assert statement calls a function which may have desired side effects: 'isRank1Or8'.\n", errout.str());
    */
  }

  void memberFunctionCallInAssert()
  {
    /* knownBug TFS(#158013)
    check("struct SquarePack {\n"
          "   void Foo(){}\n"
          "};\n"
          "void foo(SquarePack s) {\n"
          "   assert( s.Foo() );\n"
          "}");
    ASSERT_EQUALS("[test.cpp:5]: (warning) Assert statement calls a function which may have desired side effects: 'Foo'.\n", errout.str());
    */

    check("struct SquarePack {\n"
          "   static void Foo(){}\n"
          "};\n"
          "void foo(SquarePack& s) {\n"
          "   assert( s.Foo() );\n"
          "}");
    ASSERT_EQUALS("", errout.str());

    check("struct SquarePack {\n"
          "};\n"
          "void foo(SquarePack& s) {\n"
          "   assert( s.Foo() );\n"
          "}");
    ASSERT_EQUALS("", errout.str());
  }

  void assignmentInAssert()
  {
    /* knownBug TFS(#158013)
    check("void f() {\n"
          "    int a; a = 1;\n"
          "    assert(a = 2);\n"
          "}\n"
         );
    ASSERT_EQUALS("[test.cpp:3]: (warning) Assert statement modifies 'a'.\n", errout.str());
    */
    check("void f(int a) {\n"
          "    assert(a == 2);\n"
          "}\n"
         );
    ASSERT_EQUALS("", errout.str());
    /* knownBug TFS(#158013)
    check("void f(int a, int b) {\n"
          "    assert((a == 2) && (b = 1));\n"
          "}\n"
         );
    ASSERT_EQUALS("[test.cpp:2]: (warning) Assert statement modifies 'b'.\n", errout.str());
    */
    /* knownBug TFS(#158013)
    check("void f() {\n"
          "    int a; a = 0;\n"
          "    assert(a += 2);\n"
          "}\n"
         );
    ASSERT_EQUALS("[test.cpp:3]: (warning) Assert statement modifies 'a'.\n", errout.str());
    */
    /* knownBug TFS(#158013)
    check("void f() {\n"
          "    int a; a = 0;\n"
          "    assert(a *= 2);\n"
          "}\n"
         );
    ASSERT_EQUALS("[test.cpp:3]: (warning) Assert statement modifies 'a'.\n", errout.str());
    */
    /* knownBug TFS(#158013)
    check("void f() {\n"
          "    int a; a = 0;\n"
          "    assert(a -= 2);\n"
          "}\n"
         );
    ASSERT_EQUALS("[test.cpp:3]: (warning) Assert statement modifies 'a'.\n", errout.str());
    */
    /* knownBug TFS(#158013)
    check("void f() {\n"
          "    int a = 0;\n"
          "    assert(a--);\n"
          "}\n");
    ASSERT_EQUALS("[test.cpp:3]: (warning) Assert statement modifies 'a'.\n", errout.str());
    */
  }
};

// REGISTER_TEST(TestAssert)
void main()
{
  TestAssert test;
  test.run();
}
