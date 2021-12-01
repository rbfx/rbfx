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

#include <EASTL/span.h>
#include <Urho3D/Resource/Resource.h>
#include <Urho3D/IO/Archive.h>
#include <Urho3D/IO/ArchiveSerialization.h>
#include "ParticleGraphPin.h"

namespace Urho3D
{
class ParticleGraphPin;
class ParticleGraphNodeInstance;

class URHO3D_API ParticleGraphNode : public Object
{
    URHO3D_OBJECT(ParticleGraphNode, Object)

public:
    /// Construct.
    explicit ParticleGraphNode(Context* context);

    /// Destruct.
    ~ParticleGraphNode() override;

    /// Get number of pins.
    virtual unsigned NumPins() const = 0;

    /// Get pin by index.
    virtual ParticleGraphPin& GetPin(unsigned index) = 0;

    /// Evaluate size required to place new node instance.
    virtual unsigned EvaluateInstanceSize() = 0;

    /// Place new instance at the provided address.
    virtual ParticleGraphNodeInstance* CreateInstanceAt(void* ptr, ParticleGraphLayerInstance* layer) = 0;

    /// Serialize from/to archive. Return true if successful.
    virtual bool Serialize(Archive& archive);

protected:
    /// Evaluate runtime output pin type.
    virtual bool EvaluateOutputPinType(ParticleGraphPin& pin);

    /// Set pin name.
    /// This method is protected so it can only be accessable to nodes that allow pin renaming.
    void SetPinName(unsigned pinIndex, const ea::string& name);

    /// Set pin type.
    /// This method is protected so it can only be accessable to nodes that allow pin renaming.
    void SetPinValueType(unsigned pinIndex, VariantType type);

    friend class ParticleGraphAttributeBuilder;
};

/// Serialize Object.
bool SerializeValue(Archive& archive, const char* name, SharedPtr<ParticleGraphNode>& value);

///// Serialize Object.
//template <>
//inline bool SerializeValue<ParticleGraphNode>(
//    Archive& archive, const char* name, SharedPtr<ParticleGraphNode>& value)
//{
//    return SerializeValueImpl(archive, name, value);
//}

}
