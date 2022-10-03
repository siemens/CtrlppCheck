//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2022 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//

//--------------------------------------------------------------------------------
// used libraries (#uses)

//--------------------------------------------------------------------------------
// declare variables and constans

//--------------------------------------------------------------------------------
/*! @brief Handler with files.
 *
 * @details Class to handle with files.
 * @author lschopp
 */
class File
{
//--------------------------------------------------------------------------------
//@public members
//--------------------------------------------------------------------------------
  //------------------------------------------------------------------------------
  /** @brief Default c-tor.
    @param dirPath Full path to directory.
  */
  public File(string filePath = "")
  {
    setFilePath(filePath);
  }
  
  //------------------------------------------------------------------------------
  /** @brief Function set the file path
    @param dirPath Full path to file.
  */
  public void setFilePath(const string &filePath)
  {
    _filePath = filePath;
  }
  
  //------------------------------------------------------------------------------
  /** @brief Function returns full native path to file.
    @return Full native path to file.
    @exception Empty string.
  */
  public string getFilePath()
  {
    return makeNativePath(_filePath);
  }
  
  public string getName()
  {
    return baseName(_filePath);
  }  
  
  //------------------------------------------------------------------------------
  /** @brief Function creates new empty file.
    @return Error code. Returns 0 when successfull created, otherwise -1.
  */
  public int mk()
  {
    if ( exists() )
      return 0;
    
    fclose(fopen(getFilePath(), "wb+"));
    
    if ( !exists() )
      return -1;
    
    return 0;
  }
  
  //------------------------------------------------------------------------------
  /** @brief Function checks if file exist.
    @return Returns TRUE when file exist, otherwise FALSE.
  */
  public bool exists()
  {
    return isfile(_filePath);
  }
  
  //------------------------------------------------------------------------------
  /** @brief Function removes file.
    @details Function remove file.
    @return Error code. Returns 0 when successfull removed, otherwise -1.
  */
  public int rm()
  {
    if ( !exists() )
      return 0;
    
    if ( !remove(getFilePath()) )
      return 0;
    
    return -1;
  }
  
  //------------------------------------------------------------------------------
  public bool isExample()
  {
    return (strpos(getFilePath(), makeNativePath("examples/")) > 0);
  }
  
  //------------------------------------------------------------------------------
  public bool isTest()
  {
    return (strpos(getFilePath(), makeNativePath("tests/")) > 0);
  }
  
  //------------------------------------------------------------------------------
  public string getRelPath(string key = "")
  {
    string absPath = getFilePath();
    
    if ( absPath == "" )
      return "";
  
    absPath = makeNativePath(absPath);
    dyn_string dsProjs;
    for (int i = 1; i <= SEARCH_PATH_LEN ; i++)
    {
      dsProjs[i] = makeNativePath(getPath("", "", "", i));   
    }
  
    // check if absPath start with some proj paths
    for (int i = 1; i <= dynlen(dsProjs) ; i++)
    {
      if ( strpos(absPath, dsProjs[i]) == 0 ) 
        return substr(absPath, strlen(dsProjs[i]));
    }
  
    // in remote-ui must be proj pahts not equal
    dyn_string keys = makeDynString(key);
  
    if ( key == "" ) 
      keys = makeDynString(PANELS_REL_PATH, SCRIPTS_REL_PATH, CONFIG_REL_PATH);// ...

    for (int i = 1; i <= dynlen(keys) ; i++)
    {
      string key = makeNativePath(keys[i]);
      int keyPos = strpos(absPath, key);
      if ( keyPos == 0 )
        return absPath;
      else if ( keyPos > 0 )
        return substr(absPath, keyPos);
    }
  
    return "";
  }
  
  //------------------------------------------------------------------------------
  public bool isPatternMatch(const string &pattern)
  {
    return patternMatch(pattern, makeUnixPath(_filePath));
  }
  
//--------------------------------------------------------------------------------
//@private members
//--------------------------------------------------------------------------------  
  string _filePath;
};
