//
// Copyright (c) 2021 the rbfx project.
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
