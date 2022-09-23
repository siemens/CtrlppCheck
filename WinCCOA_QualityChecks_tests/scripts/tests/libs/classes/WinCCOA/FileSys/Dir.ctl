/*!
 * @brief Tests for class: Dir
 *
 *
 * @author lschopp
 */

//--------------------------------------------------------------------------------
// used libraries (#uses)
#uses "classes/WinCCOA/FileSys/Dir" /*!< tested object */
#uses "classes/WinCCOA/StTest" /*!< oaTest basic class */

//--------------------------------------------------------------------------------
// declare variables and constans

//--------------------------------------------------------------------------------
class TstDir : StTest
{
  public dyn_string _getAllTcIds()
  {
    // list with our testcases
    return makeDynString("Dir");
  }

  public int _startTest()
  {
    const string tcId = _getTcId();

    switch( tcId )
    {
      case "Dir":
      {
        Dir dir = Dir();
        Dir dir2 = Dir("non existing/ path");
        
        oaUnitAssertEqual(tcId, dir.getDirPath(), "");
        oaUnitAssertEqual(tcId, dir2.getDirPath(), makeNativePath("non existing/ path"));
        
        oaUnitAssertFalse(tcId, dir.exists());        
        oaUnitAssertFalse(tcId, dir2.exists());
        
        dir.setDirPath(PROJ_PATH);
        oaUnitAssertEqual(tcId, dir.getDirPath(), makeNativePath(PROJ_PATH));
        oaUnitAssertTrue(tcId, dir.exists());
        
        dir.setDirPath(PROJ_PATH + createUuid());
        oaUnitAssertFalse(tcId, dir.exists());
        oaUnitAssertEqual(tcId, dir.mk(), 0);
        oaUnitAssertTrue(tcId, dir.exists());
        oaUnitAssertEqual(tcId, dir.mk(), 0);
        oaUnitAssertTrue(tcId, dir.exists());
        oaUnitAssertEqual(tcId, dir.rm(), 0);     
        oaUnitAssertFalse(tcId, dir.exists());

        string dirPath = dir.getDirPath();
        dir.setDirPath(dirPath + "/a/b/123/c");
        oaUnitAssertFalse(tcId, dir.exists());
        oaUnitAssertEqual(tcId, dir.mk(), 0);
        oaUnitAssertTrue(tcId, dir.exists());
        oaUnitAssertEqual(tcId, dir.rm(), 0);
        oaUnitAssertFalse(tcId, dir.exists());
        
        
        dir.setDirPath(dirPath);
        oaUnitAssertTrue(tcId, dir.exists());
        oaUnitAssertEqual(tcId, dir.rm(), 0);
        oaUnitAssertFalse(tcId, dir.exists());

        oaUnitAssertEqual(tcId, dir.rm(), -1);
        oaUnitAssertEqual(tcId, dir2.rm(), -1);
                
        dir.setDirPath("");
        oaUnitAssertEqual(tcId, dir.rm(), -1);
        
        return 0;
      }
    }

    return -1;
  }
};

//--------------------------------------------------------------------------------
main()
{
  TstDir test = TstDir();

  test.startAll();

  exit(0);
}
