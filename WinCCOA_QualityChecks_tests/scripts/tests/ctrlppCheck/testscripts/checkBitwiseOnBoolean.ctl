// start options: --inconclusive

// tests for checkBitwiseOnBoolean
// CheckBool::checkBitwiseOnBoolean

bool scriptBool;
global bool gb;

class C
{
  public bool b;
  public static bool staticB;
};

bool getTrue()
{
  return TRUE;
}

void err()
{
  bool b1, b2;
  dyn_bool db;
  dn[1] = true;

  // 3 errors
  if ( b1 & b2 & db[1] )
  {
    dbg();
  }
  
  // 2 errors
  if ( b1 & (b2 && b1) )
  {
    dbg();
  }
  
  // 1 error
  if ( true & 12)
  {
    dbg();
  }
  
  // 2 errors
  if ( b1 & getTrue() )
  {
    dbg();
  }
  
  // 2 errors
  C c1;
  if ( c1.b & b1 )
  {
    dbg();
  }

  // 2 errors
  if ( b1 & C::staticB )
  {
    dbg();
  }
  
  // 2 errors
  if ( b1 | b2)
  {
    dbg();
  }
}

// no errors correct operators
void noErr()
{
  bool b1, b2;
  dyn_bool db;
  db[1] = true;

  if ( b1 && b2 && db[1] && db)
  {
    dbg();
  }
  
  if ( b1 || b2)
  {
    dbg();
  }
}
