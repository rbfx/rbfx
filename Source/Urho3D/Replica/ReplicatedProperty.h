// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#pragma once

#include <EASTL/functional.h>
#include <EASTL/unordered_map.h>

#include <Urho3D/Core/Variant.h>

namespace Urho3D
{

/// Declarative value tracked for network replication.
class URHO3D_API ReplicatedProperty
{
public:
    ReplicatedProperty() = default;
    explicit ReplicatedProperty(const ea::string& name, const Variant& value = Variant(), bool reliable = true,
        bool ownerOnly = false);

    const ea::string& GetName() const { return name_; }
    void SetName(const ea::string& name) { name_ = name; }

    const Variant& GetValue() const { return value_; }
    /// Assign a value and mark the property dirty only when it actually changed.
    bool SetValue(const Variant& value);

    bool IsDirty() const { return dirty_; }
    void SetDirty(bool dirty = true) { dirty_ = dirty; }
    bool IsReliable() const { return reliable_; }
    void SetReliable(bool reliable) { reliable_ = reliable; }
    bool IsOwnerOnly() const { return ownerOnly_; }
    void SetOwnerOnly(bool ownerOnly) { ownerOnly_ = ownerOnly; }

private:
    ea::string name_;
    Variant value_;
    bool dirty_{true};
    bool reliable_{true};
    bool ownerOnly_{};
};

/// Collection of declarative properties used to build and apply replication deltas.
class URHO3D_API ReplicatedPropertySet
{
public:
    /// Register a property. Existing properties with the same name are replaced.
    bool Register(const ReplicatedProperty& property);
    /// Remove a property by name.
    bool Unregister(const ea::string& name);
    /// Find a mutable property.
    ReplicatedProperty* Find(const ea::string& name);
    /// Find a property.
    const ReplicatedProperty* Find(const ea::string& name) const;

    /// Set a property value, returning false if the property is unknown.
    bool SetValue(const ea::string& name, const Variant& value);
    /// Capture dirty values. Reliable filtering is useful for separate packet channels.
    StringVariantMap CaptureDirty(bool reliableOnly = false) const;
    /// Clear dirty flags for all properties, or only reliable/unreliable properties.
    void ClearDirty(bool reliableOnly = false);
    /// Apply a delta. Unknown names are ignored by default and reported through the return value.
    bool ApplyDelta(const StringVariantMap& values, bool markDirty = false);
    /// Return the registered properties in deterministic registration-independent lookup form.
    const ea::unordered_map<ea::string, ReplicatedProperty>& GetProperties() const { return properties_; }

private:
    ea::unordered_map<ea::string, ReplicatedProperty> properties_;
};

} // namespace Urho3D
