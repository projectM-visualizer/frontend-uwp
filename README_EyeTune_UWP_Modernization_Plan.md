# EyeTune UWP Modernization --- Project Planning

**Project:** EyeTune / projectM UWP frontend modernization\
**Repository:**
[projectM-visualizer/frontend-uwp](https://github.com/projectM-visualizer/frontend-uwp)\
**Contribution model:** Work in a local clone, push feature branches,
and submit pull requests upstream\
**Application platform:** Universal Windows Platform (UWP)\
**Planning status:** Initial architecture and contribution plan;
technical feasibility items remain to be verified

------------------------------------------------------------------------

## 1. Project objective

Modernize the existing EyeTune UWP frontend in its original upstream
repository. Preserve the application's identity, UWP platform target,
and useful historical functionality while incrementally improving its
build system, ProjectM integration, rendering, audio handling, testing,
and CI.

This is an upstream contribution effort---not a new standalone
application or a replacement repository.

The developer will clone the upstream repository, make changes on local
feature branches, commit and push those branches, and open pull requests
for maintainer review.

### Non-goals

-   Do not migrate the application to WinUI 3.
-   Do not create a separate `EyeTune.Next` repository as the primary
    implementation.
-   Do not commit directly to the upstream default branch.
-   Do not replace the original application identity or branding without
    maintainer agreement.
-   Do not claim Xbox compatibility until it has been demonstrated on
    the intended platform.

## 2. Upstream repository and contribution context

Repository: <https://github.com/projectM-visualizer/frontend-uwp>

Treat this repository's source, README, issues, pull requests, tags, and
complete Git history as the primary historical evidence for EyeTune.
Before implementing changes, inspect the repository and identify the
current state of its build system, UWP project, native dependencies,
ProjectM integration, rendering path, and audio pipeline.

The historical project documentation has identified modernization
opportunities including:

-   Replacing manually maintained Visual Studio project/dependency
    arrangements with a reproducible CMake-based native build where
    appropriate.
-   Updating integration to the supported ProjectM C API.
-   Adding automated GitHub Actions build and validation workflows.

These are investigation and planning targets, not assumptions that
should be implemented without first checking the current repository and
upstream ProjectM version.

Preserve existing repository history. Prefer small, independently
reviewable pull requests over a wholesale rewrite.

## 3. Product and platform direction

### UWP is a firm requirement

The application remains a UWP application. Use the UWP application model
and the XAML framework already appropriate to the repository. Do not
migrate to WinUI 3.

The intended targets are:

1.  Windows UWP desktop.
2.  Xbox One UWP, subject to platform and hardware validation.
3.  Xbox Series X/S compatibility as a forward-compatibility objective,
    subject to validation on actual hardware or supported test
    infrastructure.

Confirm current Microsoft platform requirements before selecting target
SDKs, architectures, packaging settings, or Store distribution strategy.
In particular, verify Xbox-specific UWP restrictions and current
publishing eligibility rather than relying on historical assumptions.

### Architecture principles

-   Keep the UWP presentation layer separate from the native
    visualization engine.
-   Keep ProjectM's reusable audio/preset processing separate from
    platform-specific rendering.
-   Isolate platform-specific audio acquisition behind interfaces.
-   Isolate graphics API dependencies behind a rendering abstraction.
-   Keep native dependencies pinned and reproducibly buildable.
-   Preserve existing behavior unless a change is documented, tested,
    and justified.
-   Make Xbox support an explicit feasibility and validation
    requirement.

## 4. Proposed high-level architecture

``` text
                 EyeTune UWP Application
                   C# / XAML / UWP
                           |
                           v
                 EyeTune Application Core
          Navigation | Settings | Presets | Lifecycle
                           |
                           v
                  Native C++ Integration
                           |
             +-------------+-------------+
             |                           |
             v                           v
       Audio abstraction          ProjectM core adapter
             |                           |
      Platform backend             PCM / preset state
             |                           |
             +-------------+-------------+
                           |
                           v
                 Rendering abstraction
                           |
              +------------+------------+
              |                         |
              v                         v
        Windows renderer          Xbox renderer
        feasibility path           feasibility path
```

This is a target architecture, not a claim that every component already
exists or that the current ProjectM renderer can run unchanged on Xbox.

### Application layer

-   UWP application lifecycle and navigation.
-   XAML user interface.
-   Settings and configuration.
-   Preset selection and management.
-   Audio-source selection where supported by the platform.
-   Device, suspension, and resume handling.

### Native integration layer

-   C++ adapter around the supported ProjectM API.
-   Stable, narrow interop boundary between managed and native code.
-   Audio sample format conversion and buffering.
-   Preset loading and state management.
-   Rendering coordination.

Avoid exposing ProjectM C++ implementation classes directly across the
C#/native boundary. Prefer a small, versioned C-compatible interface
where practical, adapted to the actual API and UWP constraints.

### ProjectM core

Investigate the current supported ProjectM release and its C API. Keep
the ProjectM dependency pinned to a known revision or release and
document the exact integration contract.

Do not assume that ProjectM's current OpenGL renderer can be used
unchanged in every UWP or Xbox environment.

### Rendering

The historical EyeTune implementation used an OpenGL ES-oriented
rendering approach for its original UWP targets. Modernization must
first establish what graphics APIs and context creation mechanisms are
available on each target.

Investigate: - Existing OpenGL ES integration and its dependencies. -
Whether the selected ProjectM version supports the required rendering
path. - Windows UWP graphics capabilities. - Xbox graphics API
requirements and restrictions. - Whether a Direct3D renderer, renderer
adaptation, or supported translation layer is technically feasible. -
GPU resource lifecycle, device loss, resize, fullscreen, and frame
pacing.

Do not begin a large renderer rewrite until a minimal rendering proof of
concept has been built and tested.

## 5. Audio architecture and feasibility

Audio capture must be platform-specific and abstracted from ProjectM.

### Windows desktop

Investigate supported UWP audio APIs and capabilities. Do not assume
desktop WASAPI loopback capture is available to a UWP application simply
because it is available to a traditional desktop application.

The desired desktop experience is visualization of audio from other
applications, including browsers and media players. Verify whether this
is possible under the selected UWP execution and capability model. If it
is not, document the limitation and identify supported alternatives.

### Xbox

Do not assume Xbox applications can capture another application's audio
mix or use desktop WASAPI loopback. Determine what audio data an Xbox
UWP application can legally and technically access.

A realistic fallback may be visualizing audio that EyeTune itself plays
or receives through a supported input path. This must be tested and
described accurately.

### Audio pipeline design

Where a supported source exists, use a pipeline conceptually similar to:

``` text
Platform audio source
        |
        v
Capture / input adapter
        |
        v
PCM normalization and conversion
        |
        v
Bounded audio buffer
        |
        v
ProjectM PCM input
        |
        v
Preset evaluation and visualization
```

Requirements:

-   Do not render inside an audio capture callback.
-   Avoid blocking work in real-time audio callbacks.
-   Handle format, sample-rate, and channel-count changes.
-   Handle silence, underruns, overruns, and device interruptions.
-   Keep audio acquisition independent from rendering.
-   Validate threading and buffer ownership.
-   Document platform limitations and required capabilities.

## 6. EyeTune functionality to preserve and investigate

Use the original source and Git history to determine what EyeTune
actually implemented. Do not attribute unverified features to the
original application.

Create an inventory covering:

-   Application startup and navigation.
-   Original visualizer experience.
-   Renderer initialization and shutdown.
-   Audio acquisition and PCM delivery.
-   Preset enumeration, selection, and transitions.
-   Settings and persisted preferences.
-   Fullscreen and display behavior.
-   Device lifecycle and suspension/resume.
-   Existing Xbox or controller-related behavior, if present.
-   Existing limitations and known bugs.

Potential improvements, subject to upstream scope and review:

-   User-imported `.milk` presets.
-   Favorites and preset playlists.
-   Search and categorization.
-   Configurable preset duration and transitions.
-   Better audio-source selection where supported.
-   Gamepad navigation for Xbox.
-   Improved error handling and performance diagnostics.

Preset and texture redistribution must be audited separately. ProjectM
licensing does not automatically grant redistribution rights for every
preset or texture collection.

## 7. Repository layout for planning and implementation

Keep the existing source layout where possible. Add documentation and
specs without needlessly reorganizing working code.

Suggested additions:

``` text
frontend-uwp/
├── .github/
│   └── workflows/
├── docs/
│   ├── architecture/
│   ├── historical/
│   └── decisions/
├── specs/
│   ├── 001-repository-assessment/
│   ├── 002-cmake-modernization/
│   ├── 003-projectm-api-migration/
│   ├── 004-rendering-modernization/
│   ├── 005-audio-modernization/
│   ├── 006-uwp-feature-restoration/
│   └── 007-ci-cd/
├── [existing source and project files]
├── [existing license files]
└── README.md
```

The exact location of the spec-kit constitution and specification files
should follow the installed spec-kit version and its conventions. Do not
overwrite existing repository files without inspecting them.

## 8. Spec-kit implementation plan

Use GitHub spec-kit to manage the work as a sequence of small, testable
specifications. Use Claude Code as the implementation agent, with human
review of all changes.

Recommended specification sequence:

  -----------------------------------------------------------------------
  ID                      Specification           Main outcome
  ----------------------- ----------------------- -----------------------
  001                     Historical repository   Evidence-based
                          assessment              inventory and baseline

  002                     CMake modernization     Reproducible native
                                                  build

  003                     ProjectM API migration  Supported ProjectM
                                                  integration

  004                     Rendering modernization Validated graphics
                                                  integration

  005                     Audio modernization     Supported platform
                                                  audio pipeline

  006                     UWP feature restoration Presets, settings, UI,
                                                  and lifecycle

  007                     CI/CD                   Automated build and
                                                  pull-request validation
  -----------------------------------------------------------------------

Suggested spec-kit command flow (confirm command names against the
installed version):

``` text
/speckit.constitution
/speckit.specify
/speckit.clarify
/speckit.plan
/speckit.checklist
/speckit.tasks
/speckit.analyze
/speckit.implement
```

For each specification:

1.  Inspect the existing implementation and relevant Git history.
2.  Write requirements and explicit acceptance criteria.
3.  Identify platform, licensing, and dependency constraints.
4.  Plan the smallest safe implementation.
5.  Generate tasks with tests and documentation.
6.  Analyze for inconsistencies and missing requirements.
7.  Implement incrementally.
8.  Build and run available tests.
9.  Review the complete diff for unintended changes.
10. Commit only the scoped work.

Claude Code must not invent platform APIs, claim unexecuted tests
passed, silently change application identity, or make broad unrelated
refactors. Require evidence for Xbox and UWP compatibility claims.

## 9. Git workflow and pull-request strategy

Clone the upstream repository and work locally:

``` bash
git clone https://github.com/projectM-visualizer/frontend-uwp.git
cd frontend-uwp

git remote -v
git status
git log --oneline --decorate --all
```

Create a branch for the first assessment:

``` bash
git switch -c 001-repository-assessment
```

Use separate branches for subsequent contributions. Suggested names:

``` text
001-repository-assessment
002-cmake-modernization
003-projectm-api-migration
004-rendering-modernization
005-audio-modernization
006-uwp-feature-restoration
007-ci-cd
```

Before creating a branch, check the upstream default branch and current
contribution conventions. Keep each branch focused. If a later change
depends on an unmerged PR, clearly identify that dependency and use an
appropriate stacked-PR strategy.

Push only the feature branch:

``` bash
git add docs specs
git commit -m "docs: assess historical EyeTune architecture"

git push -u origin 001-repository-assessment
```

Then open a pull request against the upstream repository's default
branch.

Never force-push or modify upstream branches without explicit
authorization and maintainer agreement.

### Pull-request expectations

Every PR should include:

-   Purpose and scope.
-   Technical approach.
-   Relevant specification and acceptance criteria.
-   Build and test commands executed.
-   Actual test results.
-   Supported and unsupported target platforms.
-   Screenshots or logs where useful.
-   Licensing and third-party dependency implications.
-   Known limitations and follow-up work.

Avoid combining a large native dependency migration, UI redesign,
renderer rewrite, and audio redesign into one PR.

## 10. Phased delivery plan

### Phase 0 --- Historical reconstruction and feasibility

Deliverable: `docs/historical/eyetune-reconstruction.md`

-   Inspect all repository files and Git history.
-   Identify original application architecture and functionality.
-   Map current ProjectM dependencies and API usage.
-   Document the existing audio and rendering paths.
-   Identify build-system and CI problems.
-   Establish current UWP and Xbox constraints.
-   Create a dependency and license inventory.
-   Produce a risk register and proposed PR sequence.

**Exit criterion:** Maintainers and contributors can understand the
existing implementation and agree on the modernization direction.

### Phase 1 --- Build-system modernization

-   Establish a reproducible native build.
-   Introduce or improve CMake where appropriate.
-   Pin dependency versions.
-   Preserve UWP project compatibility.
-   Document local build instructions.
-   Add build validation where practical.

**Exit criterion:** A clean checkout can be built using documented steps
on the supported development environment.

### Phase 2 --- ProjectM API modernization

-   Select and pin a compatible ProjectM release.
-   Build the native dependency for required UWP architectures.
-   Integrate the supported C API.
-   Validate initialization, PCM delivery, preset loading, rendering
    calls, and shutdown.
-   Document native interop and ownership rules.

**Exit criterion:** A minimal UWP proof of concept uses the selected
ProjectM API reliably.

### Phase 3 --- Rendering feasibility and modernization

-   Reproduce the existing rendering path.
-   Validate the selected ProjectM renderer on Windows UWP.
-   Investigate Xbox graphics constraints.
-   Build a minimal Xbox rendering proof of concept if possible.
-   Determine whether a separate renderer implementation is required.
-   Measure frame rate, memory use, and stability.

**Exit criterion:** Rendering support and limitations are demonstrated
for each claimed target.

### Phase 4 --- Audio modernization

-   Inventory existing audio acquisition.
-   Validate supported Windows UWP audio sources.
-   Investigate Xbox audio input capabilities.
-   Implement platform adapters and PCM normalization.
-   Test buffering, threading, silence, and device lifecycle.

**Exit criterion:** Each advertised audio mode is demonstrated on its
target platform.

### Phase 5 --- EyeTune functionality and UX

-   Preserve existing user workflows.
-   Improve preset handling where within scope.
-   Add user preset import only after file-access and packaging behavior
    are verified.
-   Validate navigation and controller support on Xbox.
-   Test suspension, resume, and display lifecycle.

**Exit criterion:** Core EyeTune workflows work on the supported
targets.

### Phase 6 --- CI, packaging, and release readiness

-   Add GitHub Actions for appropriate build and test jobs.
-   Validate package architecture and identity.
-   Verify signing and deployment instructions.
-   Document Developer Mode testing where applicable.
-   Audit licenses and redistribution rights.
-   Prepare maintainer-facing release notes.

**Exit criterion:** Pull requests receive repeatable automated
validation and packages can be produced using documented procedures.

## 11. Risks and validation gates

  -----------------------------------------------------------------------
  Risk                    Why it matters          Required response
  ----------------------- ----------------------- -----------------------
  UWP audio restrictions  Desktop loopback        Build a
                          assumptions may not     platform-specific audio
                          apply                   proof of concept

  Xbox audio restrictions Cross-application       Define and test
                          capture may not be      supported Xbox audio
                          available               sources

  OpenGL compatibility    Existing ProjectM       Validate graphics path
                          renderer may not run on before renderer
                          Xbox                    redesign

  Native dependency       ProjectM and            Verify architectures
  support                 dependencies may not    and toolchain early
                          build for all UWP       
                          targets                 

  Xbox packaging and      Developer Mode and      Verify current
  publishing              Store distribution have Microsoft rules
                          distinct requirements   

  Preset and texture      Assets may have         Audit each asset
  licensing               separate redistribution collection
                          terms                   

  Large PR scope          Maintainers may find    Submit incremental,
                          broad rewrites          independently testable
                          difficult to review     PRs
  -----------------------------------------------------------------------

A successful Windows desktop build is not evidence of Xbox
compatibility. A successful Developer Mode deployment is not
automatically evidence of Store submission eligibility.

## 12. First milestone: historical reconstruction PR

The first contribution should be documentation and assessment, not a
major source-code rewrite.

Create:

`docs/historical/eyetune-reconstruction.md`

Include:

1.  Repository and commit-history overview.
2.  Original EyeTune architecture.
3.  Existing UWP project and application identity.
4.  Native dependencies and ProjectM API usage.
5.  Audio acquisition and PCM pipeline.
6.  Rendering initialization and graphics dependencies.
7.  Preset and asset handling.
8.  Build and CI status.
9.  Windows and Xbox feasibility findings.
10. Licensing and redistribution considerations.
11. Proposed migration sequence.
12. Open questions for maintainers.

The assessment must distinguish verified facts from hypotheses and
unanswered questions.

### Initial acceptance criteria

-   No application source behavior is changed in the assessment PR.
-   Historical claims cite repository files, commits, or issues.
-   Build instructions have been tested or clearly marked unverified.
-   Platform constraints are supported by current documentation or a
    reproducible experiment.
-   Open questions are explicitly listed.
-   The PR is small enough for maintainers to review comfortably.

------------------------------------------------------------------------

## 13. Working principles

-   The upstream repository is the project home.
-   UWP remains the application platform.
-   EyeTune's history and identity are respected.
-   The native engine stays decoupled from the UI.
-   Platform-specific capabilities are verified rather than assumed.
-   All changes are made on feature branches.
-   Pull requests are the unit of upstream contribution.
-   Human review and maintainer approval remain mandatory.
-   Tests and compatibility claims must reflect actual execution.
-   Preserve existing licensing and third-party notices.

## 14. Reference links

-   Original frontend repository:
    https://github.com/projectM-visualizer/frontend-uwp
-   projectM organization: https://github.com/projectM-visualizer
-   ProjectM core: https://github.com/projectM-visualizer/projectm
-   GitHub spec-kit: https://github.com/github/spec-kit
-   Microsoft UWP documentation:
    https://learn.microsoft.com/windows/uwp/
-   Microsoft Xbox Developer Mode documentation:
    https://learn.microsoft.com/windows/uwp/xbox-apps/devkit-activation

------------------------------------------------------------------------

**Planning note:** This document defines the intended contribution
strategy and architecture. It is not a claim that the proposed APIs,
graphics paths, audio modes, build configurations, or Xbox targets have
already been validated. Phase 0 must verify the current repository and
platform requirements before implementation decisions are finalized.
