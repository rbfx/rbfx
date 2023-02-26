//
// Copyright (c) 2017 James Montemagno / Refractored LLC
// Copyright (c) 2023 the rbfx project.
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

#include "../Core/IteratorRange.h"
#include "../Core/WorkQueue.h"
#include "../IO/Log.h"
#include "../Monetization/BillingManager.h"
#include "../Resource/XMLFile.h"

#include <iomanip>

namespace Urho3D
{

namespace
{

/// Helper class to iterate over XML element children with given name.
class XmlChildIterator : public ea::iterator<EASTL_ITC_NS::forward_iterator_tag, XMLElement>
{
public:
    XmlChildIterator() = default;
    XmlChildIterator(const XMLElement& firstChild, const char* name)
        : element_{firstChild}
        , name_{name}
    {
    }

    bool operator==(const XmlChildIterator& rhs) const
    {
        if (!element_ && !rhs.element_)
            return true;
        else if (!element_ || !rhs.element_)
            return false;
        else
            return element_.GetNode() == rhs.element_.GetNode();
    }

    bool operator!=(const XmlChildIterator& rhs) const
    {
        return !(*this == rhs);
    }

    XmlChildIterator& operator++()
    {
        URHO3D_ASSERT(element_);
        element_ = element_.GetNext(name_);
        return *this;
    }

    XmlChildIterator operator++(int)
    {
        auto copy = *this;
        ++(*this);
        return copy;
    }

    XMLElement operator*() const
    {
        URHO3D_ASSERT(element_);
        return element_;
    }

    XMLElement* operator->() const
    {
        URHO3D_ASSERT(element_);
        return const_cast<XMLElement*>(&element_);
    }

private:
    XMLElement element_;
    const char* name_{};
};

auto ForEachChild(const XMLElement& element, const char* name)
{
    return MakeIteratorRange(XmlChildIterator{element.GetChild(name), name}, XmlChildIterator{});
}

}



BillingManager::BillingManager(Context* context)
    : Object(context)
{
}

BillingManager::~BillingManager()
{
}

} // namespace Urho3D
