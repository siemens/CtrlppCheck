/*!
 * @brief Tests for lib: fileSys
 *
 * @author lschopp
 */

//--------------------------------------------------------------------------------
// used libraries (#uses)
#uses "fileSys" /*!< tested object */
#uses "classes/WinCCOA/StTest" /*!< oaTest basic class */

//--------------------------------------------------------------------------------
// declare variables and constans

//--------------------------------------------------------------------------------
class TstFileSys : StTest
{  
  public dyn_string _getAllTcIds()
  {
    // list with our testcases
    return makeDynString("fileSys getFileNamesRecursive");
  }

  public int _startTest()
  {
    const string tcId = _getTcId();

    switch( tcId )
    {
      case "fileSys getFileNamesRecursive":
      {
        oaUnitAssertEqual(tcId, getFileNamesRecursive(""), makeDynString());
        oaUnitAssertEqual(tcId, getFileNamesRecursive("non existin path"), makeDynString());
        oaUnitAssertEqual(tcId, dynlen(getFileNamesRecursive(PROJ_PATH, "panel*")), 0);
        oaUnitAssertEqual(tcId, dynlen(getFileNamesRecursive(PROJ_PATH, "*.ctl", FILTER_DIRS)), 0);
        oaUnitAssertGreater(tcId, dynlen(getFileNamesRecursive(PROJ_PATH, "*.ctl")), 0);
        return 0;
      }
    }

    return -1;
  }
};

//--------------------------------------------------------------------------------
main()
{
  TstFileSys test = TstFileSys();

  test.start();

  exit(0);
}
