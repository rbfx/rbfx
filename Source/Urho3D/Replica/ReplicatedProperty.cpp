// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#include "ReplicatedProperty.h"

namespace Urho3D
{

ReplicatedProperty::ReplicatedProperty(const ea::string& name, const Variant& value, bool reliable, bool ownerOnly)
    : name_(name)
    , value_(value)
    , reliable_(reliable)
    , ownerOnly_(ownerOnly)
{
}

bool ReplicatedProperty::SetValue(const Variant& value)
{
    if (value_ == value)
        return false;

    value_ = value;
    dirty_ = true;
    return true;
}

bool ReplicatedPropertySet::Register(const ReplicatedProperty& property)
{
    if (property.GetName().empty())
        return false;

    properties_[property.GetName()] = property;
    return true;
}

bool ReplicatedPropertySet::Unregister(const ea::string& name)
{
    return properties_.erase(name) != 0;
}

ReplicatedProperty* ReplicatedPropertySet::Find(const ea::string& name)
{
    const auto iter = properties_.find(name);
    return iter != properties_.end() ? &iter->second : nullptr;
}

const ReplicatedProperty* ReplicatedPropertySet::Find(const ea::string& name) const
{
    const auto iter = properties_.find(name);
    return iter != properties_.end() ? &iter->second : nullptr;
}

bool ReplicatedPropertySet::SetValue(const ea::string& name, const Variant& value)
{
    ReplicatedProperty* property = Find(name);
    return property && (property->SetValue(value) || true);
}

StringVariantMap ReplicatedPropertySet::CaptureDirty(bool reliableOnly) const
{
    StringVariantMap result;
    for (const auto& item : properties_)
    {
        const ReplicatedProperty& property = item.second;
        if (property.IsDirty() && (!reliableOnly || property.IsReliable()))
            result[item.first] = property.GetValue();
    }
    return result;
}

void ReplicatedPropertySet::ClearDirty(bool reliableOnly)
{
    for (auto& item : properties_)
    {
        ReplicatedProperty& property = item.second;
        if (!reliableOnly || property.IsReliable())
            property.SetDirty(false);
    }
}

bool ReplicatedPropertySet::ApplyDelta(const StringVariantMap& values, bool markDirty)
{
    bool allKnown = true;
    for (const auto& item : values)
    {
        ReplicatedProperty* property = Find(item.first);
        if (!property)
        {
            allKnown = false;
            continue;
        }

        const bool changed = property->SetValue(item.second);
        if (!markDirty && changed)
            property->SetDirty(false);
    }
    return allKnown;
}

} // namespace Urho3D
