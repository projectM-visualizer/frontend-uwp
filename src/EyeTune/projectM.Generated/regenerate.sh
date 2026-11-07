#!/bin/bash
# Regenerates the files in this folder from the src/projectm submodule.
# Run this from Git Bash after moving the submodule to a new commit.
#
# The Visual Studio projects cannot run projectM's CMake build, so its generated
# headers and embedded shaders are checked in here instead. Requires CMake 3.21+
# and a Visual Studio install with the v142 toolset.
set -e

here="$(cd "$(dirname "$0")" && pwd)"
source="$here/../../projectm"
build="$(mktemp -d)"
trap 'rm -rf "$build"' EXIT

cmake -S "$source" -B "$build" -A x64 -T v142 \
    -DENABLE_GLES=ON -DENABLE_PLAYLIST=ON -DENABLE_SDL_UI=OFF \
    -DBUILD_TESTING=OFF -DBUILD_SHARED_LIBS=ON
cmake --build "$build" --config Release --target MilkdropPreset Renderer UserSprites -j 8

mkdir -p "$here/include/projectM-4"
cp "$build/include/config.h" "$here/include/"
cp "$build"/src/api/include/projectM-4/*.h "$here/include/projectM-4/"
cp "$build"/src/playlist/include/projectM-4/*.h "$here/include/projectM-4/"
cp "$build"/src/libprojectM/MilkdropPreset/MilkdropStaticShaders.* "$here/"
cp "$build/src/libprojectM/Renderer/BuiltInTransitionsResources.hpp" "$here/"
cp "$build/src/libprojectM/UserSprites/SpriteShaders.hpp" "$here/"

echo "Done. Review the changes, then update EyeTune.Shared.vcxitems if projectM added or removed source files."
