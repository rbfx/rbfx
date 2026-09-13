// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "ParticleGraphMemory.h"
#include "ParticleGraphEffect.h"

#include "../Resource/Resource.h"
#include "../IO/Archive.h"

namespace Urho3D
{

class ParticleGraphLayerInstance;

enum class ParticleGraphPinFlag
{
    /// No flags set.
    None = 0x0,
    /// Output pin is the same as unset Input pin flag.
    Output = 0x0, //ParticleGraphPinFlag::None,
    /// Input pin.
    Input = 0x1,
    /// Pin name defined in runtime.
    MutableName = 0x2,
    /// Pin value type defined in runtime.
    MutableType = 0x4,
};
URHO3D_FLAGSET(ParticleGraphPinFlag, ParticleGraphPinFlags);

/// Reference to a pin buffer in a particle graph.
struct ParticleGraphPinRef
{
    ParticleGraphPinRef()
        : type_(ParticleGraphContainerType::Auto)
        , index_(0)
    {
    }
    ParticleGraphPinRef(ParticleGraphContainerType type, unsigned index)
        : type_(type)
        , index_(index)
    {
    }
    ParticleGraphContainerType type_;
    unsigned index_;
};

/// Pin of a node in particle graph.
class ParticleGraphPin
{
public:
    /// Construct default pin.
    ParticleGraphPin();
    /// Construct pin.
    ParticleGraphPin(ParticleGraphPinFlags flags, const ea::string& name, VariantType type = VAR_NONE,
        ParticleGraphContainerType container = ParticleGraphContainerType::Auto);
    /// Construct pin.
    ParticleGraphPin(ParticleGraphPinFlags flags, const ea::string& name, ParticleGraphContainerType container);

    /// Get input pin flag.
    bool IsInput() const { return flags_.Test(ParticleGraphPinFlag::Input); }

    /// Get pin flags.
    ParticleGraphPinFlags GetFlags() const { return flags_; }

    /// Name of the pin for visual editor.
    const ea::string& GetName() const { return name_; }

    /// Name hash of the pin.
    StringHash GetNameHash() const { return nameHash_; }

    /// Requested value type of the pin. VAR_NONE for autodetected value type.
    VariantType GetRequestedType() const { return requestedValueType_; }

    /// Value type of the pin evaluated at the runtime.
    VariantType GetValueType() const { return valueType_; }

    /// Get attribute index for sparse span.
    unsigned GetAttributeIndex() const { return attributeIndex_; }

    /// Get reference to memory descriptor for the pin.
    ParticleGraphPinRef GetMemoryReference() const { return memory_; }

    ParticleGraphContainerType GetContainerType() const { return memory_.type_; }

    /// Get a copy of the pin setup but with a different value type.
    ParticleGraphPin WithType(VariantType type) const;

    /// Set source node and pin indices.
    bool SetSource(unsigned nodeIndex, unsigned pinIndex = 0);

    /// Get true if connected to node.
    bool GetConnected() const;

    /// Get connected node index.
    unsigned GetConnectedNodeIndex() const { return sourceNode_; }

    /// Get connected pin index.
    unsigned GetConnectedPinIndex() const { return sourcePin_; }

protected:
    /// Set pin name and hash.
    bool SetName(const ea::string& name);
    /// Set pin value type.
    bool SetValueType(VariantType valueType);

    /// Get input pin flag.
    void SetIsInput(bool isInput);

private:
    /// Container type: span, sparse or scalar.
    ParticleGraphContainerType containerType_{ParticleGraphContainerType::Auto};

    /// Value type at runtime.
    VariantType valueType_{VAR_NONE};

    /// Name of the pin for visual editor.
    ea::string name_;
    /// Pin name hash.
    StringHash nameHash_;

    /// Source node.
    unsigned sourceNode_;
    /// Source node pin index.
    unsigned sourcePin_{};


    /// Is input pin.
    ParticleGraphPinFlags flags_{ParticleGraphPinFlag::Input};

    /// Value type (float, vector3, etc).
    VariantType requestedValueType_{VAR_NONE};

    /// Index of attribute. Only valid for sparse pins.
    unsigned attributeIndex_{};

    /// Reference to a memory block that corresponds to the pin value.
    ParticleGraphPinRef memory_;

    friend class ParticleGraphAttributeBuilder;
    friend class ParticleGraphNode;
};

template <typename T> struct ParticleGraphTypedPin : public ParticleGraphPin
{
    typedef T Type;

    ParticleGraphTypedPin(ParticleGraphPinFlags flags, const char* name)
        : ParticleGraphPin(flags, name, GetVariantType<T>())
    {
    }
    ParticleGraphTypedPin(const char* name)
        : ParticleGraphPin(ParticleGraphPinFlag::Input, name, GetVariantType<T>())
    {
    }
};

}
