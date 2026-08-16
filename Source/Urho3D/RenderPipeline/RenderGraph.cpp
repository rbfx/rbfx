//
// Copyright (c) 2026 the rbfx-blueprint project.
//
// This file is distributed under the MIT license.
//

#include "Urho3D/RenderPipeline/RenderGraph.h"

#include <EASTL/algorithm.h>

namespace Urho3D
{

void RenderGraph::Reset()
{
    resources_.clear();
    passes_.clear();
    executionOrder_.clear();
    compiledResources_.clear();
    lastError_.clear();
    transientAliasGroupCount_ = 0;
    compiled_ = false;
}

RenderGraphResourceHandle RenderGraph::CreateResource(const RenderGraphResourceDesc& sourceDesc)
{
    RenderGraphResourceDesc desc = sourceDesc;
    desc.imported = false;
    resources_.push_back(desc);
    compiled_ = false;
    return static_cast<RenderGraphResourceHandle>(resources_.size() - 1);
}

RenderGraphResourceHandle RenderGraph::ImportResource(const RenderGraphResourceDesc& sourceDesc)
{
    RenderGraphResourceDesc desc = sourceDesc;
    desc.imported = true;
    desc.transient = false;
    resources_.push_back(desc);
    compiled_ = false;
    return static_cast<RenderGraphResourceHandle>(resources_.size() - 1);
}

const RenderGraphResourceDesc* RenderGraph::GetResource(RenderGraphResourceHandle handle) const
{
    return IsValidResource(handle) ? &resources_[handle] : nullptr;
}

RenderGraphPassHandle RenderGraph::AddPass(const RenderGraphPassDesc& desc)
{
    PassRecord record;
    record.desc = desc;
    passes_.push_back(record);
    compiled_ = false;
    return static_cast<RenderGraphPassHandle>(passes_.size() - 1);
}

bool RenderGraph::IsValidResource(RenderGraphResourceHandle handle) const
{
    return handle != InvalidRenderGraphResource && handle < resources_.size();
}

void RenderGraph::SetError(const ea::string& message)
{
    lastError_ = message;
    compiled_ = false;
}

bool RenderGraph::ValidatePasses()
{
    for (unsigned passIndex = 0; passIndex < passes_.size(); ++passIndex)
    {
        const PassRecord& pass = passes_[passIndex];
        if (pass.desc.name.empty())
        {
            SetError("RenderGraph pass name cannot be empty");
            return false;
        }

        for (unsigned previous = 0; previous < passIndex; ++previous)
        {
            if (passes_[previous].desc.name == pass.desc.name)
            {
                SetError("RenderGraph contains duplicate pass: " + pass.desc.name);
                return false;
            }
        }

        for (unsigned useIndex = 0; useIndex < pass.desc.resources.size(); ++useIndex)
        {
            const RenderGraphResourceUse& use = pass.desc.resources[useIndex];
            if (!IsValidResource(use.resource))
            {
                SetError("RenderGraph pass references an invalid resource");
                return false;
            }
            for (unsigned previousUse = 0; previousUse < useIndex; ++previousUse)
            {
                if (pass.desc.resources[previousUse].resource == use.resource)
                {
                    SetError("RenderGraph pass reads or writes the same resource more than once: " + pass.desc.name);
                    return false;
                }
            }
        }
    }
    return true;
}

bool RenderGraph::Compile()
{
    executionOrder_.clear();
    compiledResources_.clear();
    transientAliasGroupCount_ = 0;
    lastError_.clear();
    compiled_ = false;

    if (!ValidatePasses())
        return false;

    for (PassRecord& pass : passes_)
    {
        pass.outgoing.clear();
        pass.incoming = 0;
    }

    ea::vector<RenderGraphPassHandle> lastWriter(resources_.size(), InvalidRenderGraphPass);
    ea::vector<ea::vector<RenderGraphPassHandle>> readers(resources_.size());

    auto addDependency = [this](RenderGraphPassHandle from, RenderGraphPassHandle to)
    {
        if (from == InvalidRenderGraphPass || from == to)
            return;

        for (RenderGraphPassHandle existing : passes_[from].outgoing)
        {
            if (existing == to)
                return;
        }
        passes_[from].outgoing.push_back(to);
        ++passes_[to].incoming;
    };

    for (RenderGraphPassHandle passIndex = 0; passIndex < passes_.size(); ++passIndex)
    {
        for (const RenderGraphResourceUse& use : passes_[passIndex].desc.resources)
        {
            auto& resourceReaders = readers[use.resource];
            if (use.write)
            {
                addDependency(lastWriter[use.resource], passIndex);
                for (RenderGraphPassHandle reader : resourceReaders)
                    addDependency(reader, passIndex);
                resourceReaders.clear();
                lastWriter[use.resource] = passIndex;
            }
            else
            {
                addDependency(lastWriter[use.resource], passIndex);
                resourceReaders.push_back(passIndex);
            }
        }
    }

    ea::vector<RenderGraphPassHandle> ready;
    for (RenderGraphPassHandle passIndex = 0; passIndex < passes_.size(); ++passIndex)
    {
        if (passes_[passIndex].incoming == 0)
            ready.push_back(passIndex);
    }

    while (!ready.empty())
    {
        const RenderGraphPassHandle passIndex = ready.front();
        ready.erase(ready.begin());
        executionOrder_.push_back(passIndex);

        for (RenderGraphPassHandle dependent : passes_[passIndex].outgoing)
        {
            if (--passes_[dependent].incoming == 0)
            {
                auto insertion = ready.begin();
                while (insertion != ready.end() && *insertion < dependent)
                    ++insertion;
                ready.insert(insertion, dependent);
            }
        }
    }

    if (executionOrder_.size() != passes_.size())
    {
        SetError("RenderGraph contains a dependency cycle");
        executionOrder_.clear();
        return false;
    }

    ea::vector<unsigned> executionPositions(passes_.size(), InvalidRenderGraphPass);
    for (unsigned position = 0; position < executionOrder_.size(); ++position)
        executionPositions[executionOrder_[position]] = position;

    compiledResources_.reserve(resources_.size());
    for (RenderGraphResourceHandle resource = 0; resource < resources_.size(); ++resource)
    {
        RenderGraphCompiledResource compiled;
        compiled.handle = resource;
        compiled.desc = resources_[resource];

        for (RenderGraphPassHandle passIndex = 0; passIndex < passes_.size(); ++passIndex)
        {
            for (const RenderGraphResourceUse& use : passes_[passIndex].desc.resources)
            {
                if (use.resource != resource)
                    continue;
                const unsigned position = executionPositions[passIndex];
                if (compiled.firstPass == InvalidRenderGraphPass || position < compiled.firstPass)
                    compiled.firstPass = position;
                if (compiled.lastPass == InvalidRenderGraphPass || position > compiled.lastPass)
                    compiled.lastPass = position;
            }
        }

        compiledResources_.push_back(compiled);
    }

    ea::vector<RenderGraphResourceDesc> aliasDescriptions;
    ea::vector<unsigned> aliasLastPass;
    for (RenderGraphCompiledResource& resource : compiledResources_)
    {
        if (resource.desc.imported || !resource.desc.transient || resource.firstPass == InvalidRenderGraphPass)
            continue;

        unsigned aliasGroup = InvalidRenderGraphResource;
        for (unsigned group = 0; group < aliasDescriptions.size(); ++group)
        {
            if (aliasDescriptions[group] == resource.desc && aliasLastPass[group] < resource.firstPass)
            {
                aliasGroup = group;
                break;
            }
        }

        if (aliasGroup == InvalidRenderGraphResource)
        {
            aliasGroup = static_cast<unsigned>(aliasDescriptions.size());
            aliasDescriptions.push_back(resource.desc);
            aliasLastPass.push_back(resource.lastPass);
        }
        else
            aliasLastPass[aliasGroup] = resource.lastPass;

        resource.aliasGroup = aliasGroup;
    }

    transientAliasGroupCount_ = static_cast<unsigned>(aliasDescriptions.size());
    compiled_ = true;
    return true;
}

bool RenderGraph::Execute(unsigned frameIndex)
{
    if (!compiled_ && !Compile())
        return false;

    for (RenderGraphPassHandle passIndex : executionOrder_)
    {
        const auto& callback = passes_[passIndex].desc.execute;
        if (callback)
        {
            const RenderGraphPassContext context(*this, passIndex, frameIndex);
            callback(context);
        }
    }
    return true;
}

} // namespace Urho3D
