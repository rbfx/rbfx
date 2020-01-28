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

#include "../Precompiled.h"

#include "../IO/ArchiveSerialization.h"
#include "../Math/SphericalHarmonics.h"

#include "../DebugNew.h"

namespace Urho3D
{

const SphericalHarmonics9 SphericalHarmonics9::ZERO;
const SphericalHarmonicsColor9 SphericalHarmonicsColor9::ZERO;
const SphericalHarmonicsDot9 SphericalHarmonicsDot9::ZERO;

bool SerializeValue(Archive& archive, const char* name, SphericalHarmonicsDot9& value)
{
    if (ArchiveBlock block = archive.OpenUnorderedBlock(name))
    {
        SerializeValue(archive, "Ar", value.Ar_);
        SerializeValue(archive, "Ag", value.Ag_);
        SerializeValue(archive, "Ab", value.Ab_);
        SerializeValue(archive, "Br", value.Br_);
        SerializeValue(archive, "Bg", value.Bg_);
        SerializeValue(archive, "Bb", value.Bb_);
        SerializeValue(archive, "C", value.C_);
        return true;
    }
    return false;
}

}
