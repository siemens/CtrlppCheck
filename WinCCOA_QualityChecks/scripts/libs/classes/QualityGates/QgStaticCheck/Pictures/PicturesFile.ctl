//--------------------------------------------------------------------------------
/**
  @file $relPath
  @copyright Copyright 2023 SIEMENS AG
             SPDX-License-Identifier: GPL-3.0-only
*/

#uses "std"
#uses "spaceCheck"
#uses "classes/QualityGates/QgResult"
#uses "classes/QualityGates/QgSettings"

class PicturesFile
{
//-----------------------------------------------------------------------------
//@public members
//-----------------------------------------------------------------------------
  public shared_ptr <QgResult> result;

  //---------------------------------------------------------------------------
  public PicturesFile(const string &path)
  {
    _path = path;
    // !! extension must be written lowercase, that NonCaseSensitive works
  }

  //---------------------------------------------------------------------------
  public int calculate()
  {
    if (!isfile(_path))
      return -1;

    _size = getFileSize(_path);
    _extension = getExt(_path);

    return 0;
  }

  //---------------------------------------------------------------------------
  public uint getMaxSize()
  {
    // 1MB in == 1048576 bytes
    return (uint)1048576;
  }

  //---------------------------------------------------------------------------
  public int validate()
  {
    const mapping dollars = makeMapping("file.name", getName());
    result = new QgResult("QgStaticCheck_Pictures", "file", dollars);

    {
      shared_ptr<QgSettings> settings = new QgSettings("PicturesFile.file.size");

      if (settings.isEnabled())
      {
        const mapping dollars = makeMapping("file.name", getName(), "file.size", byteSizeToString(_size));
        shared_ptr <QgResult> assertion = new QgResult("QgStaticCheck_Pictures", "file.size", dollars);
        assertion.assertLessEqual(_size, settings.getHighLimit(getMaxSize()), settings.getScorePoints());
        assertion.value = byteSizeToString(_size); // to see it with unit
        assertion.upperBound = byteSizeToString(settings.getHighLimit(getMaxSize())); // to see it with unit
        result.addChild(assertion);
      }
    }

    {
      shared_ptr<QgSettings> settings = new QgSettings("PicturesFile.file.extension");

      if (settings.isEnabled())
      {
        const mapping dollars = makeMapping("file.name", getName(), "file.extension", _extension);
        shared_ptr <QgResult> assertion = new QgResult("QgStaticCheck_Pictures", "file.extension", dollars);
        assertion.assertDynContains(settings.getReferenceValues(), strtolower(_extension), settings.getScorePoints());
        result.addChild(assertion);
      }
    }
    return 0;
  }

  //---------------------------------------------------------------------------
  public string getName()
  {
    return baseName(_path);
  }

//-----------------------------------------------------------------------------
//@private members
//-----------------------------------------------------------------------------
  string _extension;
  uint _size;
  string _path;
};
