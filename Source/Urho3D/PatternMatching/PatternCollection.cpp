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

#include "PatternCollection.h"

#include "../IO/ArchiveSerializationVariant.h"
#include "../IO/ArchiveSerializationContainer.h"

namespace Urho3D
{
void PatternCollection::SerializableEventPrototype::SerializeInBlock(Archive& archive)
{
    SerializeValue(archive, "name", serializabeEventId_);
    SerializeOptionalValue(archive, "args", serializabeArguments_, EmptyObject{});
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

void PatternCollection::Clear()
{
    serializableRecords_.clear();
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

void PatternCollection::SerializeInBlock(Archive& archive) { SerializeInBlock(archive, "patterns"); }
/// Serialize content from/to archive. May throw ArchiveException.
void PatternCollection::SerializeInBlock(Archive& archive, const char* elementName)
{
    SerializeOptionalValue(archive, elementName, serializableRecords_, EmptyObject{},
        [&](Archive& archive, const char* name, auto& value) { SerializeVector(archive, name, value, "pattern"); });
}

} // namespace Urho3D
