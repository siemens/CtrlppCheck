// error id: badPerformanceInLoops


  /*
    library config
    
    
	<function name="dpGet">
		<notInLoop/>
		<returnValue type="int"/>
		<arg nr="1"/>
		<arg nr="2">
			<variadic/>
		</arg>
	</function>
	<function name="dpGetCache">
		<notInLoop>inconclusive</notInLoop>
		<returnValue type="int"/>
		<arg nr="1"/>
		<arg nr="2">
			<variadic/>
		</arg>
	</function>
    
    */

void main()
{
  anytype a;
  // no error
  dpGet("dpe", a);    
  string s = substr("dpe", a);
    
  for(int i; i; i++)
  {
    // error
    delay(1);
    // error
    dpGet("dpe", a);
    // no error
    dpSet("dpe", a);
    // error - incoclusive
    dpGetCache("dpe", a);
  }
  
  // no error
  dpGet("dpe", a);    
  s = substr("dpe", a);
}
