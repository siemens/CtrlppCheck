// test for function arg count
void main()
{
// this is error only when you use the option check-lib-config.
// the ctrlppcheck check arguments only for functions defined in config-file
  string s1 = retS();
  string s2 = retS("ABC");
  string s3 = retS("B", 2);
  
  bool b1 = dpExists(); // to less arguments
  bool b2 = dpExists("dpe"); // OK
  bool b3 = dpExists("dpe2", 2);  // to much arguments
  dpExists(); // missing arg, missing ret value
  dpSubStr(); // missing arg, missing ret value
  
  char c = dpExists(); // missing arg
  
  anytype v;

  dpGet();  // missing arg
  dpGet("abc"); // missing arg
  dpGet("abc", v); // OK
  /// @warning it is not possible to check paired arguments. So this line will be OK for cpp check.
  dpGet("abc", v, "cde");
  anytype v2;
  dpGet("abc", v, "cde", v2);
  
  //methods with 0 or 1 optional argument
  unsigned ui1 = getUserId();
  unsigned ui1 = getUserId("root");
  
  //methods with optional argument not at the end 
  dpConnect("callbackFunk", false, "ExampleDP_AlertHdl1.");
  dpConnect("callbackFunk", false, "ExampleDP_AlertHdl1.", "ExampleDP_AlertHdl2.");
  dpConnect("callbackFunk", "ExampleDP_AlertHdl1.");
  dpConnect("callbackFunk", "ExampleDP_AlertHdl1.", "ExampleDP_AlertHdl2.");
  
  //same method in different version with same arg count but different return value + function in function test
  dyn_string alias = dpGetAlias(dpNames("*ExampleDP_*.:"));
  string alias = dpGetAlias("ExampleDP_AlertHdl1.");
}

string retS(const string &arg)
{
  return "ABC" + arg;
}