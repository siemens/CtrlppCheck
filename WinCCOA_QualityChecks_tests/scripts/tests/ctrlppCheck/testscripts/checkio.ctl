// start options:
// $License: NOLICENSE
//--------------------------------------------------------------------------------
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
  const int ci_val;
  const float cf_val;
  int val = 0;
  string str = "";
  string out;
  blob bl; 

  sscanf("isad = 1", "%s = %d", str, val);

  sprintf(str, "%s", false); // Wrong argument type
  sprintf(str, "%u", "xyz"); // Wrong argument type
  sprintf(str, "%u%s", 1); // Too few arguments
  sprintf(str, "", 1); // Too much arguments

  //SCANF

  //%s
  sscanf("", "%s", val); //error id: invalidScanfArgType_s

  //%c
  // sscanf("", "%c", );

  //%x %X %u %o %d %n //error id:invalidScanfArgType_int
  sscanf("", "%x", "abc"); 
  sscanf("", "%X", "abc");
  sscanf("", "%u", "abc");
  sscanf("", "%o", "abc");
  sscanf("", "%d", "abc");
  sscanf("", "%n", "abc");
  
  sscanf("", "%x", str);
  sscanf("", "%X", str);
  sscanf("", "%u", str);
  sscanf("", "%o", str);
  sscanf("", "%d", str);
  sscanf("", "%n", str);

  sscanf("", "%x", ci_val);
  sscanf("", "%X", ci_val);
  sscanf("", "%u", ci_val);
  sscanf("", "%o", ci_val);
  sscanf("", "%d", ci_val);
  sscanf("", "%n", ci_val);

  sscanf("", "%lx", val);
  sscanf("", "%lX", val);
  sscanf("", "%lu", val);
  sscanf("", "%lo", val);
  sscanf("", "%ld", val);
  sscanf("", "%ln", val);


  //%e %E %f %g %G %a //error id:invalidScanfArgType_float
  sscanf("", "%e", "abc"); 
  sscanf("", "%E", "abc");
  sscanf("", "%f", "abc");
  sscanf("", "%g", "abc");
  sscanf("", "%G", "abc");
  sscanf("", "%a", "abc");
  
  sscanf("", "%e", str); 
  sscanf("", "%E", str);
  sscanf("", "%f", str);
  sscanf("", "%g", str);
  sscanf("", "%G", str);
  sscanf("", "%a", str);

  sscanf("", "%e", cf_val); 
  sscanf("", "%E", cf_val);
  sscanf("", "%f", cf_val);
  sscanf("", "%g", cf_val);
  sscanf("", "%G", cf_val);
  sscanf("", "%a", cf_val);

  sscanf("", "%e", val); 
  sscanf("", "%E", val);
  sscanf("", "%f", val);
  sscanf("", "%g", val);
  sscanf("", "%G", val);
  sscanf("", "%a", val);

  //PRINTF

  //%s //error id: invalidPrintfArgType_s
  sprintf(out, "%s", val); //error id: invalidScanfArgType_s

  //%c %x %X %o //error id: invalidPrintfArgType_uint
  sprintf(out, "%c", "abc"); 
  sprintf(out, "%x", "abc"); 
  sprintf(out, "%C", "abc"); 
  sprintf(out, "%o", "abc");

  sprintf(out, "%c", str); 
  sprintf(out, "%x", str); 
  sprintf(out, "%C", str); 
  sprintf(out, "%o", str);

  sprintf(out, "%lc", val); 
  sprintf(out, "%lx", val); 
  sprintf(out, "%lC", val); 
  sprintf(out, "%lo", val); 

  //%d //error id: invalidPrintfArgType_sint
  sprintf(out, "%d", "abc"); 

  sprintf(out, "%d", str); 
  
  sprintf(out, "%ld", val); 

  //%u

  //%e %E %f %g %G //error id: invalidPrintfArgType_float
  sprintf("", "%e", "abc"); 
  sprintf("", "%E", "abc");
  sprintf("", "%f", "abc");
  sprintf("", "%g", "abc");
  sprintf("", "%G", "abc");
  
  sprintf("", "%e", str); 
  sprintf("", "%E", str);
  sprintf("", "%f", str);
  sprintf("", "%g", str);
  sprintf("", "%G", str);

  //%l

  //FILE OPERATIONS

  fflush(stdin);

  file f1; //CLOSED //fflush() called on input stream &apos;stdin&apos; may result in undefined behaviour on non-linux systems. error id: fflushOnInputStream

  //POSITIONING
  rewind(f1); //Used file that is not opened. error id: useClosedFile
  fflush (f1); //Used file that is not opened. error id: useClosedFile
  fseek(f1, 30, SEEK_SET); //Used file that is not opened. error id: useClosedFile

  //READ
  fgets(str, 30, f1); //Used file that is not opened. error id: useClosedFile
  fread(f1, bl, 4); //Used file that is not opened. error id: useClosedFile
  fscanf(f1, "%1s = %d", str, val); //Used file that is not opened. error id: useClosedFile

  //WRITE
  fputs("adc", f1);//Used file that is not opened. error id: useClosedFile
  fwrite(f1, bl, 30); //Used file that is not opened. error id: useClosedFile
  fprintf(f1, "%s = %d", str, val); //Used file that is not opened. error id: useClosedFile

  //CLOSE
  fclose(f1) //Used file that is not opened. error id: useClosedFile

  //UNIMPORTANT
  feof(f1); //Used file that is not opened. error id: useClosedFile
  ferror(f1); //Used file that is not opened. error id: useClosedFile
  ftell(f1); //Used file that is not opened. error id: useClosedFile

  file f2 = fopen("adadd", "a"); //APPEND
  fseek(f2, 30, SEEK_SET); //Repositioning operation performed on a file opened in append mode has no effect. error id: seekOnAppendedFile

  file f3 = fopen("adadd", "w"); //WRITE
  fgets(str, 30, f3); //Read operation on a file that was opened only for writing. error id: readWriteOnlyFile
  fread(f3, bl, 4); //Read operation on a file that was opened only for writing. error id: readWriteOnlyFile
  fscanf(f3, "%1s = %d", str, val); //Read operation on a file that was opened only for writing. error id: readWriteOnlyFile

  file f4 = fopen("adaad", "r+"); //READ WRITE
  fputs("test", f4);  //error id: IOWithoutPositioning
  fgets(str, 30, f4); //Read and write operations without a call to a positioning function (fseek or rewind) or fflush in between result in undefined behaviour.

  fflush(f4);

  fgets(str, 30, f4); //error id: IOWithoutPositioning
  fputs("test", f4); //Read and write operations without a call to a positioning function (fseek or rewind) or fflush in between result in undefined behaviour.

  file f5 = fopen("addad", "r");
  fputs("adc", f5); //Write operation on a file that was opened only for reading. error id: writeReadOnlyFile
  fwrite(f5, bl, 30); //Write operation on a file that was opened only for reading. error id: writeReadOnlyFile
  fprintf(f5, "%s = %d", str, val); //Write operation on a file that was opened only for reading. error id: writeReadOnlyFile


}
