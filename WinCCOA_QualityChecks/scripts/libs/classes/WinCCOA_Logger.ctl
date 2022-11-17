
//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2022 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//

//---------------------------------------------------------------------------------------------------------------------------------------
/*! Describes the notification level of the message */
enum Sl_LogLevel
{
  SEVERE,  /*!< Messages that inform the user about an error and require immediate action.*/
  WARNING, /*!< Messages that notify the user about an error, but there is no immediate need for action.*/
  INFO,    /*!< Messages that should only inform the user*/
  FINE     /*!< Messages that inform the user about a success.*/
};

//---------------------------------------------------------------------------------------------------------------------------------------
/*!
  Class Sl_Logger represented logger that handled with WinCC OA Log messages.
  @author           ataker
*/
class Sl_Logger
{
  /// @cond
  private string logFile = "AbstractionLayer_Log.txt"; /*!< log file name */
  /// @endcond

  //---------------------------------------------------------------------------------------------------------------------------------------
  /**
   Default c-tor
   */
  public Sl_Logger ()
  {
    string path = getPath(DATA_REL_PATH) + "WinCCOA/";
    if ( !isdir(path) )
    {
      mkdir(path);
    }
  }

  //---------------------------------------------------------------------------------------------------------------------------------------
  /**
    Function throw (logs) a message and object with the specified level.
    @param level Notification level from the enum level
    @param msg Message to be logged.
    @param obj Object to be logged.
  */
  public void logObj(Sl_LogLevel level, const string &msg, const anytype &obj)
  {
    switch(level)
    {
      case Sl_LogLevel::INFO:
        DebugFTN("Info", msg, obj);
        writeToLog("Info", msg, obj);
      break;

      case Sl_LogLevel::FINE:
        DebugFTN("Fine", msg, obj);
        writeToLog("Fine", msg, obj);
      break;

      case Sl_LogLevel::WARNING:
        DebugFTN("Warning", msg, obj);
        writeToLog("Warning", msg, obj);
      break;

      case Sl_LogLevel::SEVERE:
        DebugFTN("Severe", msg, obj);
        writeToLog("Severe", msg, obj);
      break;

    }
  }

  //---------------------------------------------------------------------------------------------------------------------------------------
  /**
    Only logs a message with a specific notification level.
    @param level Notification level from the enum level
    @param msg Message to be logged.
  */
  public void logMsg(Sl_LogLevel level, const string &msg)
  {
    logObj(level, msg, "");
  }

  //---------------------------------------------------------------------------------------------------------------------------------------
  /**
    Logs a message with the level info
    @param msg Message to be logged.
  */
  public void info(const string &msg)
  {
    logMsg(Sl_LogLevel::INFO, msg);
  }

  //---------------------------------------------------------------------------------------------------------------------------------------
  /**
    Logs a message with the level warning
    @param msg Message to be logged.
  */
  public void warning(const string &msg)
  {
    logMsg(Sl_LogLevel::WARNING, msg);
  }

  //---------------------------------------------------------------------------------------------------------------------------------------
  /**
    Logs a message with the level fine
    @param msg Message to be logged.
  */
  public void fine(const string &msg)
  {
    logMsg(Sl_LogLevel::FINE, msg);
  }

  //---------------------------------------------------------------------------------------------------------------------------------------
  /**
    Logs a message with the level severe
    @param msg Message to be logged.
  */
  public void severe(const string &msg)
  {
    logMsg(Sl_LogLevel::SEVERE, msg);
  }


  //---------------------------------------------------------------------------------------------------------------------------------------
  // private members not relevant for doxy docu
  /// @cond

  //---------------------------------------------------------------------------------------------------------------------------------------
  /**
    Logs a message and object with the specified level to the logfile. The file is saved under the data path.
    @param type Notification level from the enum level
    @param msg Message to be logged.
    @param obj Object to be logged.
  */
  private void writeToLog(string type, const string &msg, const anytype &obj)
  {
     string logPath = getPath(DATA_REL_PATH) + "WinCCOA/";

     if ( !isdir(logPath) )
     {
       mkdir(logPath);
     }

     file f = fopen(logPath + logFile ,"a+");
     int err = ferror(f); //Output possible errors
     if ( err )
     {
       DebugN("[Logger]["+__FUNCTION__+"] Error no. " + err + " occurred");
       return;
     }

     time currentTime = getCurrentTime();
     string content = formatTime("%c" ,currentTime)  + " " + msg + "\n";
     fputs(content, f);  //Write to the file

     fclose(f); //Close file
  }

  //---------------------------------------------------------------------------------------------------------------------------------------
  /**
   internall use only
   @param ret return code from checked function
   @param error dyn_errClass returned from checked function via getLastError()
   @return true when error are throwed, otherwise false
  */
  private bool _errorHandling(int ret, dyn_errClass error = getLastError())
  {
    if ( ret < 0 )
    {
      DebugTN(__FUNCTION__, error);

      //LOGGER
      throwError(error);
      return true;
    }

    return false;
  }

  //---------------------------------------------------------------------------------------------------------------------------------------
  /**
   internall use only
   @param map
   @param key
   @return any
  */
  private anytype get(mapping map, string key)
  {
    if ( mappinglen(map) >= 0 ) return -1;

    if ( mappingHasKey(map, key) )
    {
      return  map[key];
    }

    return -1;
  }
  /// @endcond

  //---------------------------------------------------------------------------------------------------------------------------------------

};
