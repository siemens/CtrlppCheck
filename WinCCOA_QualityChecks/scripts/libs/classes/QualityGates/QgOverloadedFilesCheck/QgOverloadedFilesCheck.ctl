//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2022 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//

#uses "classes/QualityGates/QgBase"
#uses "classes/QualityGates/QgSettings"
#uses "fileSys"


class QgOverloadedFilesCheck
{
//--------------------------------------------------------------------------------
//@public members
//--------------------------------------------------------------------------------
  /**
    List with allowed files.
  */
  public dyn_string allowedFiles = makeDynString(CONFIG_REL_PATH + "powerconfig",
                                                 DATA_REL_PATH +  "RDBSetup/ora/RDB_config_template.sql",
                                                 SCRIPTS_REL_PATH + "userDrivers.ctl",
                                                 SCRIPTS_REL_PATH + "userPara.ctl",
                                                 LIBS_REL_PATH +  "aesuser.ctl",
                                                 LIBS_REL_PATH + "asModifyDisplay.ctl",
                                                // (officially not part of product)
                                                 LIBS_REL_PATH + "driverSettings_HOOK.ctl",
                                                 PANELS_REL_PATH + "vision/aes/_AS_propFilterExtended.pnl",
                                                 PANELS_REL_PATH + "vision/aes/_ES_propFilterExtended.pnl");

  //------------------------------------------------------------------------------
  /** @brief claculate projects files.
    @details make list with ralative pathes of all relevant files
    @return 0 when successfull, otherwise -1
  */
  public int calculate()
  {
    dyn_string files = getFileNamesRecursive(PROJ_PATH);
    dynSort(files);
    //
    for(int i = 1; i <= dynlen(files); i++)
    {
      string path = files[i];
      const string relPath = substr(path, strlen(PROJ_PATH));

      if ( strpos(relPath, makeNativePath(CONFIG_REL_PATH)) == 0 ||
           strpos(relPath, makeNativePath(DB_REL_PATH)) == 0 ||
           strpos(relPath, makeNativePath(LOG_REL_PATH)) == 0 ||
           relPath == makeNativePath("data/AddonInformation.json") )
      {
        continue;
      }

      dynAppend(_relPaths, relPath);
    }

    return 0;
  }

  //------------------------------------------------------------------------------
  /** @brief Function validates list of relative pathes.
    @details Check if the files is overloaded and append the result in result list.
    @return 0 when successfull, otherwise -1
  */
  public int validate()
  {
    QgVersionResult::lastErr = "";
    result = new QgVersionResult();


    if ( dynlen(_relPaths) > 0 )
    {
      result.setMsgCatName("QgStaticCheck_OverloadedFiles");
      result.setAssertionText("filesList");

      for(int i = 1; i <= dynlen(_relPaths); i++)
      {
        const string relPath = _relPaths[i];

        const string overloadedFrom = findSourceProj(relPath);
        shared_ptr<QgSettings> settings = new QgSettings("OverloadedFilesCheck.isOverloadedAllowed");

        if ( settings.isEnabled() )
        {
          shared_ptr <QgVersionResult> assertion = new QgVersionResult();
          assertion.setMsgCatName("QgStaticCheck_OverloadedFiles");
        
          if ( isAllowed(relPath) )
          {
            assertion.setAssertionText("assert.isOverloadedAllowed", makeMapping("file.name", relPath));
            assertion.setReasonText("reason.isOverloadedAllowed", makeMapping("file.name", relPath,
                                                                              "file.isOverloadedFrom", overloadedFrom));
            assertion.allowNextErr(TRUE);
            assertion.assertFalse(overloadedFrom != "", settings.getScorePoints()); // negative logic, for better look in store
            assertion.referenceValue = (overloadedFrom != ""); // reference value faken, for better look in store
          }
          else
          {
            assertion.setAssertionText("assert.isOverloaded", makeMapping("file.name", relPath));
            assertion.setReasonText("reason.isOverloaded", makeMapping("file.name", relPath,
                                                                       "file.isOverloadedFrom", overloadedFrom));
            assertion.assertFalse(overloadedFrom != "", settings.getScorePoints()); // negative logic, for better look in store

          }
        
          shared_ptr <QgVersionResult> fileChildData = new QgVersionResult();
          fileChildData.text = relPath;
          fileChildData.addChild(assertion);

          // add child to results
          result.addChild(fileChildData);
        }
      }

    }

    return 0;
  }

  //------------------------------------------------------------------------------
  /** @brief Function check if file is allowed to be overwriten.
    @param relPath file relative path.
    @return TRUE when allowed be overwriten, otherwise FALSE.
  */
  public bool isAllowed(const string &relPath)
  {
    return dynContains(allowedFiles, makeUnixPath(relPath)) > 0;
  }


  //------------------------------------------------------------------------------
  /** @brief Function fine name of the source project.
    @details Returns name of the source project. In case of WinCC OA returns the version number
    @param relPath Native realtive path to the project file.
    @return Project name whithin is the file located.
    @exception EMpty string. File does not exist in sub projects or version.
  */
  public static string findSourceProj(const string &relPath)
  {
    bool isOverload = FALSE;
    for(int i = 2; i <= SEARCH_PATH_LEN; i++)
    {
      string subProjPath = getPath("", "", -1, i);
      if ( !isfile(subProjPath + relPath) )
        continue;

      if ( i == SEARCH_PATH_LEN )
        return "WinCC OA " + baseName(subProjPath);
      else
        return "Sub-project: " + baseName(subProjPath);
    }

    return "";
  }

  //------------------------------------------------------------------------------
  public shared_ptr <QgVersionResult> result; //!< Overloaded file check result

//--------------------------------------------------------------------------------
//@private members
//--------------------------------------------------------------------------------
  dyn_string _relPaths; //!< List with project files, to be checked
};
