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
};
