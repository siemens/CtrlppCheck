#uses "classes/QualityGates/Tools/CppCheck/TestFixture"

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
