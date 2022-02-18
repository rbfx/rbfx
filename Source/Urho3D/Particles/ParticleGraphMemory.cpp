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

#include "ParticleGraphMemory.h"

namespace Urho3D
{

/// Construct ParticleGraphSpan.
ParticleGraphSpan::ParticleGraphSpan()
    : offset_(0)
    , size_(0)
{
}

/// Construct ParticleGraphSpan.
ParticleGraphSpan::ParticleGraphSpan(unsigned offset, unsigned sizeInBytes_)
    : offset_(offset)
    , size_(sizeInBytes_)
{
}

void ParticleGraphAttributeLayout::Reset(unsigned offset, unsigned capacity)
{
    capacity_ = capacity;
    position_ = offset;
    attributes_.clear();
}

unsigned ParticleGraphAttributeLayout::GetOrAddAttribute(const ea::string& name, VariantType type)
{
    const auto nameHash = StringHash(name);

    //Look for attribute with linear complexity. We going to have low number of attributes so it doesn't worth to build a map.
    for (unsigned i=0; i<attributes_.size(); ++i)
    {
        if (attributes_[i].nameHash_ == nameHash && attributes_[i].type_ == type)
        {
            return i;
        }
    }

    unsigned i = attributes_.size();
    unsigned size = GetVariantTypeSize(type) * capacity_;
    attributes_.push_back(AttrSpan{name, nameHash, type, ParticleGraphSpan(position_, size)});
    position_ += size;
    return i;
}

void ParticleGraphBufferLayout::Reset(unsigned capacity)
{
    position_ = 0;
    capacity_ = capacity;
}

unsigned ParticleGraphBufferLayout::Allocate(ParticleGraphContainerType container, VariantType type)
{
    assert(container != ParticleGraphContainerType::Auto);
    unsigned index = spans_.size();
    unsigned size = ((container == ParticleGraphContainerType::Scalar) ? 1 : capacity_) * GetVariantTypeSize(type);
    spans_.push_back(PinSpan{container, type, ParticleGraphSpan(position_, size)});
    position_ += size;
    return index;
}

} // namespace Urho3D
