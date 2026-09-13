// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Graphics/PipelineStateTracker.h"

#include "Urho3D/Core/AssertBase.h"

namespace Urho3D
{

void PipelineStateSubscription::Reset(PipelineStateTracker* sender, PipelineStateTracker* subscriber)
{
    // Add new reference first
    if (subscriber && sender)
        sender->AddSubscriberReference(subscriber);

    // Remove old reference
    if (subscriber_ && sender_)
        sender_->RemoveSubscriberReference(subscriber_);

    // Relink pointers
    subscriber_ = subscriber;
    sender_ = sender;
}

PipelineStateSubscription PipelineStateTracker::CreateDependency(PipelineStateTracker* sender)
{
    return { sender, this };
}

void PipelineStateTracker::AddSubscriberReference(PipelineStateTracker* subscriber)
{
    if (!subscriber)
        return;
    auto iter = FindSubscriberIter(subscriber);
    if (iter != subscribers_.end())
        ++iter->second;
    else
        subscribers_.emplace_back(subscriber, 1u);
    subscriber->MarkPipelineStateHashDirty();
}

void PipelineStateTracker::RemoveSubscriberReference(PipelineStateTracker* subscriber)
{
    if (!subscriber)
        return;
    auto iter = FindSubscriberIter(subscriber);
    URHO3D_ASSERT(iter != subscribers_.end());
    --iter->second;
    if (iter->second == 0)
        subscribers_.erase(iter);
    subscriber->MarkPipelineStateHashDirty();
}

PipelineStateTracker::~PipelineStateTracker()
{
}

void PipelineStateTracker::MarkPipelineStateHashDirty()
{
    const unsigned oldHash = pipelineStateHash_.exchange(0, std::memory_order_relaxed);
    if (oldHash != 0)
    {
        for (const auto& item : subscribers_)
            item.first->MarkPipelineStateHashDirty();
    }
}

PipelineStateTracker::DependantVector::iterator PipelineStateTracker::FindSubscriberIter(PipelineStateTracker* subscriber)
{
    const auto pred = [&](const ea::pair<PipelineStateTracker*, unsigned>& elem) { return elem.first == subscriber; };
    return ea::find_if(subscribers_.begin(), subscribers_.end(), pred);
}

}
