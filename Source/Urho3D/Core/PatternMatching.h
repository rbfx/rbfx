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

#include "../Core/Variant.h"

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

private:
    ea::fixed_vector<Element, 4> elements_;
    bool dirty_{};

    friend class PatternCollection;
};

/// Collection of patterns
class URHO3D_API PatternCollection
{
private:
    struct EventPrototype
    {
        // Serialize content from/to archive. May throw ArchiveException.
        void SerializeInBlock(Archive& archive);

        // Event identifier.
        StringHash eventId_;
        // Event arguments.
        StringVariantMap arguments_;
    };

    struct Record
    {
        int startIndex_{};
        int length_{};
        // One or more event prototypes.
        ea::fixed_vector<EventPrototype, 1> events_;
    };

    struct Element
    {
        // Serialize content from/to archive. May throw ArchiveException.
        void SerializeInBlock(Archive& archive);

        // Element key.
        StringHash key_;
        // Minimum matching value.
        float min_{std::numeric_limits<float>::lowest()};
        // Maximum matching value.
        float max_{std::numeric_limits<float>::max()};
    };

public:
    void Clear();
    /// Start new pattern creation.
    int BeginPattern();
    /// Add key requirement to the current pattern.
    void AddKey(const ea::string& key);
    /// Add key with range requirement to the current pattern.
    void AddKey(const ea::string& key, float min, float max);
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

private:
    ea::unordered_map<StringHash, ea::string> strings_;
    ea::vector<Record> records_;
    ea::vector<Element> elements_;
    bool dirtyPattern_{};
    bool dirty_{};
};

}
