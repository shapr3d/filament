/*
 * Copyright (C) 2022 The Android Open Source Project
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

#ifndef TNT_FILAMENT_HWRENDERPRIMITIVEFACTORY_H
#define TNT_FILAMENT_HWRENDERPRIMITIVEFACTORY_H

#include "Bimap.h"

#include <backend/DriverApiForward.h>
#include <backend/DriverEnums.h>
#include <backend/Handle.h>

#include <utils/Allocator.h>

#include <functional>

#include <stddef.h>
#include <stdint.h>

namespace filament {

class FEngine;

class HwRenderPrimitiveFactory {
public:
    using Handle = backend::RenderPrimitiveHandle;

    HwRenderPrimitiveFactory();
    ~HwRenderPrimitiveFactory() noexcept;

    HwRenderPrimitiveFactory(HwRenderPrimitiveFactory const& rhs) = delete;
    HwRenderPrimitiveFactory(HwRenderPrimitiveFactory&& rhs) noexcept = delete;
    HwRenderPrimitiveFactory& operator=(HwRenderPrimitiveFactory const& rhs) = delete;
    HwRenderPrimitiveFactory& operator=(HwRenderPrimitiveFactory&& rhs) noexcept = delete;

    void terminate(backend::DriverApi& driver) noexcept;

    struct Parameters { // 20 bytes
        backend::VertexBufferHandle vbh;            // 4
        backend::IndexBufferHandle ibh;             // 4
        backend::PrimitiveType type;                // 4
    };

    Handle create(backend::DriverApi& driver,
            backend::VertexBufferHandle vbh,
            backend::IndexBufferHandle ibh,
            backend::PrimitiveType type) noexcept;

    void destroy(backend::DriverApi& driver, Handle handle) noexcept;

private:
    struct Key {
        Parameters params;
        mutable uint32_t refs;  // 4 bytes
    };

    struct KeyHash {
        size_t operator()(Key const& p) const noexcept;
    };

    friend bool operator==(Key const& lhs, Key const& rhs) noexcept;


    struct Value { // 4 bytes
        Handle handle;
    };

    struct ValueHash {
        size_t operator()(Value const& p) const noexcept {
            std::hash<Handle::HandleId> const h;
            return h(p.handle.getId());
        }
    };

    friend bool operator==(Value const& lhs, Value const& rhs) noexcept {
        return lhs.handle == rhs.handle;
    }

    // Size of the arena used for the "set" part of the bimap

    // Shapr: Each entry in this set corresponds to a RenderableBatch.
    // We saw workspaces with over 100K batches, so we had to increase the arena size from 4 to 16 MBs to accommodate 100K+ entires.
    // The pool below uses 64B entries so 16 MB is enough for ~260K entries. Watch out when bumping Filament!
    // The pool entry size was replaced with sizeof(Key) in v1.50.5 and arena size was decreased in v1.55.0!
    static constexpr size_t SET_ARENA_SIZE = 16 * 1024 * 1024;

    // Arena for the set<>, using a pool allocator inside a heap area.
    using PoolAllocatorArena = utils::Arena<
            utils::PoolAllocatorWithFallback<sizeof(Key)>,
            utils::LockingPolicy::NoLock,
            utils::TrackingPolicy::Untracked,
            utils::AreaPolicy::HeapArea>;


    // Arena where the set memory is allocated
    PoolAllocatorArena mArena;

    // The special Bimap
    Bimap<Key, Value, KeyHash, ValueHash,
            utils::STLAllocator<Key, PoolAllocatorArena>> mBimap;
};

} // namespace filament

#endif // TNT_FILAMENT_HWRENDERPRIMITIVEFACTORY_H
