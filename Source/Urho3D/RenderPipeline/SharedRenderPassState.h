// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Core/NonCopyable.h"
#include "Urho3D/Scene/Serializable.h"

#include <EASTL/fixed_hash_map.h>

namespace Urho3D
{

class Camera;
class RenderBuffer;
class RenderBufferManager;
class RenderPipelineInterface;

/// State of render pipeline that can be accessed by render path and render passes.
struct URHO3D_API SharedRenderPassState : public NonCopyable
{
    static constexpr unsigned MaxRenderBuffers = 128;

    static constexpr StringHash AlbedoBufferId = "GeometryBuffer.Albedo"_sh;
    static constexpr StringHash SpecularBufferId = "GeometryBuffer.Specular"_sh;
    static constexpr StringHash NormalBufferId = "GeometryBuffer.Normal"_sh;

    RenderPipelineInterface* renderPipelineInterface_{};
    WeakPtr<Camera> renderCamera_;
    SharedPtr<RenderBufferManager> renderBufferManager_;
    ea::fixed_hash_map<StringHash, SharedPtr<RenderBuffer>, MaxRenderBuffers> renderBuffers_;

    SharedRenderPassState();
    ~SharedRenderPassState();
};

} // namespace Urho3D
