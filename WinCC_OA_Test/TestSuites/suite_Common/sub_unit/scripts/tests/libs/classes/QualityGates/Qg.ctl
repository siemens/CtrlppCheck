/*!
 * @brief Tests for class: Qg
 *
 * @author lschopp
 */

//--------------------------------------------------------------------------------
// used libraries (#uses)
#uses "classes/QualityGates/Qg" /*!< tested object */
#uses "classes/oaTest/OaTest"

//--------------------------------------------------------------------------------
// declare variables and constans

//--------------------------------------------------------------------------------
class TstQg : OaTest
{
  public dyn_string getAllTestCaseIds()
  {
    // list with our testcases
    return makeDynString("Qg::get/setId()");
  }

  protected int startTestCase(const string tcId)
  {
    switch( tcId )
    {
      case "Qg::get/setId()":
      {
        assertEqual(Qg::getId(), "");
        Qg::setId("some one ID");
        assertEqual(Qg::getId(), "some one ID");        
        return 0;
      }
    }

    return -1;
  }
};

//--------------------------------------------------------------------------------
main()
{
  TstQg test = TstQg();

  test.startAll();

  exit(0);
}
