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

#include "PatternMatching.h"

#include "Urho3D/IO/ArchiveSerializationVariant.h"

namespace Urho3D
{
void PatternCollection::EventPrototype::SerializeInBlock(Archive& archive)
{
    SerializeValue(archive, "name", eventId_);
    SerializeValue(archive, "args", arguments_);
}
/// Remove all keys from the query.
void PatternQuery::Clear()
{
    elements_.clear();
    dirty_ = false;
}

/// Add key requirement to the query.
void PatternQuery::SetKey(StringHash key)
{
    for (auto& element: elements_)
    {
        if (element.key_ == key)
            return;
    }
    auto& element = elements_.emplace_back();
    element.key_ = key;
    element.value_ = 1.0f;
    dirty_ |= elements_.size() > 1 && elements_[elements_.size()-2].key_ > elements_.back().key_;
}

/// Add key with associated value to the current query.
void PatternQuery::SetKey(StringHash key, float value) {
    for (auto& element : elements_)
    {
        if (element.key_ == key)
        {
            element.value_ = value;
            return;
        }
    }
    auto& element = elements_.emplace_back();
    element.key_ = key;
    element.value_ = value;
    dirty_ |= elements_.size() > 1 && elements_[elements_.size() - 2].key_ > elements_.back().key_;
}

/// Remove key from the query.
void PatternQuery::RemoveKey(const ea::string& key) {
    for (auto it=elements_.begin(); it!=elements_.end(); ++it)
    {
        if (it->key_ == key)
        {
            elements_.erase_unsorted(it);
            dirty_ = !elements_.empty();
            return;
        }
    }
}

/// Commit changes and recalculate derived members.
void PatternQuery::Commit()
{
    if (!dirty_)
        return;
    dirty_ = false;
    std::sort(elements_.begin(), elements_.end(), [](Element& a, Element& b) { return a.key_ < b.key_; });
}

void PatternCollection::Clear()
{
    elements_.clear();
    strings_.clear();
    records_.clear();
    dirty_ = false;
    dirtyPattern_ = false;
}
/// Start new pattern creation.
int PatternCollection::BeginPattern()
{
    if (dirtyPattern_)
    {
        URHO3D_LOGERROR("Starting new pattern without commiting last one.");
        CommitPattern();
    }
    const int result = records_.size();
    auto& rec = records_.emplace_back();
    rec.startIndex_ = elements_.size();
    rec.length_ = 0;
    dirtyPattern_ = true;
    dirty_ = true;

    return result;
}

/// Add key requirement to the current pattern.
void PatternCollection::AddKey(const ea::string& key)
{
    StringHash hash = key;
    strings_[hash] = key;
    auto& element = elements_.emplace_back();
    element.key_ = key;
    ++records_.back().length_;
}

/// Add key with range requirement to the current pattern.
void PatternCollection::AddKey(const ea::string& key, float min, float max)
{
    StringHash hash = key;
    strings_[hash] = key;
    auto& element = elements_.emplace_back();
    element.key_ = key;
    element.min_ = min;
    element.max_ = max;
    ++records_.back().length_;
}

/// Add event to the current pattern.
void PatternCollection::AddEvent(const ea::string& eventId, const StringVariantMap& variantMap)
{
    StringHash hash = eventId;
    strings_[hash] = eventId;
    auto& event = records_.back().events_.emplace_back();
    event.eventId_ = hash;
    event.arguments_ = variantMap;
}

/// Commit changes and recalculate derived members.
void PatternCollection::CommitPattern()
{
    auto& pattern = records_.back();
    std::sort(elements_.begin() + pattern.startIndex_, elements_.end(),
        [](Element& a, Element& b) { return a.key_ < b.key_; });
    dirtyPattern_ = false;
}

void PatternCollection::Commit()
{
    dirty_ = false;
    //TODO: Sort collection for optimal search
}

int PatternCollection::Query(const PatternQuery& query) const
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
            bestMatchIndex = index;
            bestMatchLength = record.length_;
        }
    }
    return bestMatchIndex;
}

void PatternCollection::SerializeInBlock(Archive& archive)
{
    auto block = archive.OpenArrayBlock("patterns", records_.size());
    if (archive.IsInput())
    {
        Clear();
        const auto numPatterns = block.GetSizeHint();
        for (int i=0; i<numPatterns; ++i)
        {
            ArchiveBlock patternBlock = archive.OpenUnorderedBlock("pattern");
            {
                BeginPattern();
                {
                    auto elementsArray = archive.OpenArrayBlock("elements");
                    const auto numElements = elementsArray.GetSizeHint();
                    for (int index = 0; index < numElements; ++index)
                    {
                        ArchiveBlock elementBlock = archive.OpenUnorderedBlock("element");
                        ea::string keyStr;
                        float min, max;
                        SerializeValue(archive, "key", keyStr);
                        SerializeOptionalValue(archive, "min", min, std::numeric_limits<float>::lowest());
                        SerializeOptionalValue(archive, "max", max, std::numeric_limits<float>::max());
                        AddKey(keyStr, min, max);
                    }
                }
                {
                    auto eventsArray = archive.OpenArrayBlock("events");
                    const auto numEvents = eventsArray.GetSizeHint();
                    for (int index = 0; index < numEvents; ++index)
                    {
                        ArchiveBlock eventBlock = archive.OpenUnorderedBlock("event");
                        ea::string keyStr;
                        StringVariantMap args;
                        SerializeValue(archive, "name", keyStr);
                        SerializeValue(archive, "args", args);
                        AddEvent(keyStr, args);
                    }
                }
                CommitPattern();
            }
        }
        Commit();
    }
    else
    {
        for (auto& record : records_)
        {
            ArchiveBlock block = archive.OpenUnorderedBlock("pattern");
            {
                auto elementsArray = archive.OpenArrayBlock("elements", record.length_);
                for (int index = record.startIndex_; index < record.startIndex_ + record.length_; ++index)
                {
                    auto& element = elements_[index];
                    ArchiveBlock elementBlock = archive.OpenUnorderedBlock("element");
                    auto& keyStr = strings_[element.key_];
                    SerializeValue(archive, "key", keyStr);
                    if (element.min_ > std::numeric_limits<float>::lowest())
                    {
                        SerializeValue(archive, "min", element.min_);
                    }
                    if (element.max_ < std::numeric_limits<float>::max())
                    {
                        SerializeValue(archive, "max", element.max_);
                    }
                }
            }
            {
                auto eventsArray = archive.OpenArrayBlock("events", record.events_.size());
                for (auto& event : record.events_)
                {
                    ArchiveBlock eventBlock = archive.OpenUnorderedBlock("event");
                    auto& eventName = strings_[event.eventId_];
                    SerializeValue(archive, "name", eventName);
                    SerializeValue(archive, "args", event.arguments_);
                }
            }
        }
    }
}

}
