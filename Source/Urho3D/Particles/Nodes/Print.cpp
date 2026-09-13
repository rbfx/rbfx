// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../../Precompiled.h"

#include "Print.h"

#include "../../IO/Log.h"
#include "../Helpers.h"
#include "../ParticleGraphLayerInstance.h"
#include "../ParticleGraphSystem.h"
#include "../Span.h"
#include "../UpdateContext.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{
namespace
{
    template <typename T>
    void LogSpan(LogLevel level, unsigned numParticles, const SparseSpan<T>& span)
    {
        for (unsigned i=0; i<numParticles;++i)
        {
            Variant v{span[i]};
            Urho3D::Log::GetLogger().Write(level, v.ToString());
        }
    }

    template <typename T> struct LogPin
    {
        void operator()(const UpdateContext& context, const ParticleGraphPin& pin0)
        {
            const unsigned numParticles = context.indices_.size();

            LogSpan<T>(LOG_INFO, numParticles, context.GetSpan<T>(pin0.GetMemoryReference()));
        }
    };

}

Print::Print(Context* context)
    : ParticleGraphNode(context)
    , pins_{ParticleGraphPin(ParticleGraphPinFlag::Input, "value")}
{
}

void Print::RegisterObject(ParticleGraphSystem* context)
{
    context->AddReflection<Print>();
}

Print::Instance::Instance(Print* node)
    : node_(node)
{
}

void Print::Instance::Update(UpdateContext& context)
{
    const auto& pin0 = node_->pins_[0];
    SelectByVariantType<LogPin>(pin0.GetValueType(), context, pin0);
};

} // namespace ParticleGraphNodes
} // namespace Urho3D
