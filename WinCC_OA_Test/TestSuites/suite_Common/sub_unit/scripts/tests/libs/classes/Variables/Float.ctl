/*!
 * @brief Tests for class: Float
 *
 * @author lschopp
 */

//--------------------------------------------------------------------------------
// used libraries (#uses)
#uses "classes/Variables/Float" /*!< tested object */
#uses "classes/oaTest/OaTest"

//--------------------------------------------------------------------------------
// declare variables and constans

//--------------------------------------------------------------------------------
class TstFloat : OaTest
{

  public dyn_string getAllTestCaseIds()
  {
    // list with our testcases
    return makeDynString("Float");
  }

  protected int startTestCase(const string tcId)
  {
    switch (tcId)
    {
      case "Float":
      {
        float f1, f2;
        f1 = 100;
        f2 = 33;
        Float f = Float(f1 / f2);

        assertEqual((string)f.round(), "3.03");
        assertEqual((string)f.round(4), "3.0303");
        assertEqual((string)f.round(0), "3");

        f.set(f2 / f1);
        assertEqual((string)f.round(), "0.33");

        f.set(0);
        assertEqual((string)f.round(), "0");

        f.set(1.4356);
        assertEqual((string)f.round(), "1.44");
        assertEqual(f.get(), 1.4356);

        f.set(1.4323);
        assertEqual((string)f.round(), "1.43");
        assertEqual(f.get(), 1.4323);

        f.set(1.4323);
        assertEqual((string)f.round(-1), "1");

        return 0;
      }
    }

    return -1;
  }
};

//--------------------------------------------------------------------------------
main()
{
  TstFloat test = TstFloat();

  test.startAll();

  exit(0);
}
