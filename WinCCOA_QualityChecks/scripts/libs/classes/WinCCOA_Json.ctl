//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2022 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//

#uses "json"
#uses "classes/WinCCOA_Logger"

//---------------------------------------------------------------------------------------------------------------------------------------
/*!JSON file handler.
   @author ataker
 */
class Sl_Jsonfile
{
  private static Sl_Logger LOGGER = Sl_Logger();
  private string filePath;  /*!< full path to the json file */

  //---------------------------------------------------------------------------------------------------------------------------------------
  /**
    JSON file handler.

    @param filePath Absolute filepath of an (existing) file.
    @param createFile When TRUE file will be created when it is not existing

    @author           ataker
  */
  public Sl_Jsonfile(const string filePath = "", bool createFile = false)
  {
    this.filePath = filePath;

    if ( createFile && !(this.exists()) )
    {
      this.create();
    }
  }

  //---------------------------------------------------------------------------------------------------------------------------------------
  /**
    Function the absolute file path to the jenkisn file
    @return absolute path to jenkins file
  */
  public string getPath()
  {
    return this.filePath;
  }

  //---------------------------------------------------------------------------------------------------------------------------------------
  /**
    Function sets path to the jenkins file.

    @param filePath <B>Absolute filepath</B> of an (existing) file.
  */
  public void setPath(string filePath)
  {
    this.filePath = filePath;
  }

  //---------------------------------------------------------------------------------------------------------------------------------------
  /**
    Function checks if the file exists.
    @return Returns TRUE if the file exists, otherwise FALSE.
  */
  public bool exists()
  {
    LOGGER.info("["+__FUNCTION__+"] Check if file exist.");
    return isfile(this.filePath);
  }

  //---------------------------------------------------------------------------------------------------------------------------------------
  /**
    Function creates the file if it does not exist.
    @return error code:
    \li  0 File was created successfully.
    \li -1 Missing file path.
    \li -2 File already exists.
    \li -3 File couldn't be created.
  */
  public int create()
  {
    if ( this.filePath == "" )
    {
      LOGGER.warning("["+__FUNCTION__+"] Missing file path.");
      return -1;
    }

    if ( this.exists() )
    {
      LOGGER.warning("["+__FUNCTION__+"] File already exists.");
      return -2;
    }

    LOGGER.info("["+__FUNCTION__+"] Convert file to string. File path: " + this.filePath);
    if ( fclose(fopen(this.filePath, "wb+")) )
    {
      LOGGER.severe("["+__FUNCTION__+"] Could not create file.");
      return -3;
    }

    return 0;
  }

  //-------------------------------------------------------------------------------------------------------------------------------------
  /**
    Function reads a file and returns the decrypted JSON string.
    @param jsonVal Reference value which contains the decrypted JSON string
    @return error code:
      \li 0 Data was read successfully.
      \li -1 File can't be read.
      \li -2 Missing internal arguments.

    @warning refrence value jsonVal are not cleared in case of error.
  */
  public int read(anytype &jsonVal)
  {

    if ( access(this.filePath, R_OK) )
    {
      LOGGER.severe("["+__FUNCTION__+"] File can't be read.");
      return -1;
    }

    string json;

    LOGGER.info("["+__FUNCTION__+"] Convert file to string. File path: " + this.filePath);
    if ( !fileToString(this.filePath, json) || json == "" )
    {
      LOGGER.severe("["+__FUNCTION__+"] Missing arguments.");
      return -2;
    }

    LOGGER.info("["+__FUNCTION__+"] Decode json string");
    jsonVal = json_strToVal(json);
    return 0;
  }

  //-------------------------------------------------------------------------------------------------------------------------------------
  /**
    Writes data in the form of a JSON string to the specified file.
    @param data Data to be written to the files.
    @param compactFormat Format of JSON string. True means compact, False means human readable format.
    @return error code:
      \li  0 Data was successfully written to the file as a JSON string.
      \li -1 File is not available.
      \li -2 An error occurred.
      \li -3 Couldn't write string into file.
  */
  public int write(const anytype &data, bool compactFormat = FALSE)
  {
    if ( access(this.filePath, W_OK) )
    {
      LOGGER.severe("["+__FUNCTION__+"] File is not available.");
      return -1;
    }

    LOGGER.info("["+__FUNCTION__+"] Opening file. File path: " + this.filePath);
    file f = fopen(this.filePath, "wb+");

    int err = ferror(f); //Output possible errors
    if ( err != 0 )
    {
      LOGGER.severe("["+__FUNCTION__+"] An error occurred. Error code: " + err);
      return -2;
    }

    string json = jsonEncode(data, compactFormat);

    LOGGER.info("["+__FUNCTION__+"] Write json string into file.");
    err = fputs(json, f);
    fclose(f);

    if ( err >= 0 )
    {
      LOGGER.severe("["+__FUNCTION__+"] Couldn't write string into file");
      return -3;
    }

    return 0;
  }
};
