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

#include "PatternQuery.h"

#include <EASTL/sort.h>

namespace Urho3D
{

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
    ea::sort(elements_.begin(), elements_.end(), [](const Element& a, const Element& b) { return a.key_ < b.key_; });
    return true;
}

}
