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

#include "Log.h"
#include "Urho3D/IO/Log.h"

#include "Helpers.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{
namespace 
{
    template <typename T>
    void LogSpan(unsigned numParticles, T span)
    {
        for (unsigned i=0; i<numParticles;++i)
        {
            Urho3D::Log::GetLogger().Error(ea::to_string(span[i]));
        }
    }


    template <typename T> struct LogPin
    {
        void operator()(UpdateContext& context, const ParticleGraphNodePin& pin0)
        {
            const unsigned numParticles = context.indices_.size();

            switch (pin0.GetContainerType())
            {
            case ParticleGraphContainerType::Span:
                LogSpan(numParticles, context.GetSpan<T>(pin0));
                break;
            case ParticleGraphContainerType::Scalar:
                LogSpan(1, context.GetScalar<T>(pin0));
                break;
            case ParticleGraphContainerType::Sparse:
                LogSpan(numParticles, context.GetSparse<T>(pin0));
                break;
            default:
                assert(!"Invalid pin container type");
                break;
            }
        }
    };

}

Log::Instance::Instance(Log* node)
    : node_(node)
{
}

void Log::Instance::Update(UpdateContext& context)
{
    const auto& pin0 = node_->pins_[0];
    SelectByVariantType<LogPin>(pin0.GetValueType(), context, pin0);
};

} // namespace ParticleGraphNodes
} // namespace Urho3D
