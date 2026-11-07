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

Clone with submodules, since projectM and its dependencies live in `src/projectm`:

```
git clone --recurse-submodules https://github.com/Coastrnaut/frontend-uwp.git
```

For an existing clone, run `git submodule update --init --recursive`.

Clone to a short path such as `C:\GIT`. Some preset filenames are long enough that checkout fails in a deeply nested folder.

Open `src/EyeTune.WindowsUniversal.Application.sln` in Visual Studio with the v142 toolset and UWP workload installed, restore the NuGet packages, then build for x64 or x86. Packages are signed with the certificate named in the app's `.vcxproj`, which isn't stored in this repository. Add your own, or restore and build unsigned from a Developer Command Prompt:

```
MSBuild.exe src/EyeTune.WindowsUniversal.Application.sln -t:restore -p:RestorePackagesConfig=true
MSBuild.exe src/EyeTune.WindowsUniversal.Application.sln -p:Configuration=Release -p:Platform=x64 -p:AppxPackageSigningEnabled=false
```

### Updating projectM

The Visual Studio projects compile projectM's sources directly, without its CMake build:

1. Move `src/projectm` to the new commit and update its submodules.
2. Run `src/EyeTune/projectM.Generated/regenerate.sh` in Git Bash to refresh the generated headers and shaders.
3. Update `src/EyeTune/EyeTune.Shared/EyeTune.Shared.vcxitems` if source files were added or removed.

Presets are kept in `src/EyeTune/presets`, because projectM no longer includes any.

## Call for help

The project files are still maintained by hand. Since the core team currently has no resources and lacks the required experience with writing UWP apps, we're in need of volunteers to help prepare this app for a future update. Here's a short To-Do list:

- Remove the outdated VS solution files and [generate the project with CMake](https://cmake.org/cmake/help/latest/manual/cmake-toolchains.7.html#cross-compiling-for-windows-10-universal-applications), which will make it easier to keep it up to date with any new VS releases.
- Add new CI/GitHub Actions build for building future releases.

If you're interested in taking one or more of the above tasks, please feel free to fork, implement and create a pull request. If there are any questions, the projectM team is available on [the projectM Discord server](https://discord.gg/mMrxAqaa3W). Ideally, join the [development channel](https://discord.com/channels/737206408482914387/737600388944887819).
