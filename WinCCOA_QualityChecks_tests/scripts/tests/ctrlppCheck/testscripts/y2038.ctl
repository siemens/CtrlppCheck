// start options:
// error id: y2038valueLost, y2038overflow, y2038canNotCastError, y2038unkownTypeError
// check handling of time variable after year 2038

struct C
{
  int i;
  float f;
  time t;

  public static time retT()
  {
    return getCurrentTime();
  }
};

time retT()
{
  return getCurrentTime();
}

// wrong casting time to int after y2038
int getTicksWrong()
{
  return (int)getCurrentTime();
}

// correct casting time to uint after y2038
uint getTicksOk()
{
  return (uint)getCurrentTime();
}

void printTime(const time tParam)
{
  // DebugN() use anytypes, therefor it is ok
  DebugN(tParam);
}

void main()
{
  C c;
  int i;
  // wrong assign time to int after y2038
  int i = c.t;

  time t;

  // correct assing time to string
  string s = t;
  s = (string)t;
  // wrong assing string to time
  t = (string)i;

  // wrong cast time to int after y2038
  s = (int)t;

  // correct cast time to float after y2038
  s = (float)t;

  // inconclusive check of undefined variables
  t = undefVar;

  // wrong assing time to int after y2038
  c.i = t;
  i = getCurrentTime();
  i = c.retT();
  i = retT();

  // xml lib functions
  // wrong assignt int to time after y2038
  formatTime("%c", i);
  // correct time to time
  formatTime("%c", t);

  // ctrl functions
  // wrong assignt int to time after y2038
  printTime(i);
  // correct time to time
  printTime(t);

  //undefined variable type will be ignored
  blaType bla;
  bla = t;
  t = bla;
}