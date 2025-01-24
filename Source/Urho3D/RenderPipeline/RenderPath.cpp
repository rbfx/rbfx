// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/RenderPipeline/RenderPath.h"

namespace Urho3D
{

RenderPath::RenderPath(Context* context)
    : SimpleResource(context)
{
}

RenderPath::~RenderPath() = default;

void RenderPath::RegisterObject(Context* context)
{
    context->AddFactoryReflection<RenderPath>();
}

void RenderPath::CollectParameters(StringVariantMap& params) const
{
    for (const RenderPass* pass : passes_)
        pass->CollectParameters(params);
}

void RenderPath::InitializeView(RenderPipelineView* view)
{
    for (RenderPass* pass : passes_)
        pass->InitializeView(view);
}

void RenderPath::UpdateParameters(
    const RenderPipelineSettings& settings, const EnabledRenderPasses& enabledPasses, const StringVariantMap& params)
{
    const unsigned numPasses = passes_.size();
    if (enabledPasses.size() != numPasses)
    {
        URHO3D_ASSERTLOG(false,
            "Number of passes in render path ({}) does not match with enabled passes array size ({})", numPasses,
            enabledPasses.size());
        return;
    }

    for (unsigned i = 0; i < numPasses; ++i)
    {
        RenderPass* pass = passes_[i];
        pass->SetEnabled(enabledPasses[i].second);
        pass->UpdateParameters(settings, params);
    }

    traits_ = {};
    for (const RenderPass* pass : passes_)
    {
        const RenderPassTraits& passTraits = pass->GetTraits();

        if (passTraits.needReadWriteColorBuffer_)
            traits_.needReadWriteColorBuffer_ = true;
        if (passTraits.needBilinearColorSampler_)
            traits_.needBilinearColorSampler_ = true;
    }
}

void RenderPath::Update(const SharedRenderPassState& sharedState)
{
    for (RenderPass* pass : passes_)
    {
        if (pass->IsEnabledEffectively())
            pass->Update(sharedState);
    }
}

void RenderPath::Render(const SharedRenderPassState& sharedState)
{
    for (RenderPass* pass : passes_)
    {
        if (pass->IsEnabledEffectively())
            pass->Render(sharedState);
    }
}

void RenderPath::SerializeInBlock(Archive& archive)
{
    if (archive.IsInput())
        passes_.clear();

    SerializeOptionalValue(archive, "passes", passes_);
}

SharedPtr<RenderPath> RenderPath::Clone() const
{
    auto clone = MakeShared<RenderPath>(context_);

    const unsigned numPasses = passes_.size();
    for (unsigned i = 0; i < numPasses; ++i)
    {
        const auto passClone = passes_[i]->Clone();
        if (passClone && passClone->IsInstanceOf<RenderPass>())
            clone->passes_.emplace_back(StaticCast<RenderPass>(passClone));
    }

    return clone;
}

EnabledRenderPasses RenderPath::RepairEnabledRenderPasses(const EnabledRenderPasses& sourcePasses) const
{
    if (sourcePasses.size() == passes_.size())
    {
        bool isMatching = true;
        for (unsigned i = 0; i < passes_.size(); ++i)
        {
            if (passes_[i]->GetPassName() != sourcePasses[i].first)
            {
                isMatching = false;
                break;
            }
        }
        if (isMatching)
            return sourcePasses;
    }

    EnabledRenderPasses result;
    for (const RenderPass* pass : passes_)
    {
        const auto isMatching = [&](const ea::pair<ea::string, bool>& elem) { return elem.first == pass->GetPassName(); };
        const auto iterMatching = ea::find_if(sourcePasses.begin(), sourcePasses.end(), isMatching);
        const bool isEnabled = iterMatching != sourcePasses.end() ? iterMatching->second : pass->IsEnabledByDefault();
        result.emplace_back(pass->GetPassName(), isEnabled);
    }
    return result;
}

void RenderPath::AddPass(RenderPass* pass)
{
    passes_.emplace_back(pass);
}

void RenderPath::RemovePass(RenderPass* pass)
{
    const auto iter = ea::find(passes_.begin(), passes_.end(), pass);
    if (iter != passes_.end())
        passes_.erase(iter);
}

void RenderPath::ReorderPass(RenderPass* pass, unsigned index)
{
    if (index >= passes_.size())
        return;

    const auto iter = ea::find(passes_.begin(), passes_.end(), pass);
    if (iter != passes_.end())
    {
        SharedPtr<RenderPass> passHolder{pass};
        passes_.erase(iter);
        passes_.insert_at(index, SharedPtr<RenderPass>(pass));
    }
}

} // namespace Urho3D
