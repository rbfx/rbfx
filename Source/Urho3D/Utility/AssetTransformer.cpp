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

#include "../IO/Log.h"
#include "../Utility/AssetTransformer.h"

#include <EASTL/sort.h>
#include <EASTL/unordered_set.h>

namespace Urho3D
{

AssetTransformerContext::AssetTransformerContext(Context* context, const ea::string& resourceName, const ea::string& fileName)
    : context_(context)
    , resourceName_(resourceName)
    , fileName_(fileName)
{
}

AssetTransformer::AssetTransformer(Context* context)
    : Serializable(context)
{
}

bool AssetTransformer::IsApplicable(const AssetTransformerContext& ctx, const AssetTransformerVector& transformers)
{
    for (AssetTransformer* transformer : transformers)
    {
        if (transformer->IsApplicable(ctx))
            return true;
    }
    return false;
}

bool AssetTransformer::Execute(AssetTransformerContext& ctx, const AssetTransformerVector& transformers)
{
    for (AssetTransformer* transformer : transformers)
    {
        if (transformer->IsApplicable(ctx))
        {
            ctx.AddAppliedTransformer(transformer);
            if (!transformer->Execute(ctx))
                return false;
        }
    }
    return true;
}

void AssetTransformer::SetFlavor(const ea::string& value)
{
    if (value.empty())
        flavor_ = "*";
    else if (!value.starts_with("*"))
        flavor_ = Format("*.{}", value);
    else
        flavor_ = value;
}

}
