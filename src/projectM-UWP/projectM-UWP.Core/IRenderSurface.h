// What the app needs from a graphics surface. The Windows implementation is
// AngleSurface. Another platform can provide its own without changing the app.
#pragma once

namespace projectM_UWP {
namespace Core {

class IRenderSurface
{
public:
    virtual ~IRenderSurface() = default;

    virtual int Width() const = 0;
    virtual int Height() const = 0;

    // Shows the finished frame. Returns false if the graphics device was lost.
    virtual bool Present() = 0;
};

} // namespace Core
} // namespace projectM_UWP
