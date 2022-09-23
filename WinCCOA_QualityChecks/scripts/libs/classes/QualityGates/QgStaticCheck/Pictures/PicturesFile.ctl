#uses "std"
#uses "classes/QualityGates/QgBase"
#uses "classes/QualityGates/QgSettings"
#uses "CtrlOaUnit"

class PicturesFile
{
  public PicturesFile(const string &path)
  {
    _path = path;
	// !! extention must be written lowercase, that NonCaseSensitive works
  }
    
  public int calculate()
  {
    if ( !isfile(_path) )
      return -1;
    
    _size = getFileSize(_path);
    _extention = getExt(_path);
    
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
      
      if ( settings.isEnabled() )
      {
        shared_ptr <QgVersionResult> assertion = new QgVersionResult();
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
      shared_ptr<QgSettings> settings = new QgSettings("PicturesFile.file.extention");  
          
      if ( settings.isEnabled() )
      {
        shared_ptr <QgVersionResult> assertion = new QgVersionResult();
        assertion.setAssertionText("assert.file.extention");
        assertion.setReasonText("reason.file.extention", makeMapping("file.name", getName(),
                                                                     "file.extention", _extention));
        assertion.assertDynContains(settings.getReferenceValues(), strtolower(_extention), settings.getScorePoints());
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
  
  string _extention;
  uint _size;
  string _path;
};
