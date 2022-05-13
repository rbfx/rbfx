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

#include "../IO/ArchiveSerializationContainer.h"
#include "../IO/ArchiveSerializationVariant.h"
#include "../IO/FileSystem.h"
#include "../Resource/XMLFile.h"
#include "../IO/Deserializer.h"
#include "Urho3D/Resource/XMLArchive.h"

namespace Urho3D
{
void PatternCollection::SerializableEventPrototype::SerializeInBlock(Archive& archive)
{
    SerializeValue(archive, "name", serializabeEventId_);
    SerializeOptionalValue(archive, "args", serializabeArguments_, EmptyObject{});
}
void PatternCollection::SerializableEventPrototype::Commit()
{
    eventId_ = serializabeEventId_;
    arguments_.insert(serializabeArguments_.begin(), serializabeArguments_.end());
}
void PatternCollection::SerializableElement::SerializeInBlock(Archive& archive)
{
    SerializeValue(archive, "word", word_);
    SerializeOptionalValue(archive, "min", min_, DefaultMin);
    SerializeOptionalValue(archive, "max", max_, DefaultMax);
}
void PatternCollection::SerializableRecord::SerializeInBlock(Archive& archive)
{
    SerializeOptionalValue(archive, "name", name_, {});
    SerializeOptionalValue(archive, "predicate", predicate_, EmptyObject{},
        [&](Archive& archive, const char* name, auto& value) { SerializeVector(archive, name, value, "key"); });
    SerializeOptionalValue(archive, "events", events_, EmptyObject{},
        [&](Archive& archive, const char* name, auto& value) { SerializeVector(archive, name, value, "event"); });
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
    // needsSorting_   |= elements_.size() > 1 && elements_[elements_.size() - 2].key_ > elements_.back().key_;
    dirty_ = true;
}

/// Add key with associated value to the current query.
void PatternQuery::SetKey(StringHash key, float value) {
    for (auto& element : elements_)
    {
        if (element.key_ == key)
        {
            dirty_ |= element.value_ != value;
            element.value_ = value;
            return;
        }
    }
    auto& element = elements_.emplace_back();
    element.key_ = key;
    element.value_ = value;
    //needsSorting_ |= elements_.size() > 1 && elements_[elements_.size() - 2].key_ > elements_.back().key_;
    dirty_ = true;
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
bool PatternQuery::Commit()
{
    if (!dirty_)
        return false;
    dirty_ = false;
    std::sort(elements_.begin(), elements_.end(), [](Element& a, Element& b) { return a.key_ < b.key_; });
    return true;
}

void PatternCollection::Clear()
{
    serializableRecords_.clear();
    elements_.clear();
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
    const int result = serializableRecords_.size();
    serializableRecords_.emplace_back();
    dirtyPattern_ = true;
    dirty_ = true;

    return result;
}

/// Add key requirement to the current pattern.
void PatternCollection::AddKey(const ea::string& key)
{
    if (!dirtyPattern_)
    {
        URHO3D_LOGERROR("BeginPattern should be called before AddKey");
        BeginPattern();
    }
    auto& element = serializableRecords_.back().predicate_.emplace_back();
    element.word_ = key;
}

/// Add key with range requirement to the current pattern.
void PatternCollection::AddKey(const ea::string& key, float min, float max)
{
    if (!dirtyPattern_)
    {
        URHO3D_LOGERROR("BeginPattern should be called before AddKey");
        BeginPattern();
    }
    auto& element = serializableRecords_.back().predicate_.emplace_back();
    element.word_ = key;
    element.min_ = min;
    element.max_ = max;
}
/// Add key with range requirement to the current pattern.
void PatternCollection::AddKeyGreaterOrEqual(const ea::string& key, float min) { AddKey(key, min, DefaultMax); }

/// Add key with range requirement to the current pattern.
void PatternCollection::AddKeyLessOrEqual(const ea::string& key, float max) { AddKey(key, DefaultMin, max); }

/// Add event to the current pattern.
void PatternCollection::AddEvent(const ea::string& eventId, const StringVariantMap& variantMap)
{
    if (!dirtyPattern_)
    {
        URHO3D_LOGERROR("BeginPattern should be called before AddEvent");
        BeginPattern();
    }
    auto& event = serializableRecords_.back().events_.emplace_back();
    event.serializabeEventId_ = eventId;
    event.serializabeArguments_ = variantMap;
}

/// Commit changes and recalculate derived members.
void PatternCollection::CommitPattern()
{
    if (!dirtyPattern_)
    {
        URHO3D_LOGERROR("BeginPattern should be called before CommitPattern");
        BeginPattern();
    }
    dirtyPattern_ = false;
}

void PatternCollection::Commit()
{
    dirty_ = false;
    int index = 0;
    for (auto& rec: serializableRecords_)
    {
        auto& record = records_.emplace_back();
        record.startIndex_ = elements_.size();
        record.recordId_ = index;
        ++index;
        for (auto& key: rec.predicate_)
        {
            auto& element = elements_.emplace_back();
            element.key_ = key.word_;
            element.min_ = key.min_;
            element.max_ = key.max_;
            ++record.length_;
        }
        for (auto& event : rec.events_)
        {
            event.Commit();
        }
        std::sort(elements_.begin() + record.startIndex_, elements_.end(),
            [](Element& a, Element& b) { return a.key_ < b.key_; });

    }
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
            bestMatchIndex = record.recordId_;
            bestMatchLength = record.length_;
        }
    }
    return bestMatchIndex;
}

void PatternCollection::SerializeInBlock(Archive& archive)
{
    SerializeVector(archive, "patterns", serializableRecords_, "pattern");
    if (archive.IsInput())
    {
        Commit();
    }
}

void PatternCollection::SendEvent(int patternIndex, Object* object, bool broadcast) const
{
    if (patternIndex < 0 || patternIndex >= serializableRecords_.size())
        return;
    for (auto& event : serializableRecords_[patternIndex].events_)
    {
        object->SendEvent(event.eventId_, event.arguments_);
    }
}
unsigned PatternCollection::GetNumEvents(int patternIndex) const
{
    if (patternIndex < 0 || patternIndex >= serializableRecords_.size())
        return 0;
    return serializableRecords_[patternIndex].events_.size();
}
StringHash PatternCollection::GetEventId(int patternIndex, unsigned eventIndex) const
{
    if (patternIndex < 0 || patternIndex >= serializableRecords_.size())
        return StringHash{};
    return serializableRecords_[patternIndex].events_[eventIndex].eventId_;
}

const VariantMap& PatternCollection::GetEventArgs(int patternIndex, unsigned eventIndex) const
{
    static VariantMap empty;
    if (patternIndex < 0 || patternIndex >= serializableRecords_.size())
        return empty;
    return serializableRecords_[patternIndex].events_[eventIndex].arguments_;
}

PatternDatabase::PatternDatabase(Context* context): Resource(context)
{
}

PatternDatabase::~PatternDatabase() = default;

void PatternDatabase::RegisterObject(Context* context)
{
    context->RegisterFactory<PatternDatabase>();
}

bool PatternDatabase::BeginLoad(Deserializer& source)
{
    ea::string extension = GetExtension(source.GetName());

    patterns_.Clear();

    const auto xmlFile = MakeShared<XMLFile>(context_);
    if (!xmlFile->Load(source))
        return false;

    XMLInputArchive archive{xmlFile};
    SerializeInBlock(archive);
    return true;
}

/// Save resource. Return true if successful.
bool PatternDatabase::Save(Serializer& dest) const
{
    const auto xmlFile = MakeShared<XMLFile>(context_);
    XMLOutputArchive archive{xmlFile};
    const_cast<PatternDatabase*>(this)->SerializeInBlock(archive);
    xmlFile->Save(dest);
    return true;
}

void PatternDatabase::SerializeInBlock(Archive& archive)
{
    patterns_.SerializeInBlock(archive);
}

}
