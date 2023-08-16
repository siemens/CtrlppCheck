//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2022 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//

#uses "classes/ErrorHdl/OaLogger"
#uses "classes/file/File"
#uses "classes/QualityGates/QgStaticCheck/Panels/PanelFile/PanelFileScript"
#uses "CtrlXml"
#uses "classes/QualityGates/QgStaticCheck/Panels/PanelFile/PanelFileShape"

class PanelFile
{
//--------------------------------------------------------------------------------
//@public members
//--------------------------------------------------------------------------------
  public dyn_anytype shapes;
  public mapping properties;
  public mapping events;
  public string strContent;

  //------------------------------------------------------------------------------
  public PanelFile(const string relPath = "")
  {
    setPath(relPath);
  }

  //------------------------------------------------------------------------------
  public void setPath(const string &relPath)
  {
    _relPath = relPath;
  }

  //------------------------------------------------------------------------------
  static public void setSourceDirPath(const string &path)
  {
    _sourceDir = path;
  }

  //------------------------------------------------------------------------------
  public bool exists()
  {
    return (_relPath != "" && isfile(getFullPath()));
  }

  //------------------------------------------------------------------------------
  public int toXml()
  {
    string cmd;

    if (_WIN32)
      cmd = getPath(BIN_REL_PATH, getComponentName(UI_COMPONENT) + ".exe");
    else
      cmd = getPath(BIN_REL_PATH, getComponentName(UI_COMPONENT)) + " -platform offscreen";

    if (cmd == "")
      return -1;

    cmd += " -p " + makeNativePath(_relPath) + " -xmlConvert=XML -proj " + PROJ;

    int rc = system(cmd);

    if (rc)
    {
      logger.warning("Can not convert panel into xml format: " + cmd);
      return -2;
    }

    return 0;
  }

  //------------------------------------------------------------------------------
  public bool isBackUp()
  {
    return (strpos(getExt(_relPath), "bak") == 0);
  }

  //------------------------------------------------------------------------------
  public string getFullPath()
  {
    return _sourceDir + PANELS_REL_PATH + _relPath;
  }

  //------------------------------------------------------------------------------
  public string getRelPath()
  {
    return _relPath;
  }

  //------------------------------------------------------------------------------
  public int read()
  {
    File panelFile = File(getFullPath());

    panelFile.read(this.strContent);

    if (this.strContent != "")
    {
      _isXml = this.strContent.startsWith("<?xml version=");
      ///@todo clrify, how it looks when the xml file is crypted?
      _isCrypted = this.strContent.startsWith("PVSS_CRYPTED_PANEL");
      return 0;
    }

    logger.warning("Panel contetn is empty, " + getFullPath());
    return 2;
  }

  //------------------------------------------------------------------------------
  public bool isXmlFormat()
  {
    return _isXml;
  }

  //------------------------------------------------------------------------------
  public bool isCrypted()
  {
    return _isCrypted;
  }

  //------------------------------------------------------------------------------
  public string getVersion()
  {
    return _version;
  }

  //------------------------------------------------------------------------------
  public int getCountOfProperties()
  {
    return mappinglen(properties);
  }

  //------------------------------------------------------------------------------
  public int getCountOfEvents()
  {
    return mappinglen(events);
  }

  //------------------------------------------------------------------------------
  public int getCountOfShapes()
  {
    return dynlen(shapes);
  }

  //------------------------------------------------------------------------------
  public mapping getProperties()
  {
    return properties;
  }

  //------------------------------------------------------------------------------
  public mapping getEvents()
  {
    return events;
  }

  //------------------------------------------------------------------------------
  public dyn_anytype getShapes()
  {
    return shapes;
  }

  //------------------------------------------------------------------------------
  public int load()
  {
    if (!isXmlFormat())
    {
      logger.warning("Panel contetn is not XML format, " + getFullPath());
      return -1; // no other formats are possible to read
    }

    string errMsg;
    int errLine, errColumn;
    _xmlDoc = xmlDocumentFromString(strContent, errMsg, errLine, errColumn);

    if (_xmlDoc < 0)
    {
      logger.warning("Can not read XML document. " +
                     errMsg + " at line " +
                     errLine + " at column " +
                     errColumn + ", "
                     + getFullPath());
      return -2;
    }

    int node;
    mapping map;
    string nodeName;

    node = xmlFirstChild(_xmlDoc);
    node = xmlNextSibling(_xmlDoc, node);
    map = xmlElementAttributes(_xmlDoc, node);
    nodeName = xmlNodeValue(_xmlDoc, node);

    if (!mappingHasKey(map, "version"))
    {
      logger.warning("The panel does not contains version information, " + getFullPath());
      return -3;
    }

    _version = map["version"];

    node = xmlFirstChild(_xmlDoc, node);

    while (node >= 0)
    {
      nodeName = xmlNodeName(_xmlDoc, node);;

      switch (nodeName)
      {
        case "properties":
          _readProps(xmlFirstChild(_xmlDoc, node), properties);
          break;

        case "events":
          _readEvents(node, events);
          break;

        case "shapes":
          _readShapes(node, shapes);
          break;

        default:
          logger.warning("InternFailure: undefined nodeName, " + nodeName + ", " + getFullPath());
      }

      node = xmlNextSibling(_xmlDoc, node);
    }

    return 0;
  }

  //------------------------------------------------------------------------------
  public int calculate()
  {
    int count = 0;

    for (int i = 1; i <= dynlen(shapes); i++)
    {
      shapes[i].calculate();

      _ccn  += shapes[i].getCCN();
      _nloc += shapes[i].getNLOC();
      _avgCcn  += shapes[i].getAvgCCN();
      _avgNloc += shapes[i].getAvgNLOC();
      count++;
    }

    for (int i = 1; i <= mappinglen(events); i++)
    {
      const string key = mappingGetKey(events, i);
      events[key].calculate();

      _ccn  += events[key].getCCN();
      _nloc += events[key].getNLOC();
      _avgCcn  += events[key].getAvgCCN();
      _avgNloc += events[key].getAvgNLOC();
      count++;
    }

    // avarege per dir ??? not sure if is correct so
    if (count > 0)
    {
      _avgCcn = _avgCcn / count;
      _avgNloc = _avgNloc / count;
    }
  }

  //------------------------------------------------------------------------------
  public int getCCN()
  {
    return _ccn;
  }

  //------------------------------------------------------------------------------
  public float getAvgCCN()
  {
    return _avgCcn;
  }

  //------------------------------------------------------------------------------
  public int getNLOC()
  {
    return _nloc;
  }

  //------------------------------------------------------------------------------
  public float getAvgNLOC()
  {
    return _avgNloc;
  }

//--------------------------------------------------------------------------------
//@protected members
//--------------------------------------------------------------------------------


//--------------------------------------------------------------------------------
//@private members
//--------------------------------------------------------------------------------

  //------------------------------------------------------------------------------
  int _readShapes(int node, dyn_anytype &shapes)
  {
    if (node < 0)
      return -1;

    int shNode = xmlFirstChild(_xmlDoc, node);
    PanelFileShape sh = PanelFileShape();

    while (!_readShape(shNode, sh))
    {
      dynAppend(shapes, sh);
      shNode = xmlNextSibling(_xmlDoc, shNode);
    }

    return 0;
  }

  //------------------------------------------------------------------------------
  int _readShape(int node, PanelFileShape &sh)
  {
    if (node < 0)
      return -1;

    string nodeName = xmlNodeName(_xmlDoc, node);
    mapping map =  xmlElementAttributes(_xmlDoc, node);

    if ((nodeName == "reference") && !mappingHasKey(map, "shapeType"))
    {
      map["shapeType"] = "PANEL_REFERENCE"; // refernec does not contains the type. So push it here.
    }

    if ((nodeName == "shape") || (nodeName == "reference"))
    {
      mapping props;
      mapping events;
      int scNode = xmlFirstChild(_xmlDoc, node);

      while (scNode >= 0)
      {
        nodeName = xmlNodeName(_xmlDoc, scNode);

        switch (nodeName)
        {
          case "properties":
            _readProps(xmlFirstChild(_xmlDoc, scNode), props);
            break;

          case "events":
            _readEvents(scNode, events);
            break;

          default:
            logger.warning("InternFailure: undefined nodeName, " + nodeName + ", " + getFullPath());

        }

        scNode = xmlNextSibling(_xmlDoc, scNode);
      }

      sh.initFromMap(map);
      sh.setProperties(props);
      sh.setEvents(events);
      return 0;
    }

    logger.warning("Unknown shape type, " + nodeName + ", " + getFullPath());
    return -2;
  }

  //------------------------------------------------------------------------------
  int _readEvents(int node, mapping &events)
  {
    if (node < 0)
      return -1;

    string name = xmlNodeName(_xmlDoc, node);

    if (name != "events")
      return -1;

    int evNode = xmlFirstChild(_xmlDoc, node);

    while (!_readEvent(evNode, events))
      evNode = xmlNextSibling(_xmlDoc, evNode);

    return 0;
  }

  //------------------------------------------------------------------------------
  int _readEvent(int node, mapping &events)
  {
    if (node < 0)
      return -1;

    string name = xmlNodeName(_xmlDoc, node);
    mapping map =  xmlElementAttributes(_xmlDoc, node);

    if (name == "script")
    {
      name = map["name"];
      PanelFileScript script = PanelFileScript(name);
      int scNode = xmlFirstChild(_xmlDoc, node);

      script.setScript(xmlNodeValue(_xmlDoc, scNode));

      events[name] = script;
    }

    return 0;
  }

  //------------------------------------------------------------------------------
  int _readProps(int node, anytype &result)
  {
    if (node < 0)
      return -1;

    if (xmlNodeType(_xmlDoc, node) == XML_TEXT_NODE)
    {
      string value = xmlNodeValue(_xmlDoc, node);
      result = value;
      return 0;
    }
    else if (xmlNodeType(_xmlDoc, node) == XML_ELEMENT_NODE)
    {
      mapping map = xmlElementAttributes(_xmlDoc, node);

      if (mappingHasKey(map, "name"))
      {
        if (getType(result) == ANYTYPE_VAR)
          result = makeMapping();

        string key = map["name"];
        anytype a;

        if (!_readProps(xmlFirstChild(_xmlDoc, node), a))
          result[key] = a;
      }

    }

    _readProps(xmlNextSibling(_xmlDoc, node), result);
    return 0;
  }


  //------------------------------------------------------------------------------
  bool _isXml;
  bool _isCrypted;
  static string _sourceDir = PROJ_PATH;
  string _version;
  int _xmlDoc;
  string _relPath;
  int _nloc, _ccn;
  float _avgCcn, _avgNloc;
  OaLogger logger;
};
