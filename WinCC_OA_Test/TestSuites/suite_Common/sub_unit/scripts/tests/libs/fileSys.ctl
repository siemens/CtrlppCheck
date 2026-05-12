//--------------------------------------------------------------------------------
/**
  @file $relPath
  @copyright Copyright 2026 SIEMENS AG
             SPDX-License-Identifier: GPL-3.0-only
*/

/*!
 * @brief Tests for library: fileSys
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
    return makeDynString("fileSys");
  }

  protected int startTestCase(const string tcId)
  {
    switch (tcId)
    {
      case "fileSys":
      {
        dyn_string paths = getSubProjPathes();
        int n = dynlen(paths);

        assertTrue(n >= 0);

        // The function starts at getPath(..., 2) and appends consecutively.
        if (n > 0)
        {
          assertEqual(paths[1], getPath("", "", -1, 2));
          assertEqual(paths[n], getPath("", "", -1, n + 1));
        }

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
  test.startAll();
  exit(0);
}
