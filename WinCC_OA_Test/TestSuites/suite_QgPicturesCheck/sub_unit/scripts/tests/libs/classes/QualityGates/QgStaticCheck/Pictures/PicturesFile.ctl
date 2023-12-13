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
#uses "classes/QualityGates/QgStaticCheck/Pictures/PicturesFile" /*!< tested object */
#uses "classes/oaTest/OaTest"
#uses "classes/QualityGates/QgResult"

//--------------------------------------------------------------------------------
class TstQgPicturesFile : OaTest
{

  //------------------------------------------------------------------------------
  public dyn_string getAllTestCaseIds()
  {
    // list with our testcases
    return makeDynString("PicturesFile::validate");
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
      case "PicturesFile::validate":
      {
        string tmpDir = _makeTmpDir();
        PicturesFile picture = PicturesFile(tmpDir + "file1.thhhhxt");

        // *** invalid extension
        fclose(fopen(tmpDir + "file1.thhhhxt", "wb+"));
        picture.calculate();

        assertEqual(picture.validate(), 0);
        assertEqual(picture.result.errorPoints, 1);
        assertDynContains(QgResult::getLastErrors(), "file.extension");
        QgResult::clearLastErr();
        remove(tmpDir + "file1.thhhhxt");

        // *** valid file
        picture = PicturesFile(getPath(PICTURES_REL_PATH, "WinCCOA_QualityChecks/WinCCOA_QualityChecks.png"));
        // copyFile(getPath(PICTURES_REL_PATH, "WinCCOA_QualityChecks/WinCCOA_QualityChecks.png"), tmpDir);
        picture.calculate();
        assertEqual(picture.validate(), 0);
        assertEqual(picture.result.errorPoints, 0);

        return 0;
      }
    }

    return -1;
  }

  //------------------------------------------------------------------------------
  string _makeTmpDir()
  {
    string tmpDir = PROJ_PATH + PICTURES_REL_PATH + "QgPictureDir/";

    // create tmp-dir for this test
    if (isdir(tmpDir))
      rmdir(tmpDir, TRUE);

    mkdir(tmpDir);

    return tmpDir;
  }
};

//--------------------------------------------------------------------------------
void main()
{
  TstQgPicturesFile test;
  test.startAll();
}
