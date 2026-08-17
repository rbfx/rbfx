// SPDX-License-Identifier: MIT

#pragma once

#include <Urho3D/Urho3D.h>
#include <Urho3D/Core/Variant.h>

#include <EASTL/functional.h>
#include <EASTL/string.h>
#include <EASTL/unordered_map.h>
#include <EASTL/vector.h>

namespace Urho3D
{

/// A single Blackboard value change delivered to observers.
struct URHO3D_API BlackboardChange
{
    ea::string key;
    Variant oldValue;
    Variant newValue;
    bool existed{};
};

/// Typed key-value store shared by behavior trees, EQS and Blueprint gameplay logic.
class URHO3D_API Blackboard
{
public:
    using ChangeCallback = ea::function<void(const BlackboardChange&)>;

    bool Set(const ea::string& key, const Variant& value);
    bool Remove(const ea::string& key);
    void Clear();
    bool Has(const ea::string& key) const;
    Variant Get(const ea::string& key) const;
    const StringVariantMap& GetValues() const { return values_; }
    ea::vector<ea::string> GetKeys() const;

    unsigned BindOnChanged(ChangeCallback callback);
    bool UnbindOnChanged(unsigned callbackId);

private:
    void Notify(const BlackboardChange& change);

    StringVariantMap values_;
    ea::unordered_map<unsigned, ChangeCallback> callbacks_;
    unsigned nextCallbackId_{1};
};

} // namespace Urho3D
