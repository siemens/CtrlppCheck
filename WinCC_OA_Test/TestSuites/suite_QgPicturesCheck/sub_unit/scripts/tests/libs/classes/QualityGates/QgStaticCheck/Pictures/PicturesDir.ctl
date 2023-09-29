/*!
 * @brief Tests for class: Qg
 *
 * @author lschopp
 */

//--------------------------------------------------------------------------------
// used libraries (#uses)
#uses "classes/QualityGates/QgStaticCheck/Pictures/PicturesDir"/*!< tested object */
#uses "classes/oaTest/OaTest"
#uses "classes/QualityGates/QgVersionResult"

//--------------------------------------------------------------------------------
class TstQg : OaTest
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
    if ( OaTest::setUp() )
     return -1;

    // eliminate fail positives
    QgVersionResult::enableOaTestCheck = false;
    return 0;
  }

  //------------------------------------------------------------------------------
  protected int startTestCase(const string tcId)
  {
    switch( tcId )
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
        string tmpDir = _makeTmpDir();
        
        // not existing
        assertEqual(dir.calculate(), -1);
        assertEqual(dir.getCountOfFiles(), 0);
        assertEqual(dir.getCountOfFilesRecursive(), 0);
        assertEqual(dir.getCountOfSubDirs(), 0);
        
        // existing, but empty
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

        // check empty
        dir.calculate();
        assertEqual(dir.validate(), 0);
        assertEqual(dir.result.errorPoints, 1);
        assertEqual(dir.result.totalPoints, 1);
        assertEqual(QgVersionResult::lastErr, "reason.dir.isEmpty");
//         return ;
        
        // check with 10 files, try it with different extentions
        fclose(fopen(tmpDir + "file1.png", "wb+"));
        fclose(fopen(tmpDir + "file2.PNG", "wb+"));
        fclose(fopen(tmpDir + "file3.jPg", "wb+"));
        fclose(fopen(tmpDir + "file4.JpG", "wb+"));
        fclose(fopen(tmpDir + "file5.png", "wb+"));
        fclose(fopen(tmpDir + "file6.png", "wb+"));
        fclose(fopen(tmpDir + "file7.png", "wb+"));
        fclose(fopen(tmpDir + "file8.png", "wb+"));
        fclose(fopen(tmpDir + "file9.png", "wb+"));
        fclose(fopen(tmpDir + "file10.png", "wb+"));
        dir.calculate();
        
        assertEqual(dir.validate(), 0);
        assertEqual(dir.result.errorPoints, 0);
        assertEqual(dir.result.totalPoints, 23);
        assertEqual(dir.getCountOfFiles(), 10);
        
        // to mutch files
        fclose(fopen(tmpDir + "file11.png", "wb+"));
        dir.calculate();
        
        assertEqual(dir.validate(), 0);
        assertEqual(dir.result.errorPoints, 1);
        assertEqual(dir.result.totalPoints, 25);
        assertEqual(QgVersionResult::lastErr, "reason.dir.filesCount");
        assertEqual(dir.getCountOfFiles(), 11);
//         DebugN(dir.result.children);
        remove(tmpDir + "file11.png");
        
        // try with 5 sub dirs
        mkdir(tmpDir + "subDir1");
        fclose(fopen(tmpDir + "subDir1/file1.png", "wb+"));
        mkdir(tmpDir + "subDir2");
        fclose(fopen(tmpDir + "subDir2/file1.png", "wb+"));
        mkdir(tmpDir + "subDir3");
        fclose(fopen(tmpDir + "subDir3/file1.png", "wb+"));
        mkdir(tmpDir + "subDir4");
        fclose(fopen(tmpDir + "subDir4/file1.png", "wb+"));
        mkdir(tmpDir + "subDir5");
        fclose(fopen(tmpDir + "subDir5/file1.png", "wb+"));
        dir.calculate();
        
        assertEqual(dir.validate(), 0);
        assertEqual(dir.result.errorPoints, 0);
        assertEqual(dir.result.totalPoints, 48);
        
        // to mutch sub dirs
        mkdir(tmpDir + "subDir6");
        fclose(fopen(tmpDir + "subDir6/file1.png", "wb+"));
        dir.calculate();
        
        assertEqual(dir.validate(), 0);
        assertEqual(dir.result.errorPoints, 1);
        assertEqual(QgVersionResult::lastErr, "reason.dir.subDirCount");
//         DebugN(dir);
        rmdir(tmpDir + "subDir6", TRUE);
        
        // invalid extention, is a indirect test fi pictureFile. So it is tested that file errors are added
        fclose(fopen(tmpDir + "subDir5/file1.txt", "wb+"));
        dir.calculate();

        assertEqual(dir.validate(), 0);
        assertEqual(dir.result.errorPoints, 1);
        assertEqual(QgVersionResult::lastErr, "reason.file.extention");
//         DebugN(dir);
        
        // claen up after test
//         rmdir(tmpDir, TRUE);
        return 0;
      }
    }

    return -1;
  }
  
  //------------------------------------------------------------------------------
  string _makeTmpDir()
  {    
    string tmpDir = dirName(tmpnam()) + "QgPictureDir/";

    // create tmp-dir for this test
    if ( isdir(tmpDir) )
      rmdir(tmpDir, TRUE);
        
    mkdir(tmpDir);
    
    return tmpDir;
  }
};

//--------------------------------------------------------------------------------
void main()
{
  TstQg test;
  test.startAll();
}
