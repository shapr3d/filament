/*
 * Copyright (C) 2024 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "gfx/GfxDriver.h"
#include "CommandStreamDispatcher.h"

#include <algorithm>

namespace filament::backend {

UTILS_NOINLINE
Driver* GfxDriver::create(const Platform::DriverConfig& driverConfig) {
    size_t defaultSize = 8 * 1024U * 1024U; // TODO: find good default value for handle arena size (FILAMENT_GFX_HANDLE_ARENA_SIZE_IN_MB)
    Platform::DriverConfig validConfig {driverConfig};
    validConfig.handleArenaSize = std::max(driverConfig.handleArenaSize, defaultSize);
    return new GfxDriver(validConfig);
}

Dispatcher GfxDriver::getDispatcher() const noexcept {
    return ConcreteDispatcher<GfxDriver>::make();
}

// TODO: find proper pool ratios for handle allocator
GfxDriver::GfxDriver(const Platform::DriverConfig& driverConfig) noexcept
        : mHandleAllocator("Handles", driverConfig.handleArenaSize, {1,11,500}) {
}

GfxDriver::~GfxDriver() noexcept = default;

ShaderModel GfxDriver::getShaderModel() const noexcept {
#if defined(IOS) // TODO: add VISIONOS too (maybe include SPLib/SPPlatform.h?)
    return ShaderModel::MOBILE;
#else
    return ShaderModel::DESKTOP;
#endif
}

// explicit instantiation of the Dispatcher
template class ConcreteDispatcher<GfxDriver>;

// Synchronous API implementations

void GfxDriver::terminate() {
}

Handle<HwStream> GfxDriver::createStreamNative(void* nativeStream) {
    return {};
}

Handle<HwStream> GfxDriver::createStreamAcquired() {
    return {};
}

void GfxDriver::setAcquiredImage(Handle<HwStream> sh, void* image,
        CallbackHandler* handler, StreamCallback cb, void* userData) {
}

void GfxDriver::setStreamDimensions(Handle<HwStream> sh, uint32_t width, uint32_t height) {
}

int64_t GfxDriver::getStreamTimestamp(Handle<HwStream> sh) {
    return 0;
}

void GfxDriver::updateStreams(DriverApi* driver) {
}

FenceStatus GfxDriver::getFenceStatus(Handle<HwFence> fh) {
    return FenceStatus::CONDITION_SATISFIED;
}

bool GfxDriver::isTextureFormatSupported(TextureFormat format) {
    return true;
}

bool GfxDriver::isTextureSwizzleSupported() {
    return false;
}

bool GfxDriver::isTextureFormatMipmappable(TextureFormat format) {
    return true;
}

bool GfxDriver::isRenderTargetFormatSupported(TextureFormat format) {
    return true;
}

bool GfxDriver::isFrameBufferFetchSupported() {
    // TODO: copy code from OIT::Init()
    return false;
}

bool GfxDriver::isFrameBufferFetchMultiSampleSupported() {
    return false;
}

bool GfxDriver::isFrameTimeSupported() {
    return true;
}

bool GfxDriver::isAutoDepthResolveSupported() {
    return false;
}

bool GfxDriver::isSRGBSwapChainSupported() {
    return true;
}

bool GfxDriver::isStereoSupported() {
    return false;
}

bool GfxDriver::isParallelShaderCompileSupported() {
    return false;
}

bool GfxDriver::isDepthStencilResolveSupported() {
    return false;
}

uint8_t GfxDriver::getMaxDrawBuffers() {
    return MRT::MAX_SUPPORTED_RENDER_TARGET_COUNT;
}

size_t GfxDriver::getMaxUniformBufferSize() {
    return 65536u; // 64KB default
}

bool GfxDriver::getTimerQueryValue(Handle<HwTimerQuery> tqh, uint64_t* elapsedTime) {
    return false;
}

bool GfxDriver::isWorkaroundNeeded(Workaround workaround) {
    return false;
}

FeatureLevel GfxDriver::getFeatureLevel() {
    return FeatureLevel::FEATURE_LEVEL_1;
}

math::float2 GfxDriver::getClipSpaceParams() {
    return math::float2{ 1.0f, 0.0f };
}

void GfxDriver::setupExternalResource(intptr_t externalResource) {
}

// Asynchronous API implementations

void GfxDriver::tick(int) {
}

void GfxDriver::beginFrame(int64_t monotonic_clock_ns, uint32_t frameId) {
}

void GfxDriver::setFrameScheduledCallback(Handle<HwSwapChain> sch,
        FrameScheduledCallback callback, void* user) {
}

void GfxDriver::setFrameCompletedCallback(Handle<HwSwapChain> sch,
        CallbackHandler* handler, CallbackHandler::Callback callback, void* user) {
}

void GfxDriver::setPresentationTime(int64_t monotonic_clock_ns) {
}

void GfxDriver::endFrame(uint32_t frameId) {
}

void GfxDriver::flush(int) {
}

void GfxDriver::finish(int) {
}

void GfxDriver::resetState(int) {
}

// Resource creation (return handles)

Handle<HwVertexBuffer> GfxDriver::createVertexBufferS() noexcept {
    return {};
}

void GfxDriver::createVertexBufferR(Handle<HwVertexBuffer> vbh,
        uint8_t bufferCount, uint8_t attributeCount, uint32_t vertexCount,
        AttributeArray attributes) {
}

Handle<HwIndexBuffer> GfxDriver::createIndexBufferS() noexcept {
    return {};
}

void GfxDriver::createIndexBufferR(Handle<HwIndexBuffer> ibh,
        ElementType elementType, uint32_t indexCount) {
}

Handle<HwBufferObject> GfxDriver::createBufferObjectS() noexcept {
    return {};
}

void GfxDriver::createBufferObjectR(Handle<HwBufferObject> boh,
        uint32_t byteCount, BufferObjectBinding bindingType, BufferUsage usage) {
}

Handle<HwBufferObject> GfxDriver::importBufferObjectS() noexcept {
    return {};
}

void GfxDriver::importBufferObjectR(Handle<HwBufferObject> boh,
        intptr_t id, BufferObjectBinding bindingType, BufferUsage usage, uint32_t byteCount) {
}

Handle<HwTexture> GfxDriver::createTextureS() noexcept {
    return {};
}

void GfxDriver::createTextureR(Handle<HwTexture> th,
        SamplerType target, uint8_t levels, TextureFormat format,
        uint8_t samples, uint32_t width, uint32_t height, uint32_t depth,
        TextureUsage usage) {
}

Handle<HwTexture> GfxDriver::createTextureSwizzledS() noexcept {
    return {};
}

void GfxDriver::createTextureSwizzledR(Handle<HwTexture> th,
        SamplerType target, uint8_t levels, TextureFormat format,
        uint8_t samples, uint32_t width, uint32_t height, uint32_t depth,
        TextureUsage usage, TextureSwizzle r, TextureSwizzle g,
        TextureSwizzle b, TextureSwizzle a) {
}

Handle<HwTexture> GfxDriver::importTextureS() noexcept {
    return {};
}

void GfxDriver::importTextureR(Handle<HwTexture> th,
        intptr_t id, SamplerType target, uint8_t levels, TextureFormat format,
        uint8_t samples, uint32_t width, uint32_t height, uint32_t depth,
        TextureUsage usage) {
}

Handle<HwSamplerGroup> GfxDriver::createSamplerGroupS() noexcept {
    return {};
}

void GfxDriver::createSamplerGroupR(Handle<HwSamplerGroup> sbh,
        uint32_t size, utils::FixedSizeString<32> debugName) {
}

Handle<HwRenderPrimitive> GfxDriver::createRenderPrimitiveS() noexcept {
    return {};
}

void GfxDriver::createRenderPrimitiveR(Handle<HwRenderPrimitive> rph,
        Handle<HwVertexBuffer> vbh, Handle<HwIndexBuffer> ibh,
        PrimitiveType pt, uint32_t offset, uint32_t minIndex,
        uint32_t maxIndex, uint32_t count) {
}

Handle<HwProgram> GfxDriver::createProgramS() noexcept {
    return {};
}

void GfxDriver::createProgramR(Handle<HwProgram> ph, Program&& program) {
}

Handle<HwRenderTarget> GfxDriver::createDefaultRenderTargetS() noexcept {
    return {};
}

void GfxDriver::createDefaultRenderTargetR(Handle<HwRenderTarget> rth, int) {
}

Handle<HwRenderTarget> GfxDriver::createRenderTargetS() noexcept {
    return {};
}

void GfxDriver::createRenderTargetR(Handle<HwRenderTarget> rth,
        TargetBufferFlags targetBufferFlags, uint32_t width, uint32_t height,
        uint8_t samples, MRT color, TargetBufferInfo depth,
        TargetBufferInfo stencil) {
}

Handle<HwFence> GfxDriver::createFenceS() noexcept {
    return {};
}

void GfxDriver::createFenceR(Handle<HwFence> fh, int) {
}

Handle<HwSwapChain> GfxDriver::createSwapChainS() noexcept {
    return {};
}

void GfxDriver::createSwapChainR(Handle<HwSwapChain> sch, void* nativeWindow, uint64_t flags) {
}

Handle<HwSwapChain> GfxDriver::createSwapChainHeadlessS() noexcept {
    return {};
}

void GfxDriver::createSwapChainHeadlessR(Handle<HwSwapChain> sch,
        uint32_t width, uint32_t height, uint64_t flags) {
}

Handle<HwTimerQuery> GfxDriver::createTimerQueryS() noexcept {
    return {};
}

void GfxDriver::createTimerQueryR(Handle<HwTimerQuery> tqh, int) {
}

// Resource destruction

void GfxDriver::destroyVertexBuffer(Handle<HwVertexBuffer> vbh) {
}

void GfxDriver::destroyIndexBuffer(Handle<HwIndexBuffer> ibh) {
}

void GfxDriver::destroyBufferObject(Handle<HwBufferObject> boh) {
}

void GfxDriver::destroyRenderPrimitive(Handle<HwRenderPrimitive> rph) {
}

void GfxDriver::destroyProgram(Handle<HwProgram> ph) {
}

void GfxDriver::destroyRenderTarget(Handle<HwRenderTarget> rth) {
}

void GfxDriver::destroySamplerGroup(Handle<HwSamplerGroup> sbh) {
}

void GfxDriver::destroyTexture(Handle<HwTexture> th) {
}

void GfxDriver::destroySwapChain(Handle<HwSwapChain> sch) {
}

void GfxDriver::destroyStream(Handle<HwStream> sh) {
}

void GfxDriver::destroyTimerQuery(Handle<HwTimerQuery> tqh) {
}

void GfxDriver::destroyFence(Handle<HwFence> fh) {
}

// Resource updates

void GfxDriver::setIndexBufferObject(Handle<HwIndexBuffer> ibh, Handle<HwBufferObject> boh) {
}

void GfxDriver::setVertexBufferObject(Handle<HwVertexBuffer> vbh, uint32_t index,
        Handle<HwBufferObject> boh) {
}

void GfxDriver::updateBufferObject(Handle<HwBufferObject> boh, BufferDescriptor&& p,
        uint32_t byteOffset) {
    scheduleDestroy(std::move(p));
}

void GfxDriver::updateBufferObjectUnsynchronized(Handle<HwBufferObject> boh, BufferDescriptor&& p,
        uint32_t byteOffset) {
    scheduleDestroy(std::move(p));
}

void GfxDriver::resetBufferObject(Handle<HwBufferObject> boh) {
}

void GfxDriver::setMinMaxLevels(Handle<HwTexture> th, uint32_t minLevel, uint32_t maxLevel) {
}

void GfxDriver::update3DImage(Handle<HwTexture> th,
        uint32_t level, uint32_t xoffset, uint32_t yoffset, uint32_t zoffset,
        uint32_t width, uint32_t height, uint32_t depth,
        PixelBufferDescriptor&& data) {
    scheduleDestroy(std::move(data));
}

void GfxDriver::setExternalImage(Handle<HwTexture> th, void* image) {
}

void GfxDriver::setExternalImagePlane(Handle<HwTexture> th, void* image, uint32_t plane) {
}

void GfxDriver::setExternalStream(Handle<HwTexture> th, Handle<HwStream> sh) {
}

void GfxDriver::generateMipmaps(Handle<HwTexture> th) {
}

void GfxDriver::updateSamplerGroup(Handle<HwSamplerGroup> sbh, BufferDescriptor&& data) {
    scheduleDestroy(std::move(data));
}

// Rendering commands

void GfxDriver::beginRenderPass(Handle<HwRenderTarget> rth, const RenderPassParams& params) {
}

void GfxDriver::endRenderPass(int) {
}

void GfxDriver::nextSubpass(int) {
}

void GfxDriver::makeCurrent(Handle<HwSwapChain> drawSch, Handle<HwSwapChain> readSch) {
}

void GfxDriver::commit(Handle<HwSwapChain> sch) {
}

void GfxDriver::bindUniformBuffer(uint32_t index, Handle<HwBufferObject> ubh) {
}

void GfxDriver::bindBufferRange(BufferObjectBinding bindingType, uint32_t index,
        Handle<HwBufferObject> ubh, uint32_t offset, uint32_t size) {
}

void GfxDriver::unbindBuffer(BufferObjectBinding bindingType, uint32_t index) {
}

void GfxDriver::bindSamplers(uint32_t index, Handle<HwSamplerGroup> sbh) {
}

void GfxDriver::insertEventMarker(char const* string, uint32_t len) {
}

void GfxDriver::pushGroupMarker(char const* string, uint32_t len) {
}

void GfxDriver::popGroupMarker(int) {
}

void GfxDriver::startCapture(int) {
}

void GfxDriver::stopCapture(int) {
}

void GfxDriver::readPixels(Handle<HwRenderTarget> src,
        uint32_t x, uint32_t y, uint32_t width, uint32_t height,
        PixelBufferDescriptor&& p) {
    scheduleDestroy(std::move(p));
}

void GfxDriver::readBufferSubData(Handle<HwBufferObject> src,
        uint32_t offset, uint32_t size, BufferDescriptor&& p) {
    scheduleDestroy(std::move(p));
}

void GfxDriver::blitDEPRECATED(TargetBufferFlags buffers,
        Handle<HwRenderTarget> dst, Viewport dstRect,
        Handle<HwRenderTarget> src, Viewport srcRect,
        SamplerMagFilter filter) {
}

void GfxDriver::resolve(
        Handle<HwTexture> dst, uint8_t dstLevel, uint8_t dstLayer,
        Handle<HwTexture> src, uint8_t srcLevel, uint8_t srcLayer) {
}

void GfxDriver::blit(
        Handle<HwTexture> dst, uint8_t dstLevel, uint8_t dstLayer, math::uint2 dstOrigin,
        Handle<HwTexture> src, uint8_t srcLevel, uint8_t srcLayer, math::uint2 srcOrigin,
        math::uint2 size) {
}

void GfxDriver::draw(PipelineState pipelineState, Handle<HwRenderPrimitive> rph,
        uint32_t instanceCount) {
}

void GfxDriver::dispatchCompute(Handle<HwProgram> program, math::uint3 workGroupCount) {
}

void GfxDriver::beginTimerQuery(Handle<HwTimerQuery> tqh) {
}

void GfxDriver::endTimerQuery(Handle<HwTimerQuery> tqh) {
}

void GfxDriver::compilePrograms(CompilerPriorityQueue priority,
        CallbackHandler* handler, CallbackHandler::Callback callback, void* user) {
    if (callback) {
        scheduleCallback(handler, user, callback);
    }
}

} // namespace filament::backend

