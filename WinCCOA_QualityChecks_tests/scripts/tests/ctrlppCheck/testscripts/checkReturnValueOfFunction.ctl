// start options: --inconclusive

// test for CheckReturnValueOfFunction::returnValueMatch

bool retTrue()
{
    return TRUE;
}

int retInt()
{
    return 10;
}


int declared_int()
{
  string retType;
  switch( retType )
  {
      // FAIL: function is declared int, it must returns some value
      case "declared_int" :
        return;

      // OK: function is declared int and returns int value
      case "declared_int_return_int" :
        return 10;
      
      // OK: function is declared int, but it returns boolean.
      // Because the casting from bool to int is allowed.
      case "declared_int_return_expression" :
        return (retType == "BC");

      // OK: function is declared int, but it returns boolean.
      // Because the casting from bool to int is allowed.
      case "declared_int_return_function_return_value_bool" :
        return retTrue();

      case "declared_int_return_function_return_value_string" :
        return "string";

      // OK: function is declared int, and return int from called by function getInt()
      case "declared_int_return_function_return_value_int" :
        return retInt();
  }
}

main()
{
  string retType;
  switch( retType )
  {      
      // OK: function is per default void, can not return value
      case "default_void" :
        return;

      // FAIL: function per default void can not return value
      case "default_void_return_int" :
        return 10;
      
      // FAIL: function per default void can not return value
      case "default_void_return_expression" :
        return (retType == "BC");

      // FAIL: function per default void can not return value   
      case "default_void_return_function_return_value" :
        return retTrue();
  }
}

void declared_void()
{
  string retType;
  switch( retType )
  {
      // OK: function is declared void, can not return value
      case "declared_void" :
        return;

      // FAIL: function is declared void can not return value
      case "declared_void_return_int" :
        return 10;
      
      // FAIL: function is declared void can not return value
      case "declared_void_return_expression" :
        return (retType == "BC");

      // FAIL: function is declared void can not return value   
      case "declared_void_return_function_return_value" :
        return retTrue();
  }
}

voidFunc1(){
    return;
}

voidFunc2(int &i){
    i++;
}


// test for keyword synchronized
// #169
synchronized int fsSynchronize()
{
  return;
}


// test for keyword void
// #165
synchronized void fsSynchronize()
{
  return 2;
}