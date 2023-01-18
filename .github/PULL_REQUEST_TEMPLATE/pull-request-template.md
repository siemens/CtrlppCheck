---
name: Pull request template
about: Describe this issue template's purpose here.
title: ''
labels: ''
assignees: ''

---

# Request for a change on public repository on [CtrlppCheck](https://github.com/siemens/CtrlppCheck)

## Basic information
<!-- Comment:
A great PR typically begins with the line below.
-->


<!-- replace XXXXX with the numeric part of the issue ID you created in GitHub -->
See #XXXXX
<!-- in case this PR solves Github issue use close #### or closes, closed, fix, fixes, fixed, resolve, resolves, resolved -->

<!-- Comment:
If the issue is not fully described in Github, add more information here (justification, pull request links, etc.).

 * We do not require Github issues for minor improvements.
 * Bug fixes should have a Github issue to facilitate the backporting process.
 * Major new features should have a Github issue.
-->


## Technical information


### Testing done

<!-- Comment:
Provide a clear description of how this change was tested.
At minimum this should include proof that a computer has executed the changed lines.
Ideally this should include an automated test or an explanation as to why this change has no tests.
Note that automated test coverage is less than complete, so a successful PR build does not necessarily imply that a computer has executed the changed lines.
If automated test coverage does not exist for the lines you are changing, **you must describe** the scenario(s) in which you manually tested the change.
For frontend changes, include screenshots of the relevant page(s) before and after the change.
For refactoring and code cleanup changes, exercise the code before and after the change and verify the behavior remains the same.
-->

### Proposed upgrade guidelines

N/A

### Localizations

<!-- Comment:
+ Be sure any localization files are moved to /msg/ files.
+ Please describe here which language has been translated by you.
+ English text's are mandatory for new entries.
-->

- [ ] English
- [ ] German

### Submitter checklist

- [ ] The Github issue, if it exists, is well-described.
- [ ] The changelog entries and upgrade guidelines are appropriate for the audience affected by the change (users or developers, depending on the change) and are in the imperative mood.
  - The changelog generator for plugins uses the **pull request title as the changelog entry**.
  - Fill in the **Proposed upgrade guidelines** section only if there are breaking changes or changes that may require extra steps from users during the upgrade.
- [ ] There is automated testing or an explanation that explains why this change has no tests.
- [ ] For dependency updates, there are links to external changelogs and, if possible, full differentials.
- [ ] Any localizations are transferred to /msg/ files.
<!-- TBD
- [ ] Changes in the interface are documented also as [examples](docs/examples/readme.md).
-->

### Maintainer checklist

Before the changes are marked as `ready-for-merge`:

<!-- TBD, maybe done in GitHub rules
- [ ] There is at least one (1) approval for the pull request and no outstanding requests for change.
- [ ] Conversations in the pull request are over, or it is explicit that a reviewer is not blocking the change.
-->
- [ ] Changelog entries in the **pull request title** and/or **Proposed changelog entries** are accurate, human-readable, and in the imperative mood.
<!-- TBD, maybe done in GitHub rules
- [ ] Proper changelog labels are set so that the changelog can be generated automatically. See also [release-drafter-labels](...).
-->
- [ ] If the change needs additional upgrade steps from users, the `upgrade-guide-needed` label is set and there is a **Proposed upgrade guidelines** section in the pull request title (see [example](...)).
- [ ] C++ and Control(++) code changes are tested by automated test.
- [ ] WinCC OA guidelines for C++ and Control(++) coding have been met.
- [ ] Result of pipeline build proving error/warning free code.
- [ ] Result of automatic tests proving regression free.
