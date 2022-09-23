//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2022 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//

/// @cond WinCCOA_intern

//--------------------------------------------------------------------------------
// used libraries (#uses)
#uses "fileSys"
#uses "csv"
#uses "CtrlXml"

//--------------------------------------------------------------------------------
class OaTestToXml
{
  public void setData(const dyn_dyn_string &csv, string testSuiteName)
  {
    _testSuiteName = testSuiteName;
    _header = csv[1];
    _csv = csv;
    dynRemove(_csv, 1);
  }
  
  public void parseData()
  {
    if ( _xmlDocNum == -1 )
    {
      _xmlDocNum = xmlNewDocument();
      _parentNode = xmlAppendChild(_xmlDocNum, -1, XML_ELEMENT_NODE, "testsuites");
    }
   
    _addTestSuite();
    
        
//     xmlSetElementAttribute(_xmlDocNum, _parentNode, "duration", _sumDuration);
  }
  
  
  public void writeDocument(const string filePath)
  {
    xmlDocumentToFile(_xmlDocNum, filePath);
    xmlCloseDocument(_xmlDocNum);
    dynClear(_csv);
    _xmlDocNum = -1;
  }
  
  private void _addTestSuite()
  {
    _tsNode = xmlAppendChild(_xmlDocNum, _parentNode, XML_ELEMENT_NODE, "testsuite");
    xmlSetElementAttribute(_xmlDocNum, _tsNode, "name", _testSuiteName);
    
    _tsTimestamp = "";
    _tsTime = 0;
    _tsErrors = 0;
    _tsTests = 0;
    _tsFailures = 0;
    _tsSkipped = 0;
    _tsErr = 0;
    
    for(int i = 1; i <= dynlen(_csv); i++)
      _addTestCase(_csv[i]);
    
    string sTime;
    if ( dynlen(_csv) > 0 )
    {
      _tsTimestamp = _csv[1][_idxStartTimeStamp];
      time t1 = _csv[1][_idxStartTimeStamp];
      time t2 = _csv[dynlen(_csv)][_idxEndTimeStamp];
      time tDiff = t2 - t1;
      _tsTime = (float)tDiff;
      sTime = _tsTime;
      strreplace(sTime, ",", ".");
    }
    xmlSetElementAttribute(_xmlDocNum, _tsNode, "timestamp", _tsTimestamp);
    xmlSetElementAttribute(_xmlDocNum, _tsNode, "time", sTime);
    xmlSetElementAttribute(_xmlDocNum, _tsNode, "errors", _tsErrors);
    xmlSetElementAttribute(_xmlDocNum, _tsNode, "tests", _tsTests);
    xmlSetElementAttribute(_xmlDocNum, _tsNode, "failures", _tsFailures);
    xmlSetElementAttribute(_xmlDocNum, _tsNode, "skipped", _tsSkipped);
    
    _sumDuration = _sumDuration + _tsTime;
  }
   
  private void _addTestCase(const dyn_string &tcItems)
  {
    if ( dynlen(tcItems) < _idxMethod )
    {
      DebugTN(__FUNCTION__, "missing items", tcItems);
      return;
    }
    const string method = tcItems[_idxMethod];
    
    if ( method == "oaUnitInfo" )
      return; // junit don supported info state
    
    _tsTests++;
    
    _tcNode = xmlAppendChild(_xmlDocNum, _tsNode, XML_ELEMENT_NODE, "testcase");
    xmlSetElementAttribute(_xmlDocNum, _tcNode, "class", "");
    xmlSetElementAttribute(_xmlDocNum, _tcNode, "name", tcItems[_idxTcId]);
    
    string duration = tcItems[_idxDuration];
    strreplace(duration, ",", ".");
    xmlSetElementAttribute(_xmlDocNum, _tcNode, "time", duration);
    
    const string state = tcItems[_idxState];
    
    if ( state == "Pass" )
      return;
    else if ( state == "Fail" )
      _addFailure(tcItems, "failure");
    else if ( state == "Aborted" )
      _addFailure(tcItems, "error");
    else // undefined state is allways a error
      _addFailure(tcItems, "failure");
  }
  
  private void _addFailure(const dyn_string &tcItems, string type)
  { 
    string msg = tcItems[_idxErrMsg];
    if ( tcItems[_idxNote] )
      msg = msg + " Note: " + tcItems[_idxNote];
      
    const string knownBug = tcItems[_idxKnownBug];
    
    if( knownBug != "" )
    {
      type = "skipped";
      _tsSkipped++;
      msg = msg + " Known bug: " + knownBug;
    }
    else if ( type == "failure" )
      _tsFailures++;
    else
      _tsErrors++;
    
    int node = xmlAppendChild(_xmlDocNum, _tcNode, XML_ELEMENT_NODE, type);
    int attr = xmlAppendChild(_xmlDocNum, node, XML_TEXT_NODE, type);
    
   
    xmlSetElementAttribute(_xmlDocNum, node, "message", msg);
    xmlSetNodeValue(_xmlDocNum, attr, tcItems[_idxStackTrace]);
  }
  
  
  int _idxTcId = 1;
  int _idxStartTimeStamp = 2;
  int _idxEndTimeStamp = 3;
  int _idxDuration = 4;
  int _idxMethod = 5;
  int _idxState = 6;
  int _idxErrMsg = 13;
  int _idxKnownBug = 14;
  int _idxNote = 15;
  int _idxStackTrace = 18;
  
  float _sumDuration;
  
  string _tsTimestamp;
  float _tsTime;
  int _tsErrors;
  int _tsTests;
  int _tsFailures;
  int _tsSkipped;
  int _tsErr;
  
  int _parentNode = -1;
  int _tsNode = -1;
  int _tcNode = -1;
  int _xmlDocNum = -1;
  
  string _testSuiteName;
  dyn_string _header;
  dyn_dyn_string _csv;
};


//--------------------------------------------------------------------------------
main()
{
  string resultDir = makeNativePath(getenv("WORKSPACE") + "/test/");
  
//   string resultDir = getPath(DATA_REL_PATH);
  if ( resultDir == "" || !isdir(resultDir) )
  {
    DebugTN("test result dir does not exists. Check env variable WORKSPACE", resultDir);
    exit(0);
  }
  
  dyn_string fileNames = getFileNamesRecursive(resultDir);
  
  OaTestToXml parser = OaTestToXml();
    
  for(int i = 1; i <= dynlen(fileNames); i++)
  {
    if ( !isfile(fileNames[i]) || getExt(fileNames[i]) == "xml" )
      continue;
    
    const string testFilePath = fileNames[i];
  
    dyn_dyn_string csv;
    csvFileRead(testFilePath, csv, ";");
  
    if ( dynlen(csv) < 1 )
    {
      DebugTN("test result is missing", testFilePath);
      exit(-1);
    }
  
    string testSuiteName = baseName(testFilePath);
    testSuiteName = delExt(testSuiteName);
    parser.setData(csv, testSuiteName);
    parser.parseData();
  }
  
  parser.writeDocument(resultDir + "result.xml");
}


// @endcond
