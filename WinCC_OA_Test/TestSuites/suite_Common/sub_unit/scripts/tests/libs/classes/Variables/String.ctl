//--------------------------------------------------------------------------------
/**
  @file $relPath
  @copyright Copyright 2026 SIEMENS AG
             SPDX-License-Identifier: GPL-3.0-only
*/

/*!
 * @brief Tests for class: String
 */

//--------------------------------------------------------------------------------
// used libraries (#uses)
#uses "classes/Variables/String" /*!< tested object */
#uses "classes/oaTest/OaTest"

//--------------------------------------------------------------------------------
class TstString : OaTest
{
  public dyn_string getAllTestCaseIds()
  {
    return makeDynString(
      "String_ctor_empty",
      "String_ctor_with_value",
      "String_get_set",
      "String_endsWith_true",
      "String_endsWith_false",
      "String_endsWith_empty"
    );
  }

  protected int startTestCase(const string tcId)
  {
    switch (tcId)
    {
      case "String_ctor_empty":
      {
        String s;
        assertEqual(s.get(), "");
        return 0;
      }

      case "String_ctor_with_value":
      {
        String s = String("hello world");
        assertEqual(s.get(), "hello world");
        return 0;
      }

      case "String_get_set":
      {
        String s;
        assertEqual(s.get(), "");

        s.set("test value");
        assertEqual(s.get(), "test value");

        s.set("another");
        assertEqual(s.get(), "another");

        s.set(); // reset to default
        assertEqual(s.get(), "");
        return 0;
      }

      case "String_endsWith_true":
      {
        String s = String("filename.ctl");
        assertTrue(s.endsWith(".ctl"));
        assertTrue(s.endsWith("ctl"));
        assertTrue(s.endsWith("l"));
        assertTrue(s.endsWith("filename.ctl"));

        String path = String("/path/to/file.xml");
        assertTrue(path.endsWith(".xml"));
        assertTrue(path.endsWith("file.xml"));
        return 0;
      }

      case "String_endsWith_false":
      {
        String s = String("filename.ctl");
        assertFalse(s.endsWith(".xml"));
        assertFalse(s.endsWith("CTL")); // case sensitive
        assertFalse(s.endsWith("filename"));
        assertFalse(s.endsWith("x"));
        return 0;
      }

      case "String_endsWith_empty":
      {
        String s = String("test");
        assertTrue(s.endsWith("")); // empty string always matches

        String empty = String("");
        assertTrue(empty.endsWith(""));
        return 0;
      }
    }

    return -1;
  }
};

//--------------------------------------------------------------------------------
main()
{
  TstString test;
  test.startAll();
  exit(0);
}
