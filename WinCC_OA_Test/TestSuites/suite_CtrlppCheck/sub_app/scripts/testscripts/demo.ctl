// start options: --inconclusive --check-library --inline-suppr
void main()
{
  int iA, iB = rand(); 
  int iC = rand();
  string sDp = "ExampleDP_AlertHdl1.";

  subfunc("subfunc"); 
  subfunc2(true,iA);
  subfunc3(sDp);
  semicol();
  
  if (iA >= B)
  {
    //do something
  }
  else if (iA >= iC)
  {
   //do something
  }
  else if (iA >= iC) 
  {
    //do something
  }
  
  if (!dpExists(sDp))  
    DebugTN("DP",sDp,bRdp);   
}

int subfunc (string s = "")
{
  if (s != "")
    return 1; 
  
  return 0; 
}

bool subfunc2(bool bA)
{
  if(bA)
  {
    if(g_A == 1)
    {
      //do something
    }
    else
    {
      //do something
    }
  }
}

string subfunc3 (string s = "")
{
  if (s != "")
    return s; 
  
  dpSubStr(sDp,DPSUB_SYS_DP_EL_CONF_DET_ATT); 
   
  return ""; 
  DebugTN(0); 
}

int semicol()
{
  int a = rand(), b = rand();
  
  // error
  if ( a == b );
  {  }
  
  // no error
  if ( a>= b )
  {  }
  
  // error
  if ( a <= b );
    DebugTN(a);
    
  // error
  if ( a > b );
  
  
  DebugTN(a);
}
