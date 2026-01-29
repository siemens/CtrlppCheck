//--------------------------------------------------------------------------------
/**
  @file $relPath
  @copyright Copyright 2023 SIEMENS AG
             SPDX-License-Identifier: GPL-3.0-only
*/

/*!
 * @brief Tests for class: OaLogger
 */

//--------------------------------------------------------------------------------
// used libraries (#uses)
#uses "std"
#uses "classes/ErrorHdl/OaLogger" /*!< tested object */
#uses "classes/oaTest/OaTest"

//--------------------------------------------------------------------------------
class MockOaLogger : OaLogger
{
  public bool hasFatalBeenCalled = FALSE;
  public int callCount = 0;
  public int lastOriginalPrio = 0;
  public int lastEffectivePrio = 0;
  public anytype lastCodeOrError;
  public anytype lastNote;
  public anytype lastNote2;
  public anytype lastNote3;

  protected _throw(const anytype &codeOrError,
                   const int prio,
                   const anytype &note,
                   const anytype &note2,
                   const anytype &note3)
  {
    int prioToUse = prio;
    callCount++;

    lastOriginalPrio = prio;
    lastCodeOrError = codeOrError;
    lastNote = note;
    lastNote2 = note2;
    lastNote3 = note3;

    this.hasFatalBeenCalled = prio == PRIO_FATAL;
    if (this.hasFatalBeenCalled)
    {
      prioToUse = PRIO_SEVERE; // avoid killing the manager during tests
    }

    lastEffectivePrio = prioToUse;

    // Keep the unit test manager alive:
    // - OaLogger uses throwError() for PRIO_INFO/PRIO_WARNING which can lead to aborts
    //   in this test environment.
    // - For PRIO_SEVERE (and downgraded PRIO_FATAL) we still forward to validate the
    //   throwing behavior.
    if (prioToUse == PRIO_INFO || prioToUse == PRIO_WARNING)
      return;

    OaLogger::_throw(codeOrError, prioToUse, note, note2, note3);
  }
};

//--------------------------------------------------------------------------------
class TstOaLogger : OaTest
{
  public dyn_string getAllTestCaseIds()
  {
    return makeDynString("OaLogger");
  }

  protected int startTestCase(const string tcId)
  {
    switch (tcId)
    {
      case "OaLogger":
      {
        MockOaLogger logger;

        bool thrown;

        // info() should not abort the test manager (mock suppresses throwError())
        thrown = FALSE;
        try { logger.info("info message"); } catch { thrown = TRUE; }
        assertFalse(thrown);
        assertFalse(logger.hasFatalBeenCalled);
        assertTrue(logger.lastOriginalPrio == PRIO_INFO);
        assertTrue(logger.lastEffectivePrio == PRIO_INFO);
        assertTrue(isA(logger.lastCodeOrError, STRING_VAR));
        assertTrue((string)logger.lastCodeOrError == "info message");
        assertTrue(isNull(logger.lastNote));
        assertTrue(isNull(logger.lastNote2));
        assertTrue(isNull(logger.lastNote3));

        thrown = FALSE;
        try { logger.info(1); } catch { thrown = TRUE; }
        assertFalse(thrown);
        assertFalse(logger.hasFatalBeenCalled);
        assertTrue(logger.lastOriginalPrio == PRIO_INFO);
        assertTrue(logger.lastEffectivePrio == PRIO_INFO);
        assertTrue((int)logger.lastCodeOrError == 1);
        assertTrue(isNull(logger.lastNote));

        thrown = FALSE;
        try { logger.info(2, "note"); } catch { thrown = TRUE; }
        assertFalse(thrown);
        assertFalse(logger.hasFatalBeenCalled);
        assertTrue(logger.lastOriginalPrio == PRIO_INFO);
        assertTrue(logger.lastEffectivePrio == PRIO_INFO);
        assertTrue((int)logger.lastCodeOrError == 2);
        assertTrue(isA(logger.lastNote, STRING_VAR));
        assertTrue((string)logger.lastNote == "note");
        assertTrue(isNull(logger.lastNote2));
        assertTrue(isNull(logger.lastNote3));

        thrown = FALSE;
        try { logger.info(3, "note", "note2"); } catch { thrown = TRUE; }
        assertFalse(thrown);
        assertFalse(logger.hasFatalBeenCalled);
        assertTrue(logger.lastOriginalPrio == PRIO_INFO);
        assertTrue(logger.lastEffectivePrio == PRIO_INFO);
        assertTrue(isA(logger.lastNote2, STRING_VAR));
        assertTrue((string)logger.lastNote2 == "note2");

        thrown = FALSE;
        try { logger.info(4, "note", "note2", "note3"); } catch { thrown = TRUE; }
        assertFalse(thrown);
        assertFalse(logger.hasFatalBeenCalled);
        assertTrue(logger.lastOriginalPrio == PRIO_INFO);
        assertTrue(logger.lastEffectivePrio == PRIO_INFO);
        assertTrue(isA(logger.lastNote3, STRING_VAR));
        assertTrue((string)logger.lastNote3 == "note3");

        // warning() should not abort the test manager (mock suppresses throwError())
        thrown = FALSE;
        try { logger.warning("warning message"); } catch { thrown = TRUE; }
        assertFalse(thrown);
        assertFalse(logger.hasFatalBeenCalled);
        assertTrue(logger.lastOriginalPrio == PRIO_WARNING);
        assertTrue(logger.lastEffectivePrio == PRIO_WARNING);
        assertTrue(isA(logger.lastCodeOrError, STRING_VAR));
        assertTrue((string)logger.lastCodeOrError == "warning message");

        thrown = FALSE;
        try { logger.warning(10); } catch { thrown = TRUE; }
        assertFalse(thrown);
        assertFalse(logger.hasFatalBeenCalled);
        assertTrue(logger.lastOriginalPrio == PRIO_WARNING);
        assertTrue(logger.lastEffectivePrio == PRIO_WARNING);
        assertTrue((int)logger.lastCodeOrError == 10);

        thrown = FALSE;
        try { logger.warning(11, "note"); } catch { thrown = TRUE; }
        assertFalse(thrown);
        assertFalse(logger.hasFatalBeenCalled);
        assertTrue(logger.lastOriginalPrio == PRIO_WARNING);
        assertTrue(logger.lastEffectivePrio == PRIO_WARNING);
        assertTrue((int)logger.lastCodeOrError == 11);
        assertTrue(isA(logger.lastNote, STRING_VAR));
        assertTrue((string)logger.lastNote == "note");

        thrown = FALSE;
        try { logger.warning(12, "note", "note2"); } catch { thrown = TRUE; }
        assertFalse(thrown);
        assertFalse(logger.hasFatalBeenCalled);
        assertTrue(logger.lastOriginalPrio == PRIO_WARNING);
        assertTrue(logger.lastEffectivePrio == PRIO_WARNING);
        assertTrue((int)logger.lastCodeOrError == 12);
        assertTrue(isA(logger.lastNote2, STRING_VAR));
        assertTrue((string)logger.lastNote2 == "note2");

        thrown = FALSE;
        try { logger.warning(13, "note", "note2", "note3"); } catch { thrown = TRUE; }
        assertFalse(thrown);
        assertFalse(logger.hasFatalBeenCalled);
        assertTrue(logger.lastOriginalPrio == PRIO_WARNING);
        assertTrue(logger.lastEffectivePrio == PRIO_WARNING);
        assertTrue((int)logger.lastCodeOrError == 13);
        assertTrue(isA(logger.lastNote3, STRING_VAR));
        assertTrue((string)logger.lastNote3 == "note3");

        // fatal() must be mocked: PRIO_FATAL kills the manager.
        // In the mock it is downgraded to PRIO_SEVERE, therefore it must throw.
        thrown = FALSE;
        try
        {
          logger.fatal("fatal message");
        }
        catch
        {
          thrown = TRUE;
        }
        assertTrue(thrown);
        assertTrue(logger.hasFatalBeenCalled);
        assertTrue(logger.lastOriginalPrio == PRIO_FATAL);
        assertTrue(logger.lastEffectivePrio == PRIO_SEVERE);
        assertTrue(isA(logger.lastCodeOrError, STRING_VAR));
        assertTrue((string)logger.lastCodeOrError == "fatal message");

        // severe() MUST throw
        thrown = FALSE;
        try
        {
          logger.severe("severe message");
        }
        catch
        {
          thrown = TRUE;
        }
        assertTrue(thrown);

        // severe() with custom msg catalog must still throw
        thrown = FALSE;
        try
        {
          OaLogger catLogger = OaLogger("UnitTest");
          catLogger.severe(99, "note");
        }
        catch
        {
          thrown = TRUE;
        }
        assertTrue(thrown);

        return 0;
      }
    }

    return -1;
  }
};

//--------------------------------------------------------------------------------
main()
{
  TstOaLogger test;
  test.startAll();
  exit(0);
}
