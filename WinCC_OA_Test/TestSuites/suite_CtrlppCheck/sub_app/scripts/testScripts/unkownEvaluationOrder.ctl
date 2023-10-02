// start options: --inconclusive

/**
  @author ataker91
*/

//--------------------------------------------------------------------------------
// used libraries (#uses)

//--------------------------------------------------------------------------------
// variables and constants

//--------------------------------------------------------------------------------
/**
*/
main()
{
    dyn_string chars = makeDynString("a", "b", "c", "d", "e");
    int i = 0;

    DebugN(chars[++i], 
           chars[++i],
           chars[++i],
           chars[++i],
           chars[++i]);
}
