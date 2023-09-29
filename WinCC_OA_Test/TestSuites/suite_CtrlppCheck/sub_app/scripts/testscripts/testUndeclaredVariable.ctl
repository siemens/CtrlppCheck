int scriptVar;

int value = unknownVar;

void main()
{
  // locale defined variable OK
  string localeVar = "abc";
  retS(localeVar);

  // assign to undeclared variable
  unknownVar = 1;
  
  // test to supress this message
  // ctrlppcheck-suppress undeclaredVariable
  unknownVar = 2;
  // ctrlppcheck-suppress undeclaredVariable
  unknownVar = 3;
  
  // script global variable
  scriptVar = 2;
  retS(scriptVar);
  
  // assign undeclared to script lobal variable
  scriptVar = unknownVar;
  
  // check other operators with undeclared variable
  unknownVar << 1;
  unknownVar >> 1;
  bool b = unknownVar | scriptVar;
  unknownVar || 1;
  unknownVar && 1;
  bool b2 = unknownVar & scriptVar;
  unknownVar == 1;
  
  // assign undeclared to it self
  unknownVar = unknownVar;  
  
  // assign return value from function
  unknownVar = retS("");
  unknownVar = maxINT();
  unknownVar = undeclaredFunction();
  
  // use undeclared variable in a function, this does not work just now
  retS(unknownVar);
}

string retS(const string &arg)
{
  return "ABC" + arg;
}

//#66 unknown variable
void negation()
{
  bool bValue; // no error
  dpGet("bla.", bValue); // no error
  dpSet("bla.", !bValue); // no error
}


void funcUsage()
{
  uk1 = (uint)2;
  
  uk2 = uk1;
  uk1 = !uk1;
  
  // undefined variable in function call
  dpGet("bla.", uk1);
  dpSet("bla.", !uk1);
  bool b = dpExists(uk1);
  // undefined variable from function return value
  uk2 = dpExists("bla.");
  
  // casting with undefined variable
  int i = (long)uk1;
}

void special()
{
  this.enable = TRUE; // typical panel code
  string s = $someDollar; // ignore dollars
}


class C{
  int i = 0;
  int i2 = i + 2;
  void retSomeI()
  {
    return i + i2 + i3; // i3 is not defined
  }
};


enum E{
  a = 1,
  b
};

void control_structures()
{
  //if / else
  if ( int_var > 0 )
  {
    const int i = 2;
  }
  else
  {
  }
  
  //switch / case / break / default
  switch (float_var)
  {
    case 1.0 :
    {
      break;
    }
 
    default:
    {
    }
  }
  
  //while / continue
  while (bool_var)
  {
    continue;
  }
  
  //for
  for (int i=1; i <10 ; i++)
  {
  } 
  
  //do-while
  do
  {
  }
  while(bool_var);
  
  //try / catch / finally / throw
  
  try
  {
    throw(errClass_var);      // optional
  }
  catch            
  {                                                       
  }
  finally         
  {                                 
  } 
}

int cast(){
  int i = 60;
  string str = "";

  i = ((int) str); //no error

  return i;
}