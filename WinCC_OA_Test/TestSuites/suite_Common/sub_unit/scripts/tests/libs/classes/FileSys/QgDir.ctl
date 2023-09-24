/*!
 * @brief Tests for class: QgDir
 *
 * @author lschopp
 */

//--------------------------------------------------------------------------------
// used libraries (#uses)
#uses "classes/FileSys/QgDir" /*!< tested object */
#uses "classes/oaTest/OaTest"

//--------------------------------------------------------------------------------
class TstDir : OaTest
{
  public dyn_string getAllTestCaseIds()
  {
    // list with our testcases
    return makeDynString("QgDir");
  }

  protected int startTestCase(const string tcId)
  {
    switch( tcId )
    {
      case "QgDir":
      {
        QgDir dir = QgDir();
        QgDir dir2 = QgDir("non existing/ path");
        
        assertEqual(dir.getDirPath(), "");
        assertEqual(dir2.getDirPath(), makeNativePath("non existing/ path"));
        
        assertFalse(dir.exists());        
        assertFalse(dir2.exists());
        
        dir.setDirPath(PROJ_PATH);
        assertEqual(dir.getDirPath(), makeNativePath(PROJ_PATH));
        assertTrue(dir.exists());
        
        dir.setDirPath(PROJ_PATH + createUuid());
        assertFalse(dir.exists());
        assertEqual(dir.mk(), 0);
        assertTrue(dir.exists());
        assertEqual(dir.mk(), 0);
        assertTrue(dir.exists());
        assertEqual(dir.rm(), 0);     
        assertFalse(dir.exists());

        string dirPath = dir.getDirPath();
        dir.setDirPath(dirPath + "/a/b/123/c");
        assertFalse(dir.exists());
        assertEqual(dir.mk(), 0);
        assertTrue(dir.exists());
        assertEqual(dir.rm(), 0);
        assertFalse(dir.exists());
        
        
        dir.setDirPath(dirPath);
        assertTrue(dir.exists());
        assertEqual(dir.rm(), 0);
        assertFalse(dir.exists());

        assertEqual(dir.rm(), -1);
        assertEqual(dir2.rm(), -1);
                
        dir.setDirPath("");
        assertEqual(dir.rm(), -1);
        
        return 0;
      }
    }

    return -1;
  }
};

//--------------------------------------------------------------------------------
main()
{
  TstDir test;
  test.startAll();
}
