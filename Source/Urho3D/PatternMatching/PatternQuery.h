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

#include "../Math/StringHash.h"

#include <EASTL/fixed_vector.h>

namespace Urho3D
{
class PatternIndex;

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
    /// Commit changes and recalculate derived members. Returns true if any changes were made to the pattern.
    bool Commit();

    unsigned GetNumKeys() const { return elements_.size(); }
    StringHash GetKeyHash(unsigned index) const { return elements_[index].key_; }
    float GetValue(unsigned index) const { return elements_[index].value_; }

private:
    ea::fixed_vector<Element, 4> elements_;
    bool dirty_{};

    friend class PatternIndex;
};

}
