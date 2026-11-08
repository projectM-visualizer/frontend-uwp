projectM-UWP: Universal Windows Platform Frontend App
====================================================

projectM-UWP is the Universal Windows Platform (UWP) application for projectM, published in the Microsoft Store as projectM-visualizer. It brings the projectM/Milkdrop music visualizer to Windows through OpenGL ES, using ANGLE on Direct3D 11.

If you're looking for a classic desktop application, you might prefer [the SDL2-based frontend](https://github.com/projectM-visualizer/frontend-sdl).

Get projectM-visualizer from the Microsoft Store: https://apps.microsoft.com/detail/9MVZC1D9Z7PX

The older EyeTune listing (`9NDCVH0VCWJN`) is a separate product on another developer account. It doesn't get these updates.

## Features

- Visualizes whatever your PC is playing, or the microphone
- 29 bundled presets, with shuffle and smooth transitions
- Favorites, with a favorites-only mode
- Import your own `.milk` and `.prjm` presets
- Adjustable preset duration, and settings that are kept between runs
- Audio keeps working when the default device changes or the app resumes

## Controls

| Action | Keyboard | Gamepad |
|---|---|---|
| Help and current preset | `F1`, `Esc` | Menu, View |
| Next preset | `Right`, `N` | Right trigger, D-pad right |
| Previous preset | `Left`, `P` | Left trigger, D-pad left |
| Random preset | `Enter`, `R` | A, X, Y |
| Lock or unlock preset | `Space`, `Ctrl` | Left or right shoulder |
| Add or remove favorite | `L` | |
| Favorites only | `V` | |
| Import presets | `I` | |
| Preset duration | `+`, `-` | |
| System audio or microphone | `M` | |
| Experimental presets | `Down` | D-pad down |
| Fullscreen | `Ctrl+F` | |

## Platform support

| Platform | Status |
|---|---|
| Windows x64 | Tested |
| Windows x86 | Tested |
| Xbox | Not tested. A console release existed historically, but compatibility isn't claimed |
| ARM | Not supported |

See [platform feasibility](design.md#platform-feasibility) for details.

## Building

You need CMake 3.21 or newer and Visual Studio with the UWP workload and the v142 toolset.

Clone with submodules, since projectM and its dependencies live in `src/projectm`:

```
git clone --recurse-submodules https://github.com/Coastrnaut/frontend-uwp.git
```

For an existing clone, run `git submodule sync --recursive`, then `git submodule update --init --recursive`.

Clone to a short path such as `C:\GIT`. Some preset filenames are long enough that checkout fails in a deeply nested folder.

Configure and build with a preset:

```
cmake --preset x64
cmake --build --preset x64-release
```

The presets are `x64` and `x86` to configure, and `x64-debug`, `x64-release`, `x86-debug` and `x86-release` to build. Configuring downloads ANGLE from nuget.org, so the first run needs internet access.

To use the Visual Studio IDE, open `build/x64/projectM-UWP.sln` after configuring. Change project settings in `CMakeLists.txt`, because that solution is regenerated.

Packages are written to `build/<preset>/AppPackages`. To combine both architectures into one bundle and check it, build `x64-release` and `x86-release`, then run:

```
powershell -File scripts/make-bundle.ps1
powershell -File scripts/validate-bundle.ps1
```

### Publishing to the Store

The manifest carries the identity Partner Center assigned to [projectM-visualizer](https://apps.microsoft.com/detail/9MVZC1D9Z7PX):

| Manifest field | Value |
|---|---|
| `Package/Identity/Name` | `coaster.projectM-visualizer` |
| `Package/Identity/Publisher` | `CN=D9ED5A20-CFEF-4A74-9651-3E6DDAF76CC5` |
| `Package/Properties/PublisherDisplayName` | coaster |
| `Package/Properties/DisplayName` | projectM-visualizer |

The package family name is `coaster.projectM-visualizer_cybm9vgwr20yc`, and the Store ID is `9MVZC1D9Z7PX`. `validate-bundle.ps1` fails if the identity, publisher or display name differs from these.

To release:

1. Raise `Version` in `Package.appxmanifest` above the last submitted version. The fourth number must stay 0.
2. Build `x64-release` and `x86-release`, then run `make-bundle.ps1` and `validate-bundle.ps1`.
3. Optionally, run the Windows App Certification Kit from an administrator PowerShell:

   ```
   & "${env:ProgramFiles(x86)}\Windows Kits\10\App Certification Kit\appcert.exe" test -appxpackagepath build\bundle\Release\projectM-UWP_<version>_x86_x64.appxbundle -reportoutputpath build\wack.xml
   ```

4. In Partner Center, start a submission for projectM-visualizer and upload `build/bundle/Release/projectM-UWP_<version>_x86_x64.appxbundle` under Packages.

Store screenshots are in `store/screenshots`: one fullscreen 2752×1966 PNG of each preset that loads, and one of the help overlay. `presets.txt` lists which preset each shows, and `contact-sheet.jpg` shows them all at once. Partner Center takes up to 10 per device family.

Upload the bundle unsigned. The Store signs it. Release builds don't produce symbol files, so the Store's crash reports won't have function names.

### Signing

Signing is only needed to install a package outside the Store. Builds are unsigned by default. To sign, pass your certificate when configuring. This path hasn't been tested with a real certificate yet.

```
cmake --preset x64 -DPROJECTM_UWP_PACKAGE_CERTIFICATE=C:/path/to/key.pfx -DPROJECTM_UWP_PACKAGE_THUMBPRINT=<thumbprint>
```

### Updating projectM

The submodule tracks upstream [projectM](https://github.com/projectM-visualizer/projectm), pinned to a tested commit. It's on the development branch because the current release doesn't have what the UWP port needs ([decision 6](design.md#6-track-projectms-development-branch)).

Move `src/projectm` to the new commit, update its submodules, then configure again. projectM's own CMake build provides the source list, generated headers and shaders. Then run the manual checks in [testing](design.md#testing), on x86 as well as x64.

## Testing

```
cmake -S tests -B build/tests
cmake --build build/tests --config Release
ctest --test-dir build/tests -C Release --output-on-failure
```

GitHub Actions runs these tests, builds every configuration and validates the bundle on each push and pull request. CI doesn't launch the app, so see [testing](design.md#testing) for manual checks and how to run an unsigned build.

The app writes frame rate, audio level and errors to `projectM-UWP.log` in its `LocalState` folder.

## Project layout

| Path | Contents |
|---|---|
| `src/projectM-UWP/projectM-UWP.WindowsUniversal.Application` | The app: window, input, audio capture and rendering |
| `src/projectM-UWP/projectM-UWP.Core` | Platform-independent code, with unit tests |
| `src/projectM-UWP/projectM-UWP.Shared` | UWP replacements applied to projectM |
| `src/projectM-UWP/presets` | Bundled presets |
| `src/projectm` | projectM, as an unmodified submodule |
| `tests` | Unit tests |
| `scripts` | Bundle creation and validation |
| `store` | Store screenshots |
| `design.md` | Architecture, decisions, testing, licensing, history and status |

## Known limitations

- Xbox and signing are untested, and no build has gone through Store certification yet.
- The bundled presets' redistribution rights need auditing. See [licensing](design.md#licensing).
- There's no on-screen preset browser or search, because the app has no XAML interface.
- In favorites-only mode, newly imported presets stay hidden until you add them as favorites or turn the mode off.
- Microphone capture and x86 keyboard controls haven't been confirmed yet.
- projectM is pinned to an unreleased commit, with a local fix for an x86 crash until upstream merges [projectM-visualizer/projectm#1059](https://github.com/projectM-visualizer/projectm/pull/1059).

See [design](design.md#status) for what's done and what's open.

## Contributing

Contributions are welcome. Please feel free to fork, implement and create a pull request. If there are any questions, the projectM team is available on [the projectM Discord server](https://discord.gg/mMrxAqaa3W). Ideally, join the [development channel](https://discord.com/channels/737206408482914387/737600388944887819).

## License

projectM-UWP is licensed under the [LGPL 2.1](LICENSE.md). See [licensing](design.md#licensing) for third-party components.
