#define EGL_EGL_PROTOTYPES 1
#include "PlatformEGL_ANGLE_D3D11.h"

#include "OpenGLDriverFactory.h"

#include <utils/debug.h>

#include <EGL/eglext_angle.h>

using namespace utils;

namespace filament {
using namespace backend;


PlatformEGL_ANGLE_D3D11::PlatformEGL_ANGLE_D3D11(void* d3dDevice) noexcept {
    assert_invariant(d3dDevice);
    auto eglCreateDeviceANGLE = (PFNEGLCREATEDEVICEANGLEPROC) eglGetProcAddress("eglCreateDeviceANGLE");
    assert_invariant(eglCreateDeviceANGLE);
    mEGLDevice = eglCreateDeviceANGLE(EGL_D3D11_DEVICE_ANGLE, d3dDevice, nullptr);
    assert_invariant(mEGLDevice != EGL_NO_DEVICE_EXT);
}

backend::Driver* PlatformEGL_ANGLE_D3D11::createDriver(void* sharedContext) noexcept {
    const EGLint displayAttribs[] = {
        EGL_PLATFORM_ANGLE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_NATIVE_PLATFORM_TYPE_ANGLE,
        EGL_EXPERIMENTAL_PRESENT_PATH_ANGLE, EGL_EXPERIMENTAL_PRESENT_PATH_FAST_ANGLE,
        // automatically call the IDXGIDevice3::Trim method on behalf of the application when it gets suspended
        // calling IDXGIDevice3::Trim when an application is suspended is a Windows Store application certification requirement
        EGL_PLATFORM_ANGLE_ENABLE_AUTOMATIC_TRIM_ANGLE, EGL_TRUE,
        EGL_NONE
    };

    auto eglGetPlatformDisplayEXT = (PFNEGLGETPLATFORMDISPLAYEXTPROC) eglGetProcAddress("eglGetPlatformDisplayEXT");
    assert_invariant(eglGetPlatformDisplayEXT);
    mEGLDisplay = eglGetPlatformDisplayEXT(EGL_PLATFORM_DEVICE_EXT, mEGLDevice, displayAttribs);
    assert_invariant(mEGLDisplay != EGL_NO_DISPLAY);

    EGLint major, minor;
    EGLBoolean initialized = eglInitialize(mEGLDisplay, &major, &minor);
    if (UTILS_UNLIKELY(!initialized)) {
        slog.e << "eglInitialize failed" << io::endl;
        return nullptr;
    }

    auto extensions = initializeExtensions();

    EGLint configAttribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT_KHR,            //  0
        EGL_TEXTURE_SWIZZLING_TYPE, EGL_TEXTURE_SWIZZLING_BGRA, //  2
        EGL_RED_SIZE,    8,                                     //  4
        EGL_GREEN_SIZE,  8,                                     //  6
        EGL_BLUE_SIZE,   8,                                     //  8
        EGL_ALPHA_SIZE,  0,                                     // 10 : reserved to set ALPHA_SIZE below
        EGL_DEPTH_SIZE, 24,                                     // 12
        EGL_NONE
    };

    EGLint contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE, EGL_NONE, // reserved for EGL_CONTEXT_OPENGL_NO_ERROR_KHR below
        EGL_NONE
    };
#ifdef NDEBUG
    // When we don't have a shared context and we're in release mode, we always activate the
    // EGL_KHR_create_context_no_error extension.
    if (!sharedContext && extensions.has("EGL_KHR_create_context_no_error")) {
        contextAttribs[2] = EGL_CONTEXT_OPENGL_NO_ERROR_KHR;
        contextAttribs[3] = EGL_TRUE;
    }
#endif

    EGLConfig eglConfig = nullptr;

    mEGLConfig = chooseConfig(configAttribs);
    if (mEGLConfig == EGL_NO_CONFIG_KHR) {
        goto errorANGLE;
    }


    // find a transparent config
    configAttribs[10] = EGL_ALPHA_SIZE;
    configAttribs[11] = 8;
    mEGLTransparentConfig = chooseConfig(configAttribs);
    if (mEGLConfig == EGL_NO_CONFIG_KHR) {
        goto errorANGLE;
    }

    if (!extensions.has("EGL_KHR_no_config_context")) {
        // if we have the EGL_KHR_no_config_context, we don't need to worry about the config
        // when creating the context, otherwise, we must always pick a transparent config.
        eglConfig = mEGLConfig = mEGLTransparentConfig;
    }

    // Ignore initial hardcoded 1,1 window size and wait for the first Resize(),
    // as trying to call eglCreateWindowSurface() would fail yet.
    mEGLDummySurface = EGL_NO_SURFACE;

    mEGLContext = eglCreateContext(mEGLDisplay, eglConfig, (EGLContext)sharedContext, contextAttribs);
    if (mEGLContext == EGL_NO_CONTEXT && sharedContext &&
        extensions.has("EGL_KHR_create_context_no_error")) {
        // context creation could fail because of EGL_CONTEXT_OPENGL_NO_ERROR_KHR
        // not matching the sharedContext. Try with it.
        contextAttribs[2] = EGL_CONTEXT_OPENGL_NO_ERROR_KHR;
        contextAttribs[3] = EGL_TRUE;
        mEGLContext = eglCreateContext(mEGLDisplay, eglConfig, (EGLContext)sharedContext, contextAttribs);
    }
    if (UTILS_UNLIKELY(mEGLContext == EGL_NO_CONTEXT)) {
        // eglCreateContext failed
        logEglError("eglCreateContext");
        goto errorANGLE;
    }

    if (!eglMakeCurrent(mEGLDisplay, EGL_NO_CONTEXT, EGL_NO_CONTEXT, mEGLContext)) {
        // eglMakeCurrent failed
        logEglError("eglMakeCurrent");
        goto errorANGLE;
    }

    initializeGlExtensions();

    // success!!
    return OpenGLDriverFactory::create(this, sharedContext);

errorANGLE:
    // if we're here, we've failed
    if (mEGLContext) {
        eglDestroyContext(mEGLDisplay, mEGLContext);
    }

    mEGLDummySurface = EGL_NO_SURFACE;
    mEGLContext = EGL_NO_CONTEXT;

    eglTerminate(mEGLDisplay);
    eglReleaseThread();

    return nullptr;
}

void PlatformEGL_ANGLE_D3D11::terminate() noexcept {
    if (mEGLDevice != EGL_NO_DEVICE_EXT) {
        auto eglReleaseDeviceANGLE = (PFNEGLRELEASEDEVICEANGLEPROC) eglGetProcAddress("eglReleaseDeviceANGLE");
        if (eglReleaseDeviceANGLE && eglReleaseDeviceANGLE(mEGLDevice) != EGL_TRUE) {
            mEGLDevice = EGL_NO_DEVICE_EXT;
        } else {
            assert(false);
        }
    }
}

Platform::SwapChain* PlatformEGL_ANGLE_D3D11::createSwapChain(void* nativewindow, uint64_t& flags) noexcept {
    EGLSurface sur = eglCreateWindowSurface(mEGLDisplay,
            (flags & backend::SWAP_CHAIN_CONFIG_TRANSPARENT) ?
            mEGLTransparentConfig : mEGLConfig,
            static_cast<EGLNativeWindowType>(nativewindow), nullptr);

    if (UTILS_UNLIKELY(sur == EGL_NO_SURFACE)) {
        logEglError("eglCreateWindowSurface");
        return nullptr;
    }
    if (!eglSurfaceAttrib(mEGLDisplay, sur, EGL_SWAP_BEHAVIOR, EGL_BUFFER_DESTROYED)) {
        logEglError("eglSurfaceAttrib(..., EGL_SWAP_BEHAVIOR, EGL_BUFFER_DESTROYED)");
        // this is not fatal
    }

    return (SwapChain*)sur;
}

Platform::SwapChain* PlatformEGL_ANGLE_D3D11::createSwapChain(uint32_t width, uint32_t height, uint64_t& flags) noexcept {
    slog.e << "PlatformEGL_ANGLE_D3D11::createSwapChain() can only be called with native swap chain!" << io::endl;
    return nullptr;
}

EGLConfig PlatformEGL_ANGLE_D3D11::chooseConfig(const EGLint* configAttribs) const noexcept {
    // Configs are sorted in ascending order by most attributes, except for color depth, which is in descending order
    // (https://www.khronos.org/registry/EGL/sdk/docs/man/html/eglChooseConfig.xhtml)
    // Thus we search for a config with the smallest color depth which satisfies the requested values
    EGLint numConfigs;
    EGLConfig retVal = EGL_NO_CONFIG_KHR;
    if (!eglChooseConfig(mEGLDisplay, configAttribs, nullptr, 0, &numConfigs) == EGL_TRUE && numConfigs > 0) {
        logEglError("eglChooseConfig");
        return retVal;
    }

    std::vector<EGLConfig> configs(numConfigs);
    if (!eglChooseConfig(mEGLDisplay, configAttribs, configs.data(), numConfigs, &numConfigs) == EGL_TRUE) {
        logEglError("eglChooseConfig");
        return retVal;
    }
    auto configColorsSize = std::numeric_limits<EGLint>::max();
    for (const auto& config : configs) {
        EGLint redSize, greenSize, blueSize, alphaSize;
        EGLBoolean getSuccessful = EGL_TRUE;
        getSuccessful &= eglGetConfigAttrib(mEGLDisplay, config, EGL_RED_SIZE, &redSize);
        getSuccessful &= eglGetConfigAttrib(mEGLDisplay, config, EGL_GREEN_SIZE, &greenSize);
        getSuccessful &= eglGetConfigAttrib(mEGLDisplay, config, EGL_BLUE_SIZE, &blueSize);
        getSuccessful &= eglGetConfigAttrib(mEGLDisplay, config, EGL_ALPHA_SIZE, &alphaSize);
        if (getSuccessful != EGL_TRUE) {
            logEglError("eglGetConfigAttrib");
            return EGL_NO_CONFIG_KHR;
        }

        auto colorsSize = redSize + greenSize + blueSize;
        if (colorsSize < configColorsSize) {
            retVal = config;
            configColorsSize = colorsSize;
        }
    }

    return retVal;
}

} // namespace filament

// ---------------------------------------------------------------------------------------------
