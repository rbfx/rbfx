#include "./DiligentGraphicsImpl.h"
#include "../ConstantBuffer.h"
#include "../../Container/Hash.h"

#include <Diligent/Graphics/GraphicsEngine/interface/PipelineState.h>
#include <EASTL/shared_ptr.h>
namespace Urho3D
{
    class PipelineHolder {
    public:
        PipelineHolder(Diligent::IPipelineState* pipeline);
        ~PipelineHolder();
        Diligent::IPipelineState* GetPipeline();
        Diligent::IShaderResourceBinding* GetShaderResourceBinding();
    private:
        Diligent::IPipelineState* pipeline_;
        Diligent::IShaderResourceBinding* shaderRes_;
    };

    struct ClearPipelineDesc {
        unsigned hash_;
        Diligent::TEXTURE_FORMAT rtTexture_{};
        bool colorWrite_{};
        bool depthWrite_{};
        bool clearStencil_{};

        bool IsInitialized() {
            return rtTexture_ != Diligent::TEX_FORMAT_UNKNOWN;
        }
        void RecalculateHash() {
            unsigned hash = 0;
            CombineHash(hash, rtTexture_);
            CombineHash(hash, colorWrite_);
            CombineHash(hash, depthWrite_);
            CombineHash(hash, clearStencil_);

            hash_ = hash;
        }
    };

    typedef ea::unordered_map<unsigned, ea::shared_ptr<PipelineHolder>> PipelineStateMap;
    class URHO3D_API DiligentCommonPipelines {
        friend class Graphics;
    public:
        DiligentCommonPipelines(Graphics* graphics);
        void Release();
        PipelineHolder* GetOrCreateClearPipeline(ClearPipelineDesc& desc);
    private:
        Diligent::IPipelineState* CreateClearPipeline(ClearPipelineDesc& desc);
        void CreateClearShaders(Diligent::IShader** vs, Diligent::IShader** ps);

        Graphics* graphics_;

        Diligent::IShader* clearVS_;
        Diligent::IShader* clearPS_;
        PipelineStateMap clearPipelines_;
    };
}
