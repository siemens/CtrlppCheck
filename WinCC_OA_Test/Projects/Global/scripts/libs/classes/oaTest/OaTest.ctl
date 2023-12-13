// $License: NOLICENSE
/**
  @file $relPath
  @brief OaTest basic class
  @copyright Copyright 2023 SIEMENS AG
             SPDX-License-Identifier: GPL-3.0-only
  @author mPokorny
*/

//Libraries used(#uses)
#uses "classes/oaTest/OaTestBase"

//--------------------------------------------------------------------------------
/**
  @brief Overloaded OaTestBase class
*/
class OaTest : OaTestBase
{

  //------------------------------------------------------------------------------
  /**
    @brief The function checks, if the *value* contains *refValue*
    @param value The value that is tested.
    @param refValue Value to find.
    @param note Optional parameter. Test case-specific data can be set for the test case.
    It can be used to track an assertion message for a better bug analysis.
    @return oaUnitAssertTrue The function returns the error code. See also oaUnitAssertTrue()
  */
  protected int assertDynContains(const anytype &value, const anytype &refValue, const string note = "")
  {
    _fillTcData(note);
    tcData.setErrMsg("The value '" + value + "' does not contains " + refValue);
    tcData.setCurrentValue(value);
    tcData.setReferenceValue(refValue);
    tcData.setMethod(__FUNCTION__);

    int rc = oaUnitAssertTrue(getCurrentTestCaseId(), value.contains(refValue), getTestCaseData());

    if ((rc != 0) && hasKnownBugHandler())
    {
      oaTestKnownBugLoader.newBugFound(tcData);
    }

    return rc;
  }

};
