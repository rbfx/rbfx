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

#include "Attribute.h"
#include "Helpers.h"

#include "Urho3D/Resource/XMLElement.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{
namespace 
{

template <typename T> struct CopyValues
{
    void operator()(UpdateContext& context, const ParticleGraphPin& pin0)
    {
        const unsigned numParticles = context.indices_.size();
        switch (pin0.GetContainerType())
        {
        case PGCONTAINER_SCALAR:
            {
                auto src = context.GetScalar<T>(pin0.GetMemoryReference());
                auto dst = context.GetSparse<T>(ParticleGraphPinRef(PGCONTAINER_SPARSE, pin0.GetAttributeIndex()));
                for (unsigned i = 0; i < numParticles; ++i)
                {
                    dst[i] = src[i];
                }
            }
            break;
            case PGCONTAINER_SPAN:
            {
                auto src = context.GetSpan<T>(pin0.GetMemoryReference());
                auto dst = context.GetSparse<T>(ParticleGraphPinRef(PGCONTAINER_SPARSE, pin0.GetAttributeIndex()));
                for (unsigned i = 0; i < numParticles; ++i)
                {
                    dst[i] = src[i];
                }
            }
            break;
            case PGCONTAINER_SPARSE:
            {
                auto src = context.GetSparse<T>(pin0.GetMemoryReference());
                auto dst = context.GetSparse<T>(ParticleGraphPinRef(PGCONTAINER_SPARSE, pin0.GetAttributeIndex()));
                for (unsigned i = 0; i < numParticles; ++i)
                {
                    dst[i] = src[i];
                }
            }
            break;
            }
    }
};

}
Attribute::Attribute(Context* context, const ParticleGraphPin& pin)
    : ParticleGraphNode(context)
    , pins_{pin}
{
}

GetAttribute::GetAttribute(Context* context)
    : Attribute(context, ParticleGraphPin(PGPIN_NAME_MUTABLE | PGPIN_TYPE_MUTABLE, "", VAR_NONE,
                                              PGCONTAINER_SPARSE))
{
}

SetAttribute::SetAttribute(Context* context)
    : Attribute(context, ParticleGraphPin(PGPIN_INPUT | PGPIN_NAME_MUTABLE | PGPIN_TYPE_MUTABLE, "", VAR_NONE,
                                              PGCONTAINER_SPARSE))
{
}

SetAttribute::Instance::Instance(SetAttribute* node)
    : node_(node)
{
}
void SetAttribute::Instance::Update(UpdateContext& context)
{
    const ParticleGraphPin& pin0 = node_->pins_[0];
    SelectByVariantType<CopyValues>(pin0.GetValueType(), context, pin0);
};

} // namespace ParticleGraphNodes
} // namespace Urho3D
