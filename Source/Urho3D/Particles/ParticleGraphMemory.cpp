// Copyright (c) 2021 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Particles/ParticleGraphMemory.h"

#include "Urho3D/Core/AssertBase.h"

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
    URHO3D_ASSERT(container != ParticleGraphContainerType::Auto);
    unsigned index = spans_.size();
    unsigned size = ((container == ParticleGraphContainerType::Scalar) ? 1 : capacity_) * GetVariantTypeSize(type);
    spans_.push_back(PinSpan{container, type, ParticleGraphSpan(position_, size)});
    position_ += size;
    return index;
}

} // namespace Urho3D
