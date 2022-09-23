void main()
{
  if ( strlen(a) == 0 )
    DebugN("abc");
  if ( strlen(a) > 1 )
    DebugN("abc");
  if ( strlen(a) > 0 )
    DebugN("abc");
	
  for(int i = 1; i <= 10; i++)
  {
    anytype value;
    dpGet("abc", value);
  }
  
  bool value;
  while(!value)
  {
    dpGet("abc", value);
  }
  while(!value)
  {
    // dpGet("abc", value);
  }
  
  try
  {
  }
  catch
  {
  }
}