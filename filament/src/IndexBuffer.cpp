/*
 * Copyright (C) 2015 The Android Open Source Project
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

#include "details/IndexBuffer.h"

#include "details/BufferObject.h"
#include "details/Engine.h"

#include "FilamentAPI-impl.h"

namespace filament {

struct IndexBuffer::BuilderDetails {
    uint32_t mIndexCount = 0;
    IndexType mIndexType = IndexType::UINT;
    bool mBufferObjectEnabled = false;
};

using BuilderType = IndexBuffer;
BuilderType::Builder::Builder() noexcept = default;
BuilderType::Builder::~Builder() noexcept = default;
BuilderType::Builder::Builder(BuilderType::Builder const& rhs) noexcept = default;
BuilderType::Builder::Builder(BuilderType::Builder&& rhs) noexcept = default;
BuilderType::Builder& BuilderType::Builder::operator=(BuilderType::Builder const& rhs) noexcept = default;
BuilderType::Builder& BuilderType::Builder::operator=(BuilderType::Builder&& rhs) noexcept = default;

IndexBuffer::Builder& IndexBuffer::Builder::indexCount(uint32_t indexCount) noexcept {
    mImpl->mIndexCount = indexCount;
    return *this;
}

IndexBuffer::Builder& IndexBuffer::Builder::enableBufferObject(bool enabled) noexcept {
    mImpl->mBufferObjectEnabled = enabled;
    return *this;
}

IndexBuffer::Builder& IndexBuffer::Builder::bufferType(IndexType indexType) noexcept {
    mImpl->mIndexType = indexType;
    return *this;
}

IndexBuffer* IndexBuffer::Builder::build(Engine& engine) {
    return upcast(engine).createIndexBuffer(*this);
}

// ------------------------------------------------------------------------------------------------

FIndexBuffer::FIndexBuffer(FEngine& engine, const IndexBuffer::Builder& builder)
        : mIndexCount(builder->mIndexCount), mBufferObjectEnabled(builder->mBufferObjectEnabled) {
    FEngine::DriverApi& driver = engine.getDriverApi();
    mHandle = driver.createIndexBuffer(
            (backend::ElementType)builder->mIndexType,
            uint32_t(builder->mIndexCount),
            backend::BufferUsage::STATIC);

    // If buffer objects are not enabled at the API level, then we create them internally.
    if (!mBufferObjectEnabled) {
        mObjectHandle = engine.getDriverApi().createBufferObject(
            mIndexCount * ((builder->mIndexType == IndexType::UINT) ? 4U : 2U),
            filament::backend::BufferObjectBinding::INDEX,
            backend::BufferUsage::STATIC);
            engine.getDriverApi().setIndexBufferObject(mHandle, mObjectHandle);
    }
}

void FIndexBuffer::terminate(FEngine& engine) {
    FEngine::DriverApi& driver = engine.getDriverApi();
    if (!mBufferObjectEnabled) {
        driver.destroyBufferObject(mObjectHandle);
    }
    driver.destroyIndexBuffer(mHandle);
}

void FIndexBuffer::setBuffer(FEngine& engine, BufferDescriptor&& buffer, uint32_t byteOffset) {
    ASSERT_PRECONDITION(!mBufferObjectEnabled, "Please use setBufferObject()");
    engine.getDriverApi().updateBufferObject(mObjectHandle, std::move(buffer), byteOffset);
}

void FIndexBuffer::setBufferObject(FEngine& engine, FBufferObject const* bufferObject) {
    ASSERT_PRECONDITION(mBufferObjectEnabled, "Please use setBuffer()");
    ASSERT_PRECONDITION(bufferObject->getBindingType() == BufferObject::BindingType::INDEX,
        "Binding type must be INDEX.");
    auto hwBufferObject = bufferObject->getHwHandle();
    engine.getDriverApi().setIndexBufferObject(mHandle, hwBufferObject);
}

// ------------------------------------------------------------------------------------------------
// Trampoline calling into private implementation
// ------------------------------------------------------------------------------------------------

void IndexBuffer::setBuffer(Engine& engine,
        IndexBuffer::BufferDescriptor&& buffer, uint32_t byteOffset) {
    upcast(this)->setBuffer(upcast(engine), std::move(buffer), byteOffset);
}

void IndexBuffer::setBufferObject(Engine& engine, BufferObject const* bufferObject) {
    upcast(this)->setBufferObject(upcast(engine), upcast(bufferObject));
}

size_t IndexBuffer::getIndexCount() const noexcept {
    return upcast(this)->getIndexCount();
}

} // namespace filament
