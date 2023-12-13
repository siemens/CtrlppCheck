//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2023 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//

#uses "classes/QualityGates/QgResult"
#uses "classes/QualityGates/QgStaticCheck/Panels/PanelFile/PanelFileScript"
#uses "classes/QualityGates/QgSettings"

class PanelFileShape
{
//-----------------------------------------------------------------------------
//@public members
//-----------------------------------------------------------------------------

  //---------------------------------------------------------------------------
  public shared_ptr <QgResult> result;//!< Quality gate result

  //---------------------------------------------------------------------------
  public PanelFileShape()
  {
  }

  //---------------------------------------------------------------------------
  public void initFromMap(const mapping &map)
  {
    _firstLevelProps = map;
  }

  //---------------------------------------------------------------------------
  public void setProperties(const mapping &properties)
  {
    _properties = properties;
  }

  //---------------------------------------------------------------------------
  public mapping getProperties()
  {
    return _properties;
  }

  //---------------------------------------------------------------------------
  public int getCountOfProperties()
  {
    return mappinglen(_properties);
  }

  //---------------------------------------------------------------------------
  public int getCountOfEvents()
  {
    return mappinglen(_events);
  }

  //---------------------------------------------------------------------------
  public int getCountOfShapes()
  {
    //TODO
    //return dynlen(_shapes);
    return 0;
  }

  //---------------------------------------------------------------------------
  public mapping getEvents()
  {
    return _events;
  }

  //---------------------------------------------------------------------------
  public void setEvents(mapping &scripts)
  {
    _events = scripts;
  }

  //---------------------------------------------------------------------------
  public PanelFileScript getEvent(string name)
  {
    if (mappingHasKey(_events, name))
      return _events[name];

    PanelFileScript foo = PanelFileScript();
    return foo;
  }

  //---------------------------------------------------------------------------
  public string getName()
  {
    if (mappingHasKey(_firstLevelProps, "Name"))
      return _firstLevelProps["Name"];

    return "";
  }

  //---------------------------------------------------------------------------
  public static int getMaxCountOfEvents()
  {
    return 100;
  }

  //---------------------------------------------------------------------------
  public int calculate()
  {
    int count = mappinglen(_events);

    for (int i = 1; i <= count; i++)
    {
      const string key = mappingGetKey(_events, i);
      _events[key].calculate();

      _ccn  += _events[key].getCCN();
      _nloc += _events[key].getNLOC();
      _avgCcn  += _events[key].getAvgCCN();
      _avgNloc += _events[key].getAvgNLOC();
    }

    if (count > 0)
    {
      _avgCcn = _avgCcn / count;
      _avgNloc = _avgNloc / count;
    }

    return 0;
  }

  //---------------------------------------------------------------------------
  public int getCCN()
  {
    return _ccn;
  }

  //---------------------------------------------------------------------------
  public float getAvgCCN()
  {
    return _avgCcn;
  }

  //---------------------------------------------------------------------------
  public int getNLOC()
  {
    return _nloc;
  }

  //---------------------------------------------------------------------------
  public float getAvgNLOC()
  {
    return _avgNloc;
  }

  //---------------------------------------------------------------------------
  public int validate()
  {
    const mapping dollars = makeMapping("shape.name", getName());
    result = new QgResult("QgStaticCheck_Panels", "shape", dollars);

    validateCountOfProperties();
    validateCountOfEvents();
    validateCCN();
    validateAvgCCN();
    validateNLOC();
    validateAvgNLOC();
    validateEvents();
    validateProperties();

    return 0;
  }

//-----------------------------------------------------------------------------
//@protected members
//-----------------------------------------------------------------------------

  //---------------------------------------------------------------------------
  // countOfProperties
  protected int validateCountOfProperties()
  {
    shared_ptr<QgSettings> settings = new QgSettings("PanelFileShape.shape.countOfProperties");

    if (settings.isEnabled())
    {
      const mapping dollars = makeMapping("shape.name", this.getName(), "shape.countOfProperties", this.getCountOfProperties());
      shared_ptr <QgResult> assertion = new QgResult("QgStaticCheck_Panels", "shape.countOfProperties", dollars);
      assertion.info(getCountOfProperties(), settings.getScorePoints());
      result.addChild(assertion);
    }
  }

  //---------------------------------------------------------------------------
  // getCountOfEvents
  protected int validateCountOfEvents()
  {
    shared_ptr<QgSettings> settings = new QgSettings("PanelFileShape.shape.countOfEvents");

    if (settings.isEnabled())
    {
      const mapping dollars = makeMapping("shape.name", this.getName(), "shape.countOfEvents", this.getCountOfEvents());
      shared_ptr <QgResult> assertion = new QgResult("QgStaticCheck_Panels", "shape.countOfEvents", dollars);
      assertion.assertLessEqual(getCountOfEvents(), settings.getHighLimit(DEFAULT_EVENTCOUNT_HIGH), settings.getScorePoints());
      result.addChild(assertion);
    }
  }

  //---------------------------------------------------------------------------
  protected int validateCCN()
  {
    shared_ptr<QgSettings> settings = new QgSettings("PanelFileShape.shape.CCN");

    if (settings.isEnabled())
    {
      const mapping dollars = makeMapping("shape.name", this.getName(), "shape.CCN", this.getCCN());
      shared_ptr <QgResult> assertion = new QgResult("QgStaticCheck_Panels", "shape.CCN", dollars);
      assertion.info(getCCN(), settings.getScorePoints());
      result.addChild(assertion);
    }
  }

  //---------------------------------------------------------------------------
  protected int validateAvgCCN()
  {
    shared_ptr<QgSettings> settings = new QgSettings("PanelFileShape.shape.avgCCN");

    if (settings.isEnabled())
    {
      const mapping dollars = makeMapping("shape.name", this.getName(), "shape.avgCCN", this.getAvgCCN());
      shared_ptr <QgResult> assertion = new QgResult("QgStaticCheck_Panels", "shape.CCN", dollars);
      assertion.info(getCCN(), settings.getScorePoints());
      result.addChild(assertion);
    }
  }

  //---------------------------------------------------------------------------
  protected int validateNLOC()
  {
    shared_ptr<QgSettings> settings = new QgSettings("PanelFileShape.shape.NLOC");

    if (settings.isEnabled())
    {
      const mapping dollars = makeMapping("shape.name", this.getName(), "shape.NLOC", this.getNLOC());
      shared_ptr <QgResult> assertion = new QgResult("QgStaticCheck_Panels", "shape.NLOC", dollars);
      assertion.info(getCCN(), settings.getScorePoints());
      result.addChild(assertion);
    }
  }

  //---------------------------------------------------------------------------
  protected int validateAvgNLOC()
  {
    shared_ptr<QgSettings> settings = new QgSettings("PanelFileShape.shape.avgNLOC");

    if (settings.isEnabled())
    {
      const mapping dollars = makeMapping("shape.name", this.getName(), "shape.avgNLOC", this.getAvgNLOC());
      shared_ptr <QgResult> assertion = new QgResult("QgStaticCheck_Panels", "shape.avgNLOC", dollars);
      assertion.info(this.getAvgNLOC(), settings.getScorePoints());
      result.addChild(assertion);
    }
  }

  //---------------------------------------------------------------------------
  // validate events
  protected int validateEvents()
  {
    if (mappinglen(_events) > 0)
    {
      const mapping dollars = makeMapping("shape.name", this.getName());
      shared_ptr <QgResult> assertion = new QgResult("QgStaticCheck_Panels", "shape.events", dollars);

      for (int i = 1; i <= mappinglen(_events); i++)
      {
        anytype event = mappingGetValue(_events, i);
        event.validate();
        ev.addChild(event.result);
      }

      result.addChild(ev);
    }
  }

  //---------------------------------------------------------------------------
  // validate properties
  protected int validateProperties()
  {

    if (mappinglen(_properties) > 0)
    {
      const mapping dollars = makeMapping("shape.name", this.getName());
      shared_ptr <QgResult> prop = new QgResult("QgStaticCheck_Panels", "shape.properties", dollars);

      for (int i = 1; i <= mappinglen(_properties); i++)
      {
        ///@todo probably place for checking properties
        string key = mappingGetKey(_properties, i);
        const mapping dollars = makeMapping("shape.name", this.getName(), "shape.properties.key", key);
        shared_ptr <QgResult> prop = new QgResult("QgStaticCheck_Panels", "shape.properties", dollars);
        property.info(_properties[key]);

        prop.addChild(property);
      }

      result.addChild(prop);
    }
  }

//-----------------------------------------------------------------------------
//@private members
//-----------------------------------------------------------------------------

  mapping _events;
  mapping _properties;
  mapping _firstLevelProps;


  int _nloc, _ccn;
  float _avgCcn, _avgNloc;
  const int DEFAULT_EVENTCOUNT_HIGH = 100;
};
