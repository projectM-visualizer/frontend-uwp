# projectM-UWP design

How projectM-UWP is built, why, what has been verified, and what's still open. This replaces the earlier `docs/` and `specs/` folders and the modernization plan. Findings are **Verified** when they come from files, commits or a run on real hardware.

## Contents
- [Status](#status)
- [Architecture](#architecture)
- [Decisions](#decisions)
- [Platform feasibility](#platform-feasibility)
- [Testing](#testing)
- [Licensing](#licensing)
- [History](#history)
- [Specifications](#specifications)

## Status

Each stage of the modernization has a specification with acceptance criteria, listed under [Specifications](#specifications). They follow GitHub spec-kit's structure, but were written by hand.

**Done** means it was built and shown to work. **Partly** means it's built but not fully verified. **Not done** means it's still open.

| Spec | Done | Partly | Not done |
|---|---|---|---|
| [001 Historical repository assessment](#001-historical-repository-assessment) | 3 | 0 | 0 |
| [002 CMake modernization](#002-cmake-modernization) | 3 | 0 | 0 |
| [003 projectM API migration](#003-projectm-api-migration) | 3 | 0 | 0 |
| [004 Rendering modernization](#004-rendering-modernization) | 2 | 2 | 1 |
| [005 Audio modernization](#005-audio-modernization) | 4 | 2 | 1 |
| [006 UWP feature restoration](#006-uwp-feature-restoration) | 7 | 0 | 2 |
| [007 CI and release readiness](#007-ci-and-release-readiness) | 4 | 0 | 2 |

## Architecture

projectM-UWP is a native UWP app. It stays C++/CX with a `CoreWindow` and doesn't use XAML or C# ([decision 1](#1-keep-the-native-corewindow-app)).

```text
                 App (projectM-UWPApp.cpp)
       Window | Lifecycle | Input | Settings | Log
              |              |              |
              v              v              v
      AngleSurface      Visualizer     WASAPICapture
      EGL + ANGLE       projectM        WASAPI
              |          C API              |
              v              ^              v
          Direct3D 11        +------ AudioRingBuffer
```

### Layers

| Layer | Files | Rule |
|---|---|---|
| App shell | `projectM-UWPApp`, `WASAPICaptureScenario` | Knows about Windows. Doesn't call projectM or EGL directly, except for passing `eglGetProcAddress` to projectM |
| Engine | `Visualizer` | The only code that calls projectM |
| Surface | `AngleSurface`, behind `IRenderSurface` | The only code that creates EGL objects |
| Audio source | `WASAPICapture`, behind `IAudioSink` | Only delivers samples. It knows nothing about projectM |
| Core | `src/projectM-UWP/projectM-UWP.Core` | No platform dependencies, and unit tested |

Adding a platform means providing a new surface and audio source. `Visualizer` and Core don't change.

### Audio flow

```text
Output or microphone
  -> WASAPICapture, on the capture thread
  -> AudioRingBuffer, which converts to stereo
  -> Visualizer::RenderFrame, on the render thread
  -> projectm_pcm_add_int16
```

- Nothing renders in the capture callback, and it never blocks.
- The buffer holds about one second. When full, new frames are dropped and counted, because stalling would disturb the audio engine.
- Mono is copied to both sides. Surround keeps front left and right.
- Silence produces no packets in loopback mode, so projectM just gets no new audio.
- Capture restarts when the default device changes or is lost, and stops during suspend.

### Threads

| Thread | Work |
|---|---|
| UI and render | Events, drawing, starting capture, all projectM and EGL calls |
| Audio work queue | `OnSampleReady`, writing to the buffer |
| Audio callbacks | State changes, which only set flags |

`ActivateAudioInterfaceAsync` has to start on the UI thread, so callbacks set atomic flags and `ServiceCapture` acts on them once per frame.

### Ownership

| Resource | Owner | Rule |
|---|---|---|
| `projectm_handle`, `projectm_playlist_handle` | `Visualizer` | Destroy the playlist first, while the GL context is current |
| Strings from the playlist | `Visualizer` | Free with the matching `projectm_playlist_free_*` |
| `AudioRingBuffer` | `shared_ptr` held by `App` and `WASAPICapture` | Outlives the renderer, so a late sample is safe |
| EGL objects | `AngleSurface` | Clean up after `Visualizer::Destroy` |

### Device loss

When `eglSwapBuffers` fails, `App` destroys the visualizer, rebuilds the surface and creates the visualizer again. Audio keeps running.

### Build

CMake builds projectM from the unmodified submodule, then applies two replacements from outside it:

- `DynamicLibrary_uwp.cpp` replaces `DynamicLibrary_win32.cpp`, which calls APIs that UWP doesn't have.
- `uwp_compat.h` supplies `getenv`, which UWP doesn't have.

It also compiles a corrected copy of `GLProbe.cpp`. Upstream calls `glGetStringi` with the wrong calling convention, which crashes x86 Release builds. The copy stops being used once upstream's text changes.

### Data on disk

These live in the app's `LocalState` folder.

| Item | Purpose |
|---|---|
| `settings.txt` | Preferences and favorites |
| `presets\` | Imported presets |
| `projectM-UWP.log` | Frame rate, audio level, memory and errors, written every 10 seconds |

## Decisions

### 1. Keep the native CoreWindow app

#### Context

projectM-UWP has always been C++/CX on a CoreWindow, with no XAML. A C# and XAML shell, or WinUI 3, could provide a richer interface.

#### Decision

Keep the native app. UWP remains the platform, and WinUI 3 is out of scope.

#### Consequences

There's no XAML interface, so features use keys and message dialogs. Search and on-screen preset browsing aren't implemented. Rewriting the shell would risk working behaviour with no benefit to rendering or audio.

### 2. Keep OpenGL ES through ANGLE

#### Context

projectM only renders through OpenGL. UWP doesn't provide OpenGL, so ANGLE translates OpenGL ES to Direct3D 11.

#### Decision

Keep ANGLE, pinned to `WindowsStoreOpenGL` 0.777.79 by version and SHA-256 hash. Don't start a Direct3D renderer without a proof of concept showing that it's needed.

#### Consequences

The package dates from 2019 and has no ARM64 libraries. It has worked on the tested hardware.

### 3. Build with CMake and leave the submodule unmodified

#### Context

Hand-kept project files drifted from upstream, and UWP needs small changes to projectM's platform code.

#### Decision

Generate the project with CMake from `src/projectm`, and apply UWP changes from `CMakeLists.txt` without editing the submodule.

#### Consequences

Updating projectM is mostly moving the submodule. A patch that stops matching is skipped, which is safe for the `GLProbe.cpp` fix but needs a retest of x86.

### 4. Pass audio through a bounded buffer

#### Context

Capture called projectM directly from a real-time thread, which caused a teardown race and made capture depend on the renderer.

#### Decision

Capture writes to a lock-free, single-producer, single-consumer ring. The render thread drains it. When full, new frames are dropped and counted.

#### Consequences

Audio reaches projectM once per frame, which is what projectM samples at anyway. Capture and the renderer can restart independently.

### 5. Remove ARM builds

#### Context

The installed Windows SDK has no 32-bit ARM libraries, so ARM can't link. ARM64 would need different ANGLE libraries.

#### Decision

Support x64 and x86 only.

#### Consequences

Devices that need ARM aren't served. This can be revisited if an ANGLE build for ARM64 is adopted.

### 6. Track projectM's development branch

#### Context

The latest projectM release, v4.1.8, is on a maintenance branch. It lacks `projectm_create_with_opengl_load_proc` and the glad loader that the UWP port relies on, and it rejects OpenGL ES outside Linux and Android.

#### Decision

Pin the submodule to a tested commit of upstream `master`, currently `eae22de4` (4.2.0 in development).

#### Consequences

The code isn't a tagged release. Move to a release once 4.2.0 is published. Updates are deliberate and need the manual checks in [testing](#testing).

## Platform feasibility

What each platform is known to support. Only Windows desktop has been tested. A working Windows build says nothing about Xbox, and a Developer Mode install says nothing about Store acceptance.

### Summary

| Target | Status |
|---|---|
| Windows desktop x64 | **Tested.** Renders and reacts to audio |
| Windows desktop x86 | **Tested.** Renders and reacts to audio |
| Windows ARM32 | Removed. The installed SDK has no ARM32 libraries |
| Windows ARM64 | Not supported. The ANGLE package has no ARM64 libraries |
| Xbox One | **Untested.** Compatibility is not claimed |
| Xbox Series X\|S | **Untested.** Compatibility is not claimed |

### Windows audio

- `AUDCLNT_STREAMFLAGS_LOOPBACK` requires a shared-mode stream.
- Event-driven loopback needs Windows 10 version 1703 or later.
- DRM-protected streams may not be captured.
- Measured on Windows 11 as a UWP app in Developer Mode: a 220 Hz test tone at amplitude 2600 was read back at peak 2602, with no dropped frames.

Whether Store review accepts loopback capture is unknown. The manifest's `microphone` capability is declared, but Windows showed consent still at `Prompt` while loopback was working, so loopback doesn't appear to trigger the microphone prompt.

Source: [Loopback Recording](https://learn.microsoft.com/en-us/windows/win32/coreaudio/loopback-recording).

### Xbox

The repository's history shows an Xbox release existed (`5375229`, `196d0c4`), but it holds no evidence of how it behaved on a console.

Microsoft's guidance, from a page now in its [archived UWP documentation](https://learn.microsoft.com/en-us/previous-versions/windows/uwp/xbox-apps/system-resource-allocation):

| | Xbox One app | Xbox Series X\|S app |
|---|---|---|
| Foreground memory | 1 GB | 1 GB |
| CPU | Share of 2 to 4 cores | Share of 2 to 4 cores |
| GPU | Share of 45% | Share of 45% |
| DirectX | DirectX 11, feature level 10.1 | DirectX 11 and 12, feature level 11.0 |
| Architecture | x64 | x64 |

What that means for projectM-UWP:

- **Memory:** the app measured 135 to 212 MB on Windows, well within 1 GB.
- **Graphics:** ANGLE targets Direct3D 11, and feature level 10.1 is above the app's 10_0 request. Whether its shaders run acceptably on a share of the GPU is unknown. Xbox One's performance is the main risk.
- **Audio:** whether an app can capture other apps' audio on a console is unknown. The plan's expected fallback is visualizing audio that projectM-UWP itself plays, which the app doesn't have yet.
- **Input:** gamepad buttons are mapped, and have not been tested. Import, favorites and audio source keys have no gamepad mapping yet.
- **Import:** the file picker's behaviour on a console is untested.

Testing needs a console in Developer Mode. Microsoft's current rules for UWP on Xbox should be confirmed first, because that documentation has been archived.

### What would change for Xbox

The layers allow for it. A console audio source would write to `IAudioSink`, and a Direct3D renderer would sit behind `IRenderSurface`, though projectM itself only draws through OpenGL. No renderer rewrite should start before a proof of concept on real hardware.

## Testing

### Unit tests

`projectM-UWP.Core` has no platform dependencies, so its tests run on any desktop Visual Studio toolset:

```
cmake -S tests -B build/tests
cmake --build build/tests --config Release
ctest --test-dir build/tests -C Release --output-on-failure
```

They cover the audio buffer, including a threaded stress test, along with settings, favorites and frame statistics.

### Continuous integration

Every push to `master` and every pull request:

1. Runs the unit tests.
2. Builds Debug and Release for x64 and x86.
3. Builds an unsigned x86 and x64 bundle, kept for 14 days.

CI doesn't launch the app, so it can't detect rendering or audio faults, or startup crashes. The x86 crash fixed in `7c965e2` passed CI-style builds, so run the manual checks below before a release.

### Running a build

You need Developer Mode. Unsigned packages can't be installed normally, so register an unpacked package:

```powershell
Copy-Item build\x64\AppPackages\projectM-UWP_0.9.7.0_x64_Test\projectM-UWP_0.9.7.0_x64.appx build\x64\package.zip
Expand-Archive build\x64\package.zip build\x64\layout
Remove-Item build\x64\layout\AppxBlockMap.xml
Remove-Item -LiteralPath 'build\x64\layout\[Content_Types].xml'
Add-AppxPackage -Register build\x64\layout\AppxManifest.xml
```

Keep the folder until you uninstall, because the app runs from it. Debug packages also need the VCLibs Debug framework from `Dependencies` in the package folder.

### Manual checks

Run these on x64 and x86 before a release. Keys, import, settings and resume have been confirmed on x64 only. Microphone capture hasn't been confirmed, because the test session had no input device.

- [ ] Visuals appear and react to music, and stop reacting when it's paused
- [ ] `projectM-UWP.log` shows a non-zero `peak` while audio plays
- [ ] `F1` shows the current preset and help
- [ ] `N`, `P` and `R` change preset, and `Space` locks it
- [ ] `L` adds a favorite, and `V` plays favorites only
- [ ] `+` and `-` change the preset duration, and it's kept after a restart
- [ ] `I` imports a `.milk` file, and it appears in the playlist
- [ ] `M` switches to the microphone and back
- [ ] Changing the default output device keeps audio working
- [ ] Minimizing and restoring keeps audio working
- [ ] Resizing and `Ctrl+F` keep the picture correct
- [ ] A gamepad works, if one is available

### Measurements

Release builds on Windows 11, NVIDIA RTX A6000, over a remote display session running at 32 Hz. Measured from `projectM-UWP.log`.

| Measure | x64 | x86 |
|---|---|---|
| Frame rate | 32.0 fps, matching the display | 32.0 fps, matching the display |
| Memory | 135 to 212 MB | 149 to 205 MB |
| Audio frames in 10 seconds | About 442,000 | About 442,000 |
| Peak for a tone at amplitude 2600 | 2602 | 2602 |
| Dropped audio frames | 0 | 0 |

Frame rate on a local display hasn't been measured.

Over Remote Desktop, the only audio device is `Remote Audio`, which isn't always available when the app starts. The app retries, and there's no microphone unless audio input is redirected.

## Licensing

A record of what's in this repository and what its files say about licensing. This isn't legal advice, and it doesn't settle whether anything may be redistributed.

### Code

| Component | Used as | License | Source of this information |
|---|---|---|---|
| projectM-UWP | This repository | LGPL 2.1 | `LICENSE.md` |
| projectM | Submodule | LGPL 2.1 | `src/projectm/LICENSE.txt` |
| projectm-eval | Nested submodule | MIT | `vendor/projectm-eval/LICENSE.md` |
| hlslparser | Vendored by projectM | MIT | `vendor/hlslparser/LICENSE` |
| stb_image | Vendored by projectM | Public domain | Header of `vendor/stb_image/stb_image.h` |
| glad | Vendored by projectM | (WTFPL or CC0-1.0) and Apache-2.0 | SPDX tag in `vendor/glad/include/glad/gles2.h` |
| GLM | Vendored by projectM | **Not confirmed** | Check `vendor/glm` before release |
| ANGLE (`WindowsStoreOpenGL` 0.777.79) | Downloaded at configure time | **Not confirmed** | Check the package before release |
| WASAPI capture code | `WASAPICapture.*` | MIT, per its header, derived from Microsoft's sample | File headers |

LGPL 2.1 applies to how projectM is distributed with the app. Check that a Store release meets its terms, including letting users replace the library.

### Presets

**Status: unresolved. An audit is needed before a public release.**

- There are 29 presets in `src/projectM-UWP/presets/presets_projectM-UWP`.
- They came from projectM 3.1.12's `presets/presets_projectM-UWP` folder, added there in late 2019.
- None of the files contains license or copyright text.
- File names credit authors, including Geiss, Rovastar, Zylot, EoS, Stahlregen, martin and flexi. That shows attribution, not permission.
- projectM's repository license doesn't automatically cover every preset, and projectM no longer ships presets at all.

To resolve this, identify the rights holder of each preset, or replace it with one under a known license. No textures are bundled.

Imported presets belong to the user and stay on their device.

### Branding

The projectM-UWP name, publisher identity and Store association are kept unchanged. Changing them needs the owner's agreement.

## History

What this repository shows about how projectM-UWP was built, what changed during modernization, and what is still unknown. Findings are marked **Verified** (from files, commits or a run on real hardware) or **Unverified**.

### 1. Repository and history

- 86 commits, mostly by the original author: 70 by `mancoast`, 12 by `Coastrnaut`, 2 by Kai Blaschke, and one each by milkdropper and Mischa Spiegelmock. **Verified.**
- The history starts at `5375229` (2019-04-26), "EyeTune for Xbox and Windows Store".
- Store releases followed: `72cb0d9` (2019-05-03) "update / release new bundle to windows store" and `196d0c4` (2019-05-06) "update xbox app".
- The README and LGPL 2.1 license arrived in `e17b1bb` (2022-02-13). The call for help arrived in `11485e1` (2022-12-14).

### 2. Original architecture

projectM-UWP was a single native C++/CX app. There was no XAML and no C#. **Verified.**

`App` implemented `IFrameworkView` on a `CoreWindow`, created EGL through ANGLE, owned a `projectM*`, and rendered in its own loop. A second project built projectM into a DLL. Input came from `CoreWindow.KeyDown`, with gamepad buttons arriving as `VirtualKey` values.

### 3. UWP project and identity

Preserved from the original manifest and unchanged by modernization:

| Field | Value |
|---|---|
| Identity | `13183Mancoast.projectM-UWP` |
| Publisher | `CN=272C27CF-652F-4A95-9E3B-7A86D492CC37` |
| Display name | projectM, a name reserved on the Store account. It's shown in the title bar, Start menu and Store tile |
| Publisher name | Mancoast Research Institute |
| Version | 0.9.7.0 |
| Target family | `Windows.Universal` |
| Capabilities | `internetClient`, `microphone` |

### 4. Native dependencies and projectM API use

Originally: projectM 3.1.x compiled from a hand-maintained source list, using the C++ `projectM` class, plus NuGet packages `glm` and `WindowsStoreOpenGL`.

Now: projectM 4.2.0 development commit `eae22de4` from upstream, built by its own CMake, used only through the C and playlist APIs. See [the architecture overview](#architecture).

### 5. Audio pipeline

Originally, WASAPI loopback on the default render device, derived from Microsoft's capture sample. The sample's WAV writing and oscilloscope were commented out or unused, and samples went straight to projectM from the capture thread. **Verified.**

Defects found while modernizing:

| Defect | Effect | Fix |
|---|---|---|
| Samples read after `ReleaseBuffer` | Use of memory owned by the audio engine | Copied before release |
| projectM called from the capture thread | Could use a destroyed instance during teardown | Capture writes to a buffer that outlives the renderer |
| Stop never reached `Stopped`, because the state that finishes it was commented out | Capture couldn't be restarted cleanly | Stop now completes |
| More than 2 channels labelled as stereo | Garbled audio on surround devices | Front left and right are extracted |

### 6. Rendering

ANGLE on Direct3D 11, OpenGL ES 3 context, drawn with feature level fallback from 10_0+ to 9_3 to WARP. The port kept this path. It works on an NVIDIA RTX A6000 on Windows 11. **Verified.** Note that projectM 4 requires ES 3.0, so the 9_3 fallback can't actually run it.

### 7. Presets and assets

29 `.milk` presets, added upstream in October and November 2019 (`7cf01eb11`, `c58885a53`, `4bf395c4b`, in projectM's history). projectM removed all presets from its repository later, so they are now kept in `src/projectM-UWP/presets`. See [licensing](#licensing).

The "experimental" preset folder has never contained anything. It maps to a `presets_community` folder that isn't in projectM 3.1.12 or this repository. **Verified.**

### 8. Build and CI

Originally hand-written Visual Studio projects with no CI. Now CMake with presets, on the v142 toolset, with GitHub Actions. A clean clone builds from documented steps. **Verified.**

### 9. Windows and Xbox feasibility

See [platform feasibility](#platform-feasibility).

### 10. Licensing

See [licensing](#licensing).

### 11. Migration sequence

| Commit | Change |
|---|---|
| `86a584b` | projectM added as a submodule |
| `e0bdd20` | Built against projectM 4 and ported to the C API |
| `1bf0ea6` | First CI workflow |
| `2a9638e` | Project generated by CMake |
| `7c965e2` | x86 startup crash fixed |
| `83c2b9b` | Separated into audio, engine and surface layers |

### 12. Risk register

| Risk | Status | Response |
|---|---|---|
| UWP audio restrictions | Loopback works on Windows under Developer Mode. Store review is untested | Submit a build through the Store |
| Xbox audio | Unknown | Test on a console in Developer Mode |
| OpenGL on Xbox | Unknown. ANGLE targets Direct3D 11, which Xbox apps have | Test on a console |
| projectM bugs on x86 | One found and patched in `CMakeLists.txt` | Send the fix upstream |
| 2019 ANGLE package | No problem seen, but no maintained successor is in use | Replace if a preset or driver fails |
| Tracking an unreleased projectM commit | Accepted, see [decision 6](#6-track-projectms-development-branch) | Update deliberately and retest |
| Preset redistribution | Unresolved | Audit before a public release |
| No automated rendering test | CI compiles and packages only | Follow the manual checks in [testing](#testing) |

### Open questions

- Did the original Xbox release use loopback, and did it capture other apps' audio? The code suggests so, but nothing in the repository shows what happened on a console.
- Does the Store still accept updates to this listing, given that Microsoft has archived its UWP documentation?
- Who holds rights to the 29 presets?

## Specifications

### 001 Historical repository assessment

#### Goal

An evidence-based inventory of the original app.

#### Acceptance criteria

| Criterion | Status | Evidence |
|---|---|---|
| History, architecture and dependencies are documented with citations | Done | [History](#history) |
| Verified facts are separated from open questions | Done | Same document |
| A risk register exists | Done | Same document, section 12 |

### 002 CMake modernization

#### Goal

A reproducible native build.

#### Acceptance criteria

| Criterion | Status | Evidence |
|---|---|---|
| A clean checkout builds from documented steps | Done | Clean clone built all configurations |
| Dependencies are pinned | Done | Submodule commits and a hashed ANGLE download |
| The UWP project and identity are preserved | Done | Manifest unchanged |

### 003 projectM API migration

#### Goal

Use only the supported C API.

#### Acceptance criteria

| Criterion | Status | Evidence |
|---|---|---|
| The app uses only the C and playlist APIs | Done | All calls are in Visualizer |
| projectM is pinned to a known revision | Done | eae22de4 |
| Interop and ownership rules are documented | Done | [Architecture](#architecture) |

### 004 Rendering modernization

#### Goal

A validated graphics path behind an abstraction.

#### Acceptance criteria

| Criterion | Status | Evidence |
|---|---|---|
| Rendering is demonstrated on Windows x64 and x86 | Done | Confirmed by eye and by log |
| EGL is isolated behind a surface interface | Done | AngleSurface, IRenderSurface |
| Device loss recovery exists | Partly | Implemented, but not forced in a test |
| Frame rate and memory are measured | Partly | Measured on a 32 Hz remote display only |
| Rendering is demonstrated on Xbox | Not done | Needs a console |

### 005 Audio modernization

#### Goal

A supported audio pipeline, independent of rendering.

#### Acceptance criteria

| Criterion | Status | Evidence |
|---|---|---|
| No rendering or blocking work in the capture callback | Done | Capture only copies to the buffer |
| Bounded buffer with tested behaviour | Done | Unit tests include a threaded stress test |
| Channel counts are handled | Done | Mono, stereo and surround are tested in Core |
| Loopback is demonstrated on Windows | Done | Test tone measured at peak 2602 |
| Microphone source | Partly | Switching and failure recovery work on x64. Capture is unproven, because the test session had no input device |
| Device change and interruption handling | Partly | Restart and retry ran for real on x64. Changing the default output device wasn't tried |
| Xbox audio | Not done | Needs a console |

### 006 UWP feature restoration

#### Goal

Preserve existing workflows and add scoped improvements.

#### Acceptance criteria

| Criterion | Status | Evidence |
|---|---|---|
| Existing keys and presets keep working | Done | Confirmed on x64 |
| Settings persist | Done | Unit tested, and kept across a restart on x64 |
| Favorites | Done | Unit tested, and confirmed on x64 |
| User preset import | Done | Confirmed on x64 |
| Configurable preset duration | Done | Confirmed on x64 |
| Suspend and resume | Done | Audio restarted after resume on x64 |
| Diagnostics | Done | projectM-UWP.log was used for the measurements |
| Search and categories | Not done | Needs a text interface, see decision 1 |
| Gamepad navigation on Xbox | Not done | Needs a console |

### 007 CI and release readiness

#### Goal

Repeatable automated validation.

#### Acceptance criteria

| Criterion | Status | Evidence |
|---|---|---|
| Pull requests get automated builds | Done | .github/workflows/build.yml |
| Unit tests run in CI | Done | Same workflow |
| Package architecture and identity are validated | Done | Same workflow |
| Developer Mode testing is documented | Done | [Testing](#testing) |
| Signing is verified | Not done | Needs the Store certificate |
| Licenses and redistribution are audited | Not done | Inventory exists, and the preset decision is open |
