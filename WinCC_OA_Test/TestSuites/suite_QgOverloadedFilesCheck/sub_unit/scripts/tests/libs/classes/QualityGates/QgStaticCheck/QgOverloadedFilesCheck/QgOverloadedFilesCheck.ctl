//--------------------------------------------------------------------------------
/**
  @file $relPath
  @copyright Copyright 2023 SIEMENS AG
             SPDX-License-Identifier: GPL-3.0-only
*/

/*!
 * @brief Tests for class: Qg
 *
 * @author lschopp
 */

//--------------------------------------------------------------------------------
// used libraries (#uses)
#uses "classes/QualityGates/QgOverloadedFilesCheck/QgOverloadedFilesCheck"/*!< tested object */
#uses "classes/oaTest/OaTest"
#uses "classes/QualityGates/QgResult"

//--------------------------------------------------------------------------------
class TstQgOverloadedFilesCheck : OaTest
{

  //------------------------------------------------------------------------------
  public dyn_string getAllTestCaseIds()
  {
    // list with our testcases
    return makeDynString("QgOverloadedFilesCheck::ctor",
                         "QgOverloadedFilesCheck::calculate",
                         "QgOverloadedFilesCheck::validate");
  }

  //------------------------------------------------------------------------------
  public int setUp()
  {
    if (OaTest::setUp())
      return -1;

    // eliminate false positives
    QgResult::selfTest = true;
    return 0;
  }

  //------------------------------------------------------------------------------
  protected int startTestCase(const string tcId)
  {
    switch (tcId)
    {
      case "QgOverloadedFilesCheck::ctor":
      {
        QgOverloadedFilesCheck check = QgOverloadedFilesCheck();
        assertEqual(check.projPathToCheck, PROJ_PATH);
        return 0;
      }

      case "QgOverloadedFilesCheck::calculate":
      {
        QgOverloadedFilesCheck check = QgOverloadedFilesCheck();
        check.projPathToCheck = "oijas0dij0adji";

        // not existing
        assertEqual(check.calculate(), -1);

        check.projPathToCheck = PROJ_PATH;
        assertEqual(check.calculate(), 0);

        return 0;
      }

      case "QgOverloadedFilesCheck::validate":
      {
        QgOverloadedFilesCheck check = QgOverloadedFilesCheck();
        check.projPathToCheck = PROJ_PATH + "_EMPTY__/";
        mkdir(check.projPathToCheck);
        mkdir(check.projPathToCheck + SCRIPTS_REL_PATH);

        check.calculate();
        assertEqual(check.validate(), 0);
        assertEqual(check.result.errorPoints, 0);
        QgResult::clearLastErr();

        fclose(fopen(check.projPathToCheck + SCRIPTS_REL_PATH + "some-one-script.ctl", "wb+"));
        assertTrue(isfile(check.projPathToCheck + SCRIPTS_REL_PATH + "some-one-script.ctl"));
        check.calculate();
        assertEqual(check.validate(), 0);
        assertEqual(check.result.errorPoints, 0);
        QgResult::clearLastErr();
        
        // copy some one file from WinCC OA
        copyFile(WINCCOA_PATH + SCRIPTS_REL_PATH + "userPara.ctl", check.projPathToCheck + SCRIPTS_REL_PATH);
        assertTrue(isfile(check.projPathToCheck + SCRIPTS_REL_PATH + "userPara.ctl"));
        check.calculate();
        assertEqual(check.validate(), 0);
        assertEqual(check.result.errorPoints, 0); // warning only
        assertDynContains(QgResult::getLastErrors(), "isOverloadedAllowed");
        QgResult::clearLastErr();
        remove(check.projPathToCheck + SCRIPTS_REL_PATH + "about.ctl");

        
        copyFile(WINCCOA_PATH + SCRIPTS_REL_PATH + "about.ctl", check.projPathToCheck + SCRIPTS_REL_PATH);
        assertTrue(isfile(check.projPathToCheck + SCRIPTS_REL_PATH + "about.ctl"));
        check.calculate();
        assertEqual(check.validate(), 0);
        assertEqual(check.result.errorPoints, 1);
        assertDynContains(QgResult::getLastErrors(), "isOverloaded");
        QgResult::clearLastErr();
        remove(check.projPathToCheck + SCRIPTS_REL_PATH + "about.ctl");

        return 0;
      }
    }

    return -1;
  }

};

//--------------------------------------------------------------------------------
void main()
{
  TstQgOverloadedFilesCheck test;
  test.startAll();
}
