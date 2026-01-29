# Prompt: Create/Update WinCC OA Unit Tests (oaUnit)

Use this as a short **copy/paste prompt** for an AI coding agent.

## You provide (inputs)

- Target Ctrl file (repo-relative): `classes/.../*.ctl`
- WinCC OA version (default `3.20`)
- What should be verified (2–6 bullet points)

## Hard repo rules (don't violate)

- Unit tests live under:
  `WinCC_OA_Test/TestSuites/suite_Common/sub_unit/scripts/tests/`
- Mirror the production path under `scripts/tests/libs/`:
  - Tested: `classes/Variables/Float.ctl`
  - Test: `WinCC_OA_Test/TestSuites/suite_Common/sub_unit/scripts/tests/libs/classes/Variables/Float.ctl`
- Tests must `#uses "classes/oaTest/OaTest"` and call `test.startAll()` from `main()`.
- Add/update registration in:
  `WinCC_OA_Test/TestSuites/suite_Common/testProj.unit.config` (`TEST_MANAGERS` entry).
- The unit test project runs **without DB** (`PACK_SEL: 2`) → no `dp*()` usage.
- Ctrl limitation: avoid nested class definitions (define mocks at top-level).

## Known dependency constraints (no-DB incompatible)

Some libraries cannot be tested in the no-DB (`PACK_SEL: 2`) environment:

| Dependency | Impact | Examples |
|------------|--------|----------|
| `#uses "CtrlPv2Admin"` | ❌ Fails at load time | FunctionData.ctl, ToolLizard.ctl |
| `#uses "panel"` | ❌ Fails at load time | CppCheck.ctl |
| `#uses "CtrlXml"` | ⚠️ May fail | XML processing classes |
| `dp*()` functions | ❌ Runtime error | Any DB-dependent code |
| `getCatStr()` | ⚠️ Returns key | Message catalog lookups |

**Workaround:** If a tested class has these dependencies via `#uses`, the test cannot run in `PACK_SEL: 2` mode. Mark such files in `toDoTests.md` with "⚠️ deps" note.

## Ctrl language quirks

- **Operator overloading:** Don't use `obj1 + obj2` syntax. Call the method explicitly: `obj1.opPlus(obj2)`.
- **Enums from other files:** If an enum is defined in a file with incompatible dependencies, copy it locally in the test file.
- **`main()` function:** Must be declared as `void main()` or just `main()`, and must call `exit(0)` at the end.

## Interactive development workflow

For faster test iteration, use the `-startIDE` option:

```powershell
# 1. Start project with GEDI (runs once, stays open)
.\executeTests.cmd -oaVersion 3.20 -oaTestRunId Common -startIDE

# 2. Run individual tests directly (fast iteration)
& "C:\Siemens\Automation\WinCC_OA\3.20\bin\WCCOActrl.exe" -proj Common_Unit_3.20 -n tests/libs/classes/Variables/Mapping.ctl
```

The `-n` flag is required because the project has no database (`PACK_SEL: 2`).

## Copy/paste prompt

```text
You are a coding agent in a VS Code workspace.

Create or update an oaUnit unit test for:
- Target Ctrl file: <RELATIVE_CTL_PATH>
- WinCC OA version: 3.20

Required outputs:
1) Create/update the test file at:
   WinCC_OA_Test/TestSuites/suite_Common/sub_unit/scripts/tests/libs/<RELATIVE_CTL_PATH>
2) Ensure WinCC_OA_Test/TestSuites/suite_Common/testProj.unit.config has a TEST_MANAGERS entry:
   "tests/libs/<RELATIVE_CTL_PATH> -n"

Constraints:
- No DB access (PACK_SEL: 2), so no dp*().
- Keep tests deterministic.
- If the tested code can terminate/abort a manager (fatal/throwError/etc.), use a mock/stub seam to keep the test manager alive.
- Don't use operator syntax like `obj1 + obj2`, call methods explicitly: `obj1.opPlus(obj2)`.

Preferred style (use this):
- Implement getAllTestCaseIds() + startTestCase(tcId) with a switch.

Reference examples for style only:
- WinCC_OA_Test/TestSuites/suite_Common/sub_unit/scripts/tests/libs/classes/Variables/Float.ctl
- WinCC_OA_Test/TestSuites/suite_Common/sub_unit/scripts/tests/libs/classes/Variables/Mapping.ctl
- WinCC_OA_Test/TestSuites/suite_Common/sub_unit/scripts/tests/libs/classes/QualityGates/Tools/CppCheck/CppCheckSettings.ctl

What to test:
<PASTE YOUR BEHAVIOR BULLETS HERE>
```

## Minimal skeleton (preferred repo style)

```ctrl
//--------------------------------------------------------------------------------
/**
  @file $relPath
  @copyright Copyright 2026 SIEMENS AG
             SPDX-License-Identifier: GPL-3.0-only
*/

/*!
 * @brief Tests for class: <NAME>
 */

//--------------------------------------------------------------------------------
// used libraries (#uses)
#uses "<RELATIVE_CTL_PATH_WITHOUT_.ctl>" /*!< tested object */
#uses "classes/oaTest/OaTest"

//--------------------------------------------------------------------------------
class Tst<NAME> : OaTest
{
  public dyn_string getAllTestCaseIds()
  {
    return makeDynString(
      "<NAME>_testCase1",
      "<NAME>_testCase2"
    );
  }

  protected int startTestCase(const string tcId)
  {
    switch (tcId)
    {
      case "<NAME>_testCase1":
      {
        // Arrange
        // Act
        // Assert (assertTrue/assertFalse/assertEqual/assertNotEqual)
        return 0;
      }

      case "<NAME>_testCase2":
      {
        // ...
        return 0;
      }
    }

    return -1;
  }
};

//--------------------------------------------------------------------------------
main()
{
  Tst<NAME> test;
  test.startAll();
  exit(0);
}
```

## Available assertion methods (from OaTest)

| Method | Description |
|--------|-------------|
| `assertTrue(bool)` | Assert value is TRUE |
| `assertFalse(bool)` | Assert value is FALSE |
| `assertEqual(a, b)` | Assert a == b |
| `assertNotEqual(a, b)` | Assert a != b |

## Checklist before committing

- [ ] Test file created at correct path
- [ ] `testProj.unit.config` updated with TEST_MANAGERS entry
- [ ] Test runs successfully: `WCCOActrl.exe -proj Common_Unit_3.20 -n <test_path>`
- [ ] No `dp*()` calls or DB-dependent code
- [ ] Exit code is 0 (all tests pass)
