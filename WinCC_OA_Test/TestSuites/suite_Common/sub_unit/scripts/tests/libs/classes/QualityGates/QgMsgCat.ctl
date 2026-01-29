//--------------------------------------------------------------------------------
/**
  @file $relPath
  @copyright Copyright 2026 SIEMENS AG
             SPDX-License-Identifier: GPL-3.0-only
*/

/*!
 * @brief Tests for class: QgMsgCat
 */

//--------------------------------------------------------------------------------
// used libraries (#uses)
#uses "classes/QualityGates/QgMsgCat" /*!< tested object */
#uses "classes/oaTest/OaTest"

//--------------------------------------------------------------------------------
class TstQgMsgCat : OaTest
{
  public dyn_string getAllTestCaseIds()
  {
    return makeDynString(
      "QgMsgCat_ctor_default",
      "QgMsgCat_ctor_with_name",
      "QgMsgCat_setName_getName",
      "QgMsgCat_setPrio",
      "QgMsgCat_getPriorityAsText_Info",
      "QgMsgCat_getPriorityAsText_Warning",
      "QgMsgCat_getPriorityAsText_Error",
      "QgMsgCat_getPriorityAsText_Unknown",
      "QgMsgCat_getText_dollar_replacement"
    );
  }

  protected int startTestCase(const string tcId)
  {
    switch (tcId)
    {
      case "QgMsgCat_ctor_default":
      {
        QgMsgCat msgCat;

        assertEqual(msgCat.getName(), "");
        return 0;
      }

      case "QgMsgCat_ctor_with_name":
      {
        QgMsgCat msgCat = QgMsgCat("TestCatalog");

        assertEqual(msgCat.getName(), "TestCatalog");
        return 0;
      }

      case "QgMsgCat_setName_getName":
      {
        QgMsgCat msgCat;

        msgCat.setName("MyCatalog");
        assertEqual(msgCat.getName(), "MyCatalog");

        msgCat.setName("AnotherCatalog");
        assertEqual(msgCat.getName(), "AnotherCatalog");
        return 0;
      }

      case "QgMsgCat_setPrio":
      {
        QgMsgCat msgCat;

        // setPrio should not throw
        msgCat.setPrio(QgMsgCatErrPrio::Info);
        msgCat.setPrio(QgMsgCatErrPrio::Warning);
        msgCat.setPrio(QgMsgCatErrPrio::Error);
        return 0;
      }

      case "QgMsgCat_getPriorityAsText_Info":
      {
        QgMsgCat msgCat;

        assertEqual(msgCat.getPriorityAsText(QgMsgCatErrPrio::Info), "Info");
        return 0;
      }

      case "QgMsgCat_getPriorityAsText_Warning":
      {
        QgMsgCat msgCat;

        assertEqual(msgCat.getPriorityAsText(QgMsgCatErrPrio::Warning), "Warning");
        return 0;
      }

      case "QgMsgCat_getPriorityAsText_Error":
      {
        QgMsgCat msgCat;

        assertEqual(msgCat.getPriorityAsText(QgMsgCatErrPrio::Error), "Error");
        return 0;
      }

      case "QgMsgCat_getPriorityAsText_Unknown":
      {
        QgMsgCat msgCat;

        // Invalid enum value should return "Unkwon" (typo in original code)
        assertEqual(msgCat.getPriorityAsText((QgMsgCatErrPrio)99), "Unkwon");
        return 0;
      }

      case "QgMsgCat_getText_dollar_replacement":
      {
        QgMsgCat msgCat = QgMsgCat("NonExistentCatalog");

        // When catalog doesn't exist, getText returns the key itself
        // But we can test dollar replacement on the key
        mapping dollars = makeMapping("var1", "value1", "var2", 42);
        string result = msgCat.getText("test.$var1.$var2.end", dollars);

        // The key should have $var1 and $var2 replaced
        assertTrue(strpos(result, "value1") >= 0);
        assertTrue(strpos(result, "42") >= 0);
        return 0;
      }
    }

    return -1;
  }
};

//--------------------------------------------------------------------------------
main()
{
  TstQgMsgCat test;
  test.startAll();
  exit(0);
}
