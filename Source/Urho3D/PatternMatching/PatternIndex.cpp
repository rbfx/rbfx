//
// Copyright (c) 2022-2022 the rbfx project.
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

#include "PatternIndex.h"

#include "../IO/Log.h"

namespace Urho3D
{

int PatternIndex::Query(const PatternQuery& query) const
{
    if (query.dirty_)
    {
        URHO3D_LOGERROR("Can't query PatternCollection with uncommited PatternQuery");
        return -1;
    }
    int bestMatchIndex = -1;
    int bestMatchLength = -1;
    for (unsigned index = 0; index < records_.size(); ++index)
    {
        const auto& record = records_[index];
        // Skip record if we know that it doesn't contain enough keys.
        if (record.length_ <= bestMatchLength)
            continue;
        // Skip record if we know that it requires more keys than query has.
        if (query.elements_.size() < record.length_)
            continue;

        int recordIndex = record.startIndex_;
        const int recordLastIndex = record.startIndex_ + record.length_;
        int queryIndex = 0;

        while (recordIndex < recordLastIndex && queryIndex < query.elements_.size())
        {
            const auto& recordElement = elements_[recordIndex];
            const auto& queryElement = query.elements_[queryIndex];
            if (recordElement.key_ == queryElement.key_)
            {
                if (queryElement.value_ < recordElement.min_ || queryElement.value_ > recordElement.max_)
                    break;
                ++recordIndex;
                ++queryIndex;
                continue;
            }
            if (recordElement.key_ < queryElement.key_)
            {
                break;
            }
            ++queryIndex;
        }

        if (recordIndex == recordLastIndex)
        {
            bestMatchIndex = record.recordId_;
            bestMatchLength = record.length_;
        }
    }
    return bestMatchIndex;
}

void PatternIndex::SendEvent(int patternIndex, Object* object) const
{
    if (patternIndex < 0 || patternIndex >= records_.size())
        return;
    for (auto& event : records_[patternIndex].events_)
    {
        object->SendEvent(event.eventId_, event.arguments_);
    }
}
unsigned PatternIndex::GetNumEvents(int patternIndex) const
{
    if (patternIndex < 0 || patternIndex >= records_.size())
        return 0;
    return records_[patternIndex].events_.size();
}
StringHash PatternIndex::GetEventId(int patternIndex, unsigned eventIndex) const
{
    if (patternIndex < 0 || patternIndex >= records_.size())
        return StringHash{};
    return records_[patternIndex].events_[eventIndex].eventId_;
}

const VariantMap& PatternIndex::GetEventArgs(int patternIndex, unsigned eventIndex) const
{
    static VariantMap empty;
    if (patternIndex < 0 || patternIndex >= records_.size())
        return empty;
    return records_[patternIndex].events_[eventIndex].arguments_;
}

/// Build index from single collection
void PatternIndex::Build(const PatternCollection* collection) { Build(&collection, &collection + 1); }

/// Build index from multiple collections
void PatternIndex::Build(const PatternCollection** begin, const PatternCollection** end)
{
    if (!begin || !end)
        return;

    for (auto ptr = begin; ptr != end; ++ptr)
    {
        int index = 0;
        const PatternCollection* patterns = *ptr;
        if (!patterns)
            continue;

        for (auto& rec : patterns->serializableRecords_)
        {
            auto& record = records_.emplace_back();
            record.startIndex_ = elements_.size();
            record.collection_ = patterns;
            record.recordId_ = index;
            ++index;
            record.events_.resize(rec.events_.size());
            for (unsigned eventIndex = 0; eventIndex < record.events_.size(); ++eventIndex)
            {
                const auto& serializabeEvent = rec.events_[eventIndex];
                auto& newEvent = record.events_[eventIndex];
                newEvent.eventId_ = serializabeEvent.serializabeEventId_;
                newEvent.arguments_.insert(
                    serializabeEvent.serializabeArguments_.begin(), serializabeEvent.serializabeArguments_.end());
            }
            for (auto& key : rec.predicate_)
            {
                auto& element = elements_.emplace_back();
                element.key_ = key.word_;
                element.min_ = key.min_;
                element.max_ = key.max_;
                ++record.length_;
            }
            std::sort(elements_.begin() + record.startIndex_, elements_.end(),
                [](Element& a, Element& b) { return a.key_ < b.key_; });
        }
    }
}

} // namespace Urho3D
