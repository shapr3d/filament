#pragma clang diagnostic ignored "-Wmissing-prototypes"

#include <metal_stdlib>
#include <simd/simd.h>

using namespace metal;

struct SSBO1
{
    uint values1[1];
};

struct SSBO0
{
    uint values0[1];
};

static inline __attribute__((always_inline))
void callee2(thread float4& gl_FragCoord, device SSBO1& _7)
{
    int _37 = int(gl_FragCoord.x);
    _7.values1[_37]++;
}

static inline __attribute__((always_inline))
void callee(thread float4& gl_FragCoord, device SSBO1& _7, device SSBO0& _9)
{
    int _45 = int(gl_FragCoord.x);
    _9.values0[_45]++;
    callee2(gl_FragCoord, _7);
}

static inline __attribute__((always_inline))
void _29()
{
}

static inline __attribute__((always_inline))
void _31()
{
}

fragment void main0(device SSBO1& _7 [[buffer(0), raster_order_group(0)]], device SSBO0& _9 [[buffer(1), raster_order_group(0)]], float4 gl_FragCoord [[position]])
{
    callee(gl_FragCoord, _7, _9);
    _29();
    _31();
}

