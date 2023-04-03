#include "../Graphics/PipelineStateCache.h"

namespace Urho3D
{
    using namespace Diligent;
    PipelineStateCache::PipelineStateCache(Context* context) :
        Object(context),
        GPUObject(GetSubsystem<Graphics>()),
        init_(false) {
    }

    void PipelineStateCache::Init() {
        if (init_)
            return;
    }
}
