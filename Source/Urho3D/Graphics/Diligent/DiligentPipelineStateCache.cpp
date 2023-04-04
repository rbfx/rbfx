#include "../PipelineState.h"
#include "../GraphicsImpl.h"
#include <Diligent/Graphics/GraphicsEngine/interface/PipelineStateCache.h>
namespace Urho3D
{
    using namespace Diligent;
    void PipelineStateCache::CreatePSOCache(const ea::vector<uint8_t>& psoFileData) {
        graphics_->GetImpl();

        PipelineStateCacheCreateInfo ci;
        ci.Desc.Name = "PipelineStateCache";
        ci.CacheDataSize = psoFileData.size();
        ci.pCacheData = psoFileData.data();

        RefCntAutoPtr<IPipelineStateCache> cache;
        graphics_->GetImpl()->GetDevice()->CreatePipelineStateCache(ci, &cache);
        if (!cache) {
            URHO3D_LOGERROR("Could not possible to create Pipeline State Cache GPU Object.");
            return;
        }

        object_ = cache;
        URHO3D_LOGDEBUG("Pipeline State Cache GPU Object has been created.");
    }

    void PipelineStateCache::ReadPSOData(ByteVector& psoData) {
        if (!object_)
            return;
        RefCntAutoPtr<IPipelineStateCache> cache = object_.Cast<IPipelineStateCache>(IID_PipelineStateCache);

        IDataBlob* blob = nullptr;
        cache->GetData(&blob);

        if (!blob) {
            URHO3D_LOGERROR("Could not possible to read Pipeline State Cache GPU Object.");
            return;
        }

        psoData.resize(blob->GetSize());
        memcpy(psoData.data(), blob->GetDataPtr(), blob->GetSize());
    }
}
