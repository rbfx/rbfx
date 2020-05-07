//
// Copyright (c) 2008-2020 the Urho3D project.
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

#include <EASTL/hash_set.h>

#include "../Core/Object.h"
#include "../Resource/XMLFile.h"

namespace Urho3D
{

class Graphics;
class ShaderVariation;

struct ShaderCombination
{
    ShaderVariation* vertexShader_{};
    ShaderVariation* pixelShader_{};
    ShaderVariation* geometryShader_{};
    ShaderVariation* hullShader_{};
    ShaderVariation* domainShader_{};

    /// Return hash value for HashSet & ea::unordered_map.
    unsigned ToHash() const
    {
        unsigned hash = 0;
        CombineHash(hash, MakeHash(vertexShader_));
        CombineHash(hash, MakeHash(pixelShader_));
        CombineHash(hash, MakeHash(geometryShader_));
        CombineHash(hash, MakeHash(hullShader_));
        CombineHash(hash, MakeHash(domainShader_));
        return hash;
    }

    bool operator==(const ShaderCombination& rhs) const
    {
        return vertexShader_ == rhs.vertexShader_
            && pixelShader_ == rhs.pixelShader_
            && geometryShader_ == rhs.geometryShader_
            && hullShader_ == rhs.hullShader_
            && domainShader_ == rhs.domainShader_;
    }
};


/// Utility class for collecting used shader combinations during runtime for precaching.
class URHO3D_API ShaderPrecache : public Object
{
    URHO3D_OBJECT(ShaderPrecache, Object);

public:
    /// Construct and begin collecting shader combinations. Load existing combinations from XML if the file exists.
    ShaderPrecache(Context* context, const ea::string& fileName);
    /// Destruct. Write the collected shaders to XML.
    ~ShaderPrecache() override;

    /// Collect a shader combination. Called by Graphics when shaders have been set.
    void StoreShaders(ShaderVariation* vs, ShaderVariation* ps, ShaderVariation* gs, ShaderVariation* tcs, ShaderVariation* tes);

    /// Load shaders from an XML file.
    static void LoadShaders(Graphics* graphics, Deserializer& source);

private:
    /// XML file name.
    ea::string fileName_;
    /// XML file.
    XMLFile xmlFile_;
    /// Already encountered shader combinations, pointer version for fast queries.
    ea::hash_set<ShaderCombination> usedPtrCombinations_;
    /// Already encountered shader combinations.
    ea::hash_set<ea::string> usedCombinations_;
};

}
