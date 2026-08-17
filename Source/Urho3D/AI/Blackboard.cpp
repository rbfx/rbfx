// SPDX-License-Identifier: MIT

#include "Blackboard.h"

#include <algorithm>

namespace Urho3D
{

bool Blackboard::Set(const ea::string& key, const Variant& value)
{
    if (key.empty())
        return false;
    const auto iter = values_.find(key);
    const bool existed = iter != values_.end();
    const Variant oldValue = existed ? iter->second : Variant();
    if (existed && oldValue == value)
        return false;

    values_[key] = value;
    Notify({key, oldValue, value, existed});
    return true;
}

bool Blackboard::Remove(const ea::string& key)
{
    const auto iter = values_.find(key);
    if (iter == values_.end())
        return false;
    const Variant oldValue = iter->second;
    values_.erase(iter);
    Notify({key, oldValue, Variant(), true});
    return true;
}

void Blackboard::Clear()
{
    const ea::vector<ea::string> keys = GetKeys();
    for (const ea::string& key : keys)
        Remove(key);
}

bool Blackboard::Has(const ea::string& key) const
{
    return values_.find(key) != values_.end();
}

Variant Blackboard::Get(const ea::string& key) const
{
    const auto iter = values_.find(key);
    return iter != values_.end() ? iter->second : Variant();
}

ea::vector<ea::string> Blackboard::GetKeys() const
{
    ea::vector<ea::string> result;
    result.reserve(values_.size());
    for (const auto& entry : values_)
        result.push_back(entry.first);
    std::sort(result.begin(), result.end());
    return result;
}

unsigned Blackboard::BindOnChanged(ChangeCallback callback)
{
    if (!callback)
        return 0;
    const unsigned id = nextCallbackId_++;
    callbacks_[id] = ea::move(callback);
    return id;
}

bool Blackboard::UnbindOnChanged(unsigned callbackId)
{
    return callbacks_.erase(callbackId) != 0;
}

void Blackboard::Notify(const BlackboardChange& change)
{
    ea::vector<unsigned> ids;
    ids.reserve(callbacks_.size());
    for (const auto& entry : callbacks_)
        ids.push_back(entry.first);
    std::sort(ids.begin(), ids.end());
    for (unsigned id : ids)
    {
        const auto iter = callbacks_.find(id);
        if (iter != callbacks_.end() && iter->second)
            iter->second(change);
    }
}

} // namespace Urho3D
