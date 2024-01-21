//--------------------------------------------------------------------------------
/**
  @file $relPath
  @copyright Copyright 2023 SIEMENS AG
             SPDX-License-Identifier: GPL-3.0-only
*/

//--------------------------------------------------------------------------------
// used libraries (#uses)
#uses "classes/testFramework/TfTestRunner"

//--------------------------------------------------------------------------------
// declare variables and constants


//--------------------------------------------------------------------------------
/**
 * @brief This class is hook for TfTestRunner
 */
class HookTfTestRunner : TfTestRunner
{
//--------------------------------------------------------------------------------
//@public members
//--------------------------------------------------------------------------------

  //------------------------------------------------------------------------------
  /**
   * @brief Function setups the test environment.
   * @details Customized version of TfTestRunner::setupEnvironment()
   * We need to register all installed no deploy packages here and set the helper result path
   * @return Error code. Returns 0 when successful. Otherwise -1.
   */
  public int setupEnvironment()
  {
    if (TfTestRunner::setupEnvironment())
      return -1;

    string currentTestSuite = getenv("CURRENT_TEST_SUITE");

    if (currentTestSuite == "")
      currentTestSuite = "__TF__";

    string resDir = TfFileSys::getTestInstallPath(TfFileSysPath::ResultsPartly) + makeNativePath("/" + currentTestSuite + "/" + PROJ + "/");

    if (!isdir(resDir))
      mkdir(resDir);

    if (currentTestSuite != "__TF__")
    {
      oaUnitSetup(resDir + "result.json", makeMapping("Format", (int)OaTestResultFileFormat::JsonFull));
      TfErrHdl::outputFormat = OaTestResultFileFormat::JsonFull;
    }

    if (gualityGates.isRegistered())
    {
      DebugTN(__FUNCTION__, "Is registered at the moment", gualityGates);
      return 0;
    }

    gualityGates.setInstallDir(dirName(TfFileSys::getTestInstallPath()));
    gualityGates.setRunnable(false);
    gualityGates.registerProj();
    return 0;
  }

  //------------------------------------------------------------------------------
  public void onExit(const anytype &exitCode)
  {
    gualityGates.unregisterProj();
    oaUnitAssertEqual("WinCC_OA_Test_Validation", (int)exitCode, 0, "verify exit code");
    oaUnitTearDown();
    TfTestRunner::onExit(exitCode);
  }

//--------------------------------------------------------------------------------
//@protected members
//--------------------------------------------------------------------------------

//--------------------------------------------------------------------------------
//@private members
//--------------------------------------------------------------------------------
  //------------------------------------------------------------------------------
  ProjEnvProject gualityGates = ProjEnvProject("WinCCOA_QualityChecks");
};
