// start options: --inconclusive

int checkInnnerCond()
{
  int a, b, c, d;
  // error
  if ( a == b );
  {
  }
  
  // no error
  if ( a == b )
  {
  }
  
  // error
  if ( a == b );
    dbg();
    
  // error
  if ( a == b );
}

