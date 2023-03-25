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
#ifdef URHO3D_DEBUG
        ea::string dbgName = Format("{}#{}", desc_.debugName_, desc_.ToHash());
        ci.PSODesc.Name = dbgName.c_str();
#endif
        ci.GraphicsPipeline.PrimitiveTopology = DiligentPrimitiveTopology[desc_.primitiveType_];

        {
            assert(desc_.vertexShader_);
            // Create LayoutElement based vertex shader input layout
            const ea::vector<VertexElement>& shaderVertexElements = desc_.vertexShader_->GetVertexElements();
            ea::vector<VertexElement> vertexBufferElements(desc_.vertexElements_.begin(), desc_.vertexElements_.begin() + desc_.numVertexElements_);
            unsigned attribCount = 0;

            /*ea::string dbgShaderVertexElements = "";
            ea::string dbgVertexBufferElements = "";

            for (auto shaderVertexElement = shaderVertexElements.begin(); shaderVertexElement != shaderVertexElements.end(); ++shaderVertexElement)
                dbgShaderVertexElements.append(Format("Semantic: {} | Index: {} | Offset: {}\n", elementSemanticNames[shaderVertexElement->semantic_], shaderVertexElement->index_, shaderVertexElement->offset_));
            for (auto vertexBufferElement = vertexBufferElements.begin(); vertexBufferElement != vertexBufferElements.end(); ++vertexBufferElement)
                dbgVertexBufferElements.append(Format("Semantic: {} | Index: {} | Offset: {}\n", elementSemanticNames[vertexBufferElement->semantic_], vertexBufferElement->index_, vertexBufferElement->offset_));*/
            auto BuildLayoutElement = [=](const VertexElement* element, LayoutElement& layoutElement) {
                LayoutElement result = {};
                layoutElement.RelativeOffset = element->offset_;
                layoutElement.NumComponents = sNumComponents[element->type_];
                if (layoutElement.NumComponents == 0)
                    __debugbreak();
                layoutElement.ValueType = sValueTypes[element->type_];
                if (element->semantic_ == SEM_BLENDINDICES)
                    layoutElement.ValueType = VT_UINT8;
                layoutElement.IsNormalized = element->semantic_ == SEM_COLOR;
                layoutElement.BufferSlot = element->perInstance_ ? 1 : 0;
                layoutElement.Frequency = element->perInstance_ ? INPUT_ELEMENT_FREQUENCY_PER_INSTANCE : INPUT_ELEMENT_FREQUENCY_PER_VERTEX;
            };
            for (const VertexElement* shaderVertexElement = shaderVertexElements.begin(); shaderVertexElement != shaderVertexElements.end(); ++shaderVertexElement) {
                bool insert = false;
                LayoutElement layoutElement = {};
                layoutElement.InputIndex = attribCount;

                for (const VertexElement* vertexBufferElement = vertexBufferElements.begin(); vertexBufferElement != vertexBufferElements.end(); ++vertexBufferElement) {
                    if (vertexBufferElement->semantic_ != shaderVertexElement->semantic_ || vertexBufferElement->index_ != shaderVertexElement->index_)
                        continue;
                    BuildLayoutElement(vertexBufferElement, layoutElement);
                    // Remove from vector processed vertex element
                    vertexBufferElements.erase(vertexBufferElement);
                    insert = true;
                    break;
                }

                if (insert) {
                    layoutElements.push_back(layoutElement);
                    ++attribCount;
                }
            }
            // Add last semantics if has left vertex buffer elements
            for (const VertexElement* element = vertexBufferElements.begin(); element != vertexBufferElements.end(); ++element) {
                LayoutElement layoutElement = {};
                layoutElement.InputIndex = attribCount;
                BuildLayoutElement(element, layoutElement);
                layoutElements.push_back(layoutElement);
                ++attribCount;
            }

            assert(layoutElements.size());
        }

        ci.GraphicsPipeline.InputLayout.NumElements = layoutElements.size();
        ci.GraphicsPipeline.InputLayout.LayoutElements = layoutElements.data();

        ci.GraphicsPipeline.NumRenderTargets = desc_.renderTargetsFormats_.size();
        for (unsigned i = 0; i < ci.GraphicsPipeline.NumRenderTargets; ++i)
            ci.GraphicsPipeline.RTVFormats[i] = (TEXTURE_FORMAT)desc_.renderTargetsFormats_[i];
        ci.GraphicsPipeline.DSVFormat = (TEXTURE_FORMAT)desc_.depthStencilFormat_;

        if (desc_.vertexShader_)
            ci.pVS = desc_.vertexShader_->GetGPUObject().Cast<IShader>(IID_Shader);
        if (desc_.pixelShader_)
            ci.pPS = desc_.pixelShader_->GetGPUObject().Cast<IShader>(IID_Shader);
        if (desc_.domainShader_)
            ci.pDS = desc_.domainShader_->GetGPUObject().Cast<IShader>(IID_Shader);
        if (desc_.hullShader_)
            ci.pHS = desc_.hullShader_->GetGPUObject().Cast<IShader>(IID_Shader);
        if (desc_.geometryShader_)
            ci.pGS = desc_.geometryShader_->GetGPUObject().Cast<IShader>(IID_Shader);

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

        graphics->GetImpl()->GetDevice()->CreateGraphicsPipelineState(ci, &pipeline_);

        assert(pipeline_ != nullptr);

        URHO3D_LOGDEBUGF("Created Graphics Pipeline (%d)", desc_.ToHash());
    }
    void PipelineState::ReleasePipeline() {
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
