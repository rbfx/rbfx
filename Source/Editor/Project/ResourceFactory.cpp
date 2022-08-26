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

#include "../Project/ResourceFactory.h"

#include <Urho3D/IO/FileSystem.h>

#include <EASTL/tuple.h>

namespace Urho3D
{

ResourceFactory::ResourceFactory(Context* context, int group, const ea::string& title)
    : Object(context)
    , group_(group)
    , title_(title)
{
}

bool ResourceFactory::Compare(
    const SharedPtr<ResourceFactory>& lhs, const SharedPtr<ResourceFactory>& rhs)
{
    return ea::tie(lhs->group_, lhs->title_) < ea::tie(rhs->group_, rhs->title_);
}

SimpleResourceFactory::SimpleResourceFactory(Context* context,
    int group, const ea::string& title, const ea::string& fileName, const Callback& callback)
    : ResourceFactory(context, group, title)
    , fileName_(fileName)
    , callback_(callback)
{
    URHO3D_ASSERT(callback_);
}

void SimpleResourceFactory::EndCreate(const ea::string& fileName, const ea::string& resourceName)
{
    callback_(fileName, resourceName);
}

}
