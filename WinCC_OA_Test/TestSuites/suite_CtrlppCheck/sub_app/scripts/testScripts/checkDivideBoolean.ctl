// start options: --inconclusive

// tests for divideOrMultipleBoolean
// CheckBool::divideOrMultipleBoolean

bool scriptBool;
global bool gb;

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
  if ( b1 / b2 / db[1] )
  {
    dbg();
  }
  
  // 2 errors
  if ( b1 / (b2 && b1) )
  {
    dbg();
  }
  
  // 1 error
  if ( true / 12)
  {
    dbg();
  }
  
  // 2 errors
  if ( b1 / getTrue() )
  {
    dbg();
  }
  
}


void errMulitply()
{
  bool b1, b2;
  dyn_bool db;
  dn[1] = true;

  // 3 errors
  if ( b1 * b2 * db[1] )
  {
    dbg();
  }
  
  // 2 errors
  if ( b1 * (b2 && b1) )
  {
    dbg();
  }
  
  // 1 error
  if ( true * 12)
  {
    dbg();
  }
  
  // 2 errors
  if ( b1 * getTrue() )
  {
    dbg();
  }
  
}

// no errors correct operators
void noErr()
{
  int i1, i2;
  int i3 = i1 / i2 * i1;
}
