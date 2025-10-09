// Copyright (c) 2025-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Physics/CollisionGeometryDataCache.h"

#include "Urho3D/Core/WorkQueue.h"
#include "Urho3D/Graphics/Geometry.h"
#include "Urho3D/Graphics/IndexBuffer.h"
#include "Urho3D/Graphics/Model.h"
#include "Urho3D/Graphics/VertexBuffer.h"
#include "Urho3D/Resource/ResourceEvents.h"

namespace Urho3D
{

namespace
{

bool HasDynamicBuffers(Model* model, unsigned lodLevel)
{
    for (unsigned i = 0; i < model->GetNumGeometries(); ++i)
    {
        Geometry* geometry = model->GetGeometry(i, lodLevel);
        if (!geometry)
            continue;

        for (VertexBuffer* buffer : geometry->GetVertexBuffers())
        {
            if (buffer && buffer->IsDynamic())
                return true;
        }

        IndexBuffer* buffer = geometry->GetIndexBuffer();
        if (buffer && buffer->IsDynamic())
            return true;
    }

    return false;
}

} // namespace

CollisionGeometryDataCache::CollisionGeometryDataCache(Context* context, ShapeType shapeType)
    : Object(context)
    , workQueue_{context->GetSubsystem<WorkQueue>()}
    , shapeType_{shapeType}
{
}

CollisionGeometryDataCache::~CollisionGeometryDataCache()
{
}

SharedPtr<CollisionGeometryData> CollisionGeometryDataCache::GetOrCreateGeometry(Model* model, unsigned lodLevel)
{
    if (const auto cachedGeometry = GetCachedGeometry(model, lodLevel))
        return cachedGeometry;

    const auto geometry = CreateCollisionGeometryData(shapeType_, model, lodLevel);

    if (!HasDynamicBuffers(model, lodLevel))
    {
        StoreCachedGeometry(model, lodLevel, geometry);

        WeakPtr<CollisionGeometryDataCache> weakSelf{this};
        WeakPtr<Model> weakModel{model};
        workQueue_->PostTaskForMainThread([weakSelf, weakModel]
        {
            if (const auto self = weakSelf.Lock())
            {
                if (const auto model = weakModel.Lock())
                    self->SubscribeToReload(model);
            }
        });
    }

    return geometry;
}

SharedPtr<CollisionGeometryData> CollisionGeometryDataCache::GetCachedGeometry(Model* model, unsigned lodLevel) const
{
    MutexLock lock{mutex_};
    const auto iter = cache_.find(WeakPtr<Model>{model});
    return (iter != cache_.end() && lodLevel < iter->second.size()) ? iter->second[lodLevel].Lock() : nullptr;
}

void CollisionGeometryDataCache::ReleaseCachedGeometry(Model* model)
{
    MutexLock lock{mutex_};
    cache_.erase(WeakPtr<Model>{model});
}

void CollisionGeometryDataCache::Prune()
{
    MutexLock lock{mutex_};
    ea::erase_if(cache_, [](CacheMap::value_type& element)
    {
        if (!element.first)
            return true;

        const auto isNull = [](CollisionGeometryData* ptr) { return ptr == nullptr; };
        return ea::all_of(element.second.begin(), element.second.end(), isNull);
    });
}

void CollisionGeometryDataCache::StoreCachedGeometry(
    Model* model, unsigned lodLevel, const SharedPtr<CollisionGeometryData>& geometry)
{
    MutexLock lock{mutex_};
    LodVector& lodVector = cache_[WeakPtr<Model>{model}];
    if (lodVector.size() <= lodLevel)
        lodVector.resize(lodLevel + 1);
    lodVector[lodLevel] = geometry;
}

void CollisionGeometryDataCache::SubscribeToReload(Model* model)
{
    SubscribeToEvent(model, E_RELOADFINISHED, &CollisionGeometryDataCache::HandleReloadFinished);
}

void CollisionGeometryDataCache::HandleReloadFinished()
{
    if (const auto model = GetEventSender()->Cast<Model>())
        ReleaseCachedGeometry(model);
}

} // namespace Urho3D
