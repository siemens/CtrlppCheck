/*!
 * @brief Tests for class: Float 
 *
 * @author lschopp
 */

//--------------------------------------------------------------------------------
// used libraries (#uses)
#uses "classes/Variables/Float" /*!< tested object */
#uses "classes/WinCCOA/StTest" /*!< oaTest basic class */

//--------------------------------------------------------------------------------
// declare variables and constans

//--------------------------------------------------------------------------------
class TstFloat : StTest
{
  
  public dyn_string _getAllTcIds()
  {
    // list with our testcases
    return makeDynString("Float");
  }

  public int _startTest()
  {
    const string tcId = _getTcId();

    switch( tcId )
    {
      case "Float":
      {
        float f1, f2;
        f1 = 100;
        f2 = 33;
        Float f = Float(f1 / f2);
  
        oaUnitAssertEqual(tcId, (string)f.round(), "3.03");
        oaUnitAssertEqual(tcId, (string)f.round(4), "3.0303");
        oaUnitAssertEqual(tcId, (string)f.round(0), "3");
  
        f.set(f2 / f1);
        oaUnitAssertEqual(tcId, (string)f.round(), "0.33");
  
        f.set(0);
        oaUnitAssertEqual(tcId, (string)f.round(), "0");
  
        f.set(1.4356);
        oaUnitAssertEqual(tcId, (string)f.round(), "1.44");
        oaUnitAssertEqual(tcId, f.get(), 1.4356);
        
        f.set(1.4323);
        oaUnitAssertEqual(tcId, (string)f.round(), "1.43");
        oaUnitAssertEqual(tcId, f.get(), 1.4323);
                
        f.set(1.4323);
        oaUnitAssertEqual(tcId, (string)f.round(-1), "1");
        
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
