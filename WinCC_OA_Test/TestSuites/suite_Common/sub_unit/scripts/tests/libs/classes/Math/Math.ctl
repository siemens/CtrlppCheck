/*!
 * @brief Tests for class: Math
 *
 * @author lschopp
 */

//--------------------------------------------------------------------------------
// used libraries (#uses)
#uses "classes/Math/Math"         /*!< tested object */
#uses "classes/oaTest/OaTest"

//--------------------------------------------------------------------------------
// declare variables and constans

//--------------------------------------------------------------------------------
class TstMath : OaTest
{
  public dyn_string getAllTestCaseIds()
  {
    // list with our testcases
    return makeDynString("Math::getPercent()");
  }

  protected int startTestCase(const string tcId)
  {
    switch (tcId)
    {
      case "Math::getPercent()":
      {
        assertEqual(Math::getPercent(100, 0), 0.0);
        assertEqual(Math::getPercent(100, 5), 5.0);
        assertEqual(Math::getPercent(500, 5), 1.0);
        assertEqual(Math::getPercent(100, 100), 100.0);
        assertEqual(Math::getPercent(0, 100), 0.0);
        assertEqual(Math::getPercent(-100, 1), -1.0);
        return 0;
      }
    }

    return -1;
  }
};

//--------------------------------------------------------------------------------
main()
{
  TstMath test = TstMath();
  test.startAll();
  exit(0);
}
