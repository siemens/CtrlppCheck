//--------------------------------------------------------------------------------
/**
  @file $relPath
  @copyright Copyright 2026 SIEMENS AG
             SPDX-License-Identifier: GPL-3.0-only
*/

/*!
 * @brief Tests for class: Mapping
 */

//--------------------------------------------------------------------------------
// used libraries (#uses)
#uses "classes/Variables/Mapping" /*!< tested object */
#uses "classes/oaTest/OaTest"

//--------------------------------------------------------------------------------
class TstMapping : OaTest
{
  public dyn_string getAllTestCaseIds()
  {
    return makeDynString(
      "Mapping_ctor_empty",
      "Mapping_ctor_with_value",
      "Mapping_get_set",
      "Mapping_getAt_existing_key",
      "Mapping_getAt_missing_key_default",
      "Mapping_getAt_missing_key_custom_default",
      "Mapping_opPlus"
    );
  }

  protected int startTestCase(const string tcId)
  {
    switch (tcId)
    {
      case "Mapping_ctor_empty":
      {
        Mapping m;
        assertEqual(mappinglen(m.get()), 0);
        return 0;
      }

      case "Mapping_ctor_with_value":
      {
        mapping input = makeMapping("key1", "value1", "key2", 42);
        Mapping m = Mapping(input);
        assertEqual(mappinglen(m.get()), 2);
        assertEqual(m.get()["key1"], "value1");
        assertEqual(m.get()["key2"], 42);
        return 0;
      }

      case "Mapping_get_set":
      {
        Mapping m;
        assertEqual(mappinglen(m.get()), 0);

        mapping newMap = makeMapping("a", 1, "b", 2);
        m.set(newMap);
        assertEqual(mappinglen(m.get()), 2);
        assertEqual(m.get()["a"], 1);
        assertEqual(m.get()["b"], 2);
        return 0;
      }

      case "Mapping_getAt_existing_key":
      {
        Mapping m = Mapping(makeMapping("name", "test", "count", 100));
        assertEqual(m.getAt("name"), "test");
        assertEqual(m.getAt("count"), 100);
        return 0;
      }

      case "Mapping_getAt_missing_key_default":
      {
        Mapping m = Mapping(makeMapping("exists", "yes"));
        anytype result = m.getAt("missing");
        assertTrue(isNull(result));
        return 0;
      }

      case "Mapping_getAt_missing_key_custom_default":
      {
        Mapping m = Mapping(makeMapping("exists", "yes"));
        assertEqual(m.getAt("missing", "default_value"), "default_value");
        assertEqual(m.getAt("missing", -1), -1);
        return 0;
      }

      case "Mapping_opPlus":
      {
        Mapping m1 = Mapping(makeMapping("a", 1, "b", 2));
        Mapping m2 = Mapping(makeMapping("c", 3, "d", 4));

        m1.opPlus(m2);

        mapping result = m1.get();
        assertEqual(mappinglen(result), 4);
        assertEqual(result["a"], 1);
        assertEqual(result["b"], 2);
        assertEqual(result["c"], 3);
        assertEqual(result["d"], 4);

        // Test overwriting existing keys
        Mapping m3 = Mapping(makeMapping("x", 10));
        Mapping m4 = Mapping(makeMapping("x", 20, "y", 30));

        m3.opPlus(m4);

        mapping result2 = m3.get();
        assertEqual(mappinglen(result2), 2);
        assertEqual(result2["x"], 20); // overwritten
        assertEqual(result2["y"], 30);
        return 0;
      }
    }

    return -1;
  }
};

//--------------------------------------------------------------------------------
main()
{
  TstMapping test;
  test.startAll();
  exit(0);
}
