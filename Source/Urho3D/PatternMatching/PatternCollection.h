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

#pragma once

#include "../Core/Object.h"
#include <EASTL/fixed_vector.h>

namespace Urho3D
{
class Archive;
class PatternIndex;

/// Collection of patterns
class URHO3D_API PatternCollection
{
private:
    inline static const float DefaultMin = ea::numeric_limits<float>::lowest();
    inline static const float DefaultMax = ea::numeric_limits<float>::max();

    struct SerializableElement
    {
        // Serialize content from/to archive. May throw ArchiveException.
        void SerializeInBlock(Archive& archive);

        friend bool operator==(const SerializableElement& lhs, const SerializableElement& rhs)
        {
            return lhs.word_ == rhs.word_ && lhs.min_ == rhs.min_ && lhs.max_ == rhs.max_;
        }

        friend bool operator!=(const SerializableElement& lhs, const SerializableElement& rhs) { return !(lhs == rhs); }

        // Element key.
        ea::string word_;
        // Minimum matching value.
        float min_{DefaultMin};
        // Maximum matching value.
        float max_{DefaultMax};
    };
    struct SerializableEventPrototype
    {
        // Serialize content from/to archive. May throw ArchiveException.
        void SerializeInBlock(Archive& archive);

        // Serializabe event identifier.
        ea::string serializabeEventId_;
        // Serializabe event arguments.
        StringVariantMap serializabeArguments_;
    };
    struct SerializableRecord
    {
        // Serialize content from/to archive. May throw ArchiveException.
        void SerializeInBlock(Archive& archive);
        // Human readable name of the pattern
        ea::string name_;
        // One or more event prototypes.
        ea::vector<SerializableElement> predicate_;
        // One or more event prototypes.
        ea::fixed_vector<SerializableEventPrototype, 1> events_;
    };

public:
    void Clear();
    /// Start new pattern creation.
    int BeginPattern();
    /// Add key requirement to the current pattern.
    void AddKey(const ea::string& key);
    /// Add key with range requirement to the current pattern.
    void AddKey(const ea::string& key, float min, float max);
    /// Add key with range requirement to the current pattern.
    void AddKeyGreaterOrEqual(const ea::string& key, float min);
    /// Add key with range requirement to the current pattern.
    void AddKeyLessOrEqual(const ea::string& key, float max);
    /// Add event to the current pattern.
    void AddEvent(const ea::string& eventId, const StringVariantMap& variantMap);
    /// Commit changes and recalculate derived members.
    void CommitPattern();

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive);

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive, const char* elementName);

    /// Is collection empty. Required for optional field serialization.
    bool empty() const { return serializableRecords_.empty(); }

private:
    ea::vector<SerializableRecord> serializableRecords_;

    bool dirtyPattern_{};
    bool dirty_{};

    friend class PatternIndex;
};

} // namespace Urho3D
