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

#pragma once

#include "ParticleGraphMemory.h"

#include <Urho3D/Resource/Resource.h>
#include <Urho3D/IO/Archive.h>

namespace Urho3D
{
class ParticleGraphLayerInstance;

enum ParticleGraphPinFlagValues
{
    /// No flags set.
    PGPIN_NONE = 0x0,
    /// Input pin.
    PGPIN_INPUT = 0x1,
    /// Pin name defined in runtime.
    PGPIN_NAME_MUTABLE = 0x2,
    /// Pin value type defined in runtime.
    PGPIN_TYPE_MUTABLE = 0x4,
};
URHO3D_FLAGSET(ParticleGraphPinFlagValues, ParticleGraphPinFlags);

/// Reference to a pin buffer in a particle graph.
struct ParticleGraphPinRef
{
    ParticleGraphPinRef()
        : type_(PGCONTAINER_AUTO)
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
class ParticleGraphNodePin
{
public:
    /// Construct default pin.
    ParticleGraphNodePin();
    /// Construct pin.
    ParticleGraphNodePin(ParticleGraphPinFlags flags, const ea::string name, VariantType type = VAR_NONE,
                         ParticleGraphContainerType container = PGCONTAINER_AUTO);

    /// Source node.
    unsigned sourceNode_{};
    /// Source node pin index.
    unsigned sourcePin_{};

    /// Get input pin flag.
    /// @property
    bool GetIsInput() const { return flags_.Test(PGPIN_INPUT); }

    /// Name of the pin for visual editor.
    /// @property
    const ea::string& GetName() const { return name_; }

    /// Name hash of the pin.
    /// @property
    StringHash GetNameHash() const { return nameHash_; }

    /// Requested value type of the pin. VAR_NONE for autodetected value type.
    /// @property
    VariantType GetRequestedType() const { return requestedValueType_; }

    /// Value type of the pin evaluated at the runtime.
    /// @property
    VariantType GetValueType() const { return valueType_; }

    /// Get attribute index for sparse span.
    unsigned GetAttributeIndex() const { return attributeIndex_; }

    /// Get reference to memory descriptor for the pin.
    ParticleGraphPinRef GetMemoryReference() const { return memory_; }

    ParticleGraphContainerType GetContainerType() const { return memory_.type_; }

    /// Serialize from/to archive. Return true if successful.
    virtual bool Serialize(Archive& archive);

    /// Get a copy of the pin setup but with a different value type.
    ParticleGraphNodePin WithType(VariantType type) const;

protected:
    /// Set pin name and hash.
    void SetName(const ea::string& name);
    /// Set pin value type.
    void SetValueType(VariantType valueType);

    /// Get input pin flag.
    /// @property
    void SetIsInput(bool isInput);

private:
    /// Container type: span, sparse or scalar.
    ParticleGraphContainerType containerType_{PGCONTAINER_AUTO};

    /// Value type at runtime.
    VariantType valueType_{VAR_NONE};

    /// Name of the pin for visual editor.
    ea::string name_;
    /// Pin name hash.
    StringHash nameHash_;

    /// Is input pin.
    ParticleGraphPinFlags flags_{PGPIN_INPUT};

    /// Value type (float, vector3, etc).
    VariantType requestedValueType_{VAR_NONE};

    /// Index of attribute. Only valid for sparse pins.
    unsigned attributeIndex_;

    /// Reference to a memory block that corresponds to the pin value.
    ParticleGraphPinRef memory_;

    friend class ParticleGraphAttributeBuilder;
    friend class ParticleGraphNode;
};

/// Serialize pin.
bool SerializeValue(Archive& archive, const char* name, ParticleGraphNodePin& value);

}
