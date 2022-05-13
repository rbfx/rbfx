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

#include "PatternCollection.h"
#include "PatternQuery.h"
#include "../Core/Object.h"

namespace Urho3D
{

/// Optimized collection of patterns ready for queries
class URHO3D_API PatternIndex
{
    struct Element
    {
        // Element key.
        StringHash key_;
        // Minimum matching value.
        float min_{PatternCollection::DefaultMin};
        // Maximum matching value.
        float max_{PatternCollection::DefaultMax};
    };

    struct Event
    {
        // Event identifier.
        StringHash eventId_;
        // Event arguments.
        VariantMap arguments_;
    };

    struct Record
    {
        int startIndex_{};
        int length_{};
        const PatternCollection* collection_{};
        int recordId_{};
        // One or more event prototypes.
        ea::fixed_vector<Event, 1> events_;
    };

public:
    /// Build index from single collection
    void Build(const PatternCollection* collection);
    /// Build index from multiple collections
    void Build(const PatternCollection** begin, const PatternCollection** end);

    /// Sample value at given time.
    int Query(const PatternQuery& query) const;

    void SendEvent(int patternIndex, Object* object) const;

    unsigned GetNumEvents(int patternIndex) const;
    StringHash GetEventId(int patternIndex, unsigned eventIndex) const;
    const VariantMap& GetEventArgs(int patternIndex, unsigned eventIndex) const;

private:
    ea::vector<Record> records_;
    ea::vector<Element> elements_;
};


} // namespace Urho3D
