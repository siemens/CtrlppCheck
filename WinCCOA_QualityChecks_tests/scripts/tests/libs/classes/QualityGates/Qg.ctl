/*!
 * @brief Tests for class: Qg
 *
 * @author lschopp
 */

//--------------------------------------------------------------------------------
// used libraries (#uses)
#uses "classes/QualityGates/Qg" /*!< tested object */
#uses "classes/StTest" /*!< oaTest basic class */

//--------------------------------------------------------------------------------
// declare variables and constans

//--------------------------------------------------------------------------------
class TstQg : StTest
{
  public dyn_string _getAllTcIds()
  {
    // list with our testcases
    return makeDynString("Qg::get/setId()");
  }

  public int _startTest()
  {
    const string tcId = _getTcId();

    switch( tcId )
    {
      case "Qg::get/setId()":
      {
        oaUnitAssertEqual(tcId, Qg::getId(), "");
        Qg::setId("some one ID");
        oaUnitAssertEqual(tcId, Qg::getId(), "some one ID");        
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
