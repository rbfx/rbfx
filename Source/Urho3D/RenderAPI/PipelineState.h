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
#include <EASTL/variant.h>
#include <EASTL/unordered_map.h>

namespace Urho3D
{

class Geometry;
class PipelineStateCache;

/// Description of graphics pipeline state.
struct URHO3D_API GraphicsPipelineStateDesc
{
    ea::string debugName_{};

    /// Blend state.
    /// @{
    bool colorWriteEnabled_{};
    BlendMode blendMode_{};
    bool alphaToCoverageEnabled_{};
    /// @}

    /// Rasterizer state.
    /// @{
    FillMode fillMode_{};
    CullMode cullMode_{};
    float constantDepthBias_{};
    float slopeScaledDepthBias_{};
    bool scissorTestEnabled_{};
    bool lineAntiAlias_{};
    /// @}

    /// Depth-stencil state.
    /// @{
    bool depthWriteEnabled_{};
    bool stencilTestEnabled_{};
    CompareMode depthCompareFunction_{};
    CompareMode stencilCompareFunction_{};
    StencilOp stencilOperationOnPassed_{};
    StencilOp stencilOperationOnStencilFailed_{};
    StencilOp stencilOperationOnDepthFailed_{};
    unsigned stencilCompareMask_{};
    unsigned stencilWriteMask_{};
    /// @}

    /// Input layout.
    InputLayoutDesc inputLayout_;
    /// Primitive topology.
    PrimitiveType primitiveType_{};
    /// Render Target(s) and Depth Stencil formats.
    PipelineStateOutputDesc output_;
    /// Immutable Samplers.
    ImmutableSamplersDesc samplers_;
    /// Whether to use depth-stencil in read-only mode.
    bool readOnlyDepth_{};

    /// Shaders
    /// @{
    SharedPtr<RawShader> vertexShader_;
    SharedPtr<RawShader> pixelShader_;
    SharedPtr<RawShader> domainShader_;
    SharedPtr<RawShader> hullShader_;
    SharedPtr<RawShader> geometryShader_;
    /// @}

    /// Operators.
    /// @{
    auto Tie() const
    {
        return ea::tie( //
            colorWriteEnabled_, //
            blendMode_, //
            alphaToCoverageEnabled_, //
            fillMode_, //
            cullMode_, //
            constantDepthBias_, //
            slopeScaledDepthBias_, //
            scissorTestEnabled_, //
            lineAntiAlias_, //
            depthWriteEnabled_, //
            stencilTestEnabled_, //
            depthCompareFunction_, //
            stencilCompareFunction_, //
            stencilOperationOnPassed_, //
            stencilOperationOnStencilFailed_, //
            stencilOperationOnDepthFailed_, //
            stencilCompareMask_, //
            stencilWriteMask_, //
            inputLayout_, //
            primitiveType_, //
            output_, //
            samplers_, //
            readOnlyDepth_, //
            vertexShader_, //
            pixelShader_, //
            domainShader_, //
            hullShader_, //
            geometryShader_ //
        );
    }

    bool operator==(const GraphicsPipelineStateDesc& rhs) const { return Tie() == rhs.Tie(); }
    bool operator!=(const GraphicsPipelineStateDesc& rhs) const { return Tie() != rhs.Tie(); }
    unsigned ToHash() const { return ea::max(1u, MakeHash(Tie())); }
    bool IsInitialized() const { return vertexShader_ && pixelShader_; }
    /// @}
};

/// Description of compute pipeline state.
struct URHO3D_API ComputePipelineStateDesc
{
    ea::string debugName_{};

    /// Immutable Samplers.
    ImmutableSamplersDesc samplers_;
    /// Compute shader.
    SharedPtr<RawShader> computeShader_;

    /// Operators.
    /// @{
    auto Tie() const { return ea::tie(samplers_, computeShader_); }

    bool operator==(const ComputePipelineStateDesc& rhs) const { return Tie() == rhs.Tie(); }
    bool operator!=(const ComputePipelineStateDesc& rhs) const { return Tie() != rhs.Tie(); }
    unsigned ToHash() const { return ea::max(1u, MakeHash(Tie())); }
    bool IsInitialized() const { return computeShader_; }
    /// @}
};

class URHO3D_API PipelineStateDesc
{
public:
    PipelineStateDesc() = default;
    PipelineStateDesc(const GraphicsPipelineStateDesc& desc) : desc_(desc), hash_(desc.ToHash()) {}
    PipelineStateDesc(const ComputePipelineStateDesc& desc) : desc_(desc), hash_(desc.ToHash()) {}

    unsigned ToHash() const { return hash_; }

    PipelineStateType GetType() const { return static_cast<PipelineStateType>(desc_.index()); }
    const ea::string& GetDebugName() const;
    const GraphicsPipelineStateDesc* AsGraphics() const;
    const ComputePipelineStateDesc* AsCompute() const;

    /// Operators.
    /// @{
    auto Tie() const { return ea::tie(hash_, desc_); }

    bool operator==(const PipelineStateDesc& rhs) const { return Tie() == rhs.Tie(); }
    bool operator!=(const PipelineStateDesc& rhs) const { return Tie() != rhs.Tie(); }
    /// @}

private:
    ea::variant<GraphicsPipelineStateDesc, ComputePipelineStateDesc> desc_;
    unsigned hash_{};
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
    PipelineStateType GetPipelineType() const { return desc_.GetType(); }
    const PipelineStateDesc& GetDesc() const { return desc_; }
    ShaderProgramReflection* GetReflection() const { return reflection_; }
    Diligent::IPipelineState* GetHandle() const { return handle_; }
    Diligent::IShaderResourceBinding* GetShaderResourceBinding() const { return shaderResourceBinding_; }
    /// @}

private:
    void CreateGPU();
    void CreateGPU(const GraphicsPipelineStateDesc& desc);
    void CreateGPU(const ComputePipelineStateDesc& desc);
    void DestroyGPU();

    WeakPtr<PipelineStateCache> owner_;
    PipelineStateDesc desc_;

    Diligent::RefCntAutoPtr<Diligent::IPipelineState> handle_{};
    Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> shaderResourceBinding_{};
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
    SharedPtr<PipelineState> GetPipelineState(const PipelineStateDesc& desc);
    /// Get GPU Pipeline cache device object.
    Diligent::IPipelineStateCache* GetHandle() { return handle_; }

    /// Create new or return existing graphics pipeline state.
    SharedPtr<PipelineState> GetGraphicsPipelineState(const GraphicsPipelineStateDesc& desc);
    /// Create new or return existing compute pipeline state.
    SharedPtr<PipelineState> GetComputePipelineState(const ComputePipelineStateDesc& desc);

    /// Internal. Remove pipeline state with given description from cache.
    void ReleasePipelineState(const PipelineStateDesc& desc);

private:
    void UpdateCachedData();

    ByteVector cachedData_;
    Diligent::RefCntAutoPtr<Diligent::IPipelineStateCache> handle_;
    ea::unordered_map<PipelineStateDesc, WeakPtr<PipelineState>> states_;
};

}
