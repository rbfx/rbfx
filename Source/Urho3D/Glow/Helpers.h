//
// Copyright (c) 2008-2019 the Urho3D project.
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

#include "../Core/Context.h"
#include "../Graphics/Material.h"
#include "../Graphics/RenderPath.h"
#include "../Graphics/Technique.h"
#include "../Resource/ResourceCache.h"
#include "../Resource/XMLFile.h"

#include <EASTL/string.h>

#include <future>

namespace Urho3D
{

/// Parallel loop.
template <class T>
void ParallelFor(unsigned count, unsigned numTasks, const T& callback)
{
    // Post async tasks
    ea::vector<std::future<void>> tasks;
    const unsigned chunkSize = (count + numTasks - 1) / numTasks;
    for (unsigned i = 0; i < numTasks; ++i)
    {
        tasks.push_back(std::async([&, i]()
        {
            const unsigned fromIndex = i * chunkSize;
            const unsigned toIndex = ea::min(fromIndex + chunkSize, count);
            callback(fromIndex, toIndex);
        }));
    }

    // Wait for async tasks
    for (auto& task : tasks)
        task.wait();
}

/// Load render path.
inline SharedPtr<RenderPath> LoadRenderPath(Context* context, const ea::string& renderPathName)
{
    auto renderPath = MakeShared<RenderPath>();
    auto renderPathXml = context->GetCache()->GetResource<XMLFile>(renderPathName);
    if (!renderPath->Load(renderPathXml))
        return nullptr;
    return renderPath;
}

/// Return whether the material is opaque.
inline bool IsMaterialOpaque(Material* material)
{
    const Technique* technique = material->GetTechnique(0);
    if (technique && technique->GetName().contains("alpha", false))
        return false;

    if (material->GetVertexShaderDefines().contains("ALPHAMASK"))
        return false;

    if (material->GetShaderParameter("MatDiffColor").GetVector4().w_ < 1.0f - M_LARGE_EPSILON)
        return false;

    return true;
}

/// Return material diffuse color.
inline Color GetMaterialDiffuseColor(Material* material)
{
    return static_cast<Color>(material->GetShaderParameter("MatDiffColor").GetVector4());
}

/// Return material diffuse texture and UV offsets.
inline Texture* GetMaterialDiffuseTexture(Material* material, Vector4& uOffset, Vector4& vOffset)
{
    Texture* texture = material->GetTexture(TU_DIFFUSE);
    if (!texture)
        return {};

    uOffset = material->GetShaderParameter("UOffset").GetVector4();
    vOffset = material->GetShaderParameter("VOffset").GetVector4();
    return texture;
}


}
