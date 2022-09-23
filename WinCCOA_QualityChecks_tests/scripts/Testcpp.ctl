//
// CtrlppCheck
// a static code analysis tool for WinCC OA's Ctrl language
//
// Copyright 2022 SIEMENS AG
//
// SPDX-License-Identifier: GPL-3.0-only
//

void main()
{
  int i = 1;
  int ifoo3;
  string f; // variable f never used
  string dp;
  string sfoo2;

  for (i ; i<j ; i++) // variable i should always be defined in the for-loop - variable j is not defined
  {
    int d = i +1; // definitions of variables in loops is dangerous
  
    DebugTN(d);
  }
  foo("aufruf-foo"); // calls of functions with INT should have an allocation
  
  for (int k ; k <= 4 ; k++)
  {
    dpGet("ExampleDP_AlertHdl1.",dp);  // masterclass - special-bug OA
  }
  
  sfoo2 = foo2("aufruf-foo2"); // return value of function with INT is written on STRING
  
  ifoo3 = foo3("aufruf-foo3"); // function parameter is INT / return value is STRING
  
  if (a = b); //; is wrong
    DebugTN(a);
       
  while(true)
  {
    dpGet("ExampleDP_AlertHdl1.");    
  }
    
  dpConnect("cb_work","ExampleDP_AlertHdl1.");
}

int foo (string s)
{
  if (s == "")
    return; // function with INT should return a value
  
  return; // function with INT should return a value
  DebugTN("foo"); // dead code
}

int foo2 (string s)
{
  return 1;
}


int foo3 (int l)
{
  return "foo3"; // function parameter is INT / return value is STRING
}

cb_work (string dp, bool value)
{
  DebugTN(dp);
}
