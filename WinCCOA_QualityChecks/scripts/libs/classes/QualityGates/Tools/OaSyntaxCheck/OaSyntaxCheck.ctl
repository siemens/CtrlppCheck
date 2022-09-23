
/**
  @brief Simple WinCC OA syntax check.
  
  @details Classic WinCC OA syntax check.
  
  ## Debug flags
  + OaSyntaxCheck - enable all debugs specific to this tool.
*/
class OaSyntaxCheck
{
//--------------------------------------------------------------------------------
//@public members
//--------------------------------------------------------------------------------
 
  /// std output
  public string stdOut;
  /// std error output
  public string stdErr;
  /// return code from syntax check
  public int rc = -1;
  
  /**
    @brief Start syntax check for all WinnCC OA scripts, libs and panels.
    @warning Works only with current project.
    @attention It works only on platfaorm which has installed WinCC OA UI. For Linux
    platform is started with option '-platform offscreen' so it shall be possible to
    start it also without existing display.
    @attention Project must be registered an have a existing config file. Otherwise
    the the WinCC OA UI does not start.
    */
  public int checkAll()
  {
    stdOut = "";
    stdErr = "";
    string cmd;
    if ( _WIN32 )
      cmd = getPath(BIN_REL_PATH, getComponentName(UI_COMPONENT) + ".exe");
    else if ( _UNIX )
      cmd = getPath(BIN_REL_PATH, getComponentName(UI_COMPONENT));
    else
    {
      _stdErr = "this platform is not implemented";
      DebugFTN("OaSyntaxCheck", __FUNCTION__, stdErr);
      return -1;
    }
  
    cmd += " -syntax all -n -proj " + PROJ + " -log +stderr";
  
    if ( _UNIX )
      cmd += " -platform offscreen"; // because at centos gui is not opened
    
    rc = system(cmd, stdOut, stdErr);
    if ( rc )
    {
      DebugFTN("OaSyntaxCheck", __FUNCTION__, "command exited with rc = " + rc, cmd, stdOut, stdErr);
      return -2;
    }
  
    return 0;
  }
  
  /// convert std error output from syntax check to list of messages.
  public stdErrToMessages(dyn_string &msgs)
  {
    dynClear(msgs);
    strreplace(stdErr, "\r", "");
    dyn_string lines = strsplit(stdErr, "\n");
    DebugFTN("OaSyntaxCheck", "parse stderr", dynlen(lines));
    string line;
    for(int i = 1; i <= dynlen(lines); i++)
    {
      if ( strpos(lines[i], "WCCOAui") >= 0 )
      {
        if ( line != "" )
          dynAppend(msgs, line);
            
        line = lines[i];
        continue;
      }
          
      line += "\n" + lines[i];
    }
    dynAppend(msgs, line);
  }
      
};
