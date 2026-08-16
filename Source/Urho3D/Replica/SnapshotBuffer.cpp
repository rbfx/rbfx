// Copyright (c) 2026 rbfx-blueprint contributors.
// SPDX-License-Identifier: MIT

#include "SnapshotBuffer.h"

namespace Urho3D
{

SnapshotBuffer::SnapshotBuffer(unsigned capacity)
{
    SetCapacity(capacity);
}

void SnapshotBuffer::SetCapacity(unsigned capacity)
{
    capacity_ = Max(capacity, 1u);
    while (snapshots_.size() > capacity_)
        snapshots_.erase(snapshots_.begin());
}

void SnapshotBuffer::Push(const NetworkSnapshot& snapshot)
{
    if (snapshots_.empty() || snapshot.frame > snapshots_.back().frame)
        snapshots_.push_back(snapshot);
    else
    {
        bool replaced = false;
        for (auto iter = snapshots_.begin(); iter != snapshots_.end(); ++iter)
        {
            if (iter->frame == snapshot.frame)
            {
                *iter = snapshot;
                replaced = true;
                break;
            }
            if (iter->frame > snapshot.frame)
            {
                snapshots_.insert(iter, snapshot);
                replaced = true;
                break;
            }
        }
        if (!replaced)
            snapshots_.push_back(snapshot);
    }

    while (snapshots_.size() > capacity_)
        snapshots_.erase(snapshots_.begin());
}

const NetworkSnapshot* SnapshotBuffer::GetLatest() const
{
    return snapshots_.empty() ? nullptr : &snapshots_.back();
}

Variant SnapshotBuffer::Interpolate(const Variant& from, const Variant& to, float alpha)
{
    if (from.GetType() != to.GetType())
        return alpha < 0.5f ? from : to;

    switch (from.GetType())
    {
    case VAR_INT:
        return Variant(static_cast<int>(from.GetInt() + (to.GetInt() - from.GetInt()) * alpha));
    case VAR_INT64:
        return Variant(static_cast<long long>(from.GetInt64() + (to.GetInt64() - from.GetInt64()) * alpha));
    case VAR_FLOAT:
        return Variant(from.GetFloat() + (to.GetFloat() - from.GetFloat()) * alpha);
    case VAR_DOUBLE:
        return Variant(from.GetDouble() + (to.GetDouble() - from.GetDouble()) * alpha);
    default:
        return alpha < 0.5f ? from : to;
    }
}

void SnapshotBuffer::InterpolateMaps(const StringVariantMap& from, const StringVariantMap& to, float alpha,
    StringVariantMap& result)
{
    result = from;
    for (const auto& item : to)
    {
        const auto iter = from.find(item.first);
        result[item.first] = iter != from.end() ? Interpolate(iter->second, item.second, alpha) : item.second;
    }
}

bool SnapshotBuffer::Sample(float time, StringVariantMap& values) const
{
    if (snapshots_.empty())
        return false;
    if (snapshots_.size() == 1 || time <= snapshots_.front().time)
    {
        values = snapshots_.front().values;
        return true;
    }

    for (unsigned i = 1; i < snapshots_.size(); ++i)
    {
        const NetworkSnapshot& from = snapshots_[i - 1];
        const NetworkSnapshot& to = snapshots_[i];
        if (time <= to.time)
        {
            const float duration = to.time - from.time;
            const float alpha = duration > M_EPSILON ? Clamp((time - from.time) / duration, 0.0f, 1.0f) : 1.0f;
            InterpolateMaps(from.values, to.values, alpha, values);
            return true;
        }
    }

    const NetworkSnapshot& latest = snapshots_.back();
    const NetworkSnapshot& previous = snapshots_[snapshots_.size() - 2];
    const float elapsed = time - latest.time;
    const float duration = latest.time - previous.time;
    if (elapsed > maxExtrapolation_ || duration <= M_EPSILON)
    {
        values = latest.values;
        return true;
    }

    const float alpha = 1.0f + elapsed / duration;
    InterpolateMaps(previous.values, latest.values, alpha, values);
    return true;
}

} // namespace Urho3D
