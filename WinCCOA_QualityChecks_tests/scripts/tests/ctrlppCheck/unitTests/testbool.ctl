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


// #include "checkbool.h"
// #include "settings.h"
// #include "testsuite.h"
// #include "tokenize.h"
//

struct TestBool : public TestFixture {
// public
//     TestBool() : TestFixture("TestBool") {
//     }
  public TestBool(){}

// private
//     Settings settings;

    void run() {
        settings.addEnabled("style");
        settings.addEnabled("warning");
        settings.inconclusive = true;
        assertMatch = TRUE;

        TEST_CASE(bitwiseOnBoolean);      // if (bool & bool)
        TEST_CASE(incrementBoolean);
        TEST_CASE(assignBoolToPointer);
        TEST_CASE(assignBoolToFloat);

        TEST_CASE(comparisonOfBoolExpressionWithInt1);
        TEST_CASE(comparisonOfBoolExpressionWithInt2);
        TEST_CASE(comparisonOfBoolExpressionWithInt3);
        TEST_CASE(comparisonOfBoolExpressionWithInt4);

        TEST_CASE(comparisonOfBoolWithInt1);
        TEST_CASE(comparisonOfBoolWithInt2);
        TEST_CASE(comparisonOfBoolWithInt3);
        TEST_CASE(comparisonOfBoolWithInt4);
        TEST_CASE(comparisonOfBoolWithInt5);
        TEST_CASE(comparisonOfBoolWithInt6); // #4224 - integer is casted to bool
        TEST_CASE(comparisonOfBoolWithInt7); // #4846 - (!x == true)

        TEST_CASE(checkComparisonOfFuncReturningBool1);
        TEST_CASE(checkComparisonOfFuncReturningBool2);
        TEST_CASE(checkComparisonOfFuncReturningBool3);
        TEST_CASE(checkComparisonOfFuncReturningBool4);
        TEST_CASE(checkComparisonOfFuncReturningBool5);
        TEST_CASE(checkComparisonOfFuncReturningBool6);
        TEST_CASE(checkComparisonOfBoolWithBool);

        // Converting pointer addition result to bool
        TEST_CASE(pointerArithBool1);

        TEST_CASE(returnNonBool);
        TEST_CASE(returnNonBoolWinCC_OA);

        TEST_CASE(winCC_OA_const);



    }

//     void check(const char code[], bool experimental = false, const char filename[] = "scripts/test.ctl") {
        // Clear the error buffer..
//         errout.str("");
//
//         settings.experimental = experimental;
//
        // Tokenize..
//         Tokenizer tokenizer(&settings, this);
//         std::istringstream istr(code);
//         tokenizer.tokenize(istr, filename);
//
        // Check...
//         CheckBool checkBool(&tokenizer, &settings, this);
//         checkBool.runChecks(&tokenizer, &settings, this);
//         tokenizer.simplifyTokenList2();
//         checkBool.runSimplifiedChecks(&tokenizer, &settings, this);
//     }

/// @test make sence only for cpp code, not WinnCC OA relevant
    void assignBoolToPointer() {
        check("void foo(bool *p) {\n"
              "    p = false;\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 14", errout.str());
//         ASSERT_EQUALS("[scripts/test.ctl:2]: (error) Boolean value assigned to pointer.\n", errout.str());

        check("void foo(bool *p) {\n"
              "    p = (x<y);\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 14", errout.str());
//         ASSERT_EQUALS("[scripts/test.ctl:2]: (error) Boolean value assigned to pointer.\n", errout.str());

        check("void foo(bool *p) {\n"
              "    p = (x||y);\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 14", errout.str());
//         ASSERT_EQUALS("[scripts/test.ctl:2]: (error) Boolean value assigned to pointer.\n", errout.str());

        check("void foo(bool *p) {\n"
              "    p = (x&&y);\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 14", errout.str());
//         ASSERT_EQUALS("[scripts/test.ctl:2]: (error) Boolean value assigned to pointer.\n", errout.str());

        // check against potential false positives
        check("void foo(bool *p) {\n"
              "    *p = false;\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 14", errout.str());
//         ASSERT_EQUALS("", errout.str());

        // ticket #5046 - false positive: Boolean value assigned to pointer
        check("struct S {\n"
              "    bool *p;\n"
              "};\n"
              "void f() {\n"
              "    S s = {0};\n"
              "    *s.p = true;\n"
              "}\n");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 20", errout.str());
//         ASSERT_EQUALS("", errout.str());

        check("struct S {\n"
              "    bool *p;\n"
              "};\n"
              "void f() {\n"
              "    S s = {0};\n"
              "    s.p = true;\n"
              "}\n");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 20", errout.str());
//         ASSERT_EQUALS("[scripts/test.ctl:6]: (error) Boolean value assigned to pointer.\n", errout.str());

        // ticket #5627 - false positive: template
        check("void f() {\n"
              "    X *p = new ::std::pair<int,int>[rSize];\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 33", errout.str());
//         ASSERT_EQUALS("", errout.str());

        // ticket #6588 (c mode)
        check("struct MpegEncContext { int *q_intra_matrix, *q_chroma_intra_matrix; };\n"
              "void dnxhd_10bit_dct_quantize(MpegEncContext *ctx, int n, int qscale) {\n"
              "  const int *qmat = n < 4;\n" /* KO */
              "  const int *rmat = n < 4 ? " /* OK */
              "                       ctx->q_intra_matrix :"
              "                       ctx->q_chroma_intra_matrix;\n"
              "}", /*experimental=*/false, "test.c");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 28", errout.str());
//         ASSERT_EQUALS("[test.c:3]: (error) Boolean value assigned to pointer.\n", errout.str());

        // ticket #6588 (c++ mode)
        check("struct MpegEncContext { int *q_intra_matrix, *q_chroma_intra_matrix; };\n"
              "void dnxhd_10bit_dct_quantize(MpegEncContext *ctx, int n, int qscale) {\n"
              "  const int *qmat = n < 4;\n" /* KO */
              "  const int *rmat = n < 4 ? " /* OK */
              "                       ctx->q_intra_matrix :"
              "                       ctx->q_chroma_intra_matrix;\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 28", errout.str());
//         ASSERT_EQUALS("[scripts/test.ctl:3]: (error) Boolean value assigned to pointer.\n", errout.str());

        // ticket #6665
        check("void pivot_big(char *first, int compare(const void *, const void *)) {\n"
              "  char *a = first, *b = first + 1, *c = first + 2;\n"
              "  char* m1 = compare(a, b) < 0\n"
              "      ? (compare(b, c) < 0 ? b : (compare(a, c) < 0 ? c : a))\n"
              "      : (compare(a, c) < 0 ? a : (compare(b, c) < 0 ? c : b));\n"
              "}", /*experimental=*/false, "test.c");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 20", errout.str());
//         ASSERT_EQUALS("", errout.str());

        // #7381
        check("void foo(bool *p, bool b) {\n"
              "    p = b;\n"
              "    p = &b;\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 14", errout.str());
//         ASSERT_EQUALS("[scripts/test.ctl:2]: (error) Boolean value assigned to pointer.\n", errout.str());
    }

    void assignBoolToFloat() {
        check("void foo1() {\n"
              "    double d = false;\n"
              "}");
        //ASSERT_EQUALS("[scripts/test.ctl:2]: (style) Boolean value assigned to floating point variable.\n", errout.str()); no such error in code
        // Shouldn't an error of an unused variable be thrown? (Like in the ASSERT_EQUALS below)
        ASSERT_EQUALS("", errout.str());

        check("void foo2() {\n"
              "    float d = true;\n"
              "}");
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) Variable is assigned a value that is never used: 'd'\n", errout.str());

        check("void foo3() {\n"
              "    double d = (2>1);\n"
              "}");
        ASSERT_EQUALS("", errout.str());
//         ASSERT_EQUALS("[scripts/test.ctl:2]: (style) Boolean value assigned to floating point variable.\n", errout.str());

        // stability - don't crash:
        check("void foo4() {\n"
              "    unknown = false;\n"
              "}");
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) Undefined variable: unknown\n", errout.str());

        check("struct S {\n"
              "    float p;\n"
              "};\n"
              "void f() {\n"
              "    S s = {0};\n"
              "    s.p = true;\n"
              "}\n");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 48", errout.str());
//         ASSERT_EQUALS("[scripts/test.ctl:6]: (style) Boolean value assigned to floating point variable.\n", errout.str());
        check("struct S {\n"
              "    float p;\n"
              "};\n"
              "void f() {\n"
              "    S s;\n"
              "    s.p = true;\n"
              "}\n");
        ASSERT_EQUALS("[scripts/test.ctl:6]: (warning) Variable is assigned a value that is never used: 's.p'\n", errout.str());
    }

    void comparisonOfBoolExpressionWithInt1() {
        check("void f(int x) {\n"
              "    if ((x && 0x0f)==6)\n"
              "        a++;\n"
              "}\n"
             );
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) Comparison of a boolean expression with an integer other than 0 or 1.\n", errout.str());

        check("void f(int x) {\n"
              "    if ((x && 0x0f)==0)\n"
              "        a++;\n"
              "}\n"
             );
        ASSERT_EQUALS("[scripts/test.ctl:3]: (warning) Undefined variable: a\n", errout.str());

        check("void f(int x) {\n"
              "    if ((x || 0x0f)==6)\n"
              "        a++;\n"
              "}\n"
             );
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) Comparison of a boolean expression with an integer other than 0 or 1.\n", errout.str());

        check("void f(int x) {\n"
              "    if ((x || 0x0f)==0)\n"
              "        a++;\n"
              "}\n"
             );
        ASSERT_EQUALS("[scripts/test.ctl:3]: (warning) Undefined variable: a\n", errout.str());
//         ASSERT_EQUALS("", errout.str());

        check("void f(int x) {\n"
              "    if ((x & 0x0f)==6)\n"
              "        a++;\n"
              "}\n"
             );
        ASSERT_EQUALS("[scripts/test.ctl:3]: (warning) Undefined variable: a\n", errout.str());

        check("void f(int x) {\n"
              "    if ((x | 0x0f)==6)\n"
              "        a++;\n"
              "}\n"
             );
        ASSERT_EQUALS("[scripts/test.ctl:2]: (style) Expression '(X | 0xf) == 0x6' is always false.", errout.str());
//         ASSERT_EQUALS("", errout.str());


        check("void f(int x) {\n"
              "    if ((5 && x)==3)\n"
              "        a++;\n"
              "}\n"
             );
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) Comparison of a boolean expression with an integer other than 0 or 1.\n", errout.str());

        check("void f(int x) {\n"
              "    if ((5 && x)==3 || (8 && x)==9)\n"
              "        a++;\n"
              "}\n"
             );
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) Comparison of a boolean expression with an integer other than 0 or 1.\n", errout.str());

        check("void f(int x) {\n"
              "    if ((5 && x)!=3)\n"
              "        a++;\n"
              "}\n"
             );
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) Comparison of a boolean expression with an integer other than 0 or 1.\n", errout.str());


        check("void f(int x) {\n"
              "    if ((5 && x) > 3)\n"
              "        a++;\n"
              "}\n"
             );
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) Comparison of a boolean expression with an integer other than 0 or 1.\n", errout.str());

        check("void f(int x) {\n"
              "    if ((5 && x) > 0)\n"
              "        a++;\n"
              "}\n"
             );
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) Comparison of a variable having boolean value using relational (<, >, <= or >=) operator.\n"
                      "[scripts/test.ctl:3]: (warning) Undefined variable: a\n",
                      errout.str());

        check("void f(int x) {\n"
              "    if ((5 && x) < 0)\n"
              "        a++;\n"
              "}\n"
             );
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) Comparison of a boolean expression with an integer other than 0 or 1.\n", errout.str());

        check("void f(int x) {\n"
              "    if ((5 && x) < 1)\n"
              "        a++;\n"
              "}\n"
             );
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) Comparison of a variable having boolean value using relational (<, >, <= or >=) operator.\n"
                      "[scripts/test.ctl:3]: (warning) Undefined variable: a\n",
                      errout.str());

        check("void f(int x) {\n"
              "    if ((5 && x) > 1)\n"
              "        a++;\n"
              "}\n"
             );
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) Comparison of a boolean expression with an integer other than 0 or 1.\n", errout.str());


        check("void f(int x) {\n"
              "    if (0 < (5 && x))\n"
              "        a++;\n"
              "}\n"
             );
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) Comparison of a variable having boolean value using relational (<, >, <= or >=) operator.\n"
                      "[scripts/test.ctl:3]: (warning) Undefined variable: a\n",
                      errout.str());

        check("void f(int x) {\n"
              "    if (0 > (5 && x))\n"
              "        a++;\n"
              "}\n"
             );
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) Comparison of a boolean expression with an integer other than 0 or 1.\n", errout.str());

        check("void f(int x) {\n"
              "    if (1 > (5 && x))\n"
              "        a++;\n"
              "}\n"
             );
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) Comparison of a variable having boolean value using relational (<, >, <= or >=) operator.\n"
                      "[scripts/test.ctl:3]: (warning) Undefined variable: a\n",
                      errout.str());

        check("void f(int x) {\n"
              "    if (1 < (5 && x))\n"
              "        a++;\n"
              "}\n"
             );
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) Comparison of a boolean expression with an integer other than 0 or 1.\n", errout.str());

        check("void f(bool x ) {\n"
              "  if ( x > false )\n"
              "      a++;\n"
              "}\n"
             );
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) Comparison of a variable having boolean value using relational (<, >, <= or >=) operator.\n"
                      "[scripts/test.ctl:3]: (warning) Undefined variable: a\n",
                      errout.str());

        check("void f(bool x ) {\n"
              "  if ( false < x )\n"
              "      a++;\n"
              "}\n"
             );
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) Comparison of a variable having boolean value using relational (<, >, <= or >=) operator.\n"
                      "[scripts/test.ctl:3]: (warning) Undefined variable: a\n",
                      errout.str());

        check("void f(bool x ) {\n"
              "  if ( x < false )\n"
              "      a++;\n"
              "}\n"
             );
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) Comparison of a variable having boolean value using relational (<, >, <= or >=) operator.\n"
                      "[scripts/test.ctl:3]: (warning) Undefined variable: a\n",
                      errout.str());

        check("void f(bool x ) {\n"
              "  if ( false > x )\n"
              "      a++;\n"
              "}\n"
             );
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) Comparison of a variable having boolean value using relational (<, >, <= or >=) operator.\n"
                      "[scripts/test.ctl:3]: (warning) Undefined variable: a\n",
                      errout.str());

        check("void f(bool x ) {\n"
              "  if ( x >= false )\n"
              "      a++;\n"
              "}\n"
             );
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) Comparison of a variable having boolean value using relational (<, >, <= or >=) operator.\n"
                      "[scripts/test.ctl:3]: (warning) Undefined variable: a\n",
                      errout.str());

        check("void f(bool x ) {\n"
              "  if ( false >= x )\n"
              "      a++;\n"
              "}\n"
             );
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) Comparison of a variable having boolean value using relational (<, >, <= or >=) operator.\n"
                      "[scripts/test.ctl:3]: (warning) Undefined variable: a\n",
                      errout.str());

        check("void f(bool x ) {\n"
              "  if ( x <= false )\n"
              "      a++;\n"
              "}\n"
             );
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) Comparison of a variable having boolean value using relational (<, >, <= or >=) operator.\n"
                      "[scripts/test.ctl:3]: (warning) Undefined variable: a\n",
                      errout.str());

        check("void f(bool x ) {\n"
              "  if ( false <= x )\n"
              "      a++;\n"
              "}\n"
             );
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) Comparison of a variable having boolean value using relational (<, >, <= or >=) operator.\n"
                      "[scripts/test.ctl:3]: (warning) Undefined variable: a\n",
                      errout.str());

        check("typedef int (*func)(bool invert);\n"
              "void x(int, func f);\n"
              "void foo(int error) {\n"
              "  if (error == ABC) { }\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 0", errout.str());
//         ASSERT_EQUALS("", errout.str());

        check("int f() { return !a+b<c; }"); // #5072
        ASSERT_EQUALS("[scripts/test.ctl:1]: (warning) Undefined variable: a\n"
                      "[scripts/test.ctl:1]: (warning) Undefined variable: b\n"
                      "[scripts/test.ctl:1]: (warning) Undefined variable: c\n",
                      errout.str());

        check("int f() { return (!a+b<c); }");
         ASSERT_EQUALS("[scripts/test.ctl:1]: (warning) Undefined variable: a\n"
                       "[scripts/test.ctl:1]: (warning) Undefined variable: b\n"
                       "[scripts/test.ctl:1]: (warning) Undefined variable: c\n",
                       errout.str());

        {
            const string code = "void f(int x, bool y) { if ( x != y ) {} }";

            check(code, false, "scripts/test.ctl");
            ASSERT_EQUALS("[scripts/test.ctl:1]: (warning) Comparison of a boolean expression with an integer.\n", errout.str());

            // @test c-style, not oa relevant
//             check(code, false, "test.c");
//             ASSERT_EQUALS("", errout.str());
        }

        check("int f() { return (a+(b<5)<=c); }");
        ASSERT_EQUALS("[scripts/test.ctl:1]: (warning) Comparison of a variable having boolean value using relational (<, >, <= or >=) operator.\n"
                      "[scripts/test.ctl:1]: (warning) Undefined variable: a\n"
                      "[scripts/test.ctl:1]: (warning) Undefined variable: b\n"
                      "[scripts/test.ctl:1]: (warning) Undefined variable: c\n",
                      errout.str());
    }

    void comparisonOfBoolExpressionWithInt2() {
        check("void f(int x) {\n"
              "    if (!x == 10) {\n"
              "        printf(\"x not equal to 10\");\n"
              "    }\n"
              "}");
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) Comparison of a boolean expression with an integer other than 0 or 1.\n", errout.str());

        check("void f(int x) {\n"
              "    if (!x != 10) {\n"
              "        printf(\"x not equal to 10\");\n"
              "    }\n"
              "}");
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) Comparison of a boolean expression with an integer other than 0 or 1.\n", errout.str());

        check("void f(int x) {\n"
              "    if (x != 10) {\n"
              "        printf(\"x not equal to 10\");\n"
              "    }\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("void f(int x) {\n"
              "    if (10 == !x) {\n"
              "        printf(\"x not equal to 10\");\n"
              "    }\n"
              "}");
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) Comparison of a boolean expression with an integer other than 0 or 1.\n", errout.str());

        check("void f(int x) {\n"
              "    if (10 != !x) {\n"
              "        printf(\"x not equal to 10\");\n"
              "    }\n"
              "}");
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) Comparison of a boolean expression with an integer other than 0 or 1.\n", errout.str());

        check("void f(int x, int y) {\n"
              "    if (y != !x) {\n"
              "        printf(\"x not equal to 10\");\n"
              "    }\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("void f(int x, bool y) {\n"
              "    if (y != !x) {\n"
              "        printf(\"x not equal to 10\");\n"
              "    }\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("void f(int x) {\n"
              "    if (10 != x) {\n"
              "        printf(\"x not equal to 10\");\n"
              "    }\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("bool f(int x, int y) {\n"
              "    return (!y == !x);\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("int f(int a) {\n"
              "  return (x()+1 == !a);\n"
              "}");
        TODO_ASSERT_EQUALS("error", "", errout.str());

        check("void f() { if (!!a + !!b + !!c >1){} }");
        ASSERT_EQUALS("[scripts/test.ctl:1]: (warning) Comparison of a boolean expression with an integer other than 0 or 1.\n"
                      "[scripts/test.ctl:1]: (warning, inconclusive) The scope is empty: 'if'\n"
                      "[scripts/test.ctl:1]: (warning) Undefined variable: a\n"
                      "[scripts/test.ctl:1]: (warning) Undefined variable: b\n"
                      "[scripts/test.ctl:1]: (warning) Undefined variable: c\n",
                      errout.str());

        check("void f(int a, int b, int c) { if (a != !b || c) {} }");
        ASSERT_EQUALS("[scripts/test.ctl:1]: (warning, inconclusive) The scope is empty: 'if'\n", errout.str());

        check("void f(int a, int b, int c) { if (1 < !!a + !!b + !!c) {} }");
        ASSERT_EQUALS("[scripts/test.ctl:1]: (warning) Comparison of a boolean expression with an integer other than 0 or 1.\n"
                      "[scripts/test.ctl:1]: (warning, inconclusive) The scope is empty: 'if'\n",
                      errout.str());

        check("void f(int a, int b, int c) { if (1 < !(a+b)) {} }");
        ASSERT_EQUALS("[scripts/test.ctl:1]: (warning) Comparison of a boolean expression with an integer other than 0 or 1.\n",errout.str());
    }

    void comparisonOfBoolExpressionWithInt3() {
        check("int f(int x) {\n"
              "    return t<0>() && x;\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 31", errout.str());
//         ASSERT_EQUALS("", errout.str());
    }

    void comparisonOfBoolExpressionWithInt4() {
        // #5016
        check("void f() {\n"
              "  for(int i = 4; i > -1 < 5 ; --i) {}\n"
              "}");
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) Comparison of a boolean expression with an integer other than 0 or 1.\n", errout.str());

        check("bool f(int a, int b, int c) {\n"
              "  return (a > b) < c;\n"
              "}");
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) Comparison of a boolean expression with an integer.\n", errout.str());

        check("bool f(int a, int b, int c) {\n"
              "  return x(a > b) < c;\n"
              "}");
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) Comparison of a variable having boolean value using relational (<, >, <= or >=) operator.\n", errout.str());

        check("bool f(int a, int b, int c) {\n"
              "  return a > b == c;\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        // templates
        check("struct Tokenizer { TokenList list; };\n"
              "void Tokenizer::f() {\n"
              "  std::list<Token*> locationList;\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 29", errout.str());
//         ASSERT_EQUALS("", errout.str());

        // #5063 - or
        check("void f() {\n"
              "  return a > b or c < d;\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 26", errout.str());
//         ASSERT_EQUALS("", errout.str());

        check("int f() {\n"
              "  return (a < b) != 0U;\n"
              "}");
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) Undefined variable: a\n"
                      "[scripts/test.ctl:2]: (warning) Undefined variable: b\n",
                      errout.str());

        check("int f() {\n"
              "  return (a < b) != 0x0;\n"
              "}");
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) Undefined variable: a\n"
                      "[scripts/test.ctl:2]: (warning) Undefined variable: b\n",
                      errout.str());

        check("int f() {\n"
              "  return (a < b) != 42U;\n"
              "}");
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) Comparison of a boolean expression with an integer other than 0 or 1.\n", errout.str());
    }

    void checkComparisonOfFuncReturningBool1() {
        check("void f(){\n"
              "     int temp = 4;\n"
              "     if(compare1(temp) > compare2(temp)){\n"
              "         printf(\"foo\");\n"
              "     }\n"
              "}\n"
              "bool compare1(int temp){\n"
              "     if(temp==4){\n"
              "         return true;\n"
              "     }\n"
              "     else\n"
              "         return false;\n"
              "}\n"
              "bool compare2(int temp){\n"
              "     if(temp==4){\n"
              "         return false;\n"
              "     }\n"
              "     else\n"
              "         return true;\n"
              "}");
        ASSERT_EQUALS("[scripts/test.ctl:3]: (warning) Comparison of a variable having boolean value using relational (<, >, <= or >=) operator.\n", errout.str());
    }

    void checkComparisonOfFuncReturningBool2() {
        check("void f(){\n"
              " int temp = 4;\n"
              " bool a = true;\n"
              " if(compare(temp) > a){\n"
              "     printf(\"foo\");\n"
              " }\n"
              "}\n"
              "bool compare(int temp){\n"
              "  if(temp==4){\n"
              "     return true;\n"
              "  }\n"
              "    else\n"
              "     return false;\n"
              "}");
        ASSERT_EQUALS("[scripts/test.ctl:4]: (warning) Comparison of a variable having boolean value using relational (<, >, <= or >=) operator.\n", errout.str());
    }

    void checkComparisonOfFuncReturningBool3() {
        check("void f(){\n"
              " int temp = 4;\n"
              " if(compare(temp) > temp){\n"
              "         printf(\"foo\");\n"
              "   }\n"
              "}\n"
              "bool compare(int temp){\n"
              "   if(temp==4){\n"
              "     return true;\n"
              "   }\n"
              " else\n"
              "     return false;\n"
              "}");
        ASSERT_EQUALS("[scripts/test.ctl:3]: (warning) Comparison of a boolean expression with an integer.\n"
                      "[scripts/test.ctl:3]: (warning) Comparison of a variable having boolean value using relational (<, >, <= or >=) operator.\n",+
                      errout.str());
    }

    void checkComparisonOfFuncReturningBool4() {
        check("void f(){\n"
              "   int temp = 4;\n"
              " bool b = compare2(6);\n"
              " if(compare1(temp)> b){\n"
              "         printf(\"foo\");\n"
              " }\n"
              "}\n"
              "bool compare1(int temp){\n"
              " if(temp==4){\n"
              "     return true;\n"
              "     }\n"
              " else\n"
              "     return false;\n"
              "}\n"
              "bool compare2(int temp){\n"
              " if(temp == 5){\n"
              "     return true;\n"
              " }\n"
              " else\n"
              "     return false;\n"
              "}");
        ASSERT_EQUALS("[scripts/test.ctl:4]: (warning) Comparison of a variable having boolean value using relational (<, >, <= or >=) operator.\n", errout.str());
    }

    void checkComparisonOfFuncReturningBool5() {
        check("void f(){\n"
              "     int temp = 4;\n"
              "     if(compare1(temp) > !compare2(temp)){\n"
              "         printf(\"foo\");\n"
              "     }\n"
              "}\n"
              "bool compare1(int temp){\n"
              "     if(temp==4){\n"
              "         return true;\n"
              "     }\n"
              "     else\n"
              "         return false;\n"
              "}\n"
              "bool compare2(int temp){\n"
              "     if(temp==4){\n"
              "         return false;\n"
              "     }\n"
              "     else\n"
              "         return true;\n"
              "}");
        ASSERT_EQUALS("[scripts/test.ctl:3]: (warning) Comparison of a variable having boolean value using relational (<, >, <= or >=) operator.\n", errout.str());
    }

    void checkComparisonOfFuncReturningBool6() {
        check("int compare1(int temp);\n"
              "namespace Foo {\n"
              "    bool compare1(int temp);\n"
              "}\n"
              "void f(){\n"
              "    int temp = 4;\n"
              "    if(compare1(temp) > compare2(temp)){\n"
              "        printf(\"foo\");\n"
              "    }\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 22", errout.str());
//         ASSERT_EQUALS("", errout.str());

        check("namespace Foo {\n"
              "    bool compare1(int temp);\n"
              "}\n"
              "int compare1(int temp);\n"
              "void f(){\n"
              "    int temp = 4;\n"
              "    if(compare1(temp) > compare2(temp)){\n"
              "        printf(\"foo\");\n"
              "    }\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 0", errout.str());
//         ASSERT_EQUALS("", errout.str());

        check("int compare1(int temp);\n"
              "namespace Foo {\n"
              "    bool compare1(int temp);\n"
              "    void f(){\n"
              "        int temp = 4;\n"
              "        if(compare1(temp) > compare2(temp)){\n"
              "            printf(\"foo\");\n"
              "        }\n"
              "    }\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 22", errout.str());
//         ASSERT_EQUALS("[scripts/test.ctl:6]: (style) Comparison of a function returning boolean value using relational (<, >, <= or >=) operator.\n", errout.str());

        check("int compare1(int temp);\n"
              "namespace Foo {\n"
              "    bool compare1(int temp);\n"
              "    void f(){\n"
              "        int temp = 4;\n"
              "        if(::compare1(temp) > compare2(temp)){\n"
              "            printf(\"foo\");\n"
              "        }\n"
              "    }\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 22", errout.str());
//         ASSERT_EQUALS("", errout.str());

        check("bool compare1(int temp);\n"
              "void f(){\n"
              "    int temp = 4;\n"
              "    if(foo.compare1(temp) > compare2(temp)){\n"
              "        printf(\"foo\");\n"
              "    }\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 23", errout.str());
//         ASSERT_EQUALS("", errout.str());
    }

    void checkComparisonOfBoolWithBool() {

        // @test settings.experimental works for cppCheck intern only. It has no start option for this settings.
        //       Ignore that for WinCC OA tests
        /*
        const string code = "void f(){\n"
                            "    int temp = 4;\n"
                            "    bool b = compare2(6);\n"
                            "    bool a = compare1(4);\n"
                            "    if(b > a){\n"
                            "        printf(\"foo\");\n"
                            "    }\n"
                            "}\n"
                            "bool compare1(int temp){\n"
                            "    if(temp==4){\n"
                            "        return true;\n"
                            "    }\n"
                            "    else\n"
                            "        return false;\n"
                            "}\n"
                            "bool compare2(int temp){\n"
                            "    if(temp == 5){\n"
                            "        return true;\n"
                            "    }\n"
                            "    else\n"
                            "        return false;\n"
                            "}\n";
        check(code, true);
        ASSERT_EQUALS("[scripts/test.ctl:5]: (style) Comparison of a variable having boolean value using relational (<, >, <= or >=) operator.\n", errout.str());
        check(code, false);
        ASSERT_EQUALS("", errout.str());*/
    }

    void bitwiseOnBoolean() { // 3062
        check("void f(bool a, bool b) {\n"
              "    if(a & !b) {}\n"
              "}");
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning, inconclusive) Boolean variable/expression 'a' is used in bitwise operation. Did you mean '&&'?\n"
                      "[scripts/test.ctl:2]: (style) Boolean result is used in bitwise operation. Clarify expression with parentheses.\n"
                      "[scripts/test.ctl:2]: (warning, inconclusive) The scope is empty: 'if'\n",
                      errout.str());

        check("void f(bool a, bool b) {\n"
              "    if(a | !b) {}\n"
              "}");
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning, inconclusive) Boolean variable/expression 'a' is used in bitwise operation. Did you mean '||'?\n"
                      "[scripts/test.ctl:2]: (style) Boolean result is used in bitwise operation. Clarify expression with parentheses.\n"
                      "[scripts/test.ctl:2]: (warning, inconclusive) The scope is empty: 'if'\n",
                      errout.str());

        check("bool a, b;\n"
              "void f() {\n"
              "    if(a & b) {}\n"
              "}");
        ASSERT_EQUALS("[scripts/test.ctl:3]: (warning, inconclusive) Boolean variable/expression 'a' is used in bitwise operation. Did you mean '&&'?\n"
                      "[scripts/test.ctl:3]: (warning, inconclusive) Boolean variable/expression 'b' is used in bitwise operation. Did you mean '&&'?\n"
                      "[scripts/test.ctl:3]: (warning, inconclusive) The scope is empty: 'if'\n",
                      errout.str());

        check("bool a, b;\n"
              "void f() {\n"
              "    if(a & !b) {}\n"
              "}");
        ASSERT_EQUALS("[scripts/test.ctl:3]: (warning, inconclusive) Boolean variable/expression 'a' is used in bitwise operation. Did you mean '&&'?\n"
                      "[scripts/test.ctl:3]: (style) Boolean result is used in bitwise operation. Clarify expression with parentheses.\n"
                      "[scripts/test.ctl:3]: (warning, inconclusive) The scope is empty: 'if'\n",
                      errout.str());

        check("bool a, b;\n"
              "void f() {\n"
              "    if(a | b) {}\n"
              "}");
        ASSERT_EQUALS("[scripts/test.ctl:3]: (warning, inconclusive) Boolean variable/expression 'a' is used in bitwise operation. Did you mean '||'?\n"
                      "[scripts/test.ctl:3]: (warning, inconclusive) Boolean variable/expression 'b' is used in bitwise operation. Did you mean '&&'?\n"
                      "[scripts/test.ctl:3]: (warning, inconclusive) The scope is empty: 'if'\n",
                      errout.str());

        check("bool a, b;\n"
              "void f() {\n"
              "    if(a | !b) {}\n"
              "}");
        ASSERT_EQUALS("[scripts/test.ctl:3]: (warning, inconclusive) Boolean variable/expression 'a' is used in bitwise operation. Did you mean '||'?\n"
                      "[scripts/test.ctl:3]: (style) Boolean result is used in bitwise operation. Clarify expression with parentheses.\n"
                      "[scripts/test.ctl:3]: (warning, inconclusive) The scope is empty: 'if'\n",
                      errout.str());

        check("void f(bool a, int b) {\n"
              "    if(a & b) {}\n"
              "}");
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning, inconclusive) Boolean variable/expression 'a' is used in bitwise operation. Did you mean '&&'?\n"
                      "[scripts/test.ctl:2]: (warning, inconclusive) The scope is empty: 'if'\n",
                      errout.str());

        check("void f(int a, bool b) {\n"
              "    if(a & b) {}\n"
              "}");
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning, inconclusive) Boolean variable/expression 'b' is used in bitwise operation. Did you mean '&&'?\n"
                      "[scripts/test.ctl:2]: (warning, inconclusive) The scope is empty: 'if'\n",
                      errout.str());

        check("void f(int a, int b) {\n"
              "    if(a & b) {}\n"
              "}");
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning, inconclusive) The scope is empty: 'if'\n", errout.str());

        check("void f(bool b) {\n"
              "    foo(bar, &b);\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 30", errout.str());

        check("void f(bool &b) {\n"
              "    foo(bar, b);\n"
              "}");
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) Undefined variable: bar\n", errout.str());
    }

    void incrementBoolean() {
        check("bool bValue = true;\n"
              "void f() { bValue++; }");
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) Incrementing/Decrementing a variable/expression 'bValue' of type 'bool' with operator++ is not allowed. You should assign it the value 'true' or 'false' instead.\n", errout.str());

        check("void f(bool test){\n"
              "    test++;\n"
              "}");
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) Incrementing/Decrementing a variable/expression 'test' of type 'bool' with operator++ is not allowed. You should assign it the value 'true' or 'false' instead.\n"
                      "[scripts/test.ctl:2]: (warning) Assignment of function parameter 'test' has no effect outside the function.\n",
                      errout.str());

        check("void f(int test){\n"
              "    test++;\n"
              "}");
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) Assignment of function parameter 'test' has no effect outside the function.\n", errout.str());
    }

    void comparisonOfBoolWithInt1() {
        check("void f(bool x) {\n"
              "    if (x < 10) {\n"
              "        printf(\"foo\");\n"
              "    }\n"
              "}");
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) Comparison of a boolean expression with an integer other than 0 or 1.\n", errout.str());

        check("void f(bool x) {\n"
              "    if (10 >= x) {\n"
              "        printf(\"foo\");\n"
              "    }\n"
              "}");
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) Comparison of a boolean expression with an integer other than 0 or 1.\n", errout.str());

        check("void f(bool x) {\n"
              "    if (x != 0) {\n"
              "        printf(\"foo\");\n"
              "    }\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("void f(bool x) {\n"  // #3356
              "    if (x == 1) {\n"
              "    }\n"
              "}");
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning, inconclusive) The scope is empty: 'if'\n", errout.str());

        check("void f(bool x) {\n"
              "    if (x != 10) {\n"
              "        printf(\"foo\");\n"
              "    }\n"
              "}");
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) Comparison of a boolean expression with an integer other than 0 or 1.\n", errout.str());

        check("void f(bool x) {\n"
              "    if (x == 10) {\n"
              "        printf(\"foo\");\n"
              "    }\n"
              "}");
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) Comparison of a boolean expression with an integer other than 0 or 1.\n", errout.str());

        check("void f(bool x) {\n"
              "    if (x == 0) {\n"
              "        printf(\"foo\");\n"
              "    }\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("DensePropertyMap<int, true> visited;"); // #4075
        ASSERT_EQUALS("WinCC OA syntax error at pos: 16", errout.str());
//         ASSERT_EQUALS("", errout.str());
    }

    void comparisonOfBoolWithInt2() {
        check("void f(bool x, int y) {\n"
              "    if (x == y) {\n"
              "        printf(\"foo\");\n"
              "    }\n"
              "}");
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) Comparison of a boolean expression with an integer.\n", errout.str());

        check("void f(int x, bool y) {\n"
              "    if (x == y) {\n"
              "        printf(\"foo\");\n"
              "    }\n"
              "}");
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) Comparison of a boolean expression with an integer.\n", errout.str());

        check("void f(bool x, bool y) {\n"
              "    if (x == y) {\n"
              "        printf(\"foo\");\n"
              "    }\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("void f(bool x, fooClass y) {\n"
              "    if (x == y) {\n"
              "        printf(\"foo\");\n"
              "    }\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 15", errout.str());
//         ASSERT_EQUALS("", errout.str());

        check("class fooClass{};\n"
              "void f(bool x, fooClass y) {\n"
              "    if (x == y) {\n"
              "        printf(\"foo\");\n"
              "    }\n"
              "}");
        ASSERT_EQUALS("[scripts/test.ctl:1]: (warning, inconclusive) The scope is empty: 'class'\n", errout.str()); // this shall not work !?
    }

    void comparisonOfBoolWithInt3() {
        check("void f(int y) {\n"
              "    if (y > false) {\n"
              "        printf(\"foo\");\n"
              "    }\n"
              "}");
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) Comparison of a boolean expression with an integer.\n"
                      "[scripts/test.ctl:2]: (warning) Comparison of a variable having boolean value using relational (<, >, <= or >=) operator.\n",
                      errout.str());

        check("void f(int y) {\n"
              "    if (true == y) {\n"
              "        printf(\"foo\");\n"
              "    }\n"
              "}");
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) Comparison of a boolean expression with an integer.\n", errout.str());

        check("void f(bool y) {\n"
              "    if (y == true) {\n"
              "        printf(\"foo\");\n"
              "    }\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("void f(bool y) {\n"
              "    if (false < 5) {\n"
              "        printf(\"foo\");\n"
              "    }\n"
              "}");
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) Comparison of a boolean expression with an integer other than 0 or 1.\n", errout.str());
    }

    void comparisonOfBoolWithInt4() {
        check("void f(int x) {\n"
              "    if (!x == 1) { }\n"
              "}");
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning, inconclusive) The scope is empty: 'if'\n", errout.str());
    }

    void comparisonOfBoolWithInt5() {
        check("void SetVisible(int index, bool visible) {\n"
              "    bool (SciTEBase::*ischarforsel)(char ch);\n"
              "    if (visible != GetVisible(index)) { }\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 64", errout.str());
//         ASSERT_EQUALS("", errout.str());
    }

    void comparisonOfBoolWithInt6() { // #4224 - integer is casted to bool
        check("void SetVisible(bool b, int i) {\n"
              "    if (b == (bool)i) { }\n"
              "}");
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning, inconclusive) The scope is empty: 'if'\n", errout.str());
    }

    void comparisonOfBoolWithInt7() { // #4846 - (!x==true)
        check("void f(int x) {\n"
              "    if (!x == true) { }\n"
              "}");
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning, inconclusive) The scope is empty: 'if'\n", errout.str());
    }

    // @test WinCC OA does not support pointers
    void pointerArithBool1() { // #5126
        check("void f(char *p) {\n"
              "    if (p+1){}\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 12", errout.str());
//         ASSERT_EQUALS("[scripts/test.ctl:2]: (error) Converting pointer arithmetic result to bool. The bool is always true unless there is undefined behaviour.\n", errout.str());

        check("void f(char *p) {\n"
              "    do {} while (p+1);\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 12", errout.str());
//      ASSERT_EQUALS("[scripts/test.ctl:2]: (error) Converting pointer arithmetic result to bool. The bool is always true unless there is undefined behaviour.\n", errout.str());

        check("void f(char *p) {\n"
              "    while (p-1) {}\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 12", errout.str());
//         ASSERT_EQUALS("[scripts/test.ctl:2]: (error) Converting pointer arithmetic result to bool. The bool is always true unless there is undefined behaviour.\n", errout.str());

        check("void f(char *p) {\n"
              "    for (int i = 0; p+1; i++) {}\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 12", errout.str());
//         ASSERT_EQUALS("[scripts/test.ctl:2]: (error) Converting pointer arithmetic result to bool. The bool is always true unless there is undefined behaviour.\n", errout.str());

        check("void f(char *p) {\n"
              "    if (p && p+1){}\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 12", errout.str());
//         ASSERT_EQUALS("[scripts/test.ctl:2]: (error) Converting pointer arithmetic result to bool. The bool is always true unless there is undefined behaviour.\n", errout.str());

        check("void f(char *p) {\n"
              "    if (p+2 || p) {}\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 12", errout.str());
//         ASSERT_EQUALS("[scripts/test.ctl:2]: (error) Converting pointer arithmetic result to bool. The bool is always true unless there is undefined behaviour.\n", errout.str());
    }

    // @test this syntax is not supported by WinCC OA
    void returnNonBool() {
        check("bool f(void) {\n"
              "    return 0;\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 11", errout.str());
//         ASSERT_EQUALS("", errout.str());

        check("bool f(void) {\n"
              "    return 1;\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 11", errout.str());
//         ASSERT_EQUALS("", errout.str());

        check("bool f(void) {\n"
              "    return 2;\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 11", errout.str());
//         ASSERT_EQUALS("[scripts/test.ctl:2]: (style) Non-boolean value returned from function returning bool\n", errout.str());

        check("bool f(void) {\n"
              "    return -1;\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 11", errout.str());
//         ASSERT_EQUALS("[scripts/test.ctl:2]: (style) Non-boolean value returned from function returning bool\n", errout.str());

        check("bool f(void) {\n"
              "    return 1 + 1;\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 11", errout.str());
//         ASSERT_EQUALS("[scripts/test.ctl:2]: (style) Non-boolean value returned from function returning bool\n", errout.str());

        check("bool f(void) {\n"
              "    int x = 0;\n"
              "    return x;\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 11", errout.str());
//         ASSERT_EQUALS("", errout.str());

        check("bool f(void) {\n"
              "    int x = 10;\n"
              "    return x;\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 11", errout.str());
//         ASSERT_EQUALS("[scripts/test.ctl:3]: (style) Non-boolean value returned from function returning bool\n", errout.str());

        check("bool f(void) {\n"
              "    return 2 < 1;\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 11", errout.str());
//         ASSERT_EQUALS("", errout.str());

        check("bool f(void) {\n"
              "    int ret = 0;\n"
              "    if (a)\n"
              "        ret = 1;\n"
              "    return ret;\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 11", errout.str());
//         ASSERT_EQUALS("", errout.str());

        check("bool f(void) {\n"
              "    int ret = 0;\n"
              "    if (a)\n"
              "        ret = 3;\n"
              "    return ret;\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 11", errout.str());
//         ASSERT_EQUALS("[scripts/test.ctl:5]: (style) Non-boolean value returned from function returning bool\n", errout.str());

        check("bool f(void) {\n"
              "    if (a)\n"
              "        return 3;\n"
              "    return 4;\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 11", errout.str());
//         ASSERT_EQUALS("[scripts/test.ctl:3]: (style) Non-boolean value returned from function returning bool\n"
//                       "[scripts/test.ctl:4]: (style) Non-boolean value returned from function returning bool\n", errout.str());

        check("bool f(void) {\n"
              "    return;\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 11", errout.str());
//         ASSERT_EQUALS("", errout.str());

        check("bool f(void) {\n"
              "auto x = [](void) { return -1; };\n"
              "return false;\n"
              "}\n");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 11", errout.str());
//         ASSERT_EQUALS("", errout.str());

        check("bool f(void) {\n"
              "auto x = [](void) { return -1; };\n"
              "return 2;\n"
              "}\n");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 11", errout.str());
//         ASSERT_EQUALS("[scripts/test.ctl:3]: (style) Non-boolean value returned from function returning bool\n", errout.str());

        check("bool f(void) {\n"
              "auto x = [](void) -> int { return -1; };\n"
              "return false;\n"
              "}\n");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 11", errout.str());
//         ASSERT_EQUALS("", errout.str());

        check("bool f(void) {\n"
              "auto x = [](void) -> int { return -1; };\n"
              "return 2;\n"
              "}\n");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 11", errout.str());
//         ASSERT_EQUALS("[scripts/test.ctl:3]: (style) Non-boolean value returned from function returning bool\n", errout.str());
    }


    void returnNonBoolWinCC_OA() {
        check("bool f() {\n"
              "    return 0;\n"
              "}");
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning, inconclusive) Return value 'int' does not match with declaration 'bool'.\n", errout.str());

        check("bool f() {\n"
              "    return 1;\n"
              "}");
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning, inconclusive) Return value 'int' does not match with declaration 'bool'.", errout.str());

        check("bool f() {\n"
              "    return 2;\n"
              "}");
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) Non-boolean value returned from function returning bool\n"
                      "[scripts/test.ctl:2]: (warning, inconclusive) Return value 'int' does not match with declaration 'bool'.\n",
                      errout.str());

        check("bool f() {\n"
              "    return -1;\n"
              "}");
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) Non-boolean value returned from function returning bool\n"
                      "[scripts/test.ctl:2]: (warning, inconclusive) Return value 'int' does not match with declaration 'bool'.\n",
                      errout.str());

        check("bool f() {\n"
              "    return 1 + 1;\n"
              "}");
        ASSERT_EQUALS("[scripts/test.ctl:2]: (warning) Non-boolean value returned from function returning bool\n"
                      "[scripts/test.ctl:2]: (warning, inconclusive) Return value 'int' does not match with declaration 'bool'.\n",
                      errout.str());

        check("bool f() {\n"
              "    int x = 0;\n"
              "    return x;\n"
              "}");
        ASSERT_EQUALS("[scripts/test.ctl:3]: (warning, inconclusive) Return value 'int' does not match with declaration 'bool'.\n", errout.str());

        check("bool f() {\n"
              "    int x = 10;\n"
              "    return x;\n"
              "}");
        ASSERT_EQUALS("[scripts/test.ctl:3]: (warning) Non-boolean value returned from function returning bool\n"
                      "[scripts/test.ctl:3]: (warning, inconclusive) Return value 'int' does not match with declaration 'bool'.\n", errout.str());

        check("bool f() {\n"
              "    return 2 < 1;\n"
              "}");
        ASSERT_EQUALS("", errout.str());

        check("bool f() {\n"
              "    int ret = 0;\n"
              "    if (a)\n"
              "        ret = 1;\n"
              "    return ret;\n"
              "}");
        ASSERT_EQUALS("[scripts/test.ctl:5]: (warning, inconclusive) Return value 'int' does not match with declaration 'bool'.\n"
                      "[scripts/test.ctl:3]: (warning) Undefined variable: a\n",
                      errout.str());

        check("bool f() {\n"
              "    int ret = 0;\n"
              "    if (a)\n"
              "        ret = 3;\n"
              "    return ret;\n"
              "}");
        ASSERT_EQUALS("[scripts/test.ctl:5]: (warning) Non-boolean value returned from function returning bool\n"
                      "[scripts/test.ctl:5]: (warning, inconclusive) Return value 'int' does not match with declaration 'bool'.\n"
                      "[scripts/test.ctl:3]: (warning) Undefined variable: a\n",
                      errout.str());

        check("bool f() {\n"
              "    if (a)\n"
              "        return 3;\n"
              "    return 4;\n"
              "}");
        ASSERT_EQUALS("[scripts/test.ctl:3]: (warning) Non-boolean value returned from function returning bool\n"
                      "[scripts/test.ctl:4]: (warning) Non-boolean value returned from function returning bool\n"
                      "[scripts/test.ctl:3]: (warning, inconclusive) Return value 'int' does not match with declaration 'bool'.\n"
                      "[scripts/test.ctl:4]: (warning, inconclusive) Return value 'int' does not match with declaration 'bool'.\n"
                      "[scripts/test.ctl:2]: (warning) Undefined variable: a\n",
                      errout.str());

        check("bool f() {\n"
              "    return;\n"
              "}");
        ASSERT_EQUALS("WinCC OA syntax error at pos: 21", errout.str()); // this looks like a bug. Because it shall return some value, but in the line 2 is return only.
    }

    void winCC_OA_const()
    {
        check("bool f() {\n"
              "return false;\n"
              "}\n");
        ASSERT_EQUALS("", errout.str());
        check("bool f() {\n"
              "return FALSE;\n"
              "}\n");
        ASSERT_EQUALS("", errout.str());

        check("bool f() {\n"
              "return true;\n"
              "}\n");
        ASSERT_EQUALS("", errout.str());
        check("bool f() {\n"
              "return TRUE;\n"
              "}\n");
        ASSERT_EQUALS("", errout.str());

        check("bool f() {\n"
              "return TRUE == true;\n"
              "}\n");
        ASSERT_EQUALS("", errout.str());

        check("bool f() {\n"
              "return TRUE == false;\n"
              "}\n");
        ASSERT_EQUALS("", errout.str());

        check("bool f() {\n"
              "return TRUE != false;\n"
              "}\n");
        ASSERT_EQUALS("", errout.str());
    }
};

// REGISTER_TEST(TestBool)

void main()
{
  TestBool test;
  test.run();
}
