//
// Copyright (c) 2022-2022 the rbfx project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#pragma once

#include "../Graphics/PipelineState.h"
#include "../RenderPipeline/PostProcessPass.h"
#include "../RenderPipeline/RenderBuffer.h"
#include "../RenderPipeline/RenderPipelineDefs.h"
#include "../Graphics/Texture2D.h"

#include <EASTL/optional.h>

namespace Urho3D
{

class RenderBufferManager;
class RenderPipelineInterface;

/// Post-processing pass that adjusts HDR scene exposure.
class URHO3D_API AmbientOcclusionPass : public PostProcessPass
{
    URHO3D_OBJECT(AmbientOcclusionPass, PostProcessPass)

#ifdef DESKTOP_GRAPHICS
public:
    AmbientOcclusionPass(RenderPipelineInterface* renderPipeline, RenderBufferManager* renderBufferManager);

    void SetSettings(const AmbientOcclusionPassSettings& settings);

    PostProcessPassFlags GetExecutionFlags() const override
    {
        return PostProcessPassFlag::NeedColorOutputReadAndWrite | PostProcessPassFlag::NeedColorOutputBilinear;
    }

    void SetNormalBuffer(RenderBuffer* normalBuffer);

    void Execute(Camera* camera) override;

protected:

    void InitializeTextures();
    void InitializeStates();
    void EvaluateAO(ea::span<ShaderParameterDesc> shaderParameters);
    void BlurTexture(ea::span<ShaderParameterDesc> shaderParameters);
    void Blit(ea::span<ShaderParameterDesc> shaderParameters, PipelineState* state);

    AmbientOcclusionPassSettings settings_;

    SharedPtr<RenderBuffer> normalBuffer_;

    struct CachedTextures
    {
        SharedPtr<Texture2D> noise_;
        SharedPtr<RenderBuffer> currentTarget_;
        SharedPtr<RenderBuffer> previousTarget_;
    };
    CachedTextures textures_;

    struct CachedStates
    {
        SharedPtr<PipelineState> ssao_;
        SharedPtr<PipelineState> ssao_deferred_;
        SharedPtr<PipelineState> blur_;
        SharedPtr<PipelineState> blur_deferred_;
        SharedPtr<PipelineState> combine_;
        SharedPtr<PipelineState> preview_;

        bool IsValid()
        {
            return ssao_ && ssao_->IsValid() && blur_->IsValid() && ssao_deferred_->IsValid()
                && blur_deferred_->IsValid() && combine_->IsValid() && preview_->IsValid();
        }
    };
    ea::optional<CachedStates> pipelineStates_;

#endif
};

} // namespace Urho3D

