//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2023 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//

#uses "var"

class OaLogger
{
//--------------------------------------------------------------------------------
//@public members
//--------------------------------------------------------------------------------

  //------------------------------------------------------------------------------
  /**
  */
  public OaLogger(const string msgCatalog = "")
  {
    this.msgCatalog = msgCatalog;
  }

  //------------------------------------------------------------------------------
  /**
  */
  public void info(const anytype &codeOrError,
                   const anytype note = NULL,
                   const anytype note2 = NULL,
                   const anytype note3 = NULL)
  {
    _throw(codeOrError, PRIO_INFO, note, note2, note3);
  }

  //------------------------------------------------------------------------------
  /**
  */
  public void warning(const anytype &codeOrError,
                      const anytype note = NULL,
                      const anytype note2 = NULL,
                      const anytype note3 = NULL)
  {
    _throw(codeOrError, PRIO_WARNING, note, note2, note3);
  }

  //------------------------------------------------------------------------------
  /**
  */
  public void severe(const anytype &codeOrError,
                     const anytype note = NULL,
                     const anytype note2 = NULL,
                     const anytype note3 = NULL)
  {
    _throw(codeOrError, PRIO_SEVERE, note, note2, note3);
  }

  //------------------------------------------------------------------------------
  /**
  */
  public void fatal(const anytype &codeOrError,
                    const anytype note = NULL,
                    const anytype note2 = NULL,
                    const anytype note3 = NULL)
  {
    _throw(codeOrError, PRIO_FATAL, note, note2, note3);
  }


  protected string msgCatalog;
  //------------------------------------------------------------------------------
  /**
  */
  protected _throw(const anytype &codeOrError,
                   const int prio,
                   const anytype &note,
                   const anytype &note2,
                   const anytype &note3)
  {
    dyn_errClass err;

    if (isA(codeOrError, STRING_VAR))
    {
      err = makeError(msgCatalog, prio, ERR_CONTROL, 0, codeOrError);
    }
    else if (isA(codeOrError, ERRCLASS_VAR) || isA(codeOrError, DYN_ERRCLASS_VAR))
    {
      err = codeOrError;
    }
    else if (!isNull(note3))
    {
      err = makeError(msgCatalog, prio, ERR_CONTROL, (int)codeOrError, note, note2, note3);
    }
    else if (!isNull(note2))
    {
      err = makeError(msgCatalog, prio, ERR_CONTROL, (int)codeOrError, note, note2);
    }
    else if (!isNull(note))
    {
      err = makeError(msgCatalog, prio, ERR_CONTROL, (int)codeOrError, note);
    }
    else
    {
      err = makeError(msgCatalog, prio, ERR_CONTROL, (int)codeOrError);
    }

    if (prio == PRIO_SEVERE)
      throw(err);
    else
      throwError(err);
  }
};