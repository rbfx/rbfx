//
// Copyright (c) 2008-2017 the Urho3D project.
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


#include <Urho3D/Math/MathDefs.h>
#include <Urho3D/Core/StringUtils.h>


namespace Urho3D
{

class IDPool
{
public:
    /// Allocate new unique id.
    StringHash NewID()
    {
        for (;;)
        {
            unsigned hashValue = 0;
            hashValue |= Random(0x10000);
            hashValue |= Random(0x10000) << 16;

            StringHash hash(hashValue);
            if (TakeID(hash))
                return hash;
        }
        assert(false);
    }

    /// Mark id as taken.
    bool TakeID(StringHash id)
    {
        if (pool_.Contains(id))
            return false;

        pool_.Push(id);
        return true;
    }

    /// Clear all taken ids.
    void Clear()
    {
        pool_.Clear();
    }

protected:
    PODVector<StringHash> pool_;
};

}
