//--------------------------------------------------------------------------------
/**
  @file $relPath
  @copyright Copyright 2026 SIEMENS AG
             SPDX-License-Identifier: GPL-3.0-only
*/

/*!
 * @brief Tests for class: CppCheckSettings
 */

//--------------------------------------------------------------------------------
// used libraries (#uses)
#uses "classes/QualityGates/Tools/CppCheck/CppCheckSettings" /*!< tested object */
#uses "classes/oaTest/OaTest"

//--------------------------------------------------------------------------------
class TstCppCheckSettings : OaTest
{
  public dyn_string getAllTestCaseIds()
  {
    return makeDynString(
      "CppCheckSettings_default_values",
      "CppCheckSettings_addEnabled",
      "CppCheckSettings_enableXmlFormat",
      "CppCheckSettings_addRuleFile",
      "CppCheckSettings_addLibraryFile",
      "CppCheckSettings_toCmdLine_minimal",
      "CppCheckSettings_toCmdLine_with_options"
    );
  }

  protected int startTestCase(const string tcId)
  {
    switch (tcId)
    {
      case "CppCheckSettings_default_values":
      {
        CppCheckSettings settings;

        assertFalse(settings.enableLibCheck);
        assertTrue(settings.inconclusive);
        assertTrue(settings.includeSubProjects);
        assertFalse(settings.verbose);
        assertFalse(settings.errorList);
        assertFalse(settings.inlineSuppressions);
        assertEqual(settings.winccoaProjectName, PROJ);
        return 0;
      }

      case "CppCheckSettings_addEnabled":
      {
        CppCheckSettings settings;

        settings.addEnabled("all");
        dyn_string cmd = settings.toCmdLine();
        assertTrue(dynContains(cmd, "--enable=all") > 0);

        // Add another
        CppCheckSettings settings2;
        settings2.addEnabled("style");
        settings2.addEnabled("warning");
        cmd = settings2.toCmdLine();
        assertTrue(dynContains(cmd, "--enable=style,warning") > 0);
        return 0;
      }

      case "CppCheckSettings_enableXmlFormat":
      {
        CppCheckSettings settings;

        assertFalse(settings.isXmlOutEnabled());

        settings.enableXmlFormat(TRUE);
        assertTrue(settings.isXmlOutEnabled());

        dyn_string cmd = settings.toCmdLine();
        assertTrue(dynContains(cmd, "--xml") > 0);

        settings.enableXmlFormat(FALSE);
        assertFalse(settings.isXmlOutEnabled());
        return 0;
      }

      case "CppCheckSettings_addRuleFile":
      {
        CppCheckSettings settings;
        settings.includeSubProjects = FALSE; // avoid subproject paths in output

        string rulePath = "/path/to/rule.xml";
        settings.addRuleFile(rulePath);

        dyn_string cmd = settings.toCmdLine();
        bool found = FALSE;
        for (int i = 1; i <= dynlen(cmd); i++)
        {
          if (strpos(cmd[i], "--rule-file=") == 0)
          {
            found = TRUE;
            break;
          }
        }
        assertTrue(found);

        // Adding same file again should not duplicate
        settings.addRuleFile(rulePath);
        int count = 0;
        cmd = settings.toCmdLine();
        for (int i = 1; i <= dynlen(cmd); i++)
        {
          if (strpos(cmd[i], "--rule-file=") == 0)
            count++;
        }
        assertEqual(count, 1);
        return 0;
      }

      case "CppCheckSettings_addLibraryFile":
      {
        CppCheckSettings settings;
        settings.includeSubProjects = FALSE;

        string libPath = "/path/to/library.xml";
        settings.addLibraryFile(libPath);

        dyn_string cmd = settings.toCmdLine();
        bool found = FALSE;
        for (int i = 1; i <= dynlen(cmd); i++)
        {
          if (strpos(cmd[i], "--library=") == 0)
          {
            found = TRUE;
            break;
          }
        }
        assertTrue(found);

        // Adding empty path should be ignored
        int prevLen = dynlen(cmd);
        settings.addLibraryFile("");
        cmd = settings.toCmdLine();
        assertEqual(dynlen(cmd), prevLen);
        return 0;
      }

      case "CppCheckSettings_toCmdLine_minimal":
      {
        CppCheckSettings settings;
        settings.includeSubProjects = FALSE;
        settings.inconclusive = FALSE;

        dyn_string cmd = settings.toCmdLine();

        // Should contain project name
        bool hasProject = FALSE;
        for (int i = 1; i <= dynlen(cmd); i++)
        {
          if (strpos(cmd[i], "--winccoa-projectName=") == 0)
          {
            hasProject = TRUE;
            break;
          }
        }
        assertTrue(hasProject);
        return 0;
      }

      case "CppCheckSettings_toCmdLine_with_options":
      {
        CppCheckSettings settings;
        settings.includeSubProjects = FALSE;
        settings.inconclusive = TRUE;
        settings.verbose = TRUE;
        settings.inlineSuppressions = TRUE;
        settings.enableLibCheck = TRUE;
        settings.enableXmlFormat(TRUE);
        settings.addEnabled("all");

        dyn_string cmd = settings.toCmdLine();

        assertTrue(dynContains(cmd, "--inconclusive") > 0);
        assertTrue(dynContains(cmd, "-v") > 0);
        assertTrue(dynContains(cmd, "--inline-suppr") > 0);
        assertTrue(dynContains(cmd, "--check-library") > 0);
        assertTrue(dynContains(cmd, "--xml") > 0);
        assertTrue(dynContains(cmd, "--enable=all") > 0);
        return 0;
      }
    }

    return -1;
  }
};

//--------------------------------------------------------------------------------
main()
{
  TstCppCheckSettings test;
  test.startAll();
  exit(0);
}
