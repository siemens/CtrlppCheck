//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2022 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//

#uses "CtrlPv2Admin"

//---------------------------------------------------------------------------------------------------------------------------------------
class QgApp
{
  //-------------------------------------------------------------------------------------------------------------------------------------
  public QgApp(string id = "")
  {
    _isValid = FALSE;
    if ( id == "" )
    {
      /// @todo I think we can remove this class, because always ends here.
      DebugFTN("QgApp", "!!! Parameter #1 (id) is missing" );
      return;
    }

    _id = id;
    string optsPath = _getPath() + "opts.json";
    DebugFTN("QgApp", __FUNCTION__, optsPath);
    if ( !isfile(optsPath) )
    {
      DebugFTN("QgApp", "!!! options file does not exists:", optsPath);
      return;
    }
    
    string json;
    fileToString(optsPath, json);
    _options = jsonDecode(json);
    
    string resDir = _getResultsDir();
     
    if ( isdir(resDir) )
      mkdir(resDir);
     
    _isValid = TRUE;
  }
  
  //-------------------------------------------------------------------------------------------------------------------------------------
  public static QgApp getAppFromProjName(const string projName = PROJ)
  {
  // parse WinCCOA_FinalyApi_22 to app id
    dyn_string items = strsplit(projName, "_");
    if ( dynlen(items) < 3 )
    {
      QgApp app = QgApp();
      return app;
    }
    
    QgApp app = QgApp(items[3], items[2]);
    return app;
  }
 
  //-------------------------------------------------------------------------------------------------------------------------------------
  /** 
    Function creates new not runnable project.
    
    
    @return error code. Succes == 0
  */
  public int makeProj()
  {
    const string appName = getExtendedTechnicalName();
    // const string appName = getAppName();
    string sourcePath = _getPath() + "appData/" + appName;
    const string projName = _getProjName();
    const string destPath = _getProjPath();
    
    int rc;    
    dyn_string names, versions, paths;
        
    if ( !isdir(sourcePath) )
    {
      DebugFTN("QgApp", __FUNCTION__, "source dir does not exist", sourcePath, _id);
      return -2;
    }
    
    paGetProjs(names, versions, paths);
    
    if ( dynContains(names, projName) > 0 )  //Project is registered
      paDelProj(projName, TRUE);
    else if ( isdir(destPath) )
      rmdir(destPath, TRUE);
    
    dyn_string langs;    
    if ( dynlen(langs) <= 0 )
    {
      for (int i = 0 ; i < getNoOfLangs(); i++)
        langs[i+1] = getLocale(i);
    }
    
    // proj without DB
    DebugFTN("QgApp", __FUNCTION__, "crete new project", _getProjName(), dirName(_getProjPath()), langs);
    rc = paCreateProj(_getProjName(), dirName(_getProjPath()), 
                      langs, 
                      1, "System1", // system num / name
                      2, // without DB
                      "" // no ascii list
                      );
    
    if ( rc )
    {
      DebugFTN("QgApp", "!!! could not create new project", rc);
      return -3;
    }
    
    // set proj NOT runnable
    paSetProjRunnable(projName, FALSE);
    
    sourcePath = sourcePath + "/";
    // copy content to the project
    const string destPath = _getProjPath();
    const dyn_string simpleDirs = makeDynString(SCRIPTS_REL_PATH, PANELS_REL_PATH, MSG_REL_PATH,
                                                COLORDB_REL_PATH, DATA_REL_PATH, DPLIST_REL_PATH,
                                                HELP_REL_PATH, PICTURES_REL_PATH, SOURCE_REL_PATH,
                                                BIN_REL_PATH, IMAGES_REL_PATH, CONFIG_REL_PATH);
    for(int i = 1; i <= dynlen(simpleDirs); i++)
    {
      string sourceDir = makeNativePath(sourcePath + simpleDirs[i]);
      string destDir = makeNativePath(destPath + simpleDirs[i]);
      
      if ( (CONFIG_REL_PATH != simpleDirs[i]) && isdir(destDir) )
      { 
        // clean up WinCC OA dir, that we have only correct dirs / files form version (addOn)
        rmdir(destDir, TRUE);
      }
      else if ( (CONFIG_REL_PATH == simpleDirs[i]) )
      { 
        DebugFTN("QgApp", sourceDir, isfile(sourceDir + "config"), sourceDir + "config");
        // remove config file, otherwise you destrou the add on project
        //! @todo 06.05.2018: must not be possible
        if ( isfile(sourceDir + "config") )
          remove(sourceDir + "config");
      }
      else if ( (BIN_REL_PATH == simpleDirs[i]) && !isdir(sourceDir) )
      {
        // use bin_<oaversion>        
        if ( isdir(sourceDir + "_" + VERSION) )
          sourceDir = sourceDir + "_" + VERSION;
        
        const int minMinorVersion = 15;
        // copy all bin directories, really hack.
        /// @todo remove this code, when multipe pipeline ( per oa version ) are running
        /// @warning this works only for WinCC OA major version == 3
        for(int j = 0; j <= 100; j++)
        {
          int version = minMinorVersion + j;
          string sDir = sourceDir + "_3." + version;
          string dDir = destDir + "_3." + version;
          
          if ( !isdir(sDir) )
            continue;
          
          if ( !copyAllFilesRecursive(sDir, dDir) )
          {
            DebugFTN("QgApp", "!!! could not copy dir", sDir, dDir);
            return -4;
          }
        }
      }
    
      DebugFTN("QgApp", "try copy", isdir(sourceDir), sourceDir);
      if ( !isdir(sourceDir) )
        continue; // nothing to do
      
      DebugFTN("QgApp", "to", destDir);
      if ( !copyAllFilesRecursive(sourceDir, destDir) )
      {
        DebugFTN("QgApp", "!!! could not copy dir", sourceDir, destDir);
        return -4;
      }
    }
    
    // create AddonInformation.json file
    rc = _makeAddOnInfoFile();
    if ( rc )
    {
      DebugFTN("QgApp", "!!! could not create AddonInformation.json file", rc);
      return -5;
    }
    
    // add this project as subProject, so be sure we have intgrated all scripts libs ...
    rc = paSetSubProjs(_getProjName(), makeDynString(PROJ));
    if ( rc )
    {
      DebugFTN("QgApp", "!!! could not set sub projects", rc, _getProjName(), makeDynString(PROJ));
      return -6;
    }
    return 0;
  }
  
  public string getExtendedTechnicalName()
  {
    return getenv("EXTENDED_TECHNICAL_NAME");
  }
  
  //-------------------------------------------------------------------------------------------------------------------------------------
  public string getAppName()
  {
    if ( !mappingHasKey(_options, "data") ||
         !mappingHasKey(_options["data"], "filename") )
    {
      return "";
    }
    
    string filename = _options["data"]["filename"];
    filename = delExt(filename);
    
    return filename;
  }  
  
  
  
  //-------------------------------------------------------------------------------------------------------------------------------------
  public string getVersionProperty(const string &property)
  {
    if ( mappingHasKey(_options, "data") && mappingHasKey(_options["data"], property) )
    {
      return _options["data"][property];
    }
    
    return "";
  } 
  
  //-------------------------------------------------------------------------------------------------------------------------------------
  public bool isValid()
  {
    return _isValid;
  }
  
  int _makeAddOnInfoFile()
  {
    // see also https://www.dropbox.com/s/xkc6pasizej2ui2/Auto%20Generierung%20AddonInformation.docx?dl=0
    
    const string destPath = _getProjPath();
    const string addOnInfoPath = destPath + DATA_REL_PATH + "AddonInformation.json";
    
    
    if ( !isdir(destPath + DATA_REL_PATH) )
      mkdir(destPath + DATA_REL_PATH); // make dir for AddonInformation.json file
    
    
    if ( isfile(addOnInfoPath) )
      moveFile(addOnInfoPath, addOnInfoPath + ".copy"); // make copy of original file (+lShopp)
    
    if ( !mappingHasKey(_options, "data") || !mappingHasKey(_options["data"], "product_id") )
    {
      DebugFTN("QgApp", __FUNCTION__, "missing product_id", _options);
      return -1;
    }
    const string productId = _options["data"]["product_id"];
    
    string serverUrl = getenv("SERVER_HOST_NAME");
    string token = getenv("WinCCOA_TOKEN");
    if ( serverUrl == "" )
      serverUrl = "WinCCOA.com";
    
    string url = "https://" + token + 
                 serverUrl + 
                 "/rest/products?expand=contractor&filter=%5B%7B++%22property%22%3A+%22id%22%2C++%22value%22%3A+" + 
                 productId +
                 "%2C++%22operator%22%3A+%22%3D%22%7D%5D";
    mapping result;
    int rc = netGet(url, result);  
    if ( rc || result["httpStatusCode"] != 200 )
    {
      DebugFTN("QgApp", __FUNCTION__, "could not get data from store", url);
      DebugFN("QgApp", __FUNCTION__, rc, result);
      return -2;
    }
    
    string json;
    if ( mappingHasKey(result, "content") )
    {
      json = result["content"];
    }
    
    mapping addOnInfo;
	mapping data = jsonDecode(json); 
	
    try
    {
      addOnInfo["addon_extended_technical_name"] = (string)getenv("EXTENDED_TECHNICAL_NAME");
      
      addOnInfo["addon_name"]          = (string)data["data"][1]["name"];
      addOnInfo["addon_description"]   = (string)data["data"][1]["description"];
      addOnInfo["addon_item_number"]   = (string)data["data"][1]["item_number"];
      addOnInfo["addon_provider_name"] = (string)data["data"][1]["contractor"]["name"];
    
      addOnInfo["version_name"]        = (string)_options["data"]["name"];
      addOnInfo["version_internal_id"] = (string)_options["data"]["internal_number"];
      addOnInfo["version_description"] = (string)_options["data"]["description"];
      
    }
    catch
    {
      DebugFTN("QgApp", getLastException());
      DebugFTN("QgApp", __FUNCTION__, "mising key ???", "check this url", url, data, _options);
      return -3;
    }
    
    
    file f = fopen(addOnInfoPath, "wb+");
    fputs(jsonEncode(addOnInfo, FALSE), f);
    fclose(f);
    
    if ( !isfile(addOnInfoPath) )
      DebugFTN("QgApp", __FUNCTION__, "Check te file", addOnInfoPath);
      
    
    return 0;
  }
  
  //-------------------------------------------------------------------------------------------------------------------------------------
  string _getResultsDir()
  {
    return makeNativePath(_getPath() + "_results/");
  }
  
  public string getSourcePath()
  {
    return _getPath();
  }
  //-------------------------------------------------------------------------------------------------------------------------------------
  string _getPath()
  {
    string dir = getenv("WORKSPACE");
    if ( (dir != "") && isdir(dir) )
      return makeNativePath(dir + "/");
      
    if ( _id == "" )
      return "";
    
    if ( _WIN32 )
    {
      string tmpDir = dirName(tmpnam());
      return makeNativePath(tmpDir + "WinCCOA/apps/" + _id + "/");
    }
    else if ( _UNIX )
      return makeNativePath("/tmp/WinCCOA/apps/" + _id + "/");
    else
      return "";
  }
  
  public string getProjName()
  {
    return _getProjName();
  }
  //-------------------------------------------------------------------------------------------------------------------------------------
  string _getProjName()
  {
    return "WinCCOA_FinalyApi_" + _id;
  }
  
  //-------------------------------------------------------------------------------------------------------------------------------------
  string _getProjPath(const string relPath = "")
  {
    return makeNativePath(_getPath() + _getProjName() + "/" + relPath);
  }
  
  //-------------------------------------------------------------------------------------------------------------------------------------
  //                                                     members
  string _id;
  bool _isValid;
  
  /*
    {
  "id":22,
  "product_id":6,
  "name":"2",
  "filename":"Shift_Calender.zip",
  "created_at":"2017-08-29 09:08:48",
  "created_by":5,
  "updated_by":4,
  "updated_at":"2017-12-09 08:12:09",
  "is_feature":0,
  "is_bugfix":1,
  "release_date":"1970-01-01",
  "description":"Update for new version​ 3.16!",
  "internal_number":2,
  "internal_filename":"22",
  "active":null
}
   */
  mapping _options;
};
