/*
 *  Copyright 2019-2022 Diligent Graphics LLC
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  In no event and under no legal theory, whether in tort (including negligence),
 *  contract, or otherwise, unless required by applicable law (such as deliberate
 *  and grossly negligent acts) or agreed to in writing, shall any Contributor be
 *  liable for any damages, including any direct, indirect, special, incidental,
 *  or consequential damages of any character arising as a result of this License or
 *  out of the use or inability to use the software (including but not limited to damages
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and
 *  all other commercial damages or losses), even if such Contributor has been advised
 *  of the possibility of such damages.
 */

#pragma once

/// \file
/// C++ struct wrappers for basic types.

#include <vector>
#include <utility>
#include <string>
#include <unordered_set>

#include "RenderPass.h"
#include "InputLayout.h"
#include "Framebuffer.h"
#include "PipelineResourceSignature.h"
#include "PipelineState.h"
#include "BottomLevelAS.h"
#include "../../../Platforms/Basic/interface/DebugUtilities.hpp"

namespace Diligent
{

/// C++ wrapper over Diligent::SubpassDesc struct.
struct SubpassDescX
{
    SubpassDescX() noexcept
    {}

    SubpassDescX(const SubpassDesc& _Desc) :
        Desc{_Desc}
    {
        auto CopyAttachments = [](auto*& pAttachments, Uint32 Count, auto& Attachments) {
            if (Count != 0)
            {
                VERIFY_EXPR(pAttachments != nullptr);
                Attachments.assign(pAttachments, pAttachments + Count);
                pAttachments = Attachments.data();
            }
            else
            {
                pAttachments = nullptr;
            }
        };
        CopyAttachments(Desc.pInputAttachments, Desc.InputAttachmentCount, Inputs);
        CopyAttachments(Desc.pRenderTargetAttachments, Desc.RenderTargetAttachmentCount, RenderTargets);
        if (Desc.pResolveAttachments != nullptr)
            CopyAttachments(Desc.pResolveAttachments, Desc.RenderTargetAttachmentCount, Resolves);
        CopyAttachments(Desc.pPreserveAttachments, Desc.PreserveAttachmentCount, Preserves);

        SetDepthStencil(Desc.pDepthStencilAttachment);
        SetShadingRate(Desc.pShadingRateAttachment);
    }

    SubpassDescX(const SubpassDescX& _DescX) :
        SubpassDescX{static_cast<const SubpassDesc&>(_DescX)}
    {}

    SubpassDescX& operator=(const SubpassDescX& _DescX)
    {
        SubpassDescX Copy{_DescX};
        Swap(Copy);
        return *this;
    }

    SubpassDescX(SubpassDescX&& RHS) noexcept
    {
        Swap(RHS);
    }

    SubpassDescX& operator=(SubpassDescX&& RHS) noexcept
    {
        Swap(RHS);
        return *this;
    }

    SubpassDescX& AddRenderTarget(const AttachmentReference& RenderTarget, const AttachmentReference* pResolve = nullptr)
    {
        RenderTargets.push_back(RenderTarget);
        Desc.pRenderTargetAttachments    = RenderTargets.data();
        Desc.RenderTargetAttachmentCount = static_cast<Uint32>(RenderTargets.size());

        if (pResolve != nullptr)
        {
            VERIFY_EXPR(Resolves.size() < RenderTargets.size());
            while (Resolves.size() + 1 < RenderTargets.size())
                Resolves.push_back(AttachmentReference{ATTACHMENT_UNUSED, RESOURCE_STATE_UNKNOWN});
            Resolves.push_back(*pResolve);
            VERIFY_EXPR(Resolves.size() == RenderTargets.size());
            Desc.pResolveAttachments = Resolves.data();
        }

        return *this;
    }

    SubpassDescX& AddInput(const AttachmentReference& Input)
    {
        Inputs.push_back(Input);
        Desc.pInputAttachments    = Inputs.data();
        Desc.InputAttachmentCount = static_cast<Uint32>(Inputs.size());
        return *this;
    }

    SubpassDescX& AddPreserve(Uint32 Preserve)
    {
        Preserves.push_back(Preserve);
        Desc.pPreserveAttachments    = Preserves.data();
        Desc.PreserveAttachmentCount = static_cast<Uint32>(Preserves.size());
        return *this;
    }

    SubpassDescX& SetDepthStencil(const AttachmentReference* pDepthStencilAttachment) noexcept
    {
        DepthStencil                 = (pDepthStencilAttachment != nullptr) ? *pDepthStencilAttachment : AttachmentReference{};
        Desc.pDepthStencilAttachment = (pDepthStencilAttachment != nullptr) ? &DepthStencil : nullptr;
        return *this;
    }

    SubpassDescX& SetDepthStencil(const AttachmentReference& DepthStencilAttachment) noexcept
    {
        return SetDepthStencil(&DepthStencilAttachment);
    }

    SubpassDescX& SetShadingRate(const ShadingRateAttachment* pShadingRateAttachment) noexcept
    {
        ShadingRate                 = (pShadingRateAttachment != nullptr) ? *pShadingRateAttachment : ShadingRateAttachment{};
        Desc.pShadingRateAttachment = (pShadingRateAttachment != nullptr) ? &ShadingRate : nullptr;
        return *this;
    }

    SubpassDescX& SetShadingRate(const ShadingRateAttachment& ShadingRateAttachment) noexcept
    {
        return SetShadingRate(&ShadingRateAttachment);
    }

    void ClearInputs()
    {
        Inputs.clear();
        Desc.InputAttachmentCount = 0;
        Desc.pInputAttachments    = nullptr;
    }

    void ClearRenderTargets()
    {
        RenderTargets.clear();
        Resolves.clear();
        Desc.RenderTargetAttachmentCount = 0;
        Desc.pRenderTargetAttachments    = nullptr;
        Desc.pResolveAttachments         = nullptr;
    }

    void ClearPreserves()
    {
        Preserves.clear();
        Desc.PreserveAttachmentCount = 0;
        Desc.pPreserveAttachments    = nullptr;
    }

    const SubpassDesc& Get() const
    {
        return Desc;
    }

    operator const SubpassDesc&() const
    {
        return Desc;
    }

    bool operator==(const SubpassDesc& RHS) const
    {
        return static_cast<const SubpassDesc&>(*this) == RHS;
    }
    bool operator!=(const SubpassDesc& RHS) const
    {
        return !(static_cast<const SubpassDesc&>(*this) == RHS);
    }

    bool operator==(const SubpassDescX& RHS) const
    {
        return *this == static_cast<const SubpassDesc&>(RHS);
    }
    bool operator!=(const SubpassDescX& RHS) const
    {
        return *this != static_cast<const SubpassDesc&>(RHS);
    }

    void Clear()
    {
        SubpassDescX CleanDesc;
        Swap(CleanDesc);
    }

    void Swap(SubpassDescX& Other) noexcept
    {
        // Using std::swap(*this, Other) will result in infinite recusrion
        std::swap(Desc, Other.Desc);
        Inputs.swap(Other.Inputs);
        RenderTargets.swap(Other.RenderTargets);
        Resolves.swap(Other.Resolves);
        Preserves.swap(Other.Preserves);
        std::swap(DepthStencil, Other.DepthStencil);
        std::swap(ShadingRate, Other.ShadingRate);

        if (Desc.pDepthStencilAttachment != nullptr)
            Desc.pDepthStencilAttachment = &DepthStencil;
        if (Desc.pShadingRateAttachment != nullptr)
            Desc.pShadingRateAttachment = &ShadingRate;

        if (Other.Desc.pDepthStencilAttachment != nullptr)
            Other.Desc.pDepthStencilAttachment = &Other.DepthStencil;
        if (Other.Desc.pShadingRateAttachment != nullptr)
            Other.Desc.pShadingRateAttachment = &Other.ShadingRate;
    }

private:
    SubpassDesc Desc;

    std::vector<AttachmentReference> Inputs;
    std::vector<AttachmentReference> RenderTargets;
    std::vector<AttachmentReference> Resolves;
    std::vector<Uint32>              Preserves;

    AttachmentReference   DepthStencil;
    ShadingRateAttachment ShadingRate;
};

/// C++ wrapper over Diligent::RenderPassDesc.
struct RenderPassDescX
{
    RenderPassDescX() noexcept
    {}

    RenderPassDescX(const RenderPassDesc& _Desc) :
        Desc{_Desc}
    {
        if (Desc.AttachmentCount != 0)
            Attachments.assign(Desc.pAttachments, Desc.pAttachments + Desc.AttachmentCount);

        if (Desc.SubpassCount != 0)
            Subpasses.assign(Desc.pSubpasses, Desc.pSubpasses + Desc.SubpassCount);

        if (Desc.DependencyCount != 0)
            Dependencies.assign(Desc.pDependencies, Desc.pDependencies + Desc.DependencyCount);

        SyncDesc();
    }

    RenderPassDescX(const RenderPassDescX& _DescX) :
        RenderPassDescX{static_cast<const RenderPassDesc&>(_DescX)}
    {}

    RenderPassDescX& operator=(const RenderPassDescX& _DescX)
    {
        RenderPassDescX Copy{_DescX};
        std::swap(*this, Copy);
        return *this;
    }

    RenderPassDescX(RenderPassDescX&&) noexcept = default;
    RenderPassDescX& operator=(RenderPassDescX&&) noexcept = default;

    RenderPassDescX& AddAttachment(const RenderPassAttachmentDesc& Attachment)
    {
        Attachments.push_back(Attachment);
        SyncDesc();
        return *this;
    }

    RenderPassDescX& AddSubpass(const SubpassDesc& Subpass)
    {
        Subpasses.push_back(Subpass);
        SyncDesc();
        return *this;
    }

    RenderPassDescX& AddDependency(const SubpassDependencyDesc& Dependency)
    {
        Dependencies.push_back(Dependency);
        SyncDesc();
        return *this;
    }


    void ClearAttachments()
    {
        Attachments.clear();
        SyncDesc();
    }

    void ClearSubpasses()
    {
        Subpasses.clear();
        SyncDesc();
    }

    void ClearDependencies()
    {
        Dependencies.clear();
        SyncDesc();
    }

    const RenderPassDesc& Get() const
    {
        return Desc;
    }

    operator const RenderPassDesc&() const
    {
        return Desc;
    }

    bool operator==(const RenderPassDesc& RHS) const
    {
        return static_cast<const RenderPassDesc&>(*this) == RHS;
    }
    bool operator!=(const RenderPassDesc& RHS) const
    {
        return !(static_cast<const RenderPassDesc&>(*this) == RHS);
    }

    bool operator==(const RenderPassDescX& RHS) const
    {
        return *this == static_cast<const RenderPassDesc&>(RHS);
    }
    bool operator!=(const RenderPassDescX& RHS) const
    {
        return *this != static_cast<const RenderPassDesc&>(RHS);
    }

    void Clear()
    {
        RenderPassDescX CleanDesc;
        std::swap(*this, CleanDesc);
    }

private:
    void SyncDesc()
    {
        Desc.AttachmentCount = static_cast<Uint32>(Attachments.size());
        Desc.pAttachments    = Desc.AttachmentCount > 0 ? Attachments.data() : nullptr;

        Desc.SubpassCount = static_cast<Uint32>(Subpasses.size());
        Desc.pSubpasses   = Desc.SubpassCount > 0 ? Subpasses.data() : nullptr;

        Desc.DependencyCount = static_cast<Uint32>(Dependencies.size());
        Desc.pDependencies   = Desc.DependencyCount > 0 ? Dependencies.data() : nullptr;
    }

    RenderPassDesc Desc;

    std::vector<RenderPassAttachmentDesc> Attachments;
    std::vector<SubpassDesc>              Subpasses;
    std::vector<SubpassDependencyDesc>    Dependencies;
};


/// C++ wrapper over InputLayoutDesc.
struct InputLayoutDescX
{
    InputLayoutDescX() noexcept
    {}

    InputLayoutDescX(const InputLayoutDesc& _Desc) :
        Desc{_Desc}
    {
        if (Desc.NumElements != 0)
            Elements.assign(Desc.LayoutElements, Desc.LayoutElements + Desc.NumElements);

        SyncDesc(true);
    }

    InputLayoutDescX(const std::initializer_list<LayoutElement>& _Elements) :
        Elements{_Elements}
    {
        SyncDesc(true);
    }

    InputLayoutDescX(const InputLayoutDescX& _DescX) :
        InputLayoutDescX{static_cast<const InputLayoutDesc&>(_DescX)}
    {}

    InputLayoutDescX& operator=(const InputLayoutDescX& _DescX)
    {
        InputLayoutDescX Copy{_DescX};
        std::swap(*this, Copy);
        return *this;
    }

    InputLayoutDescX(InputLayoutDescX&&) noexcept = default;
    InputLayoutDescX& operator=(InputLayoutDescX&&) noexcept = default;

    InputLayoutDescX& Add(const LayoutElement& Elem)
    {
        Elements.push_back(Elem);
        auto& HLSLSemantic = Elements.back().HLSLSemantic;
        HLSLSemantic       = StringPool.emplace(HLSLSemantic).first->c_str();
        SyncDesc();
        return *this;
    }

    template <typename... ArgsType>
    InputLayoutDescX& Add(ArgsType&&... args)
    {
        Elements.emplace_back(std::forward<ArgsType>(args)...);
        auto& HLSLSemantic = Elements.back().HLSLSemantic;
        HLSLSemantic       = StringPool.emplace(HLSLSemantic).first->c_str();
        SyncDesc();
        return *this;
    }

    void Clear()
    {
        InputLayoutDescX EmptyDesc;
        std::swap(*this, EmptyDesc);
    }

    const InputLayoutDesc& Get() const
    {
        return Desc;
    }

    operator const InputLayoutDesc&() const
    {
        return Desc;
    }

    bool operator==(const InputLayoutDesc& RHS) const
    {
        return static_cast<const InputLayoutDesc&>(*this) == RHS;
    }
    bool operator!=(const InputLayoutDesc& RHS) const
    {
        return !(static_cast<const InputLayoutDesc&>(*this) == RHS);
    }

    bool operator==(const InputLayoutDescX& RHS) const
    {
        return *this == static_cast<const InputLayoutDesc&>(RHS);
    }
    bool operator!=(const InputLayoutDescX& RHS) const
    {
        return *this != static_cast<const InputLayoutDesc&>(RHS);
    }

private:
    void SyncDesc(bool CopyStrings = false)
    {
        Desc.NumElements    = static_cast<Uint32>(Elements.size());
        Desc.LayoutElements = Desc.NumElements > 0 ? Elements.data() : nullptr;

        if (CopyStrings)
        {
            for (auto& Elem : Elements)
                Elem.HLSLSemantic = StringPool.emplace(Elem.HLSLSemantic).first->c_str();
        }
    }
    InputLayoutDesc                 Desc;
    std::vector<LayoutElement>      Elements;
    std::unordered_set<std::string> StringPool;
};


template <typename BaseType>
struct DeviceObjectAttribsX : BaseType
{
    DeviceObjectAttribsX() noexcept
    {}

    DeviceObjectAttribsX(const BaseType& Base) :
        BaseType{Base},
        NameCopy{Base.Name != nullptr ? Base.Name : ""}
    {
        this->Name = NameCopy.c_str();
    }

    DeviceObjectAttribsX(const DeviceObjectAttribsX& Other) :
        BaseType{Other},
        NameCopy{Other.NameCopy}
    {
        this->Name = NameCopy.c_str();
    }

    DeviceObjectAttribsX(DeviceObjectAttribsX&& Other) noexcept :
        BaseType{std::move(Other)},
        NameCopy{std::move(Other.NameCopy)}
    {
        this->Name = NameCopy.c_str();
    }

    DeviceObjectAttribsX& operator=(const DeviceObjectAttribsX& Other)
    {
        static_cast<BaseType&>(*this) = Other;

        NameCopy   = Other.NameCopy;
        this->Name = NameCopy.c_str();

        return *this;
    }

    DeviceObjectAttribsX& operator=(const DeviceObjectAttribsX&& Other) noexcept
    {
        static_cast<BaseType&>(*this) = std::move(Other);

        NameCopy   = std::move(Other.NameCopy);
        this->Name = NameCopy.c_str();

        return *this;
    }

    void SetName(const char* NewName)
    {
        NameCopy   = NewName != nullptr ? NewName : "";
        this->Name = NameCopy.c_str();
    }

private:
    std::string NameCopy;
};


/// C++ wrapper over FramebufferDesc.
struct FramebufferDescX : DeviceObjectAttribsX<FramebufferDesc>
{
    using TBase = DeviceObjectAttribsX<FramebufferDesc>;

    FramebufferDescX() noexcept
    {}

    FramebufferDescX(const FramebufferDesc& _Desc) :
        TBase{_Desc}
    {
        if (AttachmentCount != 0)
        {
            Attachments.assign(ppAttachments, ppAttachments + AttachmentCount);
            ppAttachments = Attachments.data();
        }
    }

    FramebufferDescX(const FramebufferDescX& _DescX) :
        FramebufferDescX{static_cast<const FramebufferDesc&>(_DescX)}
    {}

    FramebufferDescX& operator=(const FramebufferDescX& _DescX)
    {
        FramebufferDescX Copy{_DescX};
        std::swap(*this, Copy);
        return *this;
    }

    FramebufferDescX(FramebufferDescX&&) noexcept = default;
    FramebufferDescX& operator=(FramebufferDescX&&) noexcept = default;

    FramebufferDescX& AddAttachment(ITextureView* pView)
    {
        Attachments.push_back(pView);
        AttachmentCount = static_cast<Uint32>(Attachments.size());
        ppAttachments   = Attachments.data();
        return *this;
    }

    void ClearAttachments()
    {
        Attachments.clear();
        AttachmentCount = 0;
        ppAttachments   = nullptr;
    }

    void Clear()
    {
        FramebufferDescX CleanDesc;
        std::swap(*this, CleanDesc);
    }

private:
    std::vector<ITextureView*> Attachments;
};


/// C++ wrapper over PipelineResourceSignatureDesc.
struct PipelineResourceSignatureDescX : DeviceObjectAttribsX<PipelineResourceSignatureDesc>
{
    using TBase = DeviceObjectAttribsX<PipelineResourceSignatureDesc>;

    PipelineResourceSignatureDescX() noexcept
    {}

    PipelineResourceSignatureDescX(const PipelineResourceSignatureDesc& _Desc) :
        TBase{_Desc}
    {
        if (NumResources != 0)
            ResCopy.assign(Resources, Resources + NumResources);

        if (NumImmutableSamplers != 0)
            ImtblSamCopy.assign(ImmutableSamplers, ImmutableSamplers + NumImmutableSamplers);

        SyncDesc(true);
    }

    PipelineResourceSignatureDescX(const std::initializer_list<PipelineResourceDesc>& _Resources,
                                   const std::initializer_list<ImmutableSamplerDesc>& _ImtblSamplers) :
        ResCopy{_Resources},
        ImtblSamCopy{_ImtblSamplers}
    {
        SyncDesc(true);
    }

    PipelineResourceSignatureDescX(const PipelineResourceSignatureDescX& _DescX) :
        PipelineResourceSignatureDescX{static_cast<const PipelineResourceSignatureDesc&>(_DescX)}
    {}

    PipelineResourceSignatureDescX& operator=(const PipelineResourceSignatureDescX& _DescX)
    {
        PipelineResourceSignatureDescX Copy{_DescX};
        std::swap(*this, Copy);
        return *this;
    }

    PipelineResourceSignatureDescX(PipelineResourceSignatureDescX&&) noexcept = default;
    PipelineResourceSignatureDescX& operator=(PipelineResourceSignatureDescX&&) noexcept = default;

    PipelineResourceSignatureDescX& AddResource(const PipelineResourceDesc& Res)
    {
        ResCopy.push_back(Res);
        ResCopy.back().Name = StringPool.emplace(Res.Name).first->c_str();
        SyncDesc();
        return *this;
    }

    PipelineResourceSignatureDescX& AddImmutableSampler(const ImmutableSamplerDesc& Sam)
    {
        ImtblSamCopy.push_back(Sam);
        ImtblSamCopy.back().SamplerOrTextureName = StringPool.emplace(Sam.SamplerOrTextureName).first->c_str();
        SyncDesc();
        return *this;
    }

    void RemoveResource(const char* ResName, SHADER_TYPE Stages = SHADER_TYPE_ALL)
    {
        VERIFY_EXPR(!IsNullOrEmptyStr(ResName));
        for (auto it = ResCopy.begin(); it != ResCopy.end();)
        {
            if (SafeStrEqual(it->Name, ResName) && (it->ShaderStages & Stages) != 0)
                it = ResCopy.erase(it);
            else
                ++it;
        }
        SyncDesc();
    }

    void RemoveImmutableSampler(const char* SamName, SHADER_TYPE Stages = SHADER_TYPE_ALL)
    {
        VERIFY_EXPR(!IsNullOrEmptyStr(SamName));
        for (auto it = ImtblSamCopy.begin(); it != ImtblSamCopy.end();)
        {
            if (SafeStrEqual(it->SamplerOrTextureName, SamName) && (it->ShaderStages & Stages) != 0)
                it = ImtblSamCopy.erase(it);
            else
                ++it;
        }
        SyncDesc();
    }

    void ClearResources()
    {
        ResCopy.clear();
        SyncDesc();
    }

    void ClearImmutableSamplers()
    {
        ImtblSamCopy.clear();
        SyncDesc();
    }

    void SetCombinedSamplerSuffix(const char* Suffix)
    {
        CombinedSamplerSuffix = Suffix != nullptr ?
            StringPool.emplace(Suffix).first->c_str() :
            PipelineResourceSignatureDesc{}.CombinedSamplerSuffix;
    }

    void Clear()
    {
        PipelineResourceSignatureDescX CleanDesc;
        std::swap(*this, CleanDesc);
    }

private:
    void SyncDesc(bool UpdateStrings = false)
    {
        NumResources = static_cast<Uint32>(ResCopy.size());
        Resources    = NumResources > 0 ? ResCopy.data() : nullptr;

        NumImmutableSamplers = static_cast<Uint32>(ImtblSamCopy.size());
        ImmutableSamplers    = NumImmutableSamplers > 0 ? ImtblSamCopy.data() : nullptr;

        if (UpdateStrings)
        {
            for (auto& Res : ResCopy)
                Res.Name = StringPool.emplace(Res.Name).first->c_str();

            for (auto& Sam : ImtblSamCopy)
                Sam.SamplerOrTextureName = StringPool.emplace(Sam.SamplerOrTextureName).first->c_str();

            if (CombinedSamplerSuffix != nullptr)
                CombinedSamplerSuffix = StringPool.emplace(CombinedSamplerSuffix).first->c_str();
        }
    }

    std::vector<PipelineResourceDesc> ResCopy;
    std::vector<ImmutableSamplerDesc> ImtblSamCopy;
    std::unordered_set<std::string>   StringPool;
};


/// C++ wrapper over PipelineResourceSignatureDesc.
struct PipelineResourceLayoutDescX : PipelineResourceLayoutDesc
{
    PipelineResourceLayoutDescX() noexcept
    {}

    PipelineResourceLayoutDescX(const PipelineResourceLayoutDesc& _Desc) :
        PipelineResourceLayoutDesc{_Desc}
    {
        if (NumVariables != 0)
            VarCopy.assign(Variables, Variables + NumVariables);

        if (NumImmutableSamplers != 0)
            ImtblSamCopy.assign(ImmutableSamplers, ImmutableSamplers + NumImmutableSamplers);

        SyncDesc(true);
    }

    PipelineResourceLayoutDescX(const std::initializer_list<ShaderResourceVariableDesc>& _Vars,
                                const std::initializer_list<ImmutableSamplerDesc>&       _ImtblSamplers) :
        VarCopy{_Vars},
        ImtblSamCopy{_ImtblSamplers}
    {
        SyncDesc(true);
    }

    PipelineResourceLayoutDescX(const PipelineResourceLayoutDescX& _DescX) :
        PipelineResourceLayoutDescX{static_cast<const PipelineResourceLayoutDesc&>(_DescX)}
    {}

    PipelineResourceLayoutDescX& operator=(const PipelineResourceLayoutDescX& _DescX)
    {
        PipelineResourceLayoutDescX Copy{_DescX};
        std::swap(*this, Copy);
        return *this;
    }

    PipelineResourceLayoutDescX(PipelineResourceLayoutDescX&&) noexcept = default;
    PipelineResourceLayoutDescX& operator=(PipelineResourceLayoutDescX&&) noexcept = default;

    PipelineResourceLayoutDescX& AddVariable(const ShaderResourceVariableDesc& Var)
    {
        VarCopy.push_back(Var);
        VarCopy.back().Name = StringPool.emplace(Var.Name).first->c_str();
        SyncDesc();
        return *this;
    }

    PipelineResourceLayoutDescX& AddImmutableSampler(const ImmutableSamplerDesc& Sam)
    {
        ImtblSamCopy.push_back(Sam);
        ImtblSamCopy.back().SamplerOrTextureName = StringPool.emplace(Sam.SamplerOrTextureName).first->c_str();
        SyncDesc();
        return *this;
    }

    void RemoveVariable(const char* VarName, SHADER_TYPE Stages = SHADER_TYPE_ALL)
    {
        VERIFY_EXPR(!IsNullOrEmptyStr(VarName));
        for (auto it = VarCopy.begin(); it != VarCopy.end();)
        {
            if (SafeStrEqual(it->Name, VarName) && (it->ShaderStages & Stages) != 0)
                it = VarCopy.erase(it);
            else
                ++it;
        }
        SyncDesc();
    }

    void RemoveImmutableSampler(const char* SamName, SHADER_TYPE Stages = SHADER_TYPE_ALL)
    {
        VERIFY_EXPR(!IsNullOrEmptyStr(SamName));
        for (auto it = ImtblSamCopy.begin(); it != ImtblSamCopy.end();)
        {
            if (SafeStrEqual(it->SamplerOrTextureName, SamName) && (it->ShaderStages & Stages) != 0)
                it = ImtblSamCopy.erase(it);
            else
                ++it;
        }
        SyncDesc();
    }

    void ClearVariables()
    {
        VarCopy.clear();
        SyncDesc();
    }

    void ClearImmutableSamplers()
    {
        ImtblSamCopy.clear();
        SyncDesc();
    }

    void Clear()
    {
        PipelineResourceLayoutDescX CleanDesc;
        std::swap(*this, CleanDesc);
    }

private:
    void SyncDesc(bool UpdateStrings = false)
    {
        NumVariables = static_cast<Uint32>(VarCopy.size());
        Variables    = NumVariables > 0 ? VarCopy.data() : nullptr;

        NumImmutableSamplers = static_cast<Uint32>(ImtblSamCopy.size());
        ImmutableSamplers    = NumImmutableSamplers > 0 ? ImtblSamCopy.data() : nullptr;

        if (UpdateStrings)
        {
            for (auto& Var : VarCopy)
                Var.Name = StringPool.emplace(Var.Name).first->c_str();

            for (auto& Sam : ImtblSamCopy)
                Sam.SamplerOrTextureName = StringPool.emplace(Sam.SamplerOrTextureName).first->c_str();
        }
    }

    std::vector<ShaderResourceVariableDesc> VarCopy;
    std::vector<ImmutableSamplerDesc>       ImtblSamCopy;
    std::unordered_set<std::string>         StringPool;
};


/// C++ wrapper over BottomLevelASDesc.
struct BottomLevelASDescX : DeviceObjectAttribsX<BottomLevelASDesc>
{
    using TBase = DeviceObjectAttribsX<BottomLevelASDesc>;

    BottomLevelASDescX() noexcept
    {}

    BottomLevelASDescX(const BottomLevelASDesc& _Desc) :
        TBase{_Desc}
    {
        if (TriangleCount != 0)
            Triangles.assign(pTriangles, pTriangles + TriangleCount);

        if (BoxCount != 0)
            Boxes.assign(pBoxes, pBoxes + BoxCount);

        SyncDesc(true);
    }

    BottomLevelASDescX(const std::initializer_list<BLASTriangleDesc>&    _Triangles,
                       const std::initializer_list<BLASBoundingBoxDesc>& _Boxes) :
        Triangles{_Triangles},
        Boxes{_Boxes}
    {
        SyncDesc(true);
    }

    BottomLevelASDescX(const BottomLevelASDescX& _DescX) :
        BottomLevelASDescX{static_cast<const BottomLevelASDesc&>(_DescX)}
    {}

    BottomLevelASDescX& operator=(const BottomLevelASDescX& _DescX)
    {
        BottomLevelASDescX Copy{_DescX};
        std::swap(*this, Copy);
        return *this;
    }

    BottomLevelASDescX(BottomLevelASDescX&&) noexcept = default;
    BottomLevelASDescX& operator=(BottomLevelASDescX&&) noexcept = default;

    BottomLevelASDescX& AddTriangleGeomerty(const BLASTriangleDesc& Geo)
    {
        Triangles.push_back(Geo);
        Triangles.back().GeometryName = StringPool.emplace(Geo.GeometryName).first->c_str();
        SyncDesc();
        return *this;
    }

    BottomLevelASDescX& AddBoxGeomerty(const BLASBoundingBoxDesc& Geo)
    {
        Boxes.push_back(Geo);
        Boxes.back().GeometryName = StringPool.emplace(Geo.GeometryName).first->c_str();
        SyncDesc();
        return *this;
    }

    void RemoveTriangleGeomerty(const char* GeoName)
    {
        RemoveGeometry(GeoName, Triangles);
    }

    void RemoveBoxGeomerty(const char* GeoName)
    {
        RemoveGeometry(GeoName, Boxes);
    }

    void ClearTriangles()
    {
        Triangles.clear();
        SyncDesc();
    }

    void ClearBoxes()
    {
        Boxes.clear();
        SyncDesc();
    }

    void Clear()
    {
        BottomLevelASDescX CleanDesc;
        std::swap(*this, CleanDesc);
    }

private:
    void SyncDesc(bool UpdateStrings = false)
    {
        TriangleCount = static_cast<Uint32>(Triangles.size());
        pTriangles    = TriangleCount > 0 ? Triangles.data() : nullptr;

        BoxCount = static_cast<Uint32>(Boxes.size());
        pBoxes   = BoxCount > 0 ? Boxes.data() : nullptr;

        if (UpdateStrings)
        {
            for (auto& Tri : Triangles)
            {
                if (Tri.GeometryName != nullptr)
                    Tri.GeometryName = StringPool.emplace(Tri.GeometryName).first->c_str();
            }

            for (auto& Box : Boxes)
            {
                if (Box.GeometryName != nullptr)
                    Box.GeometryName = StringPool.emplace(Box.GeometryName).first->c_str();
            }
        }
    }

    template <typename BLASType>
    void RemoveGeometry(const char* GeoName, std::vector<BLASType>& Geometries)
    {
        VERIFY_EXPR(!IsNullOrEmptyStr(GeoName));
        for (auto it = Geometries.begin(); it != Geometries.end();)
        {
            if (SafeStrEqual(it->GeometryName, GeoName))
                it = Geometries.erase(it);
            else
                ++it;
        }
        SyncDesc();
    }

    std::vector<BLASTriangleDesc>    Triangles;
    std::vector<BLASBoundingBoxDesc> Boxes;
    std::unordered_set<std::string>  StringPool;
};


/// C++ wrapper over RayTracingPipelineStateCreateInfo
struct RayTracingPipelineStateCreateInfoX : RayTracingPipelineStateCreateInfo
{
    RayTracingPipelineStateCreateInfoX() noexcept
    {}

    RayTracingPipelineStateCreateInfoX(const RayTracingPipelineStateCreateInfo& _Desc) :
        RayTracingPipelineStateCreateInfo{_Desc}
    {
        if (GeneralShaderCount != 0)
            GeneralShaders.assign(pGeneralShaders, pGeneralShaders + GeneralShaderCount);

        if (TriangleHitShaderCount != 0)
            TriangleHitShaders.assign(pTriangleHitShaders, pTriangleHitShaders + TriangleHitShaderCount);

        if (ProceduralHitShaderCount != 0)
            ProceduralHitShaders.assign(pProceduralHitShaders, pProceduralHitShaders + ProceduralHitShaderCount);

        SyncDesc(true);
    }

    RayTracingPipelineStateCreateInfoX(const std::initializer_list<RayTracingGeneralShaderGroup>&       _GeneralShaders,
                                       const std::initializer_list<RayTracingTriangleHitShaderGroup>&   _TriangleHitShaders,
                                       const std::initializer_list<RayTracingProceduralHitShaderGroup>& _ProceduralHitShaders) :
        GeneralShaders{_GeneralShaders},
        TriangleHitShaders{_TriangleHitShaders},
        ProceduralHitShaders{_ProceduralHitShaders}
    {
        SyncDesc(true);
    }

    RayTracingPipelineStateCreateInfoX(const RayTracingPipelineStateCreateInfoX& _DescX) :
        RayTracingPipelineStateCreateInfoX{static_cast<const RayTracingPipelineStateCreateInfo&>(_DescX)}
    {}

    RayTracingPipelineStateCreateInfoX& operator=(const RayTracingPipelineStateCreateInfoX& _DescX)
    {
        RayTracingPipelineStateCreateInfoX Copy{_DescX};
        std::swap(*this, Copy);
        return *this;
    }

    RayTracingPipelineStateCreateInfoX(RayTracingPipelineStateCreateInfoX&&) noexcept = default;
    RayTracingPipelineStateCreateInfoX& operator=(RayTracingPipelineStateCreateInfoX&&) noexcept = default;

    RayTracingPipelineStateCreateInfoX& AddGeneralShader(const RayTracingGeneralShaderGroup& GenShader)
    {
        GeneralShaders.push_back(GenShader);
        GeneralShaders.back().Name = StringPool.emplace(GenShader.Name).first->c_str();
        SyncDesc();
        return *this;
    }

    RayTracingPipelineStateCreateInfoX& AddTriangleHitShader(const RayTracingTriangleHitShaderGroup& TriHitShader)
    {
        TriangleHitShaders.push_back(TriHitShader);
        TriangleHitShaders.back().Name = StringPool.emplace(TriHitShader.Name).first->c_str();
        SyncDesc();
        return *this;
    }

    RayTracingPipelineStateCreateInfoX& AddProceduralHitShader(const RayTracingProceduralHitShaderGroup& ProcHitShader)
    {
        ProceduralHitShaders.push_back(ProcHitShader);
        ProceduralHitShaders.back().Name = StringPool.emplace(ProcHitShader.Name).first->c_str();
        SyncDesc();
        return *this;
    }

    void RemoveGeneralShader(const char* ShaderName)
    {
        RemoveShader(ShaderName, GeneralShaders);
    }

    void RemoveTriangleHitShader(const char* ShaderName)
    {
        RemoveShader(ShaderName, TriangleHitShaders);
    }

    void RemoveProceduralHitShader(const char* ShaderName)
    {
        RemoveShader(ShaderName, ProceduralHitShaders);
    }

    void SetShaderRecordName(const char* RecordName)
    {
        pShaderRecordName = RecordName != nullptr ?
            StringPool.emplace(RecordName).first->c_str() :
            nullptr;
    }

    void ClearGeneralShaders()
    {
        GeneralShaders.clear();
        SyncDesc();
    }

    void ClearTriangleHitShaders()
    {
        TriangleHitShaders.clear();
        SyncDesc();
    }

    void ClearProceduralHitShaders()
    {
        ProceduralHitShaders.clear();
        SyncDesc();
    }

    void Clear()
    {
        RayTracingPipelineStateCreateInfoX CleanDesc;
        std::swap(*this, CleanDesc);
    }

private:
    void SyncDesc(bool UpdateStrings = false)
    {
        GeneralShaderCount = static_cast<Uint32>(GeneralShaders.size());
        pGeneralShaders    = GeneralShaderCount > 0 ? GeneralShaders.data() : nullptr;

        TriangleHitShaderCount = static_cast<Uint32>(TriangleHitShaders.size());
        pTriangleHitShaders    = TriangleHitShaderCount > 0 ? TriangleHitShaders.data() : nullptr;

        ProceduralHitShaderCount = static_cast<Uint32>(ProceduralHitShaders.size());
        pProceduralHitShaders    = ProceduralHitShaderCount > 0 ? ProceduralHitShaders.data() : nullptr;

        if (UpdateStrings)
        {
            for (auto& Shader : GeneralShaders)
                Shader.Name = StringPool.emplace(Shader.Name).first->c_str();

            for (auto& Shader : TriangleHitShaders)
                Shader.Name = StringPool.emplace(Shader.Name).first->c_str();

            for (auto& Shader : ProceduralHitShaders)
                Shader.Name = StringPool.emplace(Shader.Name).first->c_str();

            if (pShaderRecordName != nullptr)
                pShaderRecordName = StringPool.emplace(pShaderRecordName).first->c_str();
        }
    }

    template <typename ShaderGroupType>
    void RemoveShader(const char* ShaderName, std::vector<ShaderGroupType>& Shaders)
    {
        VERIFY_EXPR(!IsNullOrEmptyStr(ShaderName));
        for (auto it = Shaders.begin(); it != Shaders.end();)
        {
            if (SafeStrEqual(it->Name, ShaderName))
                it = Shaders.erase(it);
            else
                ++it;
        }
        SyncDesc();
    }

    std::vector<RayTracingGeneralShaderGroup>       GeneralShaders;
    std::vector<RayTracingTriangleHitShaderGroup>   TriangleHitShaders;
    std::vector<RayTracingProceduralHitShaderGroup> ProceduralHitShaders;
    std::unordered_set<std::string>                 StringPool;
};

} // namespace Diligent
