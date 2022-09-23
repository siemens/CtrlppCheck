/*!
 * @brief Tests for class: Math
 *
 * @author lschopp
 */

//--------------------------------------------------------------------------------
// used libraries (#uses)
#uses "classes/Math/Math"         /*!< tested object */
#uses "classes/WinCCOA/StTest" /*!< oaTest basic class */

//--------------------------------------------------------------------------------
// declare variables and constans

//--------------------------------------------------------------------------------
class TstMath : StTest
{
  public dyn_string _getAllTcIds()
  {
    // list with our testcases
    return makeDynString("Math::getPercent()");
  }

  public int _startTest()
  {
    const string tcId = _getTcId();

    switch( tcId )
    {
      case "Math::getPercent()":
      {
        oaUnitAssertEqual(tcId, Math::getPercent(100, 0), 0.0);
        oaUnitAssertEqual(tcId, Math::getPercent(100, 5), 5.0);
        oaUnitAssertEqual(tcId, Math::getPercent(500, 5), 1.0);
        oaUnitAssertEqual(tcId, Math::getPercent(100, 100), 100.0);
        oaUnitAssertEqual(tcId, Math::getPercent(0, 100), 0.0);
        oaUnitAssertEqual(tcId, Math::getPercent(-100, 1), -1.0);
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
