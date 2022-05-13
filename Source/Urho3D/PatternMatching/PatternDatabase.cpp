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

#include "PatternDatabase.h"

#include "../IO/Deserializer.h"
#include "../IO/FileSystem.h"
#include "../Resource/XMLArchive.h"

namespace Urho3D
{

PatternDatabase::PatternDatabase(Context* context)
    : Resource(context)
{
}

PatternDatabase::~PatternDatabase() = default;

void PatternDatabase::RegisterObject(Context* context) { context->RegisterFactory<PatternDatabase>(); }

bool PatternDatabase::BeginLoad(Deserializer& source)
{
    ea::string extension = GetExtension(source.GetName());

    patterns_.Clear();

    const auto xmlFile = MakeShared<XMLFile>(context_);
    if (!xmlFile->Load(source))
        return false;

    XMLInputArchive archive{xmlFile};
    SerializeInBlock(archive);
    index_.Build(&patterns_);
    return true;
}

/// Save resource. Return true if successful.
bool PatternDatabase::Save(Serializer& dest) const
{
    const auto xmlFile = MakeShared<XMLFile>(context_);
    XMLOutputArchive archive{xmlFile};
    const_cast<PatternDatabase*>(this)->SerializeInBlock(archive);
    xmlFile->Save(dest);
    return true;
}

void PatternDatabase::SerializeInBlock(Archive& archive) { patterns_.SerializeInBlock(archive); }

} // namespace Urho3D
