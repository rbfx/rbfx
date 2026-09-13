// Copyright (c) 2021 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Span.h"
#include "../Core/Variant.h"

namespace Urho3D
{
/// Memory layout definition.
struct ParticleGraphSpan
{
    /// Construct.
    ParticleGraphSpan();
    /// Construct.
    ParticleGraphSpan(unsigned offset, unsigned sizeInBytes_);

    /// Offset in the buffer.
    unsigned offset_;
    /// Size in bytes.
    unsigned size_;

    /// Make a span of type T from memory buffer.
    template <typename T> ea::span<T> MakeSpan(ea::span<uint8_t> buffer) const
    {
        if (size_ == 0)
            return {};
        const auto slice = buffer.subspan(offset_, size_);
        return ea::span<T>(reinterpret_cast<T*>(slice.begin()), reinterpret_cast<T*>(slice.end()));
    }
};

/// Memory layout for attributes.
class ParticleGraphAttributeLayout
{
public:
    /// Reset layout.
    void Reset(unsigned offset, unsigned capacity);

    /// Get or add attribute.
    unsigned GetOrAddAttribute(const ea::string& name, VariantType type);

    /// Get name by attribute index.
    const ea::string& GetName(unsigned attrIndex) const { return attributes_[attrIndex].name_; }

    /// Get type by attribute index.
    VariantType GetType(unsigned attrIndex) const { return attributes_[attrIndex].type_; }

    /// Get number of attributes.
    unsigned GetNumAttributes() const { return attributes_.size(); }

    /// Get amount of required memory to host all attributes.
    unsigned GetRequiredMemory() const { return position_; }

    /// Get span by attribute index.
    ParticleGraphSpan GetSpan(unsigned attrIndex) const { return attributes_[attrIndex].span_; }

private:
    /// Attribute layout.
    struct AttrSpan
    {
        /// Name of attribute.
        ea::string name_;
        /// Attribute name hash.
        StringHash nameHash_;
        /// Type of attribute.
        VariantType type_;
        /// Location at emitter attribute buffer.
        ParticleGraphSpan span_;
    };

    /// All known attributes.
    ea::vector<AttrSpan> attributes_;

    /// Size of required attribute buffer.
    unsigned position_{};

    /// Maximal number of particles.
    unsigned capacity_{};
};

/// Memory layout for intermediate values.
class ParticleGraphBufferLayout
{

public:
    /// Reset layout.
    void Reset(unsigned capacity);

    /// Allocate span. Returns allocated span index.
    unsigned Allocate(ParticleGraphContainerType container, VariantType type);

    /// Get amount of required memory to host intermediate values.
    unsigned GetRequiredMemory() const { return position_; }

    /// Get span by index.
    const ParticleGraphSpan& operator[](unsigned index) const
    {
        return spans_[index].span_;
    }

private:
    /// Attribute layout.
    struct PinSpan
    {
        /// Container type of attribute.
        ParticleGraphContainerType container_;
        /// Type of attribute.
        VariantType type_;
        /// Location at emitter attribute buffer.
        ParticleGraphSpan span_;
    };

    /// Allocated spans.
    ea::vector<PinSpan> spans_;

    /// Size of required attribute buffer.
    unsigned position_{};

    /// Maximal number of particles.
    unsigned capacity_{};
};

} // namespace Urho3D
