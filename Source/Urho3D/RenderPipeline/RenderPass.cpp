// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/RenderPipeline/RenderPass.h"

#include "Urho3D/Core/Context.h"

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

RenderPass::RenderPass(Context* context)
    : Serializable(context)
{
}

RenderPass::~RenderPass()
{
}

void RenderPass::RegisterObject(Context* context)
{
    context->AddAbstractReflection<RenderPass>();

    URHO3D_ATTRIBUTE("Is Enabled By Default", bool, enabledByDefault_, true, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Comment", ea::string, comment_, EMPTY_STRING, AM_DEFAULT);
}

} // namespace Urho3D
