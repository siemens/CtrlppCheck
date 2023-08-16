//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2022 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//

#uses "classes/Variables/Float"
#uses "classes/QualityGates/Qg"

enum QgMsgCatErrPrio
{
  Info,
  Warning,
  Error
};


class QgMsgCat
{
  public QgMsgCat(const string msgCat = "")
  {
    setName(msgCat);
  }

  public setPrio(QgMsgCatErrPrio prio)
  {
    _prio = prio;
  }

  public setName(const string &msgCatName)
  {
    _msgCat = msgCatName;
  }

  public string getName()
  {
    return _msgCat;
  }

  public string getText(const string &catKey, const mapping dollars = makeMapping())
  {
    if (catKey == "")
      DebugFTN("QgMsgCat", __FUNCTION__, "msg cat is empty", getStackTrace(), dollars, _msgCat);

    // get string from msg catalog
    string text = _getMsgCatText(catKey);

    for (int i = 1; i <= mappinglen(dollars); i++)
    {
      string key = mappingGetKey(dollars, i);
      string value = _formatValue(dollars[key]);
      strreplace(text, "$" + key, value);
    }

    strreplace(text, "$prio", getPriorityAsText(_prio));
    strreplace(text, "$QgId", Qg::getId());

    return text;
  }

  public string getPriorityAsText(const QgMsgCatErrPrio &prio)
  {
    switch (prio)
    {
      case QgMsgCatErrPrio::Info:
        return "Info";

      case QgMsgCatErrPrio::Warning:
        return "Warning";

      case QgMsgCatErrPrio::Error:
        return "Error";

      default:
        return "Unkwon";
    }
  }

  protected string _getMsgCatText(const string &catKey)
  {
    if (!mappingHasKey(_cache, _msgCat))
      _cache[_msgCat] = makeMapping();

    if (!mappingHasKey(_cache[_msgCat], catKey))
    {
      string text = getCatStr(_msgCat, catKey);

      if (dynlen(getLastError()) > 0)   // string founded
      {
//         DebugN(__FUNCTION__, "not found", catKey, _msgCat);
        text = catKey;
      }

      _cache[_msgCat][catKey] = text;
    }

    return _cache[_msgCat][catKey];;
  }

  string _formatValue(const anytype &value)
  {
    string str;

    if (getType(value) == FLOAT_VAR)
    {
      Float f = Float(value);
      str = (string)f.round(2); // round float
    }
    else
    {
      str = (string)value;
    }

    return str;
  }

  QgMsgCatErrPrio _prio;
  string _msgCat;

  static protected mapping _cache;
};
