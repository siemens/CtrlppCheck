//--------------------------------------------------------------------------------
/**
  @file $relPath
  @copyright Copyright 2026 SIEMENS AG
             SPDX-License-Identifier: GPL-3.0-only
*/

/*!
 * @brief Tests for class: QgFile
 */

//--------------------------------------------------------------------------------
// used libraries (#uses)
#uses "classes/FileSys/QgFile" /*!< tested object */
#uses "classes/oaTest/OaTest"

//--------------------------------------------------------------------------------
class TstQgFile : OaTest
{
  public dyn_string getAllTestCaseIds()
  {
    return makeDynString(
      "QgFile_ctor_empty",
      "QgFile_ctor_with_path",
      "QgFile_setFilePath_getFilePath",
      "QgFile_getName",
      "QgFile_mk_exists_rm",
      "QgFile_isExample",
      "QgFile_isTest",
      "QgFile_isPatternMatch"
    );
  }

  protected int startTestCase(const string tcId)
  {
    switch (tcId)
    {
      case "QgFile_ctor_empty":
      {
        QgFile f;
        assertEqual(f.getFilePath(), "");
        assertEqual(f.getName(), "");
        return 0;
      }

      case "QgFile_ctor_with_path":
      {
        QgFile f = QgFile("/path/to/myfile.ctl");
        assertEqual(f.getName(), "myfile.ctl");
        return 0;
      }

      case "QgFile_setFilePath_getFilePath":
      {
        QgFile f;
        f.setFilePath("/some/path/file.txt");
        assertTrue(f.getFilePath() != "");
        assertEqual(f.getName(), "file.txt");

        f.setFilePath("/another/path.xml");
        assertEqual(f.getName(), "path.xml");
        return 0;
      }

      case "QgFile_getName":
      {
        QgFile f1 = QgFile("/long/path/to/script.ctl");
        assertEqual(f1.getName(), "script.ctl");

        QgFile f2 = QgFile("simple.txt");
        assertEqual(f2.getName(), "simple.txt");

        QgFile f3 = QgFile("/path/with.dots/file.name.ext");
        assertEqual(f3.getName(), "file.name.ext");
        return 0;
      }

      case "QgFile_mk_exists_rm":
      {
        // Create a temp file path
        string tempPath = PROJ_PATH + "data/" + createUuid() + ".tmp";
        QgFile f = QgFile(tempPath);

        // Initially should not exist
        assertFalse(f.exists());

        // Create the file
        int mkResult = f.mk();
        assertEqual(mkResult, 0);
        assertTrue(f.exists());

        // Creating again should succeed (already exists)
        mkResult = f.mk();
        assertEqual(mkResult, 0);

        // Remove the file
        int rmResult = f.rm();
        assertEqual(rmResult, 0);
        assertFalse(f.exists());

        // Removing again should succeed (already gone)
        rmResult = f.rm();
        assertEqual(rmResult, 0);
        return 0;
      }

      case "QgFile_isExample":
      {
        QgFile f1 = QgFile("/proj/scripts/examples/test.ctl");
        assertTrue(f1.isExample());

        QgFile f2 = QgFile("/proj/scripts/libs/mylib.ctl");
        assertFalse(f2.isExample());

        QgFile f3 = QgFile("/proj/panels/examples/panel.pnl");
        assertTrue(f3.isExample());
        return 0;
      }

      case "QgFile_isTest":
      {
        QgFile f1 = QgFile("/proj/scripts/tests/mytest.ctl");
        assertTrue(f1.isTest());

        QgFile f2 = QgFile("/proj/scripts/libs/mylib.ctl");
        assertFalse(f2.isTest());

        QgFile f3 = QgFile("/suite/sub_unit/scripts/tests/libs/test.ctl");
        assertTrue(f3.isTest());
        return 0;
      }

      case "QgFile_isPatternMatch":
      {
        QgFile f = QgFile("/path/to/script.ctl");

        assertTrue(f.isPatternMatch("*.ctl"));
        assertTrue(f.isPatternMatch("*script*"));
        assertTrue(f.isPatternMatch("*/to/*"));
        assertFalse(f.isPatternMatch("*.xml"));
        assertFalse(f.isPatternMatch("*panel*"));
        return 0;
      }
    }

    return -1;
  }
};

//--------------------------------------------------------------------------------
main()
{
  TstQgFile test;
  test.startAll();
  exit(0);
}
