//

/**
  @file $relPath
  @copyright $copyright
  @author Alex
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
  dyn_string ds; //size: 0 <- won't be checked by checkbufferoverrun because of array size 0
  dyn_string ds1 = makeDynString(); //size: 0 <- won't be checked by checkbufferoverrun because of array size 0
  dyn_string ds2 = makeDynString("1"); //size: 1
  dyn_string ds3 = makeDynString("1", 2, 3); //size: 3

  string s;

  //no-error
  s = ds2[1];
  s = ds3[1];
  s = ds3[2];
  s = ds3[3];

  //error
  s = ds[-1]; // Use of Uninitialized variable: ds | Array index -1 is out of bounds.
  s = ds1[-1]; // Array index -1 is out of bounds. | Array 'ds1[0]' accessed at index -1, which is out of bounds.
  s = ds2[-1]; // Array index -1 is out of bounds. | Array 'ds2[1]' accessed at index -1, which is out of bounds.

  s = ds[0];  // Use of Uninitialized variable: ds
  s = ds1[0]; // ToDo: Should be checked in future
  s = ds2[0]; // Array 'ds2[1]' accessed at index 0, which is out of bounds.

  s = ds[1];  // Use of Uninitialized variable: ds
  s = ds1[1]; // ToDo: Should be checked in future

  s = ds[2];  // Use of Uninitialized variable: ds
  s = ds1[2]; // ToDo: Should be checked in future
  s = ds2[2]; // Array 'ds2[1]' accessed at index 2, which is out of bounds.
  
  s = ds3[4]; // Array 'ds3[3]' accessed at index 4, which is out of bounds.


  
//   ds[4] = "abc";
//   s = ds3[4];

//   dynAppend(ds3, "ab");
//   s = ds3[4];
}
