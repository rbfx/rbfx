// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/RenderPipeline/RenderPass.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/RenderPipeline/RenderBuffer.h"

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

    // clang-format off
    URHO3D_ATTRIBUTE("Pass Name", ea::string, attributes_.passName_, Attributes{}.passName_, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Is Enabled By Default", bool, attributes_.isEnabledByDefault_, Attributes{}.isEnabledByDefault_, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Comment", ea::string, attributes_.comment_, Attributes{}.comment_, AM_DEFAULT);
    // clang-format on
}

const ea::string& RenderPass::GetPassName() const
{
    return !attributes_.passName_.empty() ? attributes_.passName_ : GetTypeName();
}

void RenderPass::DeclareParameter(const ea::string& name, const Variant& value, StringVariantMap& params) const
{
    if (!params.contains(name))
        params[name] = value;
}

const Variant& RenderPass::LoadParameter(const ea::string& name, const StringVariantMap& params) const
{
    const auto iter = params.find(name);
    return iter != params.end() ? iter->second : Variant::EMPTY;
}

void RenderPass::ConnectToRenderBuffer(
    WeakPtr<RenderBuffer>& renderBuffer, StringHash name, const SharedRenderPassState& sharedState, bool required) const
{
    if (!renderBuffer)
    {
        const auto iter = sharedState.renderBuffers_.find(name);
        if (iter == sharedState.renderBuffers_.end() || !iter->second)
        {
            if (required)
            {
                URHO3D_LOGERROR(
                    "Render buffer {} required by render pass '{}' is not found", name.ToDebugString(), GetPassName());
            }
            return;
        }

        renderBuffer = iter->second;
    }
}

} // namespace Urho3D
