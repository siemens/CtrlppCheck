/*!
 * @brief Tests for class: Qg
 *
 * @author lschopp
 */

//--------------------------------------------------------------------------------
// used libraries (#uses)
#uses "classes/QualityGates/QgStaticCheck/Pictures/PicturesDir"/*!< tested object */
#uses "classes/WinCCOA/StTest" /*!< oaTest basic class */

//--------------------------------------------------------------------------------
// declare variables and constans

//--------------------------------------------------------------------------------
class TstQg : StTest
{
  public dyn_string _getAllTcIds()
  {
    // list with our testcases
    return makeDynString("PicturesDir::ctor", "PicturesDir-checkLimits",
                         "PicturesDir::exists", "PicturesDir::calculate",
                         "PicturesDir::validate");
  }

  public int _startTest()
  {
    const string tcId = _getTcId();

    switch( tcId )
    {
      case "PicturesDir::ctor":
      {
        PicturesDir dir = PicturesDir();
        oaUnitAssertEqual(tcId, dir.getName(), "");
        oaUnitAssertEqual(tcId, dir.getCountOfFiles(), 0);
        oaUnitAssertEqual(tcId, dir.getCountOfSubDirs(), 0);
        
        dir.setDir(PROJ_PATH);
        oaUnitAssertEqual(tcId, dir.getName(), PROJ);
        oaUnitAssertEqual(tcId, dir.getCountOfFiles(), 0);
        oaUnitAssertEqual(tcId, dir.getCountOfSubDirs(), 0);
        
        return 0;
      }
      
      case "PicturesDir-checkLimits":
      {
        oaUnitAssertEqual(tcId, PicturesDir::getMaxCountOfSubDirs(), 5);
        oaUnitAssertEqual(tcId, PicturesDir::getMaxCountOfFiles(), 10);
        return 0;
      }
      
      case "PicturesDir::exists":
      {
        PicturesDir dir = PicturesDir();
        oaUnitAssertFalse(tcId, dir.exists());
        dir.setDir(PROJ_PATH);
        oaUnitAssertTrue(tcId, dir.exists());
        dir.setDir(PROJ_PATH + "abc");
        oaUnitAssertFalse(tcId, dir.exists());
        return 0;
      }
      
      case "PicturesDir::calculate":
      {
        PicturesDir dir = PicturesDir();
        string tmpDir = _makeTmpDir();
        
        // not existing
        oaUnitAssertEqual(tcId, dir.calculate(), -1);
        oaUnitAssertEqual(tcId, dir.getCountOfFiles(), 0);
        oaUnitAssertEqual(tcId, dir.getCountOfFilesRecursive(), 0);
        oaUnitAssertEqual(tcId, dir.getCountOfSubDirs(), 0);
        
        // existing, but empty
        dir.setDir(tmpDir);
        
        oaUnitAssertEqual(tcId, dir.calculate(), 0);
        oaUnitAssertEqual(tcId, dir.getCountOfFiles(), 0);
        oaUnitAssertEqual(tcId, dir.getCountOfFilesRecursive(), 0);
        oaUnitAssertEqual(tcId, dir.getCountOfSubDirs(), 0);
        
        // existing with 2 sub dirs and 1 file
        mkdir(tmpDir + "subDir1");
        mkdir(tmpDir + "subDir2");
        fclose(fopen(tmpDir + "file.png", "wb+"));
        
        oaUnitAssertEqual(tcId, dir.calculate(), 0);
        oaUnitAssertEqual(tcId, dir.getCountOfFiles(), 1);
        oaUnitAssertEqual(tcId, dir.getCountOfFilesRecursive(), 1);
        oaUnitAssertEqual(tcId, dir.getCountOfSubDirs(), 2);
        
        
        // existing witth 3 files in defirent sub dirs
        fclose(fopen(tmpDir + "subDir1/file.PNG", "wb+"));
        fclose(fopen(tmpDir + "subDir1/file.txt", "wb+"));
        
        oaUnitAssertEqual(tcId, dir.calculate(), 0);
        oaUnitAssertEqual(tcId, dir.getCountOfFiles(), 1);
        oaUnitAssertEqual(tcId, dir.getCountOfFilesRecursive(), 3);
        oaUnitAssertEqual(tcId, dir.getCountOfSubDirs(), 2);
        
        dyn_anytype childs = dir.getChilds();
        oaUnitAssertEqual(tcId, dynlen(childs), 2); 
        oaUnitAssertEqual(tcId, childs[1].getName(), "subDir1");
        
        
        dir.setDir(PROJ_PATH);
        oaUnitAssertTrue(tcId, dir.exists());
        dir.setDir(PROJ_PATH + "abc");
        oaUnitAssertFalse(tcId, dir.exists());
        
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
        oaUnitAssertEqual(tcId, dir.validate(), 0);
        oaUnitAssertEqual(tcId, dir.result.errorPoints, 1);
        oaUnitAssertEqual(tcId, dir.result.totalPoints, 1);
        oaUnitAssertEqual(tcId, QgVersionResult::lastErr, "reason.dir.isEmpty");
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
        
        oaUnitAssertEqual(tcId, dir.validate(), 0);
        oaUnitAssertEqual(tcId, dir.result.errorPoints, 0);
        oaUnitAssertEqual(tcId, dir.result.totalPoints, 23);
        oaUnitAssertEqual(tcId, dir.getCountOfFiles(), 10);
        
        // to mutch files
        fclose(fopen(tmpDir + "file11.png", "wb+"));
        dir.calculate();
        
        oaUnitAssertEqual(tcId, dir.validate(), 0);
        oaUnitAssertEqual(tcId, dir.result.errorPoints, 1);
        oaUnitAssertEqual(tcId, dir.result.totalPoints, 25);
        oaUnitAssertEqual(tcId, QgVersionResult::lastErr, "reason.dir.filesCount");
        oaUnitAssertEqual(tcId, dir.getCountOfFiles(), 11);
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
        
        oaUnitAssertEqual(tcId, dir.validate(), 0);
        oaUnitAssertEqual(tcId, dir.result.errorPoints, 0);
        oaUnitAssertEqual(tcId, dir.result.totalPoints, 48);
        
        // to mutch sub dirs
        mkdir(tmpDir + "subDir6");
        fclose(fopen(tmpDir + "subDir6/file1.png", "wb+"));
        dir.calculate();
        
        oaUnitAssertEqual(tcId, dir.validate(), 0);
        oaUnitAssertEqual(tcId, dir.result.errorPoints, 1);
        oaUnitAssertEqual(tcId, QgVersionResult::lastErr, "reason.dir.subDirCount");
//         DebugN(dir);
        rmdir(tmpDir + "subDir6", TRUE);
        
        // invalid extention, is a indirect test fi pictureFile. So it is tested that file errors are added
        fclose(fopen(tmpDir + "subDir5/file1.txt", "wb+"));
        dir.calculate();

        oaUnitAssertEqual(tcId, dir.validate(), 0);
        oaUnitAssertEqual(tcId, dir.result.errorPoints, 1);
        oaUnitAssertEqual(tcId, QgVersionResult::lastErr, "reason.file.extention");
//         DebugN(dir);
        
        // claen up after test
//         rmdir(tmpDir, TRUE);
        return 0;
      }
    }

    return -1;
  }
  
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
main()
{
  TstQg test = TstQg();

  test.startAll();

  exit(0);
}
