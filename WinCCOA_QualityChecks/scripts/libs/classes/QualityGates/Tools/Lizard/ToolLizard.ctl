#uses "CtrlPv2Admin"

//--------------------------------------------------------------------------------
// column indexes in lizard report file
enum LizardCsvIndx
{
  NLOC = 1,
  CCN,
  TOKEN_COUNT,
  PARAM_COUNT,
  LINES,
  LOCATION,
  FILE_PATH,
  FUNCTION_NAME,
  PARAMS
};



//--------------------------------------------------------------------------------
/**
  Lizard tool.
  
  Lizard is Cyclomatic Complexity Analyzer.
  Lizard is used to calculate ctrl scripts and get the NLOC, Functions list,
  Function argument list, CCN and count of lines in function.
  See also https://github.com/terryyin/lizard
  @warning It is external dependency.
*/
class ToolLizard
{
//@public
  //------------------------------------------------------------------------------
  public ToolLizard()
  {
  }
  
  //------------------------------------------------------------------------------
  public static synchronized string getBinDir()
  {
    if ( !initialized )
    {
      paCfgReadValue(getPath(CONFIG_REL_PATH, "config"), "qualityChecks", "lizardPath", binDir);
      if ( binDir == "" )
        binDir = getPath(DATA_REL_PATH, "lizard/");
      
      if ( binDir != "" )
      {
        binDir = makeNativePath(binDir + "/"); // add / on the end of path
        strreplace(binDir, makeNativePath("//"), makeNativePath("/")); // remove duplicated //
      }
      initialized = TRUE;
    }
    return binDir;
  }
  
  //------------------------------------------------------------------------------
  public static bool isInstalled()
  {
    return isfile(getBinDir() + "lizard.py");
  }
  
//@private
  //------------------------------------------------------------------------------
  static bool initialized;
  static string binDir;
};
