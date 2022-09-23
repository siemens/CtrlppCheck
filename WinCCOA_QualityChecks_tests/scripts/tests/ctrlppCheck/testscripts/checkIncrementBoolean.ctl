//

// check increment / decrement of boolean variable

bool scriptBool;
global bool gb;

class C
{
  public bool b;
  public static bool staticB;
};

bool isTrue()
{
  return true;
}

void inc()
{
  bool b;
  b++; // error increment boolean
  ++b; // error increment boolean
  
  dyn_bool db;
  dyn_dyn_bool ddb;

  ++db[1];  // error increment element of dyn_bool
  db[1]++;  // error increment element of dyn_bool
  ddb[1]++; // error increment element of dyn_dyn_bool

  int i = isTrue()++;   // error function returning bool
  int i2 = ++isTrue();  // error function returning bool
  
  scriptBool++;  // error increment script defined  boolean

  gb++; /// @bug #50 checkIncrementBoolean does not work with globale defined variables
  
  C c;
  c.b++;
  
  C::staticB++;
  
  unknown++; // unknown variable - stability no crash
  
  int i;
  i++; // in is OK
}


void dec()
{
  bool b;
  b--;  // error decrement boolean
  
  scriptBool--;  // error decrement script defined boolean
  gb--; /// @bug #50 checkIncrementBoolean does not work with globale defined variables
  
  C c;
  c.b--;
  
  C::staticB--;
}


void inc2()
{
  bool b;
  b+=1;  // error invalid operator
  
  scriptBool+=1; // error invalid operator on script defined boolean
  gb+=1; /// @bug #50 checkIncrementBoolean does not work with globale defined variables
  
  C c;
  c.b+=1;
  
  C::staticB+=1;
}


void dec2()
{
  bool b;
  b -= 1;  // error invalid operator
  
  scriptBool -= 1; // error invalid operator
}

void incFromFunc()
{
  int i;
  i += dpExists(""); // error function returning bool
}

void checkBrackestExpr()
{
  // some complicated expression, but not boolean comparsion
  iPercent = (100.0* ((k-1) * dynlen(dsdp) + l) / dynlen(dsdp) / dynlen(dsdpe));
 
  // 1 == 2 is definitive boolean comparsion
  int i = (1==2)++; // error: boolean comparsion ++

  if ( ( a == b) ++)  // error: boolean comparsion ++
  {
    if ( ( a + b) ++ ) // no error: int ++
    {
      dbg();
    }
  }
}