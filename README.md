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

## Call for help

This app won't build against the latest projectM library. In addition to that, the project files are quite outdated and were created manually. Since the core team currently has no resources and lacks the required experience with writing UWP apps, we're in need of volunteers to upate this app and prepare it for a future update. Here's a short To-Do list:

- Remove the outdated VS solution files and [generate the project with CMake](https://cmake.org/cmake/help/latest/manual/cmake-toolchains.7.html#cross-compiling-for-windows-10-universal-applications), which will make it easier to keep it up to date with any new VS releases.
- Update the code to match the new projectM C API which will be the only official way to use libprojectM in the future. Also makes it possible to link libprojectM as a DLL.
- Add new CI/GitHub Actions build for building future releases.

If you're interested in taking one or more of the above tasks, please feel free to fork, implement and create a pull request. If there are any questions, the projectM team is available on [the projectM Discord server](https://discord.gg/mMrxAqaa3W). Ideally, join the [development channel](https://discord.com/channels/737206408482914387/737600388944887819).
