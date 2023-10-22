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
#uses "classes/QualityGates/QgStaticCheck/Pictures/PicturesDir"/*!< tested object */
#uses "classes/oaTest/OaTest"
#uses "classes/QualityGates/QgResult"

//--------------------------------------------------------------------------------
class TstQgPicturesDir : OaTest
{

  //------------------------------------------------------------------------------
  public dyn_string getAllTestCaseIds()
  {
    // list with our testcases
    return makeDynString("PicturesDir::ctor",
                         "PicturesDir::exists",
                         "PicturesDir::calculate",
                         "PicturesDir::validate");
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
      case "PicturesDir::ctor":
      {
        PicturesDir dir = PicturesDir();
        assertEqual(dir.getName(), "");
        assertEqual(dir.getCountOfFiles(), 0);
        assertEqual(dir.getCountOfSubDirs(), 0);

        dir.setDir(PROJ_PATH);
        assertEqual(dir.getName(), PROJ);
        assertEqual(dir.getCountOfFiles(), 0);
        assertEqual(dir.getCountOfSubDirs(), 0);

        return 0;
      }

      case "PicturesDir::exists":
      {
        PicturesDir dir = PicturesDir();
        assertFalse(dir.exists());
        dir.setDir(PROJ_PATH);
        assertTrue(dir.exists());
        dir.setDir(PROJ_PATH + "abc");
        assertFalse(dir.exists());
        return 0;
      }

      case "PicturesDir::calculate":
      {
        PicturesDir dir = PicturesDir();

        // not existing
        assertEqual(dir.calculate(), -1);
        assertEqual(dir.getCountOfFiles(), 0);
        assertEqual(dir.getCountOfFilesRecursive(), 0);
        assertEqual(dir.getCountOfSubDirs(), 0);

        // existing, but empty
        string tmpDir = _makeTmpDir();
        dir.setDir(tmpDir);

        assertEqual(dir.calculate(), 0);
        assertEqual(dir.getCountOfFiles(), 0);
        assertEqual(dir.getCountOfFilesRecursive(), 0);
        assertEqual(dir.getCountOfSubDirs(), 0);

        // existing with 2 sub dirs and 1 file
        mkdir(tmpDir + "subDir1");
        mkdir(tmpDir + "subDir2");
        fclose(fopen(tmpDir + "file.png", "wb+"));

        assertEqual(dir.calculate(), 0);
        assertEqual(dir.getCountOfFiles(), 1);
        assertEqual(dir.getCountOfFilesRecursive(), 1);
        assertEqual(dir.getCountOfSubDirs(), 2);


        // existing witth 3 files in defirent sub dirs
        fclose(fopen(tmpDir + "subDir1/file.PNG", "wb+"));
        fclose(fopen(tmpDir + "subDir1/file.txt", "wb+"));

        assertEqual(dir.calculate(), 0);
        assertEqual(dir.getCountOfFiles(), 1);
        assertEqual(dir.getCountOfFilesRecursive(), 3);
        assertEqual(dir.getCountOfSubDirs(), 2);

        dyn_anytype childs = dir.getSubDirs();
        assertEqual(dynlen(childs), 2);
        assertEqual(childs[1].getName(), "subDir1");


        dir.setDir(PROJ_PATH);
        assertTrue(dir.exists());
        dir.setDir(PROJ_PATH + "abc");
        assertFalse(dir.exists());

        rmdir(tmpDir, TRUE);
        return 0;
      }

      case "PicturesDir::validate":
      {
        PicturesDir dir = PicturesDir();
        string tmpDir = _makeTmpDir();

        dir.setDir(tmpDir);

        // *** check empty
        dir.calculate();
        assertEqual(dir.validate(), 0);
        assertEqual(dir.result.errorPoints, 1);
        assertEqual(dir.result.totalPoints, 1);
        assertDynContains(QgResult::getLastErrors(), "dir.isEmpty");
        QgResult::clearLastErr();

        // *** check direcotry with 1 sub-dir without any files
        mkdir(tmpDir + "one-empty");
        assertTrue(isdir(tmpDir + "one-empty"), tmpDir + "one-empty");
        dir.calculate();
        assertEqual(dir.validate(), 0);
        // dir (2 cheks) + sub dir (1 check) 
        assertEqual(dir.result.errorPoints, 2);
        assertEqual(dir.result.totalPoints, 3);
        assertDynContains(QgResult::getLastErrors(), "dir.hasFilesRecursive");
        assertDynContains(QgResult::getLastErrors(),"dir.isEmpty");
        rmdir(tmpDir + "one-empty", true);
        assertFalse(isdir(tmpDir + "one-empty"), tmpDir + "one-empty");
        QgResult::clearLastErr();

        // *** check with 10 files, try it with different extentions
        mkdir(tmpDir + "10-files");
        dir.setDir(tmpDir + "10-files");
        fclose(fopen(dir.getDirPath() + "file1.png", "wb+"));
        fclose(fopen(dir.getDirPath() + "file2.PNG", "wb+"));
        fclose(fopen(dir.getDirPath() + "file3.jPg", "wb+"));
        fclose(fopen(dir.getDirPath() + "file4.JpG", "wb+"));
        fclose(fopen(dir.getDirPath() + "file5.png", "wb+"));
        fclose(fopen(dir.getDirPath() + "file6.png", "wb+"));
        fclose(fopen(dir.getDirPath() + "file7.png", "wb+"));
        fclose(fopen(dir.getDirPath() + "file8.png", "wb+"));
        fclose(fopen(dir.getDirPath() + "file9.png", "wb+"));
        fclose(fopen(dir.getDirPath() + "file10.png", "wb+"));
        dir.calculate();

        assertEqual(dir.getCountOfFiles(), 10);
        assertEqual(dir.getCountOfFilesRecursive(), 10);

        assertEqual(dir.validate(), 0);
        assertEqual(dir.result.errorPoints, 0);
        assertEqual(dir.result.totalPoints, 24);
        // ! after validations are the file objects removed (to spare memory)
        // therefor you will have count 0
        assertEqual(dir.getCountOfFiles(), 0);
        rmdir(tmpDir + "10-files", true);

        // *** too mutch files
        mkdir(tmpDir + "too-much-files");
        dir.setDir(tmpDir + "too-much-files");
        shared_ptr<QgSettings> settings = new QgSettings("PicturesDir.dir.filesCount");
        int limit = settings.getHighLimit(-1);
        this.info("Create " + limit + " files");

        for(int i = 1; i <= limit; i++)
        {
          fclose(fopen(dir.getDirPath() + "file" + i + ".png", "wb+"));
        }
        
        this.info("Create one-extra-file file");
        fclose(fopen(dir.getDirPath() + "one-extra-file.png", "wb+"));

        dir.calculate();

        assertEqual(dir.getCountOfFiles(), limit + 1);
        assertEqual(dir.getCountOfFilesRecursive(), limit + 1);

        assertEqual(dir.validate(), 0);
        // assertEqual(dir.result.errorPoints, 1);
        // assertEqual(dir.result.totalPoints, 25);
        assertDynContains(QgResult::getLastErrors(), "dir.filesCount");
        QgResult::clearLastErr();
        rmdir(tmpDir, true);
        assertFalse(isdir(tmpDir), tmpDir);

        // *** try with 5 sub dirs
        mkdir(tmpDir + "5-sub-dirs");
        dir.setDir(tmpDir + "5-sub-dirs");
        mkdir(dir.getDirPath() + "subDir1");
        fclose(fopen(dir.getDirPath() + "subDir1/file1.png", "wb+"));
        mkdir(dir.getDirPath() + "subDir2");
        fclose(fopen(dir.getDirPath() + "subDir2/file1.png", "wb+"));
        mkdir(dir.getDirPath() + "subDir3");
        fclose(fopen(dir.getDirPath() + "subDir3/file1.png", "wb+"));
        mkdir(dir.getDirPath() + "subDir4");
        fclose(fopen(dir.getDirPath() + "subDir4/file1.png", "wb+"));
        mkdir(dir.getDirPath() + "subDir5");
        fclose(fopen(dir.getDirPath() + "subDir5/file1.png", "wb+"));
        dir.calculate();

        assertEqual(dir.validate(), 0);
        assertEqual(dir.result.errorPoints, 0);
        assertEqual(dir.result.totalPoints, 34);
        assertEqual(dir.getCountOfFiles(), 0);
        assertEqual(dir.getCountOfFilesRecursive(), 5);
        QgResult::clearLastErr();
        rmdir(tmpDir + "5-sub-dirs", true);

        // *** too mutch sub dirs
        mkdir(tmpDir + "too-mutch-sub-dirs");
        dir.setDir(tmpDir + "too-mutch-sub-dirs");
        limit = settings.getHighLimit(-1);
        this.info("Create " + limit + " sub directories");

        for(int i = 1; i <= limit; i++)
        {
          mkdir(dir.getDirPath() + "subDir" + i);
          fclose(fopen(dir.getDirPath() + "subDir" + i + "/file1.png", "wb+"));
        }
        
        this.info("Create extra-subDir");
        mkdir(dir.getDirPath() + "extra-subDir");
        fclose(fopen(dir.getDirPath() + "extra-subDir/file1.png", "wb+"));
        dir.calculate();

        assertEqual(dir.validate(), 0);
        assertEqual(dir.result.errorPoints, 1);
        assertDynContains(QgResult::getLastErrors(), "dir.subDirCount");
        QgResult::clearLastErr();
        rmdir(tmpDir + "too-mutch-sub-dirs", true);

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
  TstQgPicturesDir test;
  test.startAll();
}
