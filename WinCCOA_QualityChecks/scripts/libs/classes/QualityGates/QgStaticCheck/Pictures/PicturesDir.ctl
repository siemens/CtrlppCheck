//--------------------------------------------------------------------------------
/**
  @file $relPath
  @copyright Copyright 2023 SIEMENS AG
             SPDX-License-Identifier: GPL-3.0-only
*/

//--------------------------------------------------------------------------------
// used libraries (#uses)
#uses "classes/QualityGates/QgStaticCheck/StaticDir"
#uses "classes/QualityGates/QgStaticCheck/Pictures/PicturesFile"

//--------------------------------------------------------------------------------
// declare variables and constans



class PicturesDir : StaticDir
{
//--------------------------------------------------------------------------------
//@public members
//--------------------------------------------------------------------------------

  //------------------------------------------------------------------------------
  public PicturesDir(const string dir = "")
  {
    setDir(dir);
  }

  //------------------------------------------------------------------------------
  public PicturesFile makeCheckFile(const string &fullPath)
  {
    PicturesFile pict = PicturesFile(fullPath);
    return pict;
  }

  //------------------------------------------------------------------------------
  public PicturesDir makeCheckSubDir(const string &fullPath)
  {
    PicturesDir dir = PicturesDir(fullPath);
    return dir;
  }

  //------------------------------------------------------------------------------
  public int validate()
  {
    StaticDir::validate();

    // validate subdirs and files
    StaticDir::validateSubDirs();
    StaticDir::validateFiles();

    return 0;
  }

  //------------------------------------------------------------------------------
  public string getSettingsRoot()
  {
    return "PicturesDir";
  }

//--------------------------------------------------------------------------------
//@private members
//--------------------------------------------------------------------------------

};

