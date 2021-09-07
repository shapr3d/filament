#ifndef TNT_FILAMENT_DRIVER_OPENGL_PLATFORM_EGL_ANGLE_D3D11_H
#define TNT_FILAMENT_DRIVER_OPENGL_PLATFORM_EGL_ANGLE_D3D11_H

#include "PlatformEGL.h"

namespace filament {

class PlatformEGL_ANGLE_D3D11 final : public PlatformEGL {
public:
    explicit PlatformEGL_ANGLE_D3D11(void* d3dDevice) noexcept;

    backend::Driver* createDriver(void* sharedContext) noexcept override;
    void terminate() noexcept override;

    SwapChain* createSwapChain(void* nativewindow, uint64_t& flags) noexcept override;
    SwapChain* createSwapChain(uint32_t width, uint32_t height, uint64_t& flags) noexcept override;

private:
    EGLConfig chooseConfig(const EGLint* configAttribs) const noexcept;

    EGLDeviceEXT mEGLDevice = EGL_NO_DEVICE_EXT;
};

} // namespace filament

#endif // TNT_FILAMENT_DRIVER_OPENGL_PLATFORM_EGL_ANGLE_D3D11_H
