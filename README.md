projectM "EyeTune" Universal Windows Platform Frontend App
==========================================================

EyeTune is the Universal Windows Platform (UWP) application for projectM, published in the Microsoft Store. It brings the projectM/Milkdrop music visualizer to Windows through OpenGL ES, using ANGLE on Direct3D 11.

If you're looking for a classic desktop application, you might prefer [the SDL2-based frontend](https://github.com/projectM-visualizer/frontend-sdl).

You can download EyeTune from the Windows Store: https://www.microsoft.com/p/projectm/9ndcvh0vcwjn

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

See [platform feasibility](docs/platform-feasibility.md) for details.

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

To use the Visual Studio IDE, open `build/x64/EyeTune.sln` after configuring. Change project settings in `CMakeLists.txt`, because that solution is regenerated.

Packages are written to `build/<preset>/AppPackages`. To combine both architectures into one bundle and check it, build `x64-release` and `x86-release`, then run:

```
powershell -File scripts/make-bundle.ps1
powershell -File scripts/validate-bundle.ps1
```

### Signing

Builds are unsigned by default. To sign, pass your certificate when configuring. This path hasn't been tested with a real certificate yet.

```
cmake --preset x64 -DEYETUNE_PACKAGE_CERTIFICATE=C:/path/to/key.pfx -DEYETUNE_PACKAGE_THUMBPRINT=<thumbprint>
```

### Updating projectM

The submodule tracks upstream [projectM](https://github.com/projectM-visualizer/projectm), pinned to a tested commit. It's on the development branch because the current release doesn't have what the UWP port needs ([decision 6](docs/decisions/0006-track-upstream-master.md)).

Move `src/projectm` to the new commit, update its submodules, then configure again. projectM's own CMake build provides the source list, generated headers and shaders. Then run the manual checks in [testing](docs/testing.md), on x86 as well as x64.

## Testing

```
cmake -S tests -B build/tests
cmake --build build/tests --config Release
ctest --test-dir build/tests -C Release --output-on-failure
```

GitHub Actions runs these tests, builds every configuration and validates the bundle on each push and pull request. CI doesn't launch the app, so see [testing](docs/testing.md) for manual checks and how to run an unsigned build.

The app writes frame rate, audio level and errors to `eyetune.log` in its `LocalState` folder.

## Project layout

| Path | Contents |
|---|---|
| `src/EyeTune/EyeTune.WindowsUniversal.Application` | The app: window, input, audio capture and rendering |
| `src/EyeTune/EyeTune.Core` | Platform-independent code, with unit tests |
| `src/EyeTune/EyeTune.Shared` | UWP replacements applied to projectM |
| `src/EyeTune/presets` | Bundled presets |
| `src/projectm` | projectM, as an unmodified submodule |
| `tests` | Unit tests |
| `scripts` | Bundle creation and validation |
| `docs` | Architecture, decisions, history, licensing and testing |
| `specs` | Modernization specifications and their status |

## Known limitations

- Xbox, Store submission and signing are untested.
- The bundled presets' redistribution rights need auditing. See [licensing](docs/licensing.md).
- There's no on-screen preset browser or search, because the app has no XAML interface.
- projectM is pinned to an unreleased commit, with a local fix for an x86 crash until upstream takes it.

See [specs](specs/README.md) for what's done and what's open.

## Contributing

Contributions are welcome. Please feel free to fork, implement and create a pull request. If there are any questions, the projectM team is available on [the projectM Discord server](https://discord.gg/mMrxAqaa3W). Ideally, join the [development channel](https://discord.com/channels/737206408482914387/737600388944887819).

## License

EyeTune is licensed under the [LGPL 2.1](LICENSE.md). See [licensing](docs/licensing.md) for third-party components.
