// start options:
// $License: NOLICENSE

//--------------------------------------------------------------------------------
/**
  @file $relPath
  @copyright $copyright
  @author mPokorny
  @test Check for possible ISO88591 issues
*/

/** @test check usage of fileToString() function
*/
void test_fileToString()
{
  string path = PROJ_PATH + CONFIG_REL_PATH + "config";
  string s;
  // wrong usage, because encoding is not defined
  fileToString(path, s);

  // correct encoding
  fileToString(path, s, "UTF8");
  
  // explicite defined ISO encoding
  fileToString(path, s, "ISO88591");
}

