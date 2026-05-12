// $License: NOLICENSE
/** Tests for the library: scripts/libs/$origLibRelPath.

  @file $relPath
  @test Unit tests for the library: scripts/libs/$origLibRelPath
  @copyright $copyright
  @author $author
 */

//-----------------------------------------------------------------------------
// Libraries used (#uses)
#uses "$origLibRelPathWithoutExtension" // tested object
#uses "classes/oaTest/OaTest" // oaTest basic class

//-----------------------------------------------------------------------------
// Variables and Constants

//-----------------------------------------------------------------------------
/** Tests for $origLibName.ctl
*/
class newTestTemplate : OaTest
{
  //---------------------------------------------------------------------------
  /**
    @test Describe the test scenario here.
   */
  public int testSetAssertionState()
  {
    // type your test script here like
    this.assertEqual("currentValue1", "currentValue1");

    return 0;
  }
};

//-----------------------------------------------------------------------------
void main()
{
  newTestTemplate test;
  test.startAll();
}
