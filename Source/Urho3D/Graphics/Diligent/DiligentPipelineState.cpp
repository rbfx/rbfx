#include "../PipelineState.h"
#include <Diligent/Graphics/GraphicsEngine/interface/PipelineState.h>
#include "DiligentLookupSettings.h"
#include "DiligentGraphicsImpl.h"

namespace Urho3D
{
    static unsigned sNumComponents[] = {
        1, // TYPE_INT
        1, // TYPE_FLOAT
        2, // TYPE_VECTOR2
        3, // TYPE_VECTOR3
        4, // TYPE_VECTOR4
        4, // TYPE_UBYTE4
        4, // TYPE_UBYTE4_NORM
    };
    static VALUE_TYPE sValueTypes[] = {
        VT_INT32,
        VT_FLOAT32,
        VT_FLOAT32,
        VT_FLOAT32,
        VT_FLOAT32,
        VT_UINT32,
        VT_UINT8
    };
    using namespace Diligent;
    void PipelineState::BuildPipeline(Graphics* graphics) {
        if (pipeline_)
            return;
        ea::vector<LayoutElement> layoutElements;
        GraphicsPipelineStateCreateInfo ci;
        ci.PSODesc.Name = desc_.debugName_.c_str();
        ci.GraphicsPipeline.PrimitiveTopology = DiligentPrimitiveTopology[desc_.primitiveType_];

        /*for (unsigned i = 0; i < desc_.numVertexElements_; ++i) {
            VertexElement vElement = desc_.vertexElements_[i];
            LayoutElement element = {};

            element.InputIndex = i;
            element.NumComponents = sNumComponents[vElement.type_];
            element.ValueType = sValueTypes[vElement.type_];
            if (vElement.semantic_ == SEM_COLOR)
                element.IsNormalized = true;
            else
                element.IsNormalized = false;
            element.Frequency = vElement.perInstance_ ? INPUT_ELEMENT_FREQUENCY_PER_INSTANCE : INPUT_ELEMENT_FREQUENCY_PER_VERTEX;
            element.InstanceDataStepRate = vElement.perInstance_ ? 1 : 0;

            layoutElements.push_back(element);
        }*/
        {
            assert(desc_.vertexShader_);
            // Create LayoutElement based vertex shader input layout
            const ea::vector<VertexElement>& vertexElements = desc_.vertexShader_->GetVertexElements();
            ea::vector<VertexElement> vertexBufferElements(desc_.vertexElements_.begin(), desc_.vertexElements_.begin() + desc_.numVertexElements_);
            unsigned attribCount = 0;
            for (const VertexElement* shaderVertexElement = vertexElements.begin(); shaderVertexElement != vertexElements.end(); ++shaderVertexElement) {
                LayoutElement layoutElement = {};
                layoutElement.InputIndex = attribCount;

                for (const VertexElement* vertexBufferElement = vertexBufferElements.begin(); vertexBufferElement != vertexBufferElements.end(); ++vertexBufferElement) {
                    if (vertexBufferElement->semantic_ != shaderVertexElement->semantic_)
                        continue;
                    layoutElement.RelativeOffset = vertexBufferElement->offset_;
                    layoutElement.NumComponents = sNumComponents[vertexBufferElement->type_];
                    layoutElement.ValueType = sValueTypes[vertexBufferElement->type_];
                    layoutElement.IsNormalized = vertexBufferElement->semantic_ == SEM_COLOR;
                    layoutElement.Frequency = vertexBufferElement->perInstance_ ? INPUT_ELEMENT_FREQUENCY_PER_INSTANCE : INPUT_ELEMENT_FREQUENCY_PER_VERTEX;
                    // Remove from vector processed vertex element
                    vertexBufferElements.erase(vertexBufferElement);
                    break;
                }

                layoutElements.push_back(layoutElement);
                ++attribCount;
            }

        }

        ci.GraphicsPipeline.InputLayout.NumElements = layoutElements.size();
        ci.GraphicsPipeline.InputLayout.LayoutElements = layoutElements.data();

        ci.GraphicsPipeline.NumRenderTargets = desc_.renderTargetsFormats_.size();
        for (unsigned i = 0; i < ci.GraphicsPipeline.NumRenderTargets; ++i)
            ci.GraphicsPipeline.RTVFormats[i] = (TEXTURE_FORMAT)desc_.renderTargetsFormats_[i];
        ci.GraphicsPipeline.DSVFormat = (TEXTURE_FORMAT)desc_.depthStencilFormat_;

        if (desc_.vertexShader_)
            ci.pVS = (IShader*)desc_.vertexShader_->GetGPUObject();
        if (desc_.pixelShader_)
            ci.pPS = (IShader*)desc_.pixelShader_->GetGPUObject();
        if (desc_.domainShader_)
            ci.pDS = (IShader*)desc_.domainShader_->GetGPUObject();
        if (desc_.hullShader_)
            ci.pHS = (IShader*)desc_.hullShader_->GetGPUObject();
        if (desc_.geometryShader_)
            ci.pGS = (IShader*)desc_.geometryShader_->GetGPUObject();

        ci.GraphicsPipeline.BlendDesc.AlphaToCoverageEnable = desc_.alphaToCoverageEnabled_;
        ci.GraphicsPipeline.BlendDesc.IndependentBlendEnable = false;
        if (ci.GraphicsPipeline.NumRenderTargets > 0) {
            ci.GraphicsPipeline.BlendDesc.RenderTargets[0].BlendEnable = DiligentBlendEnable[desc_.blendMode_];
            ci.GraphicsPipeline.BlendDesc.RenderTargets[0].SrcBlend = DiligentSrcBlend[desc_.blendMode_];
            ci.GraphicsPipeline.BlendDesc.RenderTargets[0].DestBlend = DiligentDestBlend[desc_.blendMode_];
            ci.GraphicsPipeline.BlendDesc.RenderTargets[0].BlendOp = DiligentBlendOp[desc_.blendMode_];
            ci.GraphicsPipeline.BlendDesc.RenderTargets[0].SrcBlendAlpha = DiligentSrcAlphaBlend[desc_.blendMode_];
            ci.GraphicsPipeline.BlendDesc.RenderTargets[0].DestBlendAlpha = DiligentDestAlphaBlend[desc_.blendMode_];
            ci.GraphicsPipeline.BlendDesc.RenderTargets[0].BlendOpAlpha = DiligentBlendOp[desc_.blendMode_];
            ci.GraphicsPipeline.BlendDesc.RenderTargets[0].RenderTargetWriteMask = desc_.colorWriteEnabled_ ? COLOR_MASK_ALL : COLOR_MASK_NONE;
        }

        ci.GraphicsPipeline.DepthStencilDesc.DepthEnable = true;
        ci.GraphicsPipeline.DepthStencilDesc.DepthWriteEnable = desc_.depthWriteEnabled_;
        ci.GraphicsPipeline.DepthStencilDesc.DepthFunc = DiligentCmpFunc[desc_.depthCompareFunction_];
        ci.GraphicsPipeline.DepthStencilDesc.StencilEnable = desc_.stencilTestEnabled_;
        ci.GraphicsPipeline.DepthStencilDesc.StencilReadMask = desc_.stencilCompareMask_;
        ci.GraphicsPipeline.DepthStencilDesc.StencilWriteMask = desc_.stencilWriteMask_;
        ci.GraphicsPipeline.DepthStencilDesc.FrontFace.StencilFailOp = sStencilOp[desc_.stencilOperationOnStencilFailed_];
        ci.GraphicsPipeline.DepthStencilDesc.FrontFace.StencilDepthFailOp = sStencilOp[desc_.stencilOperationOnDepthFailed_];
        ci.GraphicsPipeline.DepthStencilDesc.FrontFace.StencilPassOp = sStencilOp[desc_.stencilOperationOnPassed_];
        ci.GraphicsPipeline.DepthStencilDesc.FrontFace.StencilFunc = DiligentCmpFunc[desc_.stencilCompareFunction_];
        ci.GraphicsPipeline.DepthStencilDesc.BackFace.StencilFailOp = sStencilOp[desc_.stencilOperationOnStencilFailed_];
        ci.GraphicsPipeline.DepthStencilDesc.BackFace.StencilDepthFailOp = sStencilOp[desc_.stencilOperationOnDepthFailed_];
        ci.GraphicsPipeline.DepthStencilDesc.BackFace.StencilPassOp = sStencilOp[desc_.stencilOperationOnPassed_];
        ci.GraphicsPipeline.DepthStencilDesc.BackFace.StencilFunc = DiligentCmpFunc[desc_.stencilCompareFunction_];

        unsigned depthBits = 24;
        if (ci.GraphicsPipeline.DSVFormat == TEX_FORMAT_R16_TYPELESS)
            depthBits = 16;
        int scaledDepthBias = (int)(desc_.constantDepthBias_ * (1 << depthBits));

        ci.GraphicsPipeline.RasterizerDesc.FillMode = DiligentFillMode[desc_.fillMode_];
        ci.GraphicsPipeline.RasterizerDesc.CullMode = DiligentCullMode[desc_.cullMode_];
        ci.GraphicsPipeline.RasterizerDesc.FrontCounterClockwise = false;
        ci.GraphicsPipeline.RasterizerDesc.DepthBias = scaledDepthBias;
        ci.GraphicsPipeline.RasterizerDesc.DepthBiasClamp = M_INFINITY;
        ci.GraphicsPipeline.RasterizerDesc.SlopeScaledDepthBias = desc_.slopeScaledDepthBias_;
        ci.GraphicsPipeline.RasterizerDesc.DepthClipEnable = true;
        ci.GraphicsPipeline.RasterizerDesc.ScissorEnable = desc_.scissorTestEnabled_;
        ci.GraphicsPipeline.RasterizerDesc.AntialiasedLineEnable = desc_.lineAntiAlias_;

        ci.GraphicsPipeline.RasterizerDesc.ScissorEnable = desc_.scissorTestEnabled_;

        ci.PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;

        IPipelineState* pipelineState = nullptr;
        graphics->GetImpl()->GetDevice()->CreateGraphicsPipelineState(ci, &pipelineState);

        assert(pipelineState);

        URHO3D_LOGDEBUGF("Created Graphics Pipeline (%d)", desc_.ToHash());
        pipeline_ = pipelineState;
    }
    void PipelineState::ReleasePipeline() {
        if (pipeline_)
            static_cast<IPipelineState*>(pipeline_)->Release();
        pipeline_ = nullptr;
    }

    ShaderResourceBinding* PipelineState::CreateInternalSRB() {
        assert(pipeline_);

        IPipelineState* pipeline = static_cast<IPipelineState*>(pipeline_);
        IShaderResourceBinding* srb = nullptr;
        pipeline->CreateShaderResourceBinding(&srb, false);
        assert(srb);

        return new ShaderResourceBinding(owner_->GetSubsystem<Graphics>(), srb);
    }
}
