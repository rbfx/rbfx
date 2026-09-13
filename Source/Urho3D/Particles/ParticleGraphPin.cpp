// Copyright (c) 2021 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
