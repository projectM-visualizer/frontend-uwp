cmake_minimum_required(VERSION 3.21)

# Configure through CMakePresets.json, which selects UWP and the v142 toolset.
if(NOT CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
    message(FATAL_ERROR "EyeTune is a UWP app. Configure it with: cmake --preset x64")
endif()

project(EyeTune VERSION 0.9.7 LANGUAGES C CXX)

set(EYETUNE_PACKAGE_CERTIFICATE "" CACHE FILEPATH "Certificate (.pfx) used to sign the app package. Leave empty to build unsigned.")
set(EYETUNE_PACKAGE_THUMBPRINT "" CACHE STRING "Thumbprint of EYETUNE_PACKAGE_CERTIFICATE.")

# --- ANGLE, which provides OpenGL ES on top of Direct3D 11 ---------------------

set(ANGLE_VERSION 0.777.79)
set(ANGLE_DIR "${CMAKE_BINARY_DIR}/_deps/WindowsStoreOpenGL.${ANGLE_VERSION}")
set(ANGLE_ARCHIVE "${ANGLE_DIR}.zip")

file(DOWNLOAD
    "https://api.nuget.org/v3-flatcontainer/windowsstoreopengl/${ANGLE_VERSION}/windowsstoreopengl.${ANGLE_VERSION}.nupkg"
    "${ANGLE_ARCHIVE}"
    EXPECTED_HASH SHA256=e89899548561c7c0fea73e5c9f7aeadf5da6ba1dd1312bee5dafabf5866c4d62
    TLS_VERIFY ON
)

if(NOT EXISTS "${ANGLE_DIR}/build/native/include/EGL/egl.h")
    file(ARCHIVE_EXTRACT INPUT "${ANGLE_ARCHIVE}" DESTINATION "${ANGLE_DIR}")
endif()

if(CMAKE_VS_PLATFORM_NAME STREQUAL "Win32")
    set(ANGLE_ARCH Win32)
elseif(CMAKE_VS_PLATFORM_NAME STREQUAL "x64")
    set(ANGLE_ARCH x64)
else()
    message(FATAL_ERROR "EyeTune builds for x64 and Win32, not ${CMAKE_VS_PLATFORM_NAME}.")
endif()

set(ANGLE_BIN "${ANGLE_DIR}/build/native/Release_${ANGLE_ARCH}")

# --- projectM, built from the unmodified submodule ------------------------------

set(ENABLE_GLES ON CACHE BOOL "" FORCE)
set(ENABLE_PLAYLIST ON CACHE BOOL "" FORCE)
set(ENABLE_SDL_UI OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)

# UWP projects enable SDL checks, which turn the CRT deprecation warnings in
# projectM's vendored C code into errors.
add_compile_options(/sdl-)
add_compile_definitions(_CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_WARNINGS)

# projectM sets these only when CMake's platform is Windows, not WindowsStore.
add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN STBI_NO_DDS)

# Use the parser sources shipped with projectm-eval, so that a flex or bison on
# PATH can't rewrite files inside the submodule.
set(CMAKE_DISABLE_FIND_PACKAGE_FLEX ON)
set(CMAKE_DISABLE_FIND_PACKAGE_BISON ON)

add_subdirectory(src/projectm EXCLUDE_FROM_ALL)

# DynamicLibrary_win32.cpp calls Win32 loader APIs that UWP doesn't have.
set(UWP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/EyeTune/EyeTune.Shared")
set_source_files_properties(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/projectm/src/libprojectM/Renderer/Platform/DynamicLibrary_win32.cpp"
    TARGET_DIRECTORY Renderer
    PROPERTIES HEADER_FILE_ONLY ON
)
target_sources(Renderer PRIVATE "${UWP_DIR}/DynamicLibrary_uwp.cpp")

# GLProbe.cpp declares its glGetStringi pointer without the GL calling convention.
# On 32-bit x86 GL functions are __stdcall, so each call corrupts the stack, and
# Release builds crash at startup. Compile a corrected copy until projectM fixes
# this. Once it has, the text below won't match and the original file is used.
set(PLATFORM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/projectm/src/libprojectM/Renderer/Platform")
set(GLPROBE_BROKEN "using GetStringiFn = const GLubyte* (*) (GLenum, GLuint);")
file(READ "${PLATFORM_DIR}/GLProbe.cpp" GLPROBE_SOURCE)
string(FIND "${GLPROBE_SOURCE}" "${GLPROBE_BROKEN}" GLPROBE_MATCH)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${PLATFORM_DIR}/GLProbe.cpp")
if(GLPROBE_MATCH GREATER_EQUAL 0)
    string(REPLACE "${GLPROBE_BROKEN}"
        "using GetStringiFn = const GLubyte*(GLAD_API_PTR*)(GLenum, GLuint);"
        GLPROBE_SOURCE "${GLPROBE_SOURCE}")
    file(WRITE "${CMAKE_BINARY_DIR}/uwp/GLProbe.cpp" "${GLPROBE_SOURCE}")
    set_source_files_properties("${PLATFORM_DIR}/GLProbe.cpp"
        TARGET_DIRECTORY Renderer
        PROPERTIES HEADER_FILE_ONLY ON
    )
    target_sources(Renderer PRIVATE "${CMAKE_BINARY_DIR}/uwp/GLProbe.cpp")
    target_include_directories(Renderer PRIVATE "${PLATFORM_DIR}")
endif()

target_compile_options(Renderer PRIVATE "/FI${UWP_DIR}/uwp_compat.h")

# --- EyeTune app ----------------------------------------------------------------

set(APP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/EyeTune/EyeTune.WindowsUniversal.Application")

file(GLOB PRESETS CONFIGURE_DEPENDS "src/EyeTune/presets/presets_eyetune/*.milk")
file(GLOB APP_IMAGES CONFIGURE_DEPENDS "${APP_DIR}/Assets/*.png")
set(ANGLE_DLLS "${ANGLE_BIN}/libEGL.dll" "${ANGLE_BIN}/libGLESv2.dll")

set_source_files_properties(${PRESETS} PROPERTIES
    VS_DEPLOYMENT_CONTENT 1 VS_DEPLOYMENT_LOCATION "p")
set_source_files_properties(${APP_IMAGES} PROPERTIES
    VS_DEPLOYMENT_CONTENT 1 VS_DEPLOYMENT_LOCATION "Assets")
set_source_files_properties(${ANGLE_DLLS} PROPERTIES
    VS_DEPLOYMENT_CONTENT 1 VS_DEPLOYMENT_LOCATION ".")

add_executable(EyeTune WIN32
    "${APP_DIR}/EyeTuneApp.cpp"
    "${APP_DIR}/EyeTuneApp.h"
    "${APP_DIR}/EyeTunePlatformString.h"
    "${APP_DIR}/EyeTunePopUp.h"
    "${APP_DIR}/WASAPICapture.cpp"
    "${APP_DIR}/WASAPICapture.h"
    "${APP_DIR}/WASAPICaptureCommon.h"
    "${APP_DIR}/WASAPICaptureDeviceState.h"
    "${APP_DIR}/WASAPICaptureScenario.cpp"
    "${APP_DIR}/Package.appxmanifest"
    ${APP_IMAGES}
    ${PRESETS}
    ${ANGLE_DLLS}
)

set_target_properties(EyeTune PROPERTIES
    OUTPUT_NAME EyeTune.WindowsUniversal.Application
    VS_WINRT_COMPONENT TRUE
    VS_GLOBAL_AppxBundle Never
    VS_GLOBAL_GenerateAppxPackageOnBuild true
    VS_GLOBAL_AppxPackageDir "${CMAKE_BINARY_DIR}/AppPackages/"
)

if(EYETUNE_PACKAGE_CERTIFICATE)
    set_target_properties(EyeTune PROPERTIES
        VS_GLOBAL_AppxPackageSigningEnabled true
        VS_GLOBAL_PackageCertificateKeyFile "${EYETUNE_PACKAGE_CERTIFICATE}"
        VS_GLOBAL_PackageCertificateThumbprint "${EYETUNE_PACKAGE_THUMBPRINT}"
    )
else()
    set_target_properties(EyeTune PROPERTIES VS_GLOBAL_AppxPackageSigningEnabled false)
endif()

target_include_directories(EyeTune PRIVATE "${APP_DIR}" "${ANGLE_DIR}/build/native/include")
target_compile_definitions(EyeTune PRIVATE
    EYETUNE_WINRT
    NOMINMAX
    GL_GLEXT_PROTOTYPES
    EGL_EGLEXT_PROTOTYPES
    ANGLE_STANDALONE_BUILD
    _CRT_SECURE_NO_DEPRECATE
    _SCL_SECURE_NO_WARNINGS
)
target_compile_options(EyeTune PRIVATE /bigobj /wd4453 /wd28204 /wd4244)
target_link_directories(EyeTune PRIVATE "${ANGLE_BIN}")
target_link_libraries(EyeTune PRIVATE
    libprojectM::projectM
    libprojectM::playlist
    libEGL.lib
    libGLESv2.lib
    mincore.lib
)

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT EyeTune)
