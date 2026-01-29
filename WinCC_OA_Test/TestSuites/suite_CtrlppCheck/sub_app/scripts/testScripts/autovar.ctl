//
// tests for CheckAutoVariables
// CheckAutoVariables::assignFunctionArg
#uses "std"

struct S
{
  bool b;
};

/// CheckAutoVariables::errorUselessAssignmentArg()
/// id: uselessAssignmentArg
void foo7(anytype a, bool b, dyn_bool db, S s)
{
  a = 1;
  b = 0;
  db = makeDynBool();
  s.b = false;  // #127 this check does not work just now
  S s2;
  s = s2;
}
/// errorUselessAssignmentArg in to const references
void foo8(const anytype &a, const bool &b, const dyn_bool &db, const S &s)
{
  a = 1;
  b = 0;
  db = makeDynBool();
  s.b = false;
}

/// uselessAssignmentArg with default value
void foo9(anytype a = NULL, bool b = false, dyn_bool db = makeDynBool())
{
  a = 1;
  b = 0;
  db = makeDynBool();
}

/// No error, because the reference can be used outside of function scope
void foo10(const anytype &a, const bool &b, const dyn_bool &db, const S &s)
{
  a = 1;
  b = 0;
  db = makeDynBool();
  s.b = false;
}

/// No error, because the argument is reused in function scope
void foo11(const anytype &a, const bool &b, const dyn_bool &db, const S &s)
{
  a = 1;
  b = 0;
  db = makeDynBool();
  s.b = false;
  
  anytype a2 = a;
  bool b2 = b;
  dyn_bool db2 = db;
  S s2 = s;
}

/// smoke check for ctrl++
class CheckAutoVariables_Class
{
  CheckAutoVariables_Class(anytype a, bool b, dyn_bool db, S s)
  {
    a = 1;
    b = 0;
    db = makeDynBool();
    s.b = false;  // #127 this check does not work just now
  }
  public foo1(anytype a, bool b, dyn_bool db, S s)
  {
    a = 1;
    b = 0;
    db = makeDynBool();
    s.b = false; // #127 this check does not work just now
  }
  private foo2(anytype a, bool b, dyn_bool db, S s)
  {
    a = 1;
    b = 0;
    db = makeDynBool();
    s.b = false; // #127 this check does not work just now
  }
  protected foo3(anytype a, bool b, dyn_bool db, S s)
  {
    a = 1;
    b = 0;
    db = makeDynBool();
    s.b = false; // #127 this check does not work just now
  }
};

class CheckAutoVariables_Struct
{
  CheckAutoVariables_Struct(anytype a, bool b, dyn_bool db, S s)
  {
    a = 1;
    b = 0;
    db = makeDynBool();
    s.b = false; // #127 this check does not work just now
  }
  public foo(anytype a, bool b, dyn_bool db, S s)
  {
    a = 1;
    b = 0;
    db = makeDynBool();
    s.b = false; // #127 this check does not work just now
  }
  private foo2(anytype a, bool b, dyn_bool db, S s)
  {
    a = 1;
    b = 0;
    db = makeDynBool();
    s.b = false; // #127 this check does not work just now
  }
  protected foo3(anytype a, bool b, dyn_bool db, S s)
  {
    a = 1;
    b = 0;
    db = makeDynBool();
    s.b = false; // #127 this check does not work just now
  }
};