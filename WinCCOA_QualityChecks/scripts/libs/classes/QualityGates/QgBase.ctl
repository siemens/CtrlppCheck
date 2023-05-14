//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2022 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//

//--------------------------------------------------------------------------------
// used libraries (#uses)
#uses "classes/ErrorHdl/OaLogger"
#uses "classes/QualityGates/AddOn/FileSys/QgAddOnTmpSourceDir"
#uses "classes/QualityGates/QgResultPublisher"
#uses "classes/oaTest/OaTest"
#uses "classes/QualityGates/Qg"
#uses "classes/QualityGates/QgMsgCat"
#uses "classes/QualityGates/QgVersionResult"

//--------------------------------------------------------------------------------
// declare variables and constans

QgMsgCat myQgMsgCat = QgMsgCat();
OaTest  myTest = OaTest();


//--------------------------------------------------------------------------------
/** Error codes used in QgBase.cat
*/
enum QgBaseError
{
  Exception = 1,
  NotImplemented = 20,
  Start,
  Calculate,
  Validate,
  Done
};

//--------------------------------------------------------------------------------
/** QualityGate base class.

  Base class to handle (execute) quality gates
  @author lschopp
*/
class QgBase
{
//--------------------------------------------------------------------------------
//@public members
//--------------------------------------------------------------------------------

  //------------------------------------------------------------------------------
  /** @brief QualityGate setup.
   * @details Function setups the QualityGate.
   * @warning When you customize this function for your own QG, dont forgot to call
   *          this function. Otherwise you can losst all results.
   * @return Error code.
   * value | description
   * ------|------------
   * 0     | success
   * -1    | Setup does not work.
   */
  public int setUp()
  {
    myTest.setUp();
    myQgMsgCat.setName(Qg::getId());

    if (isEvConnOpen() && (  Qg::getId() != "" ) )
      dpSet("_WinCCOA_qgCmd.Command", Qg::getId() + ":START");
    return 0;
  }

  //------------------------------------------------------------------------------
  /** @brief Function start QualityGate.
   * @return Error code.
   * value | description
   * ------|------------
   * 0     | Success.
   * -2    | Setup does not works.
   * -3    | Tear-Down does not works.
   */
  public int start()
  {
    if (isEvConnOpen() && (  Qg::getId() != "" ) )
      dpSet("_WinCCOA_qgCmd.Command", Qg::getId() + ":START");

    int rc = _start();

    if (isEvConnOpen() && (  Qg::getId() != "" ) )
      dpSet("_WinCCOA_qgCmd.Command", Qg::getId() + ":DONE:" + rc);

    return rc;
  }

  //------------------------------------------------------------------------------
  public int calculate()
  {
    logger.severe(QgBaseError::NotImplemented, __FUNCTION__, Qg::getId());
    return 0;
  }

  //------------------------------------------------------------------------------
  public int validate()
  {
    logger.severe(QgBaseError::NotImplemented, __FUNCTION__, Qg::getId());
    return -1;
  }

  //------------------------------------------------------------------------------
  public int tearDown()
  {
    myTest.tearDown();
    if ( publish() )
      return -1;
    return 0;
  }

  //------------------------------------------------------------------------------
  public static QgResultState calculateState(const shared_ptr <QgVersionResult> result)
  {
    if ( result.hasError )
      return QgResultState::warning;

    return QgResultState::success;
  }

  //------------------------------------------------------------------------------
  public int publish()
  {
    _publisher.fields = getStoreFields();

    if ( _setMinScore )
      _publisher.result = _minScoreResult;
    else
      _publisher.result = _result;

    _publisher.state = calculateState(_publisher.result);

    if ( _publisher.publish() )
      return -1;

    return 0;
  }

  //------------------------------------------------------------------------------
  public static dyn_string getStoreFields()
  {
    return makeDynString("value", "descr", "goodRange", "totalPoints", "errorPoints", "reason");
  }

  //------------------------------------------------------------------------------
  public void setMinValidScore(const string &msgCatName,
                               const string &keyText, const string &keyReason,
                               const mapping dollars = makeMapping())
  {
    _setMinScore = TRUE;
    _minScoreResult = new QgVersionResult();
    _minScoreResult.setMsgCatName(msgCatName);
    _minScoreResult.setMinValidScore(keyText, keyReason, dollars);
  }


//--------------------------------------------------------------------------------
//@protected members
//--------------------------------------------------------------------------------
  protected QgAddOnTmpSourceDir _sourceDir = QgAddOnTmpSourceDir();
  protected QgResultPublisher _publisher = QgResultPublisher();
  protected shared_ptr<QgVersionResult> _result;

  protected bool _setMinScore = FALSE;
  protected shared_ptr <QgVersionResult> _minScoreResult;

  //------------------------------------------------------------------------------
  protected int _start()
  {
    const time startTime = getCurrentTime();
    logger.info(QgBaseError::Start, Qg::getId());
    try // exceptions happen here more often, so it should be sure to stop the QG properly
    {
      _setMinScore = FALSE;
      int rc = setUp();
      if ( rc )
      {
        logger.severe(QgBaseError::NotImplemented, __FUNCTION__, Qg::getId());
        return -1;
      }

      if ( !_setMinScore )
      {
        logger.info(QgBaseError::Calculate, Qg::getId());
        rc = calculate();
// ctrlppcheck-suppress knownConditionTrueFalse // the variable rc can be changed in the function calculate() in the derived class.
        if ( rc )
        {
          DebugFTN("QgBase", __FUNCTION__, "calculate returns some error", rc);
          return -2;
        }
      }

// ctrlppcheck-suppress duplicateCondition // The variable _setMinScore can be changed in the function calculate() in the derived class.
      if ( !_setMinScore )
      {
        logger.info(QgBaseError::Validate, Qg::getId());
        rc = validate();
        if ( rc  )
        {
          DebugFTN("QgBase", __FUNCTION__, "validate returns some error",  rc);
          return -3;
        }
      }

      rc = tearDown();
      if ( rc )
      {
        DebugFTN("QgBase", __FUNCTION__, "tearDown returns some error", rc);
        return -4;
      }
    }
    catch
    {
      // very dangerous - directly stop
      logger.warning(getLastException());
      logger.fatal(QgBaseError::Exception, Qg::getId());
      return -1; // defensive code, should never happen
    }

    float duration = getCurrentTime() - startTime;
    logger.info(QgBaseError::Done, Qg::getId(), duration);
    return 0;
  }

//--------------------------------------------------------------------------------
//@private members
//--------------------------------------------------------------------------------
  protected OaLogger logger = OaLogger("QgBase");
};
