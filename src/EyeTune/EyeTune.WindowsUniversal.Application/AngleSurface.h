// OpenGL ES window surface on top of ANGLE and Direct3D 11.
#pragma once

#include "IRenderSurface.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglplatform.h>
#include <angle_windowsstore.h>

namespace EyeTune {

class AngleSurface final : public Core::IRenderSurface
{
public:
    // Throws Platform::Exception if no usable graphics device is available.
    void Initialize(Windows::UI::Core::CoreWindow^ window);
    void Cleanup();

    int Width() const override;
    int Height() const override;
    bool Present() override;

private:
    EGLDisplay mEglDisplay = EGL_NO_DISPLAY;
    EGLContext mEglContext = EGL_NO_CONTEXT;
    EGLSurface mEglSurface = EGL_NO_SURFACE;
};

} // namespace EyeTune
