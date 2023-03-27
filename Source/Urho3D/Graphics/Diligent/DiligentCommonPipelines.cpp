#include "DiligentCommonPipelines.h"
#include "DiligentLookupSettings.h"
#include "DiligentConstantBufferManager.h"
namespace Urho3D
{
    using namespace Diligent;

    PipelineHolder::PipelineHolder(IPipelineState* pipeline) : pipeline_(pipeline), shaderRes_(nullptr) {}
    PipelineHolder::~PipelineHolder()
    {
        if (shaderRes_)
            shaderRes_->Release();
        pipeline_->Release();
    }

    Diligent::IPipelineState* PipelineHolder::GetPipeline()
    {
        return pipeline_;
    }
    Diligent::IShaderResourceBinding* PipelineHolder::GetShaderResourceBinding()
    {
        if (!shaderRes_)
            pipeline_->CreateShaderResourceBinding(&shaderRes_, true);
        return shaderRes_;
    }

    DiligentCommonPipelines::DiligentCommonPipelines(Graphics* graphics) :
        graphics_(graphics),
        clearVS_(nullptr),
        clearPS_(nullptr) {

    }
    void DiligentCommonPipelines::Release()
    {
        URHO3D_SAFE_RELEASE(clearVS_);
        URHO3D_SAFE_RELEASE(clearPS_);

        clearPipelines_.clear();
        clearVS_ = clearPS_ = nullptr;
    }
    PipelineHolder* DiligentCommonPipelines::GetOrCreateClearPipeline(ClearPipelineDesc& desc)
    {
        desc.RecalculateHash();
        auto it = clearPipelines_.find(desc.hash_);
        if (it != clearPipelines_.end())
            return it->second.get();

        IPipelineState* pipeline = CreateClearPipeline(desc);
        auto holder = ea::make_shared<PipelineHolder>(pipeline);

        clearPipelines_.insert(ea::pair(desc.hash_, holder));
        return holder.get();
    }
    Diligent::IPipelineState* DiligentCommonPipelines::CreateClearPipeline(ClearPipelineDesc& desc)
    {
        IPipelineState* pipeline = nullptr;
        GraphicsPipelineStateCreateInfo ci;
        ci.PSODesc.Name = "Clear Framebuffer Pipeline";
        ci.pVS = clearVS_;
        ci.pPS = clearPS_;

        ci.GraphicsPipeline.BlendDesc.AlphaToCoverageEnable = false;
        ci.GraphicsPipeline.BlendDesc.RenderTargets[0].BlendEnable = DiligentBlendEnable[BLEND_REPLACE];
        ci.GraphicsPipeline.BlendDesc.RenderTargets[0].SrcBlend = DiligentSrcBlend[BLEND_REPLACE];
        ci.GraphicsPipeline.BlendDesc.RenderTargets[0].DestBlend = DiligentDestBlend[BLEND_REPLACE];
        ci.GraphicsPipeline.BlendDesc.RenderTargets[0].BlendOp = DiligentBlendOp[BLEND_REPLACE];
        ci.GraphicsPipeline.BlendDesc.RenderTargets[0].SrcBlendAlpha = DiligentSrcAlphaBlend[BLEND_REPLACE];
        ci.GraphicsPipeline.BlendDesc.RenderTargets[0].DestBlendAlpha = DiligentDestAlphaBlend[BLEND_REPLACE];
        ci.GraphicsPipeline.BlendDesc.RenderTargets[0].RenderTargetWriteMask = desc.colorWrite_ ? COLOR_MASK_ALL : COLOR_MASK_NONE;
        ci.GraphicsPipeline.RasterizerDesc.CullMode = CULL_MODE_NONE;
        ci.GraphicsPipeline.RasterizerDesc.ScissorEnable = false;
        ci.GraphicsPipeline.DepthStencilDesc.DepthEnable = true;
        ci.GraphicsPipeline.DepthStencilDesc.DepthFunc = COMPARISON_FUNC_ALWAYS;
        ci.GraphicsPipeline.DepthStencilDesc.DepthWriteEnable = desc.depthWrite_;
        ci.GraphicsPipeline.DepthStencilDesc.StencilEnable = desc.clearStencil_;

        ci.GraphicsPipeline.DepthStencilDesc.StencilReadMask = M_MAX_UNSIGNED;
        ci.GraphicsPipeline.DepthStencilDesc.StencilWriteMask = M_MAX_UNSIGNED;

        ci.GraphicsPipeline.DepthStencilDesc.FrontFace.StencilPassOp = STENCIL_OP_REPLACE;
        ci.GraphicsPipeline.DepthStencilDesc.FrontFace.StencilDepthFailOp = STENCIL_OP_KEEP;
        ci.GraphicsPipeline.DepthStencilDesc.FrontFace.StencilFailOp = STENCIL_OP_KEEP;

        ci.GraphicsPipeline.DepthStencilDesc.BackFace.StencilPassOp = STENCIL_OP_REPLACE;
        ci.GraphicsPipeline.DepthStencilDesc.BackFace.StencilDepthFailOp = STENCIL_OP_KEEP;
        ci.GraphicsPipeline.DepthStencilDesc.BackFace.StencilFailOp = STENCIL_OP_KEEP;
        CreateClearShaders(&ci.pVS, &ci.pPS);

        graphics_->GetImpl()->GetDevice()->CreateGraphicsPipelineState(ci, &pipeline);

        IBuffer* frameCB = graphics_->GetImpl()->GetConstantBufferManager()->GetOrCreateBuffer(ShaderParameterGroup::SP_FRAME);
        pipeline->GetStaticVariableByName(SHADER_TYPE_PIXEL, "FrameCB")->Set(frameCB);

        return pipeline;
    }
    void DiligentCommonPipelines::CreateClearShaders(Diligent::IShader** vs, Diligent::IShader** ps)
    {

        const char* shader = R"(
                cbuffer FrameCB {
                    float4 cColor;
                }

                void VS(in uint vertexId : SV_VertexID, out float4 oPos : SV_POSITION)
                {
                    float4 pos[4];
                    pos[0] = float4(-1.0, -1.0, 0.0, 1.0);
	                pos[1] = float4(-1.0, +1.0, 0.0, 1.0);
	                pos[2] = float4(+1.0, -1.0, 0.0, 1.0);
	                pos[3] = float4(+1.0, +1.0, 0.0, 1.0);
                    oPos = pos[vertexId];
                }

                float4 PS() : SV_Target
                {
                    return cColor;
                }
            )";

        ShaderCreateInfo createInfo;
        createInfo.Desc.Name = "Clear Framebuffer Vertex";
        createInfo.Desc.ShaderType = SHADER_TYPE_VERTEX;
        createInfo.Source = shader;
        createInfo.EntryPoint = "VS";
        createInfo.SourceLanguage = SHADER_SOURCE_LANGUAGE_HLSL;

        if(clearVS_ == nullptr)
            graphics_->GetImpl()->GetDevice()->CreateShader(createInfo, &clearVS_);

        createInfo.Desc.Name = "Clear Framebuffer Pixel";
        createInfo.Desc.ShaderType = SHADER_TYPE_PIXEL;
        createInfo.EntryPoint = "PS";

        if(clearPS_ == nullptr)
            graphics_->GetImpl()->GetDevice()->CreateShader(createInfo, &clearPS_);

        *vs = clearVS_;
        *ps = clearPS_;
    }
}
