// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2023-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Core/Mutex.h"
#include "Urho3D/Core/Object.h"
#include "Urho3D/Physics/PhysicsDefs.h"

#include <EASTL/fixed_vector.h>

namespace Urho3D
{

class Model;
class WorkQueue;

/// Base class for collision shape geometry data.
struct CollisionGeometryData : public RefCounted
{
};

/// Cache of collision geometry data.
class URHO3D_API CollisionGeometryDataCache : public Object
{
    URHO3D_OBJECT(CollisionGeometryDataCache, Object);

public:
    CollisionGeometryDataCache(Context* context, ShapeType shapeType);
    ~CollisionGeometryDataCache() override;

    /// Return existing or create new collision geometry. Cached if applicable.
    /// Thread-safe as long as the Model is not being reloaded or modified.
    SharedPtr<CollisionGeometryData> GetOrCreateGeometry(Model* model, unsigned lodLevel);
    /// Return cached geometry. Does not create new geometry. Thread-safe.
    SharedPtr<CollisionGeometryData> GetCachedGeometry(Model* model, unsigned lodLevel) const;

    /// Release cache entry for model.
    void ReleaseCachedGeometry(Model* model);
    /// Prune cache of dead entries.
    void Prune();

private:
    void StoreCachedGeometry(Model* model, unsigned lodLevel, const SharedPtr<CollisionGeometryData>& geometry);
    void SubscribeToReload(Model* model);
    void HandleReloadFinished();

    static SharedPtr<CollisionGeometryData> CreateCollisionGeometryData(
        ShapeType shapeType, Model* model, unsigned lodLevel);

private:
    using LodVector = ea::fixed_vector<WeakPtr<CollisionGeometryData>, 8>;
    using CacheMap = ea::unordered_map<WeakPtr<Model>, LodVector>;

    WeakPtr<WorkQueue> workQueue_;

    ShapeType shapeType_{};
    mutable SpinLockMutex mutex_;
    CacheMap cache_;
};

} // namespace Urho3D
