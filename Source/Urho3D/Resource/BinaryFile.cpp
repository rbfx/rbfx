//
// Copyright (c) 2020-2020 the rbfx project.
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

#include "../Precompiled.h"

#include "../Core/Context.h"
#include "../IO/Deserializer.h"
#include "../IO/File.h"
#include "../IO/Log.h"
#include "../IO/Serializer.h"
#include "../Resource/BinaryFile.h"

#include "../DebugNew.h"

namespace Urho3D
{

BinaryFile::BinaryFile(Context* context) :
    Resource(context)
{
}

BinaryFile::~BinaryFile() = default;

void BinaryFile::RegisterObject(Context* context)
{
    context->RegisterFactory<BinaryFile>();
}

bool BinaryFile::BeginLoad(Deserializer& source)
{
    source.Seek(0);
    data_.resize(source.GetSize());
    if (source.Read(data_.data(), data_.size()) != data_.size())
    {
        URHO3D_LOGERROR("Can not load binary file" + source.GetName());
        return false;
    }

    SetMemoryUse(data_.size());
    return true;
}

bool BinaryFile::Save(Serializer& dest) const
{
    if (dest.Write(data_.data(), data_.size()) != data_.size())
    {
        URHO3D_LOGERROR("Can not save binary file" + GetName());
        return false;
    }

    return true;
}

bool BinaryFile::SaveFile(const ea::string& fileName) const
{
    File outFile(context_, fileName, FILE_WRITE);
    if (outFile.IsOpen())
        return Save(outFile);
    else
        return false;
}

}
