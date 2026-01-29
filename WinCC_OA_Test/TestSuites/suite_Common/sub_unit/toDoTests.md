# Unit Test Coverage - ToDo List

> **Generated:** 2026-01-29  
> **Test Coverage:** ~33% (14 of 43 libraries have tests)

## Summary

| Metric              | Count |
|---------------------|-------|
| Total Libraries     | 43    |
| Existing Tests      | 14    |
| Missing Tests       | 29    |

---

## ✅ Existing Tests (14)

| Library Path | Test Status |
|--------------|-------------|
| `fileSys.ctl` | ✅ Done |
| `classes/ErrorHdl/OaLogger.ctl` | ✅ Done |
| `classes/FileSys/QgDir.ctl` | ✅ Done |
| `classes/FileSys/QgFile.ctl` | ✅ Done |
| `classes/Math/Math.ctl` | ✅ Done |
| `classes/QualityGates/Qg.ctl` | ✅ Done |
| `classes/QualityGates/QgAddOnResultErr.ctl` | ✅ Done |
| `classes/QualityGates/QgTest.ctl` | ✅ Done |
| `classes/QualityGates/QgVersionResult.ctl` | ✅ Done |
| `classes/QualityGates/Tools/CppCheck/CppCheckError.ctl` | ✅ Done |
| `classes/QualityGates/Tools/CppCheck/CppCheckSettings.ctl` | ✅ Done |
| `classes/Variables/Float.ctl` | ✅ Done |
| `classes/Variables/Mapping.ctl` | ✅ Done |
| `classes/Variables/String.ctl` | ✅ Done |

---

## ❌ Missing Tests (29)

> **Skipped:** `gedi/qualityCheck_ext.ctl`, `scriptEditor/ctrlPPCheck_ext.ctl` (UI extensions, not in `/scripts/libs/classes/`)

### classes/QualityGates/ (4)

| Library | Priority | Notes |
|---------|----------|-------|
| `classes/QualityGates/QgBase.ctl` | Medium | Base class, has OaLogger dep |
| `classes/QualityGates/QgMsgCat.ctl` | Medium | Message catalog, needs getCatStr() |
| `classes/QualityGates/QgResultPublisher.ctl` | Low | Needs file output |
| `classes/QualityGates/QgSettings.ctl` | High (⚠️ deps) | Settings parsing, needs JSON files |

### classes/QualityGates/AddOn/ (3)

| Library | Priority | Notes |
|---------|----------|-------|
| `classes/QualityGates/AddOn/FileSys/QgAddOnResultsDir.ctl` | Low | Directory handling |
| `classes/QualityGates/AddOn/Output/QgAddOnResult.ctl` | Medium | Output result |
| `classes/QualityGates/AddOn/Output/QgAddOnScore.ctl` | Medium | Scoring logic |

### classes/QualityGates/QgCtrlppCheck/ (1)

| Library | Priority | Notes |
|---------|----------|-------|
| `classes/QualityGates/QgCtrlppCheck/QgCtrlppCheck.ctl` | Medium | CtrlppCheck wrapper |

### classes/QualityGates/QgOverloadedFilesCheck/ (1)

| Library | Priority | Notes |
|---------|----------|-------|
| `classes/QualityGates/QgOverloadedFilesCheck/QgOverloadedFilesCheck.ctl` | Medium | Overloaded files check |

### classes/QualityGates/QgStaticCheck/ (2)

| Library | Priority | Notes |
|---------|----------|-------|
| `classes/QualityGates/QgStaticCheck/StaticCodeDir.ctl` | Medium | Directory scanning |
| `classes/QualityGates/QgStaticCheck/StaticDir.ctl` | Medium | Directory scanning |

### classes/QualityGates/QgStaticCheck/CtrlCode/ (4)

| Library | Priority | Notes |
|---------|----------|-------|
| `classes/QualityGates/QgStaticCheck/CtrlCode/FunctionData.ctl` | High (⚠️ deps) | Function metrics - has CtrlPv2Admin dep |
| `classes/QualityGates/QgStaticCheck/CtrlCode/QgCtrlCodeScriptData.ctl` | High | Script metrics, testable |
| `classes/QualityGates/QgStaticCheck/CtrlCode/ScriptFile.ctl` | Medium | Script file handling |
| `classes/QualityGates/QgStaticCheck/CtrlCode/ScriptsDir.ctl` | Medium | Directory scanning |

### classes/QualityGates/QgStaticCheck/Panels/ (2)

| Library | Priority | Notes |
|---------|----------|-------|
| `classes/QualityGates/QgStaticCheck/Panels/PanelCheck.ctl` | Medium | Panel validation |
| `classes/QualityGates/QgStaticCheck/Panels/PanelsDir.ctl` | Medium | Directory scanning |

### classes/QualityGates/QgStaticCheck/Panels/PanelFile/ (3)

| Library | Priority | Notes |
|---------|----------|-------|
| `classes/QualityGates/QgStaticCheck/Panels/PanelFile/PanelFile.ctl` | Medium | Panel file parsing |
| `classes/QualityGates/QgStaticCheck/Panels/PanelFile/PanelFileScript.ctl` | Medium | Panel script extraction |
| `classes/QualityGates/QgStaticCheck/Panels/PanelFile/PanelFileShape.ctl` | Medium | Shape analysis |

### classes/QualityGates/QgStaticCheck/Pictures/ (2)

| Library | Priority | Notes |
|---------|----------|-------|
| `classes/QualityGates/QgStaticCheck/Pictures/PicturesDir.ctl` | Low | Directory scanning |
| `classes/QualityGates/QgStaticCheck/Pictures/PicturesFile.ctl` | Low | Picture file handling |

### classes/QualityGates/QgSyntaxCheck/ (1)

| Library | Priority | Notes |
|---------|----------|-------|
| `classes/QualityGates/QgSyntaxCheck/QgSyntaxCheck.ctl` | Medium | Syntax check wrapper |

### classes/QualityGates/Tools/CppCheck/ (1)

| Library | Priority | Notes |
|---------|----------|-------|
| `classes/QualityGates/Tools/CppCheck/CppCheck.ctl` | High | CppCheck integration, testable |

### classes/QualityGates/Tools/Lizard/ (1)

| Library | Priority | Notes |
|---------|----------|-------|
| `classes/QualityGates/Tools/Lizard/ToolLizard.ctl` | Medium | Lizard integration |

### classes/QualityGates/Tools/OaSyntaxCheck/ (1)

| Library | Priority | Notes |
|---------|----------|-------|
| `classes/QualityGates/Tools/OaSyntaxCheck/OaSyntaxCheck.ctl` | Medium | OA syntax check |

### classes/QualityGates/Tools/Python/ (1)

| Library | Priority | Notes |
|---------|----------|-------|
| `classes/QualityGates/Tools/Python/Python.ctl` | Low | Python path detection |

---

## Recommended Test Order (by priority)

### High Priority (Pure logic, easy to test)

1. `classes/QualityGates/QgSettings.ctl` (⚠️ needs JSON file access)
2. `classes/QualityGates/QgStaticCheck/CtrlCode/QgCtrlCodeScriptData.ctl`

> **✅ Completed:** CppCheckError.ctl, CppCheckSettings.ctl, QgAddOnResultErr.ctl, QgTest.ctl, QgVersionResult.ctl

> **⚠️ Note:** FunctionData.ctl has CtrlPv2Admin dependency - not testable with PACK_SEL: 2
> **⚠️ Note:** CppCheck.ctl has panel/CtrlXml/CtrlPv2Admin dependencies - not testable

### Medium Priority

1. `classes/QualityGates/QgBase.ctl`
2. `classes/QualityGates/QgMsgCat.ctl`
3. `classes/QualityGates/AddOn/Output/QgAddOnResult.ctl`
4. `classes/QualityGates/AddOn/Output/QgAddOnScore.ctl`
5. `classes/QualityGates/QgCtrlppCheck/QgCtrlppCheck.ctl`
6. `classes/QualityGates/QgOverloadedFilesCheck/QgOverloadedFilesCheck.ctl`
7. `classes/QualityGates/QgStaticCheck/StaticCodeDir.ctl`
8. `classes/QualityGates/QgStaticCheck/StaticDir.ctl`
9. `classes/QualityGates/QgStaticCheck/CtrlCode/ScriptFile.ctl`
10. `classes/QualityGates/QgStaticCheck/CtrlCode/ScriptsDir.ctl`
11. `classes/QualityGates/QgStaticCheck/Panels/PanelCheck.ctl`
12. `classes/QualityGates/QgStaticCheck/Panels/PanelsDir.ctl`
13. `classes/QualityGates/QgStaticCheck/Panels/PanelFile/PanelFile.ctl`
14. `classes/QualityGates/QgStaticCheck/Panels/PanelFile/PanelFileScript.ctl`
15. `classes/QualityGates/QgStaticCheck/Panels/PanelFile/PanelFileShape.ctl`
16. `classes/QualityGates/QgSyntaxCheck/QgSyntaxCheck.ctl`
17. `classes/QualityGates/Tools/Lizard/ToolLizard.ctl`
18. `classes/QualityGates/Tools/OaSyntaxCheck/OaSyntaxCheck.ctl`

### Low Priority (UI/external dependencies)

1. `classes/QualityGates/QgResultPublisher.ctl`
2. `classes/QualityGates/AddOn/FileSys/QgAddOnResultsDir.ctl`
3. `classes/QualityGates/QgStaticCheck/Pictures/PicturesDir.ctl`
4. `classes/QualityGates/QgStaticCheck/Pictures/PicturesFile.ctl`
5. `classes/QualityGates/Tools/Python/Python.ctl`
6. `gedi/qualityCheck_ext.ctl`
7. `scriptEditor/ctrlPPCheck_ext.ctl`

---

## Notes

- **PACK_SEL: 2** → No database access (`dp*()` functions unavailable)
- Tests must be registered in `testProj.unit.config` under `TEST_MANAGERS`
- Use `_throw()` mocking pattern for code that calls `fatal()`/`throwError()`
- See [createUnitTests-promt.md](createUnitTests-promt.md) for test creation guide
