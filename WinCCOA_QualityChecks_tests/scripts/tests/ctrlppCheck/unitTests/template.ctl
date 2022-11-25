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

struct TestSomething : public TestFixture
{
  public TestSomething()
  {
  }

  public void run() {
  // define settings
  settings.addEnabled("portability");

  // define test cases
  TEST_CASE(novardecl);
  // some other test cases ...
  }

  /** @test comment the test case here like: portybility test for function params
  */
  
  void functionpar() {
    check("int foo(int *p)\n"
          "{\n"
          "  int a = p;\n"
          "  return a + 4;\n"
          "}");
    ASSERT_EQUALS("[test.ctl:3]: (portability) Assigning a pointer to an integer is not portable.", errout.str());

    check("int foo(int p[])\n"
        "{\n"
        "  int a = p;\n"
        "  return a + 4;\n"
        "}");
    ASSERT_EQUALS("[test.ctl:3]: (portability) Assigning a pointer to an integer is not portable.", errout.str());
    // ... some more assertions
  }

};

void main()
{
  TestSomething test;
  test.run();
}
