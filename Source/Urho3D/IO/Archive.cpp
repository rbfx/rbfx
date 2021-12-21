//
// Copyright (c) 2017-2020 the rbfx project.
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

#include "../IO/Archive.h"

#include <cassert>

namespace Urho3D
{

ArchiveBlock::~ArchiveBlock()
{
    if (archive_)
        archive_->EndBlock();
}

bool Archive::ValidateName(ea::string_view name)
{
    // Empty names are not allowed
    if (name.empty())
        return false;

    // Name must start with letter or underscore.
    if (!isalpha(name[0]) && name[0] != '_')
        return false;

    // Name must contain only letters, digits or underscores.
    for (const char ch : name)
    {
        if (!isalnum(ch) && ch != '_')
            return false;
    }

    return true;
}

}
