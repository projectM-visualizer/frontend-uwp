projectM "EyeTune" Universal Windows Platform Frontend App
==========================================================

This project is the official Universal Windows Platform (UWP) application published in the Microsoft Store. It brings
you the awesome projectM/Milkdrop visuals to any X86-based device supported by Microsoft's Universal Windows Platform,
e.g. Surface and some Windows Mobile devices.

Due to platform limitations, this app currently does not allow adding any user-specific presets. It is also constrained
to OpenGL ES rendering, as UWP apps can run on both desktop and mobile devices.

If you look for a classic desktop application, you might rather want to
use [the SDL2-based frontend](https://github.com/projectM-visualizer/frontend-sdl).

You can download the latest EyeTune release for free from the Windows Store here:

https://www.microsoft.com/p/projectm/9ndcvh0vcwjn

## Building

You need CMake 3.21 or newer and Visual Studio with the UWP workload and the v142 toolset.

Clone with submodules, since projectM and its dependencies live in `src/projectm`:

```
git clone --recurse-submodules https://github.com/Coastrnaut/frontend-uwp.git
```

For an existing clone, run `git submodule update --init --recursive`.

Clone to a short path such as `C:\GIT`. Some preset filenames are long enough that checkout fails in a deeply nested folder.

Configure and build with a preset:

```
cmake --preset x64
cmake --build --preset x64-release
```

The presets are `x64` and `x86` to configure, and `x64-debug`, `x64-release`, `x86-debug` and `x86-release` to build. To use the Visual Studio IDE, open `build/x64/EyeTune.sln` after configuring. Change project settings in `CMakeLists.txt`, because that solution is regenerated.

Packages are written to `build/<preset>/AppPackages`. To combine both architectures into one bundle, build `x64-release` and `x86-release`, then run:

```
powershell -File scripts/make-bundle.ps1
```

### Signing

Builds are unsigned by default. To sign, pass your certificate when configuring:

```
cmake --preset x64 -DEYETUNE_PACKAGE_CERTIFICATE=C:/path/to/key.pfx -DEYETUNE_PACKAGE_THUMBPRINT=<thumbprint>
```

### Updating projectM

Move `src/projectm` to the new commit, update its submodules, then configure again. projectM's own CMake build provides the source list, generated headers and shaders.

UWP-specific replacements live in `src/EyeTune/EyeTune.Shared`, and `CMakeLists.txt` applies them to projectM's targets without changing the submodule. Presets are kept in `src/EyeTune/presets`, because projectM no longer includes any.

## Contributing

Contributions are welcome. Please feel free to fork, implement and create a pull request. If there are any questions, the projectM team is available on [the projectM Discord server](https://discord.gg/mMrxAqaa3W). Ideally, join the [development channel](https://discord.com/channels/737206408482914387/737600388944887819).
