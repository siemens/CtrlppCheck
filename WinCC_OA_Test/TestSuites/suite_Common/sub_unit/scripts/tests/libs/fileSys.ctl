/*!
 * @brief Tests for lib: fileSys
 *
 * @author lschopp
 */

//--------------------------------------------------------------------------------
// used libraries (#uses)
#uses "fileSys" /*!< tested object */
#uses "classes/oaTest/OaTest"

//--------------------------------------------------------------------------------
class TstFileSys : OaTest
{
  public dyn_string getAllTestCaseIds()
  {
    // list with our testcases
    return makeDynString("fileSys getFileNamesRecursive");
  }

  protected int startTestCase(const string tcId)
  {
    switch (tcId)
    {
      case "fileSys getFileNamesRecursive":
      {
        fclose(fopen(PROJ_PATH + LIBS_REL_PATH + "dummy.ctl", "w"));
        assertEqual(getFileNamesRecursive(""), makeDynString());
        assertEqual(getFileNamesRecursive("non existin path"), makeDynString());
        assertEqual(dynlen(getFileNamesRecursive(PROJ_PATH + PANELS_REL_PATH, "panel*")), 0);
        assertEqual(dynlen(getFileNamesRecursive(PROJ_PATH + LIBS_REL_PATH, "*.ctl", FILTER_DIRS)), 0);
        assertEqual(getFileNamesRecursive(PROJ_PATH + LIBS_REL_PATH, "*.ctl"), makeDynString(makeNativePath(PROJ_PATH + LIBS_REL_PATH + "dummy.ctl")));
        return 0;
      }
    }

    return -1;
  }
};

//--------------------------------------------------------------------------------
main()
{
  TstFileSys test;
  test.startAll();
}
