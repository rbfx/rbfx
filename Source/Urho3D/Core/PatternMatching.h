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

#include "Object.h"
#include "../Core/Variant.h"
#include "Urho3D/Resource/Resource.h"

#include <EASTL/fixed_vector.h>

namespace Urho3D
{

class Archive;
class ArchiveBlock;
class PatternCollection;

/// Collection of patterns
class URHO3D_API PatternQuery
{
private:
    struct Element
    {
        StringHash key_;
        float value_;
    };

public:
    /// Remove all keys from the query.
    void Clear();
    /// Add key requirement to the query.
    void SetKey(StringHash key);
    /// Add key with associated value to the current query.
    void SetKey(StringHash key, float value);
    /// Remove key from the query.
    void RemoveKey(const ea::string& key);
    /// Commit changes and recalculate derived members.
    void Commit();

    unsigned GetNumKeys() const { return elements_.size(); }
    StringHash GetKeyHash(unsigned index) const { return elements_[index].key_; }
    float GetValue(unsigned index) const { return elements_[index].value_; }

private:
    ea::fixed_vector<Element, 4> elements_;
    bool dirty_{};

    friend class PatternCollection;
};

/// Collection of patterns
class URHO3D_API PatternCollection
{
private:
    inline static const float DefaultMin = std::numeric_limits<float>::lowest();
    inline static const float DefaultMax = std::numeric_limits<float>::max();

    struct SerializableElement
    {
        // Serialize content from/to archive. May throw ArchiveException.
        void SerializeInBlock(Archive& archive);

        friend bool operator==(const SerializableElement& lhs, const SerializableElement& rhs)
        {
            return lhs.word_ == rhs.word_
                && lhs.min_ == rhs.min_
                && lhs.max_ == rhs.max_;
        }

        friend bool operator!=(const SerializableElement& lhs, const SerializableElement& rhs)
        {
            return !(lhs == rhs);
        }

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

        void Commit();

        // Serializabe event identifier.
        ea::string serializabeEventId_;
        // Serializabe event arguments.
        StringVariantMap serializabeArguments_;

        // Event identifier.
        StringHash eventId_;
        // Event arguments.
        VariantMap arguments_;

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


    struct Element
    {
        // Element key.
        StringHash key_;
        // Minimum matching value.
        float min_{DefaultMin};
        // Maximum matching value.
        float max_{DefaultMax};
    };

    struct Record
    {
        int startIndex_{};
        int length_{};
        int recordId_{};
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

    /// Commit changes and recalculate derived members.
    /// Query should be called only on committed collection!
    void Commit();
    /// Sample value at given time.
    int Query(const PatternQuery& query) const;

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive);

    void SendEvent(int patternIndex, Object* object, bool broadcast) const;
    unsigned GetNumEvents(int patternIndex) const;
    StringHash GetEventId (int patternIndex, unsigned eventIndex) const;
    const VariantMap& GetEventArgs (int patternIndex, unsigned eventIndex) const;

    bool empty() const { return serializableRecords_.empty(); }

private:
    ea::vector<SerializableRecord> serializableRecords_;
    ea::vector<Record> records_;
    ea::vector<Element> elements_;
    bool dirtyPattern_{};
    bool dirty_{};
};

/// Collection of patterns as resource
class URHO3D_API PatternDatabase : public Resource
{
    URHO3D_OBJECT(PatternDatabase, Resource)
public:
    /// Construct.
    explicit PatternDatabase(Context* context);
    /// Destruct.
    ~PatternDatabase() override;
    /// Register object factory.
    /// @nobind
    static void RegisterObject(Context* context);

    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    bool BeginLoad(Deserializer& source) override;

    /// Save resource. Return true if successful.
    bool Save(Serializer& dest) const override;
    /// Serialize from/to archive. Return true if successful.
    void SerializeInBlock(Archive& archive) override;

    /// Get patterns. Returns pointer for SWIG compatibility.
    PatternCollection* GetPatterns() { return &patterns_; }

private:
    PatternCollection patterns_;
};

}
