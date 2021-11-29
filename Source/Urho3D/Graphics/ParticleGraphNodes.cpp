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

#include "../Precompiled.h"

#include "../Core/Context.h"
#include "../Graphics/ParticleGraphNodes.h"

#include "Urho3D/Resource/XMLElement.h"

namespace Urho3D
{

ParticleGraphNodeInstance::ParticleGraphNodeInstance() = default;

ParticleGraphNodeInstance::~ParticleGraphNodeInstance() = default;

namespace ParticleGraphNodes
{
Attribute::Attribute()
    : ParticleGraphNode()
{
    pins_[0].containerType_ = ParticleGraphContainerType::Sparse;
}

GetAttribute::GetAttribute()
    : Attribute()
{
    pins_[0].isInput_ = false;
}

SetAttribute::SetAttribute()
    : Attribute()
{
}

const Variant& Const::GetValue()
{
    return value_;
}

void Const::SetValue(const Variant& value)
{
    value_ = value;
    pins_[0].valueType_ = value.GetType();
}
/// Save to an XML element. Return true if successful.
bool Const::Save(XMLElement& dest) const
{
    if (!ParticleGraphNode::Save(dest))
        return false;
    auto elem = dest.CreateChild("value");
    elem.SetAttribute("type", value_.GetTypeName());
    elem.SetAttribute("value", value_.ToString());
    return true;
}

}

} // namespace Urho3D

