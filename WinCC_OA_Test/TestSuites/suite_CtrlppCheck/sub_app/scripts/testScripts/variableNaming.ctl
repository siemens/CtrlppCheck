//

//const int script global
const int cig1, cig2, //makes no sense
          cig3; 
const int cig4 = 1, cig5 = 2,
          cig6 = 3;

//const string script global
const string csg1, csg2, //makes no sense
             csg3;
const string csg4 = "1", csg5 = "2",
             csg6 = "3";

//int script global
int ig1, ig2, 
    ig3;
int ig4 = 1, ig5 = 2,
    ig6 = 3;

//string script global
string sg1, sg2,
       sg3;
string sg4 = "1", sg5 = "2",
       sg6 = "3";


//-----------------------------------------------------------------------------

main()
{  
    //const int local
    const int cil1, cil2, //makes no sense
              cil3; 
    const int cil4 = 1, cil5 = 2,
              cil6 = 3;

    //const string local
    const string csl1, csl2, //makes no sense
                 csl3;
    const string csl4 = "1", csl5 = "2",
                 csl6 = "3";

    //int local
    int il1, il2, 
        il3;
    int il4 = 1, il5 = 2,
        il6 = 3;

    //string local
    string sl1, sl2,
           sl3;
    string sl4 = "1", sl5 = "2",
           sl6 = "3";
}
//-----------------------------------------------------------------------------

void constVariablesArgument(const int cia1, const int cia2 = 1, const string csa1, const string csa2 = "1")
{
   
}

//-----------------------------------------------------------------------------

void variablesArgument(int ia1, int ia2 = 1, string sa1, string sa2 = "1")
{
   
}

//-----------------------------------------------------------------------------

/** 
*/
void dummyFunction()
{  
  for ( int i = 10 ; i >= 1; i-- ) { } 
  for ( int ics = 10 ; ics >= 1; ics-- ) { } 
  
  if ( ig1 == 0 )
  {
      //const int local
    const int cics1, cics2, //makes no sense
              cics3; 
    const int cics4 = 1, cics5 = 2,
              cics6 = 3;

    //const string local
    const string cscs1, cscs2, //makes no sense
                 cscs3;
    const string cscs4 = "1", cscs5 = "2",
                 cscs6 = "3";

    //int local
    int ics1, ics2, 
        ics3;
    int ics4 = 1, ics5 = 2,
        ics6 = 3;

    //string local
    string scs1, scs2,
           scs3;
    string scs4 = "1", scs5 = "2",
           scs6 = "3";
  }     
}

//------------------------------------------------------------------------------

class VariableNaming
{
    //----------------------------------------------------------------------------
    //@public
    //----------------------------------------------------------------------------

    //public const int class
    public const int pucic1, pucic2, //makes no sense
              pucic3; 
    public const int pucic4 = 1, pucic5 = 2,
              pucic6 = 3;

    //public const string class
    public const  string pucsc1, pucsc2, //makes no sense
                 pucsc3;
    public const string pucsc4 = "1", pucsc5 = "2",
                 pucsc6 = "3";

    //public int class
    public int puic1, puic2, 
        puic3;
    public int puic4 = 1, puic5 = 2,
        puic6 = 3;

    //public string class
    public string pusc1, pusc2,
           pusc3;
    public string pusc4 = "1", pusc5 = "2",
           pusc6 = "3";

    //----------------------------------------------------------------------------
    public VariableNaming(){}

    //----------------------------------------------------------------------------
    //@protected
    //----------------------------------------------------------------------------

    //----------------------------------------------------------------------------
    //@private
    //----------------------------------------------------------------------------

    //public const int class
    private const int prcic1, prcic2, //makes no sense
                      prcic3; 
    const int prcic4 = 1, prcic5 = 2,
              prcic6 = 3;

    //public const string class
    private  string prcsc1, prcsc2, //makes no sense
                    prcsc3;
    const string prcsc4 = "1", prcsc5 = "2",
                 prcsc6 = "3";

    //public int class
    private int pric1, pric2, 
                pric3;
    int pric4 = 1, pric5 = 2,
        pric6 = 3;

    //public string class
    private string prsc1, prsc2,
                   prsc3;
    string prsc4 = "1", prsc5 = "2",
           prsc6 = "3";
};