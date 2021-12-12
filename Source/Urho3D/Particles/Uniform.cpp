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

#include "Uniform.h"

#include "Helpers.h"
#include "ParticleGraphLayerInstance.h"

namespace Urho3D
{

namespace
{

template <typename T> struct SetValue
{
    void operator()(UpdateContext& context, const ParticleGraphPin& pin0)
    {
        auto dst = context.GetScalar<T>(pin0.GetMemoryReference());

        dst[0] = context.layer_->GetUniform(pin0.GetNameHash(), pin0.GetValueType()).Get<T>();
    }
};

} // namespace

namespace ParticleGraphNodes
{
GetUniform::GetUniform(Context* context)
    : ParticleGraphNode(context)
    , pins_{ParticleGraphPin(PGPIN_NAME_MUTABLE | PGPIN_TYPE_MUTABLE, "uniform", VAR_FLOAT, PGCONTAINER_SCALAR)}
{
}

void GetUniform::SetAttributeType(VariantType valueType)
{
    SetPinValueType(0, valueType);
}

GetUniform::Instance::Instance(GetUniform* node)
    : node_(node)
{
}

void GetUniform::Instance::Update(UpdateContext& context)
{
    const ParticleGraphPin& pin0 = node_->pins_[0];
    SelectByVariantType<SetValue>(pin0.GetValueType(), context, pin0);
}

} // namespace ParticleGraphNodes

} // namespace Urho3D
