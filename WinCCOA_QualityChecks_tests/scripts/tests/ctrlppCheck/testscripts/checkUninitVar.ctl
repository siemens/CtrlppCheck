// start options: --inconclusive

// tests for uninitvar
// CheckUninitVar::valueFlowUninit

void valueFlowUninit()
{
  string s1;
  string s2 = "dpe.";
  
  // Uninitialized variable: s1 (CWE: 908)
  string s3 = s1 + s2;

  string dpeValue;
  string dpe = "dpe.";
  // Uninitialized variable: dpeValue (CWE: 908)
  dpGet(dpe + ".Config", dpeValue); 

  
  string dpeValue2;
  string dpe2;
  // Uninitialized variable: dpe2 (CWE: 908)
  // Uninitialized variable: dpeValue2 (CWE: 908)
  dpGet(dpe2 + ".Config", dpeValue2);
}