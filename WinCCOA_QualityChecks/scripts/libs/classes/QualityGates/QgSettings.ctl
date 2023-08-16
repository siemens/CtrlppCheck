//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2022 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//

//-----------------------------------------------------------------------------
// used libraries (#uses)
#uses "classes/Variables/Mapping"
#uses "classes/json/JsonFile"

//-----------------------------------------------------------------------------
// declare variables and constans

//caching von Datein in Mapping mit fileName as key

class QgSettings
{
  private string           id_               = "";
  private langString       description_      = "";
  private mixed            low_limit_            ;
  private mixed            high_limit_           ;
  private dyn_mixed        reference_values_ = makeDynMixed(-1);
  private int              score_points_     = 1;
  private bool             enabled_          = TRUE;
  private dyn_string       excluded_         = "";
  private static mapping   jsonFiles_            ;
  private const string     IDPATH_DELIMITER  = ".";

  //===========================================================================
  //@public methods
  //===========================================================================

  /**
    Generates a QgSettings class using the specified id.
    The information required for this is read from the Settings Json Files under the path ".../data/qualityGates/settings/".
    Example id: "FunctionData.function.CCN"
    @param id Specifies for which check the information should be read out
  */
  public QgSettings(const string &id)
  {
    readSettingsFromFile(id);
  }

  //---------------------------------------------------------------------------

  /**
    @return Returns the Settings Id
  */
  public string getId()
  {
    return id_;
  }

  //---------------------------------------------------------------------------

  /**
    @param id Specifies the id of the setting
  */
  public void setId(const string &id)
  {
    id_ = id;
  }

  //---------------------------------------------------------------------------
  /**
    @return Returns the description of the retrieved setting
  */
  public langString getDescription()
  {
    return description_;
  }

  //---------------------------------------------------------------------------

  /**
    @param description Sets the description for the specified check
  */
  public void setDescription(const langString &description)
  {
    description_ = description;
  }

  //---------------------------------------------------------------------------

  /**
    @param def The value to be returned in the event that this preference node
    has no value associated with key.
    @return The low Limit from the retrieved settings
    @author ataker
  */
  public anytype getLowLimit(const anytype &def)
  {
    if (getType(low_limit_) != MIXED_VAR && low_limit_ < high_limit_)
    {
      return low_limit_;
    }
    else
    {
      return def;
    }
  }

  //---------------------------------------------------------------------------

  /**
    @param high_limit Value for the lower limit at the check
  */
  public void setLowLimit(const anytype &low_limit)
  {
    low_limit_ = low_limit;
  }

  //---------------------------------------------------------------------------

  /**
    @param def The value to be returned in the event that this preference node
    has no value associated with key.
    @return The high Limit from the retrieved settings
    @author ataker
  */
  public anytype getHighLimit(const anytype &def)
  {
    if (getType(high_limit_) != MIXED_VAR && high_limit_ > low_limit_)
    {
      return high_limit_;
    }
    else
    {
      return def;
    }
  }

  //---------------------------------------------------------------------------

  /**
    @param high_limit Value for the high limit at the check
  */
  public void setHighLimit(const anytype &high_limit)
  {
    high_limit_ = high_limit;
  }

  //---------------------------------------------------------------------------

  /**
    @return Returns the score points of the retrieved setting
  */
  public int getScorePoints()
  {
    return score_points_;
  }

  //---------------------------------------------------------------------------

  /**
    @param score_points Sets how many points can be reached at the check
  */
  public void setScorePoints(const int &score_points)
  {
    if (score_points > 0)
    {
      score_points_ = score_points;
    }
  }

  //---------------------------------------------------------------------------

  /**
    @return If the check should be executed
  */
  public bool isEnabled()
  {
    return enabled_;
  }

  //---------------------------------------------------------------------------

  /**
    @param enabled FALSE to disable the test and TRUE to enable the test.
  */
  public void setEnabled(bool enabled)
  {
    enabled_ = enabled;
  }

  //---------------------------------------------------------------------------

  /**
    @return Returns the values that should excluded for the specifed check
  */
  public dyn_string getExcluded()
  {
    return excluded_;
  }

  //---------------------------------------------------------------------------

  /**
    @param excluded Set a list of regular expression that can be used in the specified test.
  */
  public void setExcluded(const dyn_string &excluded)
  {
    excluded_ = excluded;
  }

  //---------------------------------------------------------------------------

  /**
    @return Returns the reference values of the retrieved setting
  */
  public dyn_mixed getReferenceValues()
  {
    return reference_values_;
  }

  //---------------------------------------------------------------------------

  /**
    @param reference_values Values that could be used in the specified check.
    How these values are used depends on the test
  */
  public void setReferenceValues(const dyn_mixed &reference_values)
  {
    reference_values_ = reference_values;
  }

  public static dyn_mapping getAllSettingsAsMapping(bool defaultValues = false)
  {
    dyn_mapping settings;
    dyn_string SETTING_FILES = getSettingFiles();

    for (int i = 1; i <= dynlen(SETTING_FILES); i++)
    {
      string file_name = SETTING_FILES[i] + ".json";
      string file_path = "";

      if (!defaultValues)
      {
        file_path = getPath(DATA_REL_PATH, "qualityGates/settings/" + file_name);
      }
      else
      {
        for (int j = 2; j < SEARCH_PATH_LEN; j++)
        {
          string path = getPath(DATA_REL_PATH, "", getActiveLang(), j);

          if (strpos(path, "WinCCOA_QualityChecks") >= 0)
          {
            file_path = getPath(DATA_REL_PATH, "qualityGates/settings/" + file_name, getActiveLang(), j);

            if (file_path != "")
            {
              break;
            }
          }
        }
      }

      if (file_path == "")
      {
        continue;
      }

      JsonFile json_file = JsonFile(file_path);

      anytype json_data;
      int ret_val = json_file.read(json_data);

      if (ret_val < 0)
      {
        continue;
      }
      else
      {
        if (!mappingHasKey(jsonFiles_, file_name))
        {
          shared_ptr<mapping> json_data_pointer = new mapping(json_data);
          jsonFiles_[file_name] = json_data_pointer;
        }

        dynAppend(settings, json_data);
      }
    }

    return settings;
  }

  //===========================================================================
  //@private methods
  //===========================================================================

  //workaround for private static dyn_string
  private static dyn_string getSettingFiles()
  {
    return makeDynString("FunctionData", "PanelCheck",
                         "PanelFileShape", "PicturesFile",
                         "InternalCheck", "OverloadedFilesCheck",
                         "SyntaxCheck", "ScriptData",
                         "ScriptFile", "StaticCodeDir",
                         "StaticDir");
  }

  private void readSettingsFromFile(const string &id)
  {
    string file_name = getBasePart(id);

    if (file_name == "")
    {
      throw (makeError("QgSettings_Exceptions", PRIO_WARNING, ERR_PARAM, 1));
      return;
    }

    file_name += ".json";

    if (mappingHasKey(jsonFiles_, file_name))
    {
      createQgSettingsFromJsonData(jsonFiles_[file_name], id);
      return;
    }

    string file_path = getPath(DATA_REL_PATH, "qualityGates/settings/" + file_name);

    if (file_path == "")
    {
      throw (makeError("QgSettings_Exceptions", PRIO_WARNING, ERR_PARAM, 2));
      return;
    }

    JsonFile json_file = JsonFile(file_path);

    anytype json_data;
    int ret_val = json_file.read(json_data);
    shared_ptr<mapping> json_data_pointer = new mapping(json_data);

    if (ret_val < 0)
    {
      throw (makeError("QgSettings_Exceptions", PRIO_WARNING, ERR_PARAM, 3));
      return;
    }
    else
    {
      jsonFiles_[file_name] = json_data_pointer;
    }

    createQgSettingsFromJsonData(json_data_pointer, id);
  }

  /**
    @param id The idpath where the base part should be returned
    @return Returns the basepart of the specified id
  */
  private string getBasePart(const string &id)
  {
    return substr(id, 0, strpos(id, IDPATH_DELIMITER));
  }

  //---------------------------------------------------------------------------

  /**
    @param id The idpath where the last part should be removed
    @return Returns the Id without the last idpath
  */
  private string removeLastPart(const string &id)
  {
    int len;

    for (len = strlen(id); len > 0; len--)
    {
      if (id[len] == IDPATH_DELIMITER)
      {
        break;
      }
    }

    return substr(id, 0, len);
  }

  //---------------------------------------------------------------------------

  /**
    Fills the class members with the values from the json_data with the specified id
    @param json_data The retrieved mapping from the settings file
    @param id The complete id from the setting
  */
  private void createQgSettingsFromJsonData(shared_ptr<mapping> json_data, const string &id)
  {
    if (mappingHasKey(json_data, "settingsVersion") == false ||
        mappingHasKey(json_data, "checks") == false)
    {
      return;
    }

    //determine machting check for id
    dyn_mapping checks = json_data["checks"];
    mapping check;
    string id_iteration = id;


    for (int j = dynlen(strsplit(id, ".")); j >= 2; j--)
    {
      for (int i = 1; i <= dynlen(checks); i++)
      {
        if (mappingHasKey(checks[i], "id"))
        {
          if (id_iteration == checks[i]["id"])
          {
            check = checks[i];
            break;
          }
        }
      }

      if (mappinglen(check) > 0)
      {
        break;
      }
      else
      {
        id_iteration = removeLastPart(id_iteration);
      }
    }

    if (mappinglen(check) < 1)
    {
      return;
    }

    Mapping map = Mapping(check);

    //read settings depeding on the version
    switch (json_data["settingsVersion"])
    {
      case 0:
      default:

        this.setId(map.getAt("id"));
        this.setDescription(map.getAt("description"));
        this.setLowLimit(map.getAt("lowLimit"));
        this.setHighLimit(map.getAt("highLimit"));
        this.setReferenceValues(map.getAt("referenceValues", reference_values_));
        this.setScorePoints(map.getAt("scorePoints", score_points_));
        this.setEnabled(map.getAt("enabled", enabled_));
        this.setExcluded(map.getAt("excluded"));
    }
  }

  //---------------------------------------------------------------------------
};
