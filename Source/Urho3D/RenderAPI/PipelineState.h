// Copyright (c) 2017-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Container/Hash.h"
#include "Urho3D/Container/IndexAllocator.h"
#include "Urho3D/Container/RefCounted.h"
#include "Urho3D/IO/FileIdentifier.h"
#include "Urho3D/RenderAPI/DeviceObject.h"
#include "Urho3D/RenderAPI/RawShader.h"
#include "Urho3D/RenderAPI/RenderAPIDefs.h"
#include "Urho3D/RenderAPI/ShaderProgramReflection.h"

#include <Diligent/Graphics/GraphicsEngine/interface/PipelineState.h>
#include <Diligent/Graphics/GraphicsEngine/interface/PipelineStateCache.h>
#include <Diligent/Graphics/GraphicsEngine/interface/ShaderResourceBinding.h>

#include <EASTL/algorithm.h>
#include <EASTL/string.h>
#include <EASTL/unordered_map.h>

namespace Urho3D
{

class Geometry;
class PipelineStateCache;

/// Description structure used to create PipelineState.
/// Should contain all relevant information about input layout,
/// shader resources and parameters and pipeline configuration.
/// PipelineState is automatically updated on shader reload.
/// TODO(diligent): Rework this
struct PipelineStateDesc : public GraphicsPipelineStateDesc
{
    /// Debug info.
    /// @{
    ea::string debugName_{};
    /// @}

    /// Shaders
    /// @{
    SharedPtr<RawShader> vertexShader_;
    SharedPtr<RawShader> pixelShader_;
    SharedPtr<RawShader> domainShader_;
    SharedPtr<RawShader> hullShader_;
    SharedPtr<RawShader> geometryShader_;
    /// @}

    bool operator ==(const PipelineStateDesc& rhs) const
    {
        if (hash_ != rhs.hash_)
            return false;

        return GraphicsPipelineStateDesc::operator==(rhs)
            && vertexShader_ == rhs.vertexShader_
            && pixelShader_ == rhs.pixelShader_
            && geometryShader_ == rhs.geometryShader_
            && hullShader_ == rhs.hullShader_
            && domainShader_ == rhs.domainShader_;
    }

    /// Return whether the description structure is properly initialized.
    bool IsInitialized() const
    {
        return vertexShader_ && pixelShader_;
    }

    void RecalculateHash()
    {
        GraphicsPipelineStateDesc::RecalculateHash();

        CombineHash(hash_, MakeHash(vertexShader_));
        CombineHash(hash_, MakeHash(pixelShader_));
        CombineHash(hash_, MakeHash(domainShader_));
        CombineHash(hash_, MakeHash(hullShader_));
        CombineHash(hash_, MakeHash(geometryShader_));

        // Consider 0-hash invalid
        hash_ = ea::max(1u, hash_);
    }
};

/// Cooked pipeline state. It's not an Object to keep it lightweight.
class URHO3D_API PipelineState : public Object, public DeviceObject, public IDFamily<PipelineState>
{
    URHO3D_OBJECT(PipelineState, Object);

public:
    PipelineState(PipelineStateCache* owner, const PipelineStateDesc& desc);
    ~PipelineState() override;

    /// Implement DeviceObject.
    /// @{
    void Invalidate() override;
    void Restore() override;
    void Destroy() override;
    /// @}

    /// Getters
    /// @{
    bool IsValid() const { return !!reflection_; }
    const PipelineStateDesc& GetDesc() const { return desc_; }
    ShaderProgramReflection* GetReflection() const { return reflection_; }
    Diligent::IPipelineState* GetHandle() const { return handle_; }
    Diligent::IShaderResourceBinding* GetShaderResourceBinding() const { return shaderResourceBinding_; }
    /// @}

private:
    void CreateGPU();
    void DestroyGPU();

    WeakPtr<PipelineStateCache> owner_;
    PipelineStateDesc desc_;

    Diligent::RefCntAutoPtr<Diligent::IPipelineState> handle_{};
    Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> shaderResourceBinding_{};
    // TODO(diligent): We may want to actually share reflection objects between pipeline states.
    SharedPtr<ShaderProgramReflection> reflection_;
};

/// Generic pipeline state cache.
class URHO3D_API PipelineStateCache : public Object, public DeviceObject
{
    URHO3D_OBJECT(PipelineStateCache, Object);

public:
    explicit PipelineStateCache(Context* context);

    /// Initialize pipeline state cache. Optionally loads cached pipeline states from memory blob.
    void Initialize(const ByteVector& cachedData);
    /// Stores cached pipeline states to memory blob.
    const ByteVector& GetCachedData();

    /// Create new or return existing pipeline state. Returned state may be invalid.
    /// Return nullptr if description is malformed.
    SharedPtr<PipelineState> GetPipelineState(PipelineStateDesc desc);
    /// Get GPU Pipeline cache device object.
    Diligent::IPipelineStateCache* GetHandle() { return handle_; }

    /// Internal. Remove pipeline state with given description from cache.
    void ReleasePipelineState(const PipelineStateDesc& desc);

private:
    void UpdateCachedData();

    ByteVector cachedData_;
    Diligent::RefCntAutoPtr<Diligent::IPipelineStateCache> handle_;
    ea::unordered_map<PipelineStateDesc, WeakPtr<PipelineState>> states_;
};

}
