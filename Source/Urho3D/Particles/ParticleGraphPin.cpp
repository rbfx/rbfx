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

#include "ParticleGraphPin.h"

#include "ParticleGraph.h"
#include "../IO/ArchiveSerialization.h"
#include "../IO/Log.h"

namespace Urho3D
{

ParticleGraphPin::ParticleGraphPin()
    : sourceNode_(ParticleGraph::INVALID_NODE_INDEX)
    , flags_(ParticleGraphPinFlag::Input)
{
}

ParticleGraphPin::ParticleGraphPin(ParticleGraphPinFlags flags, const ea::string& name, ParticleGraphContainerType container)
    : containerType_(container)
    , name_(name)
    , nameHash_(name)
    , sourceNode_(ParticleGraph::INVALID_NODE_INDEX)
    , flags_(flags)
    , requestedValueType_(VAR_NONE)
{
}

ParticleGraphPin::ParticleGraphPin(
    ParticleGraphPinFlags flags, const ea::string& name, VariantType type, ParticleGraphContainerType container)
    : containerType_(container)
    , name_(name)
    , nameHash_(name)
    , sourceNode_(ParticleGraph::INVALID_NODE_INDEX)
    , flags_(flags)
    , requestedValueType_(type)
{
}

ParticleGraphPin ParticleGraphPin::WithType(VariantType type) const
{
    return ParticleGraphPin(flags_, name_, type, containerType_);
}

bool ParticleGraphPin::SetSource(unsigned nodeIndex, unsigned pinIndex)
{
    if (!IsInput())
    {
        URHO3D_LOGERROR(Format("Can't set source to output pin {}", GetName()));
        return false;
    }

    sourceNode_ = nodeIndex;
    sourcePin_ = pinIndex;
    return true;
}

bool ParticleGraphPin::GetConnected() const
{
    return sourceNode_ != ParticleGraph::INVALID_NODE_INDEX;
}

bool ParticleGraphPin::SetName(const ea::string& name)
{
    if (name_ == name)
        return true;

    if (!flags_.Test(ParticleGraphPinFlag::MutableName))
    {
        URHO3D_LOGERROR("Can't change name of {} pin.", GetName());
        return false;
    }
    name_ = name;
    nameHash_ = StringHash(name_);
    return true;
}

bool ParticleGraphPin::SetValueType(VariantType valueType)
{
    if (requestedValueType_ == valueType)
        return true;
    if (!flags_.Test(ParticleGraphPinFlag::MutableType))
    {
        URHO3D_LOGERROR("Can't change type of {} pin from {} to {}.", GetName(),
            Variant::GetTypeNameList()[requestedValueType_], Variant::GetTypeNameList()[valueType]);
        return false;
    }
    requestedValueType_ = valueType;
    return true;
}

void ParticleGraphPin::SetIsInput(bool isInput) { flags_.Set(ParticleGraphPinFlag::Input, isInput); }

} // namespace Urho3D
