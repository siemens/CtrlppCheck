//--------------------------------------------------------------------------------
/**
  @file $relPath
  @copyright Copyright 2023 SIEMENS AG
             SPDX-License-Identifier: GPL-3.0-only
*/

#uses "std"
#uses "spaceCheck"
#uses "classes/QualityGates/QgSettings"

class PicturesFile
{
  public PicturesFile(const string &path)
  {
    _path = path;
    // !! extension must be written lowercase, that NonCaseSensitive works
  }

  public int calculate()
  {
    if (!isfile(_path))
      return -1;

    _size = getFileSize(_path);
    _extension = getExt(_path);

    return 0;
  }

  public uint getMaxSize()
  {
    // 1MB in == 1048576 bytes
    return (uint)1048576;
  }

  public int validate()
  {
    result = new QgVersionResult();
    result.text = getName();

    {
      shared_ptr<QgSettings> settings = new QgSettings("PicturesFile.file.size");

      if (settings.isEnabled())
      {
        shared_ptr <QgVersionResult> assertion = new QgVersionResult();
        assertion.setMsgCatName("QgStaticCheck_Pictures");
        assertion.setAssertionText("assert.file.size");
        assertion.setReasonText("reason.file.size", makeMapping("file.name", getName(),
                                "file.size", byteSizeToString(_size)));
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
        shared_ptr <QgVersionResult> assertion = new QgVersionResult();
        assertion.setMsgCatName("QgStaticCheck_Pictures");
        assertion.setAssertionText("assert.file.extension");
        assertion.setReasonText("reason.file.extension", makeMapping("file.name", getName(),
                                "file.extension", _extension,
                                "allowedValues", settings.getReferenceValues()));
        assertion.assertDynContains(settings.getReferenceValues(), strtolower(_extension), settings.getScorePoints());
        result.addChild(assertion);
      }
    }
    return 0;
  }

  public string getName()
  {
    return baseName(_path);
  }


  public shared_ptr <QgVersionResult> result;

  string _extension;
  uint _size;
  string _path;
};
