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

#include "ParticleGraphPin.h"

#include "Urho3D/IO/ArchiveSerialization.h"

namespace Urho3D
{

ParticleGraphPin::ParticleGraphPin()
    : flags_(PGPIN_INPUT)
{
}
ParticleGraphPin::ParticleGraphPin(ParticleGraphPinFlags flags, const ea::string name, VariantType type,
                                           ParticleGraphContainerType container)
    : flags_(flags)
    , requestedValueType_(type)
    , containerType_(container)
{
    SetName(name);
}

bool ParticleGraphPin::Serialize(Archive& archive)
{
    if (archive.IsInput())
    {
        // Load name.
        ea::string name;
        if (!SerializeValue(archive, "name", name))
            return false;
        // Set name if it is mutable or check it.
        if (flags_.Test(PGPIN_NAME_MUTABLE))
            name_ = name;
        else if (name != name_)
        {
            archive.SetError("Pin name mismatch");
            return false;
        }
        // Load type.
        VariantType type{VAR_NONE};

      if (!SerializeEnum(archive, "valueType", Variant::GetTypeNameList(), type))
          return false;
        // Set type if it is mutable or check it.
        if (flags_.Test(PGPIN_TYPE_MUTABLE))
            requestedValueType_ = type;
        else if (requestedValueType_ != type)
        {
            archive.SetError("Pin type mismatch");
            return false;
        }
    }
    else
    {
        if (!SerializeValue(archive, "name", name_))
        {
            return false;
        }
        if (!SerializeEnum(archive, "valueType", Variant::GetTypeNameList(), requestedValueType_))
        {
            return false;
        }
    }
    if (GetIsInput())
    {
        if (!SerializeValue(archive, "sourceNode", sourceNode_))
        {
            return false;
        }
        if (!SerializeValue(archive, "sourcePin", sourcePin_))
        {
            return false;
        }
    }
    return true;
}

ParticleGraphPin ParticleGraphPin::WithType(VariantType type) const
{
    return ParticleGraphPin(flags_, name_, type, containerType_);
}

void ParticleGraphPin::SetSource(unsigned nodeIndex, unsigned pinIndex)
{
    sourceNode_ = nodeIndex;
    sourcePin_ = pinIndex;
}

void ParticleGraphPin::SetName(const ea::string& name)
{
    name_ = name;
    nameHash_ = StringHash(name_);
}

void ParticleGraphPin::SetValueType(VariantType valueType) { requestedValueType_ = valueType; }

void ParticleGraphPin::SetIsInput(bool isInput) { flags_.Set(PGPIN_INPUT, isInput); }

bool SerializeValue(Archive& archive, const char* name, ParticleGraphPin& value)
{
    if (auto block = archive.OpenUnorderedBlock(name))
    {
        return value.Serialize(archive);
    }
    return false;
}

} // namespace Urho3D
