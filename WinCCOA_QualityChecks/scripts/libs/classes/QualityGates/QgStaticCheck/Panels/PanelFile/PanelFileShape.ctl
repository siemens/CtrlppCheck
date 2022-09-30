//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2022 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//

#uses "classes/QualityGates/QgStaticCheck/Panels/PanelFile/PanelFileScript"
#uses "classes/QualityGates/QgSettings"

class PanelFileShape
{
  //------------------------------------------------------------------------------
  public shared_ptr <QgVersionResult> result;//!< Quality gate result
  
  public PanelFileShape()
  {
  }

  public void initFromMap(const mapping &map)
  {
    _firstLevelProps = map;
  }
  
  public void setProperties(const mapping &properties)
  {
    _properties = properties;
  }
    
  public mapping getProperties()
  {
    return _properties;
  }
 
  public int getCountOfProperties()
  {
    return mappinglen(_properties);
  }
  
  public int getCountOfEvents()
  {
    return mappinglen(_events);
  }
  
  public int getCountOfShapes()
  {
    //TODO
    //return dynlen(_shapes);
    return 0;
  }
  
  public mapping getEvents()
  {
    return _events;
  }
  
  public void setEvents(mapping &scripts)
  {
    _events = scripts;
  }
   
  public PanelFileScript getEvent(string name)
  {
    if ( mappingHasKey(_events, name) )
      return _events[name];
    
    PanelFileScript foo = PanelFileScript();
    return foo;
  }
  
  
  public string getName()
  {
    if ( mappingHasKey(_firstLevelProps, "Name") )
      return _firstLevelProps["Name"];
    
    return "";
  }
  
  
  public static int getMaxCountOfEvents()
  {
    return 100;
  }
  
  
  
  public int calculate()
  {
    int count = mappinglen(_events);
    for(int i = 1; i <= count; i++)
    {
      const string key = mappingGetKey(_events, i);
      _events[key].calculate();
      
      _ccn  += _events[key].getCCN();
      _nloc += _events[key].getNLOC();
      _avgCcn  += _events[key].getAvgCCN();
      _avgNloc += _events[key].getAvgNLOC();
    }
    
    if ( count > 0 )
    {
      _avgCcn = _avgCcn / count;
      _avgNloc = _avgNloc / count;
    }
    
    return 0;
  }
  
  public int getCCN()
  {
    return _ccn;
  }
  
  public float getAvgCCN()
  {
    return _avgCcn;
  }
  
  public int getNLOC()
  {
    return _nloc;
  }
  
  public float getAvgNLOC()
  {
    return _avgNloc;
  }

  
  public int validate()
  {
    QgVersionResult::lastErr = "";
    result = new QgVersionResult();
    result.text = getName();
   
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
  
    // countOfProperties
  protected int validateCountOfProperties()
  {
    shared_ptr<QgSettings> settings = new QgSettings("PanelFileShape.shape.countOfProperties");
    
    if ( settings.isEnabled() )
    {
      shared_ptr <QgVersionResult> assertion = new QgVersionResult();
      assertion.setMsgCatName("QgStaticCheck_Panels");
      assertion.setAssertionText("assert.shape.countOfProperties");
      assertion.setReasonText("reason.shape.countOfProperties", makeMapping("shape.name", getName(),
                                                                            "shape.countOfProperties", getCountOfProperties()));
      assertion.info(getCountOfProperties(), settings.getScorePoints());
      result.addChild(assertion);
    }
  }
    
    // getCountOfEvents
  protected int validateCountOfEvents()
  {
    shared_ptr<QgSettings> settings = new QgSettings("PanelFileShape.shape.countOfEvents");    
    
    if ( settings.isEnabled() )
    {
      shared_ptr <QgVersionResult> assertion = new QgVersionResult();
      assertion.setMsgCatName("QgStaticCheck_Panels");
      assertion.setAssertionText("assert.shape.countOfEvents");
      assertion.setReasonText("reason.shape.countOfEvents", makeMapping("shape.name", getName(),
                                                                        "shape.countOfEvents", getCountOfEvents()));
      assertion.assertLessEqual(getCountOfEvents(), settings.getHighLimit(DEFAULT_EVENTCOUNT_HIGH), settings.getScorePoints());
      result.addChild(assertion);
    }
  }
    
    //----------------------------------------------------------------------------
  protected int validateCCN()
  {
    shared_ptr<QgSettings> settings = new QgSettings("PanelFileShape.shape.CCN");    
    
    if ( settings.isEnabled() )
    {
      shared_ptr <QgVersionResult> assertion = new QgVersionResult();
      assertion.setMsgCatName("QgStaticCheck_Panels");
      // info only, dont check the values
      assertion.setAssertionText("assert.shape.CCN");
      assertion.setReasonText("reason.shape.CCN", makeMapping("shape.name", getName(),
                                                              "shape.CCN", getCCN()));
      assertion.info(getCCN(), settings.getScorePoints());
      result.addChild(assertion);
    }
  }
    
  protected int validateAvgCCN()
  {
    shared_ptr<QgSettings> settings = new QgSettings("PanelFileShape.shape.avgCCN");    
    
    if ( settings.isEnabled() )
    {
      shared_ptr <QgVersionResult> assertion = new QgVersionResult();
      assertion.setMsgCatName("QgStaticCheck_Panels");
      assertion.setAssertionText("assert.shape.avgCCN");
      assertion.setReasonText("reason.shape.avgCCN", makeMapping("shape.name", getName(),
                                                                 "shape.avgCCN", getAvgCCN()));
      assertion.info(getCCN(), settings.getScorePoints());
      result.addChild(assertion);
    }
  }
    
  protected int validateNLOC()
  {
    shared_ptr<QgSettings> settings = new QgSettings("PanelFileShape.shape.NLOC");    
    
    if ( settings.isEnabled() )
    {
      shared_ptr <QgVersionResult> assertion = new QgVersionResult();
      assertion.setMsgCatName("QgStaticCheck_Panels");
      assertion.setAssertionText("assert.shape.NLOC");
      assertion.setReasonText("reason.shape.NLOC", makeMapping("shape.name", getName(),
                                                               "shape.NLOC", getNLOC()));
      assertion.info(getCCN(), settings.getScorePoints());
      result.addChild(assertion);
    }
  }
    
  protected int validateAvgNLOC()
  {
    shared_ptr<QgSettings> settings = new QgSettings("PanelFileShape.shape.avgNLOC");    
    
    if ( settings.isEnabled() )
    {
      shared_ptr <QgVersionResult> assertion = new QgVersionResult();
      assertion.setMsgCatName("QgStaticCheck_Panels");
      assertion.setAssertionText("assert.shape.avgNLOC");
      assertion.setReasonText("reason.shape.avgNLOC", makeMapping("shape.name", getName(),
                                                                  "shape.avgNLOC", getAvgNLOC()));
      assertion.info(getCCN(), settings.getScorePoints());
      result.addChild(assertion);
    }
  }
    
    //----------------------------------------------------------------------------
    // validate events
  protected int validateEvents()
  {
    if ( mappinglen(_events) > 0 )
    {
      shared_ptr <QgVersionResult> ev = new QgVersionResult();
      ev.setAssertionText("shape.events");
        
      for( int i = 1; i <= mappinglen(_events); i++ )
      {      
        anytype event = mappingGetValue(_events, i);
        event.validate();
        ev.addChild(event.result);
      }
      result.addChild(ev);
    }
  }

    //----------------------------------------------------------------------------
    // validate properties
  protected int validateProperties()
  {
    shared_ptr <QgVersionResult> assertion = new QgVersionResult();
    assertion.setMsgCatName("QgStaticCheck_Panels");
    if ( mappinglen(_properties) > 0 )
    {
      shared_ptr <QgVersionResult> prop = new QgVersionResult();
      prop.setAssertionText("shape.properties");
      
      for( int i = 1; i <= mappinglen(_properties); i++ )
      {      
        ///@todo probably place for checking properties
        string key = mappingGetKey(_properties, i);
        shared_ptr <QgVersionResult> property = new QgVersionResult();
        property.setAssertionText(key);
        property.info(_properties[key]);
        
        prop.addChild(property);
      }
      
      result.addChild(prop);
    }
  }
  
  mapping _events;
  mapping _properties;
  mapping _firstLevelProps;
  

  int _nloc, _ccn;
  float _avgCcn, _avgNloc;
  const int DEFAULT_EVENTCOUNT_HIGH = 100;
};
