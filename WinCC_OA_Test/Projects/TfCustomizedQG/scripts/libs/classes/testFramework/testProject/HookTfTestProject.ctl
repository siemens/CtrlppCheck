//--------------------------------------------------------------------------------
/**
  @file $relPath
  @copyright Copyright 2023 SIEMENS AG
             SPDX-License-Identifier: GPL-3.0-only
*/

//--------------------------------------------------------------------------------
// used libraries (#uses)
#uses "classes/testFramework/testProject/TfTestProject"

//--------------------------------------------------------------------------------
/*!
 * Hook class of TfTestProject:
   Changes here overrides TfTestProject workflow
 */
class HookTfTestProject : TfTestProject
{
//--------------------------------------------------------------------------------
//@protected members
//--------------------------------------------------------------------------------

  protected dyn_string _getDefaultSubProjects()
  {
    dyn_string list = TfTestProject::_getDefaultSubProjects();
    dynAppend(list, "WinCCOA_QualityChecks");
    return list;
  }

  
  protected ProjEnvComponent gedi = new ProjEnvComponent(UI_COMPONENT);

  //------------------------------------------------------------------------------
  /** Checks if the IDE is enabled or not.
  */
  private bool useIDE()
  {
    string startIDE = getenv("startIDE");

    if (!this.isLocalProject || (startIDE == "") || (startIDE != "true"))
    {
      return false;
    }

    return true;
  }

  //------------------------------------------------------------------------------
  /** Start Gedi (WinCC OA IDE) in case of startIDE is enabled.
  */
  protected int _afterStarted()
  {
    int rc = TfTestProject::_afterStarted();

    if (rc)
      return rc;

    if (useIDE())
    {
      dyn_string arguments = makeDynString("-m", "gedi");

      if (!this.isProjWithDb(_packageSelection))
      {
        // project without DB start Gedi with -n (no event / db connection)
        dynAppend(arguments, "-n");
      }

      gedi.setProj(this.getId());
      gedi.setOptions(arguments);
      gedi.setAsync(TRUE);
      gedi.setDetached(TRUE);
      gedi.start();
    }

    return 0;
  }

  //------------------------------------------------------------------------------
  /** Start pause panel in case of startIDE is enabled.
   *  @note Tests are executed the panel is closed.
  */
  protected int _beforeStartTestManagers()
  {
    if (useIDE())
    {
      ProjEnvComponent pausePanel = new ProjEnvComponent(UI_COMPONENT);

      pausePanel.setOptions(makeDynString("-p", "OaTest/pause.pnl", "-n"));
      pausePanel.setProj(this.getId());
      pausePanel.start();
      gedi.stop();
    }

    return TfTestProject::_beforeStartTestManagers();
  }
};
