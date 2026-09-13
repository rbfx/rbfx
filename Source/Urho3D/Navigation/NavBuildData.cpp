// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include <cassert>

#include "Urho3D/Navigation/NavBuildData.h"

#include <DetourTileCache/DetourTileCacheBuilder.h>
#include <Detour/DetourAlloc.h>
#include <Recast/Recast.h>

namespace Urho3D
{

void DetourDeleter::operator()(void* p) const noexcept
{
    dtFree(p);
}

void DetourAllocation::Release()
{
    data_.release();
    dataSize_ = 0;
}

NavBuildData::NavBuildData() :
    ctx_(new rcContext(true)),
    heightField_(nullptr),
    compactHeightField_(nullptr)
{
}

NavBuildData::~NavBuildData()
{
    delete(ctx_);
    ctx_ = nullptr;
    rcFreeHeightField(heightField_);
    heightField_ = nullptr;
    rcFreeCompactHeightfield(compactHeightField_);
    compactHeightField_ = nullptr;
}

SimpleNavBuildData::SimpleNavBuildData() :
    NavBuildData(),
    contourSet_(nullptr),
    polyMesh_(nullptr),
    polyMeshDetail_(nullptr)
{
}

SimpleNavBuildData::~SimpleNavBuildData()
{
    rcFreeContourSet(contourSet_);
    contourSet_ = nullptr;
    rcFreePolyMesh(polyMesh_);
    polyMesh_ = nullptr;
    rcFreePolyMeshDetail(polyMeshDetail_);
    polyMeshDetail_ = nullptr;
}

DynamicNavBuildData::DynamicNavBuildData(const ea::shared_ptr<dtTileCacheCompressor>& compressor)
    : compressor_(compressor)
{
}

DynamicNavBuildData::~DynamicNavBuildData()
{
    rcFreeHeightfieldLayerSet(heightFieldLayers_);
    heightFieldLayers_ = nullptr;
}

}
