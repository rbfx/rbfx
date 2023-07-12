/*
 *  Copyright 2019-2023 Diligent Graphics LLC
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
#include "RenderDevice.h"
#include "../../../Platforms/Basic/interface/DebugUtilities.hpp"
#include "../../../Common/interface/RefCntAutoPtr.hpp"

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

    const SubpassDesc& Get() const noexcept
    {
        return Desc;
    }

    operator const SubpassDesc&() const noexcept
    {
        return Desc;
    }

    bool operator==(const SubpassDesc& RHS) const noexcept
    {
        return static_cast<const SubpassDesc&>(*this) == RHS;
    }
    bool operator!=(const SubpassDesc& RHS) const noexcept
    {
        return !(static_cast<const SubpassDesc&>(*this) == RHS);
    }

    bool operator==(const SubpassDescX& RHS) const noexcept
    {
        return *this == static_cast<const SubpassDesc&>(RHS);
    }
    bool operator!=(const SubpassDescX& RHS) const noexcept
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

    const RenderPassDesc& Get() const noexcept
    {
        return Desc;
    }

    operator const RenderPassDesc&() const noexcept
    {
        return Desc;
    }

    bool operator==(const RenderPassDesc& RHS) const noexcept
    {
        return static_cast<const RenderPassDesc&>(*this) == RHS;
    }
    bool operator!=(const RenderPassDesc& RHS) const noexcept
    {
        return !(static_cast<const RenderPassDesc&>(*this) == RHS);
    }

    bool operator==(const RenderPassDescX& RHS) const noexcept
    {
        return *this == static_cast<const RenderPassDesc&>(RHS);
    }
    bool operator!=(const RenderPassDescX& RHS) const noexcept
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
        const LayoutElement Elem{std::forward<ArgsType>(args)...};
        return Add(Elem);
    }

    void Clear()
    {
        InputLayoutDescX EmptyDesc;
        std::swap(*this, EmptyDesc);
    }

    const InputLayoutDesc& Get() const noexcept
    {
        return Desc;
    }

    operator const InputLayoutDesc&() const noexcept
    {
        return Desc;
    }

    bool operator==(const InputLayoutDesc& RHS) const noexcept
    {
        return static_cast<const InputLayoutDesc&>(*this) == RHS;
    }
    bool operator!=(const InputLayoutDesc& RHS) const noexcept
    {
        return !(static_cast<const InputLayoutDesc&>(*this) == RHS);
    }

    bool operator==(const InputLayoutDescX& RHS) const noexcept
    {
        return *this == static_cast<const InputLayoutDesc&>(RHS);
    }
    bool operator!=(const InputLayoutDescX& RHS) const noexcept
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


template <typename DerivedType, typename BaseType>
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

    explicit DeviceObjectAttribsX(const char* Name)
    {
        SetName(Name);
    }
    explicit DeviceObjectAttribsX(const std::string& Name)
    {
        SetName(Name);
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

    DerivedType& SetName(const char* NewName)
    {
        NameCopy   = NewName != nullptr ? NewName : "";
        this->Name = NameCopy.c_str();
        return static_cast<DerivedType&>(*this);
    }

    DerivedType& SetName(const std::string& NewName)
    {
        return SetName(NewName.c_str());
    }

private:
    std::string NameCopy;
};


/// C++ wrapper over FramebufferDesc.
struct FramebufferDescX : DeviceObjectAttribsX<FramebufferDescX, FramebufferDesc>
{
    FramebufferDescX() noexcept
    {}

    FramebufferDescX(const FramebufferDesc& _Desc) :
        DeviceObjectAttribsX{_Desc}
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

    explicit FramebufferDescX(const char* Name) :
        DeviceObjectAttribsX{Name}
    {
    }

    explicit FramebufferDescX(const std::string& Name) :
        DeviceObjectAttribsX{Name}
    {
    }

    FramebufferDescX& operator=(const FramebufferDescX& _DescX)
    {
        FramebufferDescX Copy{_DescX};
        std::swap(*this, Copy);
        return *this;
    }

    FramebufferDescX(FramebufferDescX&&) noexcept = default;
    FramebufferDescX& operator=(FramebufferDescX&&) noexcept = default;

    FramebufferDescX& SetRenderPass(IRenderPass* _pRenderPass) noexcept
    {
        pRenderPass = _pRenderPass;
        return *this;
    }

    FramebufferDescX& AddAttachment(ITextureView* pView)
    {
        Attachments.push_back(pView);
        AttachmentCount = static_cast<Uint32>(Attachments.size());
        ppAttachments   = Attachments.data();
        return *this;
    }

    FramebufferDescX& SetWidth(Uint32 _Width) noexcept
    {
        Width = _Width;
        return *this;
    }

    FramebufferDescX& SetHeight(Uint32 _Height) noexcept
    {
        Height = _Height;
        return *this;
    }

    FramebufferDescX& SetNumArraySlices(Uint32 _NumArraySlices) noexcept
    {
        NumArraySlices = _NumArraySlices;
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
struct PipelineResourceSignatureDescX : DeviceObjectAttribsX<PipelineResourceSignatureDescX, PipelineResourceSignatureDesc>
{
    PipelineResourceSignatureDescX() noexcept
    {}

    PipelineResourceSignatureDescX(const PipelineResourceSignatureDesc& _Desc) :
        DeviceObjectAttribsX{_Desc}
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

    explicit PipelineResourceSignatureDescX(const char* Name) :
        DeviceObjectAttribsX{Name}
    {
    }

    explicit PipelineResourceSignatureDescX(const std::string& Name) :
        DeviceObjectAttribsX{Name}
    {
    }

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
        return SyncDesc();
    }

    template <typename... ArgsType>
    PipelineResourceSignatureDescX& AddResource(ArgsType&&... args)
    {
        const PipelineResourceDesc Res{std::forward<ArgsType>(args)...};
        return AddResource(Res);
    }

    PipelineResourceSignatureDescX& AddImmutableSampler(const ImmutableSamplerDesc& Sam)
    {
        ImtblSamCopy.push_back(Sam);
        ImtblSamCopy.back().SamplerOrTextureName = StringPool.emplace(Sam.SamplerOrTextureName).first->c_str();
        return SyncDesc();
    }

    template <typename... ArgsType>
    PipelineResourceSignatureDescX& AddImmutableSampler(ArgsType&&... args)
    {
        const ImmutableSamplerDesc Sam{std::forward<ArgsType>(args)...};
        return AddImmutableSampler(Sam);
    }

    PipelineResourceSignatureDescX& RemoveResource(const char* ResName, SHADER_TYPE Stages = SHADER_TYPE_ALL)
    {
        VERIFY_EXPR(!IsNullOrEmptyStr(ResName));
        for (auto it = ResCopy.begin(); it != ResCopy.end();)
        {
            if (SafeStrEqual(it->Name, ResName) && (it->ShaderStages & Stages) != 0)
                it = ResCopy.erase(it);
            else
                ++it;
        }
        return SyncDesc();
    }

    PipelineResourceSignatureDescX& RemoveImmutableSampler(const char* SamName, SHADER_TYPE Stages = SHADER_TYPE_ALL)
    {
        VERIFY_EXPR(!IsNullOrEmptyStr(SamName));
        for (auto it = ImtblSamCopy.begin(); it != ImtblSamCopy.end();)
        {
            if (SafeStrEqual(it->SamplerOrTextureName, SamName) && (it->ShaderStages & Stages) != 0)
                it = ImtblSamCopy.erase(it);
            else
                ++it;
        }
        return SyncDesc();
    }

    PipelineResourceSignatureDescX& ClearResources()
    {
        ResCopy.clear();
        return SyncDesc();
    }

    PipelineResourceSignatureDescX& ClearImmutableSamplers()
    {
        ImtblSamCopy.clear();
        return SyncDesc();
    }

    PipelineResourceSignatureDescX& SetBindingIndex(Uint8 _BindingIndex) noexcept
    {
        BindingIndex = _BindingIndex;
        return *this;
    }

    PipelineResourceSignatureDescX& SetUseCombinedTextureSamplers(bool _UseCombinedSamplers) noexcept
    {
        UseCombinedTextureSamplers = _UseCombinedSamplers;
        return *this;
    }

    PipelineResourceSignatureDescX& SetCombinedSamplerSuffix(const char* Suffix)
    {
        CombinedSamplerSuffix = Suffix != nullptr ?
            StringPool.emplace(Suffix).first->c_str() :
            PipelineResourceSignatureDesc{}.CombinedSamplerSuffix;
        return *this;
    }

    PipelineResourceSignatureDescX& Clear()
    {
        PipelineResourceSignatureDescX CleanDesc;
        std::swap(*this, CleanDesc);
        return *this;
    }

private:
    PipelineResourceSignatureDescX& SyncDesc(bool UpdateStrings = false)
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

        return *this;
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

    PipelineResourceLayoutDescX& SetDefaultVariableType(SHADER_RESOURCE_VARIABLE_TYPE DefaultVarType) noexcept
    {
        DefaultVariableType = DefaultVarType;
        return *this;
    }

    PipelineResourceLayoutDescX& SetDefaultVariableMergeStages(SHADER_TYPE DefaultVarMergeStages) noexcept
    {
        DefaultVariableMergeStages = DefaultVarMergeStages;
        return *this;
    }

    PipelineResourceLayoutDescX& AddVariable(const ShaderResourceVariableDesc& Var)
    {
        VarCopy.push_back(Var);
        VarCopy.back().Name = StringPool.emplace(Var.Name).first->c_str();
        return SyncDesc();
    }

    template <typename... ArgsType>
    PipelineResourceLayoutDescX& AddVariable(ArgsType&&... args)
    {
        const ShaderResourceVariableDesc Var{std::forward<ArgsType>(args)...};
        return AddVariable(Var);
    }

    PipelineResourceLayoutDescX& AddImmutableSampler(const ImmutableSamplerDesc& Sam)
    {
        ImtblSamCopy.push_back(Sam);
        ImtblSamCopy.back().SamplerOrTextureName = StringPool.emplace(Sam.SamplerOrTextureName).first->c_str();
        return SyncDesc();
    }

    template <typename... ArgsType>
    PipelineResourceLayoutDescX& AddImmutableSampler(ArgsType&&... args)
    {
        const ImmutableSamplerDesc Sam{std::forward<ArgsType>(args)...};
        return AddImmutableSampler(Sam);
    }

    PipelineResourceLayoutDescX& RemoveVariable(const char* VarName, SHADER_TYPE Stages = SHADER_TYPE_ALL)
    {
        VERIFY_EXPR(!IsNullOrEmptyStr(VarName));
        for (auto it = VarCopy.begin(); it != VarCopy.end();)
        {
            if (SafeStrEqual(it->Name, VarName) && (it->ShaderStages & Stages) != 0)
                it = VarCopy.erase(it);
            else
                ++it;
        }
        return SyncDesc();
    }

    PipelineResourceLayoutDescX& RemoveImmutableSampler(const char* SamName, SHADER_TYPE Stages = SHADER_TYPE_ALL)
    {
        VERIFY_EXPR(!IsNullOrEmptyStr(SamName));
        for (auto it = ImtblSamCopy.begin(); it != ImtblSamCopy.end();)
        {
            if (SafeStrEqual(it->SamplerOrTextureName, SamName) && (it->ShaderStages & Stages) != 0)
                it = ImtblSamCopy.erase(it);
            else
                ++it;
        }
        return SyncDesc();
    }

    PipelineResourceLayoutDescX& ClearVariables()
    {
        VarCopy.clear();
        SyncDesc();
        return *this;
    }

    PipelineResourceLayoutDescX& ClearImmutableSamplers()
    {
        ImtblSamCopy.clear();
        SyncDesc();
        return *this;
    }

    PipelineResourceLayoutDescX& Clear()
    {
        PipelineResourceLayoutDescX CleanDesc;
        std::swap(*this, CleanDesc);
        return *this;
    }

private:
    PipelineResourceLayoutDescX& SyncDesc(bool UpdateStrings = false)
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

        return *this;
    }

    std::vector<ShaderResourceVariableDesc> VarCopy;
    std::vector<ImmutableSamplerDesc>       ImtblSamCopy;
    std::unordered_set<std::string>         StringPool;
};


/// C++ wrapper over BottomLevelASDesc.
struct BottomLevelASDescX : DeviceObjectAttribsX<BottomLevelASDescX, BottomLevelASDesc>
{
    BottomLevelASDescX() noexcept
    {}

    BottomLevelASDescX(const BottomLevelASDesc& _Desc) :
        DeviceObjectAttribsX{_Desc}
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

    explicit BottomLevelASDescX(const char* Name) :
        DeviceObjectAttribsX{Name}
    {
    }
    explicit BottomLevelASDescX(const std::string& Name) :
        DeviceObjectAttribsX{Name}
    {
    }

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
        return SyncDesc();
    }

    template <typename... ArgsType>
    BottomLevelASDescX& AddTriangleGeomerty(ArgsType&&... args)
    {
        const BLASTriangleDesc Tri{std::forward<ArgsType>(args)...};
        return AddTriangleGeomerty(Tri);
    }

    BottomLevelASDescX& AddBoxGeomerty(const BLASBoundingBoxDesc& Geo)
    {
        Boxes.push_back(Geo);
        Boxes.back().GeometryName = StringPool.emplace(Geo.GeometryName).first->c_str();
        return SyncDesc();
    }

    template <typename... ArgsType>
    BottomLevelASDescX& AddBoxGeomerty(ArgsType&&... args)
    {
        const BLASBoundingBoxDesc Box{std::forward<ArgsType>(args)...};
        return AddBoxGeomerty(Box);
    }

    BottomLevelASDescX& RemoveTriangleGeomerty(const char* GeoName)
    {
        return RemoveGeometry(GeoName, Triangles);
    }

    BottomLevelASDescX& RemoveBoxGeomerty(const char* GeoName)
    {
        return RemoveGeometry(GeoName, Boxes);
    }

    BottomLevelASDescX& SetFlags(RAYTRACING_BUILD_AS_FLAGS _Flags) noexcept
    {
        Flags = _Flags;
        return *this;
    }

    BottomLevelASDescX& SetCompactedSize(Uint64 _CompactedSize) noexcept
    {
        CompactedSize = _CompactedSize;
        return *this;
    }

    BottomLevelASDescX& SetImmediateContextMask(Uint64 _ImmediateContextMask) noexcept
    {
        ImmediateContextMask = _ImmediateContextMask;
        return *this;
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
    BottomLevelASDescX& SyncDesc(bool UpdateStrings = false)
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

        return *this;
    }

    template <typename BLASType>
    BottomLevelASDescX& RemoveGeometry(const char* GeoName, std::vector<BLASType>& Geometries)
    {
        VERIFY_EXPR(!IsNullOrEmptyStr(GeoName));
        for (auto it = Geometries.begin(); it != Geometries.end();)
        {
            if (SafeStrEqual(it->GeometryName, GeoName))
                it = Geometries.erase(it);
            else
                ++it;
        }
        return SyncDesc();
    }

    std::vector<BLASTriangleDesc>    Triangles;
    std::vector<BLASBoundingBoxDesc> Boxes;
    std::unordered_set<std::string>  StringPool;
};

/// C++ wrapper over PipelineStateCreateInfo

template <typename DerivedType, typename CreateInfoType>
struct PipelineStateCreateInfoX : CreateInfoType
{
    PipelineStateCreateInfoX() noexcept
    {}

    PipelineStateCreateInfoX(const CreateInfoType& CI) noexcept :
        CreateInfoType{CI}
    {
        SetName(this->PSODesc.Name);
        for (size_t i = 0; i < CI.ResourceSignaturesCount; ++i)
            AddSignature(CI.ppResourceSignatures[i]);
        if (CI.pPSOCache != nullptr)
            SetPipelineStateCache(CI.pPSOCache);
    }

    explicit PipelineStateCreateInfoX(const char* Name)
    {
        SetName(Name);
    }
    explicit PipelineStateCreateInfoX(const std::string& Name)
    {
        SetName(Name);
    }

    PipelineStateCreateInfoX(PipelineStateCreateInfoX&&) noexcept = default;
    PipelineStateCreateInfoX& operator=(PipelineStateCreateInfoX&&) noexcept = default;

    DerivedType& SetName(const char* Name)
    {
        if (Name != nullptr)
            this->PSODesc.Name = StringPool.emplace(Name).first->c_str();
        return static_cast<DerivedType&>(*this);
    }

    DerivedType& SetName(const std::string& Name)
    {
        return SetName(Name.c_str());
    }

    DerivedType& SetResourceLayout(const PipelineResourceLayoutDesc& LayoutDesc) noexcept
    {
        this->PSODesc.ResourceLayout = LayoutDesc;
        return static_cast<DerivedType&>(*this);
    }
    template <typename... ArgsType>
    DerivedType& SetResourceLayout(ArgsType&&... args) noexcept
    {
        const PipelineResourceLayoutDesc Layout{std::forward<ArgsType>(args)...};
        return SetResourceLayout(Layout);
    }

    DerivedType& SetImmediateContextMask(Uint64 ImmediateContextMask) noexcept
    {
        this->PSODesc.ImmediateContextMask = ImmediateContextMask;
        return static_cast<DerivedType&>(*this);
    }

    DerivedType& SetSRBAllocationGranularity(Uint32 SRBAllocationGranularity) noexcept
    {
        this->PSODesc.SRBAllocationGranularity = SRBAllocationGranularity;
        return static_cast<DerivedType&>(*this);
    }

    DerivedType& SetFlags(PSO_CREATE_FLAGS _Flags) noexcept
    {
        this->Flags = _Flags;
        return static_cast<DerivedType&>(*this);
    }

    DerivedType& AddSignature(IPipelineResourceSignature* pSignature)
    {
        if (pSignature == nullptr)
        {
            UNEXPECTED("Signature must not be null");
            return static_cast<DerivedType&>(*this);
        }

        Signatures.emplace_back(pSignature);
        this->ppResourceSignatures    = Signatures.data();
        this->ResourceSignaturesCount = static_cast<Uint32>(Signatures.size());
        VERIFY_EXPR(this->ResourceSignaturesCount <= MAX_RESOURCE_SIGNATURES);
        Objects.emplace_back(pSignature);

        return static_cast<DerivedType&>(*this);
    }

    DerivedType& RemoveSignature(IPipelineResourceSignature* pSignature)
    {
        if (pSignature == nullptr)
        {
            UNEXPECTED("Signature must not be null");
            return static_cast<DerivedType&>(*this);
        }

        for (auto it = Signatures.begin(); it != Signatures.end(); ++it)
        {
            if (*it == pSignature)
            {
                Signatures.erase(it);
                break;
            }
        }

        this->ppResourceSignatures    = Signatures.data();
        this->ResourceSignaturesCount = static_cast<Uint32>(Signatures.size());

        return RemoveObject(pSignature);
    }

    DerivedType& ClearSignatures()
    {
        for (auto* pSign : Signatures)
            RemoveObject(pSign);

        Signatures.clear();
        this->ppResourceSignatures    = nullptr;
        this->ResourceSignaturesCount = 0;

        return static_cast<DerivedType&>(*this);
    }

    DerivedType& SetPipelineStateCache(IPipelineStateCache* pPipelineStateCache)
    {
        if (this->pPSOCache != nullptr)
            RemoveObject(this->pPSOCache);
        this->pPSOCache = pPipelineStateCache;
        Objects.emplace_back(pPipelineStateCache);
        return static_cast<DerivedType&>(*this);
    }

protected:
    DerivedType& SetShader(IShader*& pDstShader, IShader* pShader)
    {
        if (pShader != nullptr)
        {
            if (pDstShader != nullptr)
                RemoveObject(pDstShader);
            pDstShader = pShader;
            Objects.emplace_back(pShader);
        }

        return static_cast<DerivedType&>(*this);
    }

    DerivedType& RemoveObject(IDeviceObject* pObject)
    {
        for (auto it = Objects.begin(); it != Objects.end();)
        {
            if (it->RawPtr() == pObject)
                it = Objects.erase(it);
            else
                ++it;
        }

        return static_cast<DerivedType&>(*this);
    }

protected:
    std::unordered_set<std::string>           StringPool;
    std::vector<RefCntAutoPtr<IDeviceObject>> Objects;
    std::vector<IPipelineResourceSignature*>  Signatures;
};


/// C++ wrapper over GraphicsPipelineStateCreateInfo
struct GraphicsPipelineStateCreateInfoX : PipelineStateCreateInfoX<GraphicsPipelineStateCreateInfoX, GraphicsPipelineStateCreateInfo>
{
    GraphicsPipelineStateCreateInfoX() noexcept
    {}

    GraphicsPipelineStateCreateInfoX(const GraphicsPipelineStateCreateInfo& CI) :
        PipelineStateCreateInfoX{CI}
    {
        if (pVS != nullptr) Objects.emplace_back(pVS);
        if (pPS != nullptr) Objects.emplace_back(pPS);
        if (pDS != nullptr) Objects.emplace_back(pDS);
        if (pHS != nullptr) Objects.emplace_back(pHS);
        if (pGS != nullptr) Objects.emplace_back(pGS);
        if (pAS != nullptr) Objects.emplace_back(pAS);
        if (pMS != nullptr) Objects.emplace_back(pMS);

        if (GraphicsPipeline.pRenderPass != nullptr)
            Objects.emplace_back(GraphicsPipeline.pRenderPass);
    }

    GraphicsPipelineStateCreateInfoX(const GraphicsPipelineStateCreateInfoX& _DescX) :
        GraphicsPipelineStateCreateInfoX{static_cast<const GraphicsPipelineStateCreateInfo&>(_DescX)}
    {}

    explicit GraphicsPipelineStateCreateInfoX(const char* Name) :
        PipelineStateCreateInfoX{Name}
    {
    }
    explicit GraphicsPipelineStateCreateInfoX(const std::string& Name) :
        PipelineStateCreateInfoX{Name}
    {
    }

    GraphicsPipelineStateCreateInfoX& operator=(const GraphicsPipelineStateCreateInfoX& _DescX)
    {
        GraphicsPipelineStateCreateInfoX Copy{_DescX};
        std::swap(*this, Copy);
        return *this;
    }

    GraphicsPipelineStateCreateInfoX(GraphicsPipelineStateCreateInfoX&&) noexcept = default;
    GraphicsPipelineStateCreateInfoX& operator=(GraphicsPipelineStateCreateInfoX&&) noexcept = default;

    GraphicsPipelineStateCreateInfoX& AddShader(IShader* pShader)
    {
        if (pShader == nullptr)
        {
            UNEXPECTED("Shader must not be null");
            return *this;
        }
        switch (pShader->GetDesc().ShaderType)
        {
            // clang-format off
            case SHADER_TYPE_VERTEX:        return SetShader(pVS, pShader);
            case SHADER_TYPE_PIXEL:         return SetShader(pPS, pShader);
            case SHADER_TYPE_GEOMETRY:      return SetShader(pGS, pShader);
            case SHADER_TYPE_HULL:          return SetShader(pHS, pShader);
            case SHADER_TYPE_DOMAIN:        return SetShader(pDS, pShader);
            case SHADER_TYPE_AMPLIFICATION: return SetShader(pAS, pShader);
            case SHADER_TYPE_MESH:          return SetShader(pMS, pShader);
            // clang-format on
            default: UNEXPECTED("Unexpected shader type"); return *this;
        }
    }

    GraphicsPipelineStateCreateInfoX& RemoveShader(IShader* pShader)
    {
        if (pShader == nullptr)
        {
            UNEXPECTED("Shader must not be null");
            return *this;
        }

        if (pVS == pShader) pVS = nullptr;
        if (pPS == pShader) pPS = nullptr;
        if (pGS == pShader) pGS = nullptr;
        if (pHS == pShader) pHS = nullptr;
        if (pDS == pShader) pDS = nullptr;
        if (pAS == pShader) pAS = nullptr;
        if (pMS == pShader) pMS = nullptr;

        return RemoveObject(pShader);
    }

    GraphicsPipelineStateCreateInfoX& SetBlendDesc(const BlendStateDesc& BSDesc) noexcept
    {
        GraphicsPipeline.BlendDesc = BSDesc;
        return *this;
    }

    template <typename... ArgsType>
    GraphicsPipelineStateCreateInfoX& SetBlendDesc(ArgsType&&... args) noexcept
    {
        const BlendStateDesc BSDesc{std::forward<ArgsType>(args)...};
        return SetBlendDesc(BSDesc);
    }

    GraphicsPipelineStateCreateInfoX& SetSampleMask(Uint32 SampleMask) noexcept
    {
        GraphicsPipeline.SampleMask = SampleMask;
        return *this;
    }

    GraphicsPipelineStateCreateInfoX& SetRasterizerDesc(const RasterizerStateDesc& RSDesc) noexcept
    {
        GraphicsPipeline.RasterizerDesc = RSDesc;
        return *this;
    }

    template <typename... ArgsType>
    GraphicsPipelineStateCreateInfoX& SetRasterizerDesc(ArgsType&&... args) noexcept
    {
        const RasterizerStateDesc RSDesc{std::forward<ArgsType>(args)...};
        return SetRasterizerDesc(RSDesc);
    }

    GraphicsPipelineStateCreateInfoX& SetDepthStencilDesc(const DepthStencilStateDesc& DSDesc) noexcept
    {
        GraphicsPipeline.DepthStencilDesc = DSDesc;
        return *this;
    }

    template <typename... ArgsType>
    GraphicsPipelineStateCreateInfoX& SetDepthStencilDesc(ArgsType&&... args) noexcept
    {
        const DepthStencilStateDesc DSSDesc{std::forward<ArgsType>(args)...};
        return SetDepthStencilDesc(DSSDesc);
    }

    GraphicsPipelineStateCreateInfoX& SetInputLayout(const InputLayoutDesc& LayoutDesc) noexcept
    {
        GraphicsPipeline.InputLayout = LayoutDesc;
        return *this;
    }

    GraphicsPipelineStateCreateInfoX& SetNumViewports(Uint8 NumViewports) noexcept
    {
        GraphicsPipeline.NumViewports = NumViewports;
        return *this;
    }

    GraphicsPipelineStateCreateInfoX& SetPrimitiveTopology(PRIMITIVE_TOPOLOGY Topology) noexcept
    {
        GraphicsPipeline.PrimitiveTopology = Topology;
        return *this;
    }

    GraphicsPipelineStateCreateInfoX& SetSubpassIndex(Uint8 SubpassIndex) noexcept
    {
        GraphicsPipeline.SubpassIndex = SubpassIndex;
        return *this;
    }

    GraphicsPipelineStateCreateInfoX& SetShadingRateFlags(PIPELINE_SHADING_RATE_FLAGS ShadingRateFlags) noexcept
    {
        GraphicsPipeline.ShadingRateFlags = ShadingRateFlags;
        return *this;
    }

    GraphicsPipelineStateCreateInfoX& AddRenderTarget(TEXTURE_FORMAT RTVFormat) noexcept
    {
        VERIFY_EXPR(GraphicsPipeline.NumRenderTargets < MAX_RENDER_TARGETS);
        GraphicsPipeline.RTVFormats[GraphicsPipeline.NumRenderTargets++] = RTVFormat;
        return *this;
    }

    GraphicsPipelineStateCreateInfoX& SetDepthFormat(TEXTURE_FORMAT DSVFormat) noexcept
    {
        GraphicsPipeline.DSVFormat = DSVFormat;
        return *this;
    }

    GraphicsPipelineStateCreateInfoX& SetSampleDesc(const SampleDesc& Desc) noexcept
    {
        GraphicsPipeline.SmplDesc = Desc;
        return *this;
    }

    GraphicsPipelineStateCreateInfoX& SetRenderPass(IRenderPass* pRenderPass)
    {
        VERIFY_EXPR(pRenderPass != nullptr);
        if (GraphicsPipeline.pRenderPass != nullptr)
            RemoveObject(GraphicsPipeline.pRenderPass);
        GraphicsPipeline.pRenderPass = pRenderPass;
        Objects.emplace_back(pRenderPass);
        return *this;
    }

    GraphicsPipelineStateCreateInfoX& SetNodeMask(Uint32 NodeMask) noexcept
    {
        GraphicsPipeline.NodeMask = NodeMask;
        return *this;
    }

    GraphicsPipelineStateCreateInfoX& Clear()
    {
        GraphicsPipelineStateCreateInfoX CleanDesc;
        std::swap(*this, CleanDesc);
        return *this;
    }
};


/// C++ wrapper over ComputePipelineStateCreateInfo
struct ComputePipelineStateCreateInfoX : PipelineStateCreateInfoX<ComputePipelineStateCreateInfoX, ComputePipelineStateCreateInfo>
{
    ComputePipelineStateCreateInfoX() noexcept
    {}

    ComputePipelineStateCreateInfoX(const ComputePipelineStateCreateInfo& CI) :
        PipelineStateCreateInfoX{CI}
    {
        if (pCS != nullptr) Objects.emplace_back(pCS);
    }

    ComputePipelineStateCreateInfoX(const ComputePipelineStateCreateInfoX& _DescX) :
        ComputePipelineStateCreateInfoX{static_cast<const ComputePipelineStateCreateInfo&>(_DescX)}
    {}

    explicit ComputePipelineStateCreateInfoX(const char* Name) :
        PipelineStateCreateInfoX{Name}
    {
    }
    explicit ComputePipelineStateCreateInfoX(const std::string& Name) :
        PipelineStateCreateInfoX{Name}
    {
    }

    ComputePipelineStateCreateInfoX& operator=(const ComputePipelineStateCreateInfoX& _DescX)
    {
        ComputePipelineStateCreateInfoX Copy{_DescX};
        std::swap(*this, Copy);
        return *this;
    }

    ComputePipelineStateCreateInfoX(ComputePipelineStateCreateInfoX&&) noexcept = default;
    ComputePipelineStateCreateInfoX& operator=(ComputePipelineStateCreateInfoX&&) noexcept = default;

    ComputePipelineStateCreateInfoX& AddShader(IShader* pShader)
    {
        if (pShader == nullptr)
        {
            UNEXPECTED("Shader must not be null");
            return *this;
        }
        if (pShader->GetDesc().ShaderType == SHADER_TYPE_COMPUTE)
            return SetShader(pCS, pShader);

        UNEXPECTED("Unexpected shader type");
        return *this;
    }

    ComputePipelineStateCreateInfoX& RemoveShader(IShader* pShader)
    {
        if (pShader == nullptr)
        {
            UNEXPECTED("Shader must not be null");
            return *this;
        }

        if (pCS == pShader) pCS = nullptr;

        return RemoveObject(pShader);
    }
};


/// C++ wrapper over TilePipelineStateCreateInfo
struct TilePipelineStateCreateInfoX : PipelineStateCreateInfoX<TilePipelineStateCreateInfoX, TilePipelineStateCreateInfo>
{
    TilePipelineStateCreateInfoX() noexcept
    {}

    TilePipelineStateCreateInfoX(const TilePipelineStateCreateInfo& CI) :
        PipelineStateCreateInfoX{CI}
    {
        if (pTS != nullptr) Objects.emplace_back(pTS);
    }

    TilePipelineStateCreateInfoX(const TilePipelineStateCreateInfoX& _DescX) :
        TilePipelineStateCreateInfoX{static_cast<const TilePipelineStateCreateInfo&>(_DescX)}
    {}

    explicit TilePipelineStateCreateInfoX(const char* Name) :
        PipelineStateCreateInfoX{Name}
    {
    }
    explicit TilePipelineStateCreateInfoX(const std::string& Name) :
        PipelineStateCreateInfoX{Name}
    {
    }

    TilePipelineStateCreateInfoX& operator=(const TilePipelineStateCreateInfoX& _DescX)
    {
        TilePipelineStateCreateInfoX Copy{_DescX};
        std::swap(*this, Copy);
        return *this;
    }

    TilePipelineStateCreateInfoX(TilePipelineStateCreateInfoX&&) noexcept = default;
    TilePipelineStateCreateInfoX& operator=(TilePipelineStateCreateInfoX&&) noexcept = default;

    TilePipelineStateCreateInfoX& SetSampleCount(Uint8 SampleCount) noexcept
    {
        TilePipeline.SampleCount = SampleCount;
        return *this;
    }

    TilePipelineStateCreateInfoX& AddShader(IShader* pShader)
    {
        if (pShader == nullptr)
        {
            UNEXPECTED("Shader must not be null");
            return *this;
        }
        if (pShader->GetDesc().ShaderType == SHADER_TYPE_TILE)
            return SetShader(pTS, pShader);

        UNEXPECTED("Unexpected shader type");
        return *this;
    }

    TilePipelineStateCreateInfoX& RemoveShader(IShader* pShader)
    {
        if (pShader == nullptr)
        {
            UNEXPECTED("Shader must not be null");
            return *this;
        }

        if (pTS == pShader) pTS = nullptr;

        return RemoveObject(pShader);
    }

    TilePipelineStateCreateInfoX& AddRenderTarget(TEXTURE_FORMAT RTVFormat) noexcept
    {
        VERIFY_EXPR(TilePipeline.NumRenderTargets < MAX_RENDER_TARGETS);
        TilePipeline.RTVFormats[TilePipeline.NumRenderTargets++] = RTVFormat;
        return *this;
    }
};

/// C++ wrapper over RayTracingPipelineStateCreateInfo
struct RayTracingPipelineStateCreateInfoX : PipelineStateCreateInfoX<RayTracingPipelineStateCreateInfoX, RayTracingPipelineStateCreateInfo>
{
    RayTracingPipelineStateCreateInfoX() noexcept
    {}

    RayTracingPipelineStateCreateInfoX(const RayTracingPipelineStateCreateInfo& _Desc) :
        PipelineStateCreateInfoX{_Desc}
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

    explicit RayTracingPipelineStateCreateInfoX(const char* Name) :
        PipelineStateCreateInfoX{Name}
    {
    }
    explicit RayTracingPipelineStateCreateInfoX(const std::string& Name) :
        PipelineStateCreateInfoX{Name}
    {
    }

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
        return SyncDesc();
    }

    template <typename... ArgsType>
    RayTracingPipelineStateCreateInfoX& AddGeneralShader(ArgsType&&... args)
    {
        const RayTracingGeneralShaderGroup GenShader{std::forward<ArgsType>(args)...};
        return AddGeneralShader(GenShader);
    }

    RayTracingPipelineStateCreateInfoX& AddTriangleHitShader(const RayTracingTriangleHitShaderGroup& TriHitShader)
    {
        TriangleHitShaders.push_back(TriHitShader);
        TriangleHitShaders.back().Name = StringPool.emplace(TriHitShader.Name).first->c_str();
        return SyncDesc();
    }

    template <typename... ArgsType>
    RayTracingPipelineStateCreateInfoX& AddTriangleHitShader(ArgsType&&... args)
    {
        const RayTracingTriangleHitShaderGroup TriHitShader{std::forward<ArgsType>(args)...};
        return AddTriangleHitShader(TriHitShader);
    }

    RayTracingPipelineStateCreateInfoX& AddProceduralHitShader(const RayTracingProceduralHitShaderGroup& ProcHitShader)
    {
        ProceduralHitShaders.push_back(ProcHitShader);
        ProceduralHitShaders.back().Name = StringPool.emplace(ProcHitShader.Name).first->c_str();
        return SyncDesc();
    }

    template <typename... ArgsType>
    RayTracingPipelineStateCreateInfoX& AddProceduralHitShader(ArgsType&&... args)
    {
        const RayTracingProceduralHitShaderGroup ProcHitShader{std::forward<ArgsType>(args)...};
        return AddProceduralHitShader(ProcHitShader);
    }

    RayTracingPipelineStateCreateInfoX& RemoveGeneralShader(const char* ShaderName)
    {
        return RemoveShader(ShaderName, GeneralShaders);
    }

    RayTracingPipelineStateCreateInfoX& RemoveTriangleHitShader(const char* ShaderName)
    {
        return RemoveShader(ShaderName, TriangleHitShaders);
    }

    RayTracingPipelineStateCreateInfoX& RemoveProceduralHitShader(const char* ShaderName)
    {
        return RemoveShader(ShaderName, ProceduralHitShaders);
    }

    RayTracingPipelineStateCreateInfoX& SetShaderRecordName(const char* RecordName)
    {
        pShaderRecordName = RecordName != nullptr ?
            StringPool.emplace(RecordName).first->c_str() :
            nullptr;
        return *this;
    }

    RayTracingPipelineStateCreateInfoX& ClearGeneralShaders()
    {
        GeneralShaders.clear();
        return SyncDesc();
    }

    RayTracingPipelineStateCreateInfoX& ClearTriangleHitShaders()
    {
        TriangleHitShaders.clear();
        return SyncDesc();
    }

    RayTracingPipelineStateCreateInfoX& ClearProceduralHitShaders()
    {
        ProceduralHitShaders.clear();
        return SyncDesc();
    }

    RayTracingPipelineStateCreateInfoX& Clear()
    {
        RayTracingPipelineStateCreateInfoX CleanDesc;
        std::swap(*this, CleanDesc);
        return *this;
    }

private:
    RayTracingPipelineStateCreateInfoX& SyncDesc(bool UpdateStrings = false)
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

        return *this;
    }

    template <typename ShaderGroupType>
    RayTracingPipelineStateCreateInfoX& RemoveShader(const char* ShaderName, std::vector<ShaderGroupType>& Shaders)
    {
        VERIFY_EXPR(!IsNullOrEmptyStr(ShaderName));
        for (auto it = Shaders.begin(); it != Shaders.end();)
        {
            if (SafeStrEqual(it->Name, ShaderName))
                it = Shaders.erase(it);
            else
                ++it;
        }
        return SyncDesc();
    }

    std::vector<RayTracingGeneralShaderGroup>       GeneralShaders;
    std::vector<RayTracingTriangleHitShaderGroup>   TriangleHitShaders;
    std::vector<RayTracingProceduralHitShaderGroup> ProceduralHitShaders;
};


/// C++ wrapper over IRenderDevice.
template <bool ThrowOnError = true>
class RenderDeviceX
{
public:
    RenderDeviceX() noexcept {}

    explicit RenderDeviceX(IRenderDevice* pDevice) noexcept :
        m_pDevice{pDevice}
    {
        DEV_CHECK_ERR(pDevice, "Device must not be null");
    }

    // clang-format off
    RenderDeviceX           (const RenderDeviceX&) noexcept = default;
    RenderDeviceX& operator=(const RenderDeviceX&) noexcept = default;
    RenderDeviceX           (RenderDeviceX&&)      noexcept = default;
    RenderDeviceX& operator=(RenderDeviceX&&)      noexcept = default;
    // clang-format on

    RefCntAutoPtr<IBuffer> CreateBuffer(const BufferDesc& BuffDesc,
                                        const BufferData* pBuffData = nullptr) noexcept(!ThrowOnError)
    {
        return CreateDeviceObject<IBuffer>("buffer", BuffDesc.Name, &IRenderDevice::CreateBuffer, BuffDesc, pBuffData);
    }

    RefCntAutoPtr<IBuffer> CreateBuffer(const Char*      Name,
                                        Uint64           Size,
                                        USAGE            Usage          = USAGE_DYNAMIC,
                                        BIND_FLAGS       BindFlags      = BIND_UNIFORM_BUFFER,
                                        CPU_ACCESS_FLAGS CPUAccessFlags = CPU_ACCESS_NONE,
                                        const void*      pData          = nullptr) noexcept(!ThrowOnError)
    {
        BufferDesc Desc;
        Desc.Name      = Name;
        Desc.Size      = Size;
        Desc.Usage     = Usage;
        Desc.BindFlags = BindFlags;

        if (Usage == USAGE_DYNAMIC && CPUAccessFlags == CPU_ACCESS_NONE)
            CPUAccessFlags = CPU_ACCESS_WRITE;
        Desc.CPUAccessFlags = CPUAccessFlags;

        BufferData InitialData;
        if (pData != nullptr)
        {
            InitialData.pData    = pData;
            InitialData.DataSize = Size;
        }

        return CreateBuffer(Desc, pData != nullptr ? &InitialData : nullptr);
    }

    RefCntAutoPtr<ITexture> CreateTexture(const TextureDesc& TexDesc,
                                          const TextureData* pData = nullptr) noexcept(!ThrowOnError)
    {
        return CreateDeviceObject<ITexture>("texture", TexDesc.Name, &IRenderDevice::CreateTexture, TexDesc, pData);
    }

    RefCntAutoPtr<IShader> CreateShader(const ShaderCreateInfo& ShaderCI) noexcept(!ThrowOnError)
    {
        return CreateDeviceObject<IShader>("shader", ShaderCI.Desc.Name, &IRenderDevice::CreateShader, ShaderCI);
    }

    template <typename... ArgsType>
    RefCntAutoPtr<IShader> CreateShader(ArgsType&&... args) noexcept(!ThrowOnError)
    {
        const ShaderCreateInfo ShaderCI{std::forward<ArgsType>(args)...};
        return CreateShader(ShaderCI);
    }

    RefCntAutoPtr<ISampler> CreateSampler(const SamplerDesc& SamDesc) noexcept(!ThrowOnError)
    {
        return CreateDeviceObject<ISampler>("sampler", SamDesc.Name, &IRenderDevice::CreateSampler, SamDesc);
    }

    RefCntAutoPtr<IResourceMapping> CreateResourceMapping(const ResourceMappingDesc& Desc) noexcept(!ThrowOnError)
    {
        RefCntAutoPtr<IResourceMapping> pResMapping;
        m_pDevice->CreateResourceMapping(Desc, &pResMapping);
        if (!pResMapping && ThrowOnError)
            LOG_ERROR_AND_THROW("Failed to create resource mapping.");
        return pResMapping;
    }

    RefCntAutoPtr<IPipelineState> CreateGraphicsPipelineState(const GraphicsPipelineStateCreateInfo& CreateInfo) noexcept(!ThrowOnError)
    {
        return CreateDeviceObject<IPipelineState>("graphics pipeline", CreateInfo.PSODesc.Name, &IRenderDevice::CreateGraphicsPipelineState, CreateInfo);
    }

    RefCntAutoPtr<IPipelineState> CreateComputePipelineState(const ComputePipelineStateCreateInfo& CreateInfo) noexcept(!ThrowOnError)
    {
        return CreateDeviceObject<IPipelineState>("compute pipeline", CreateInfo.PSODesc.Name, &IRenderDevice::CreateComputePipelineState, CreateInfo);
    }

    RefCntAutoPtr<IPipelineState> CreateRayTracingPipelineState(const RayTracingPipelineStateCreateInfo& CreateInfo) noexcept(!ThrowOnError)
    {
        return CreateDeviceObject<IPipelineState>("ray-tracing pipeline", CreateInfo.PSODesc.Name, &IRenderDevice::CreateRayTracingPipelineState, CreateInfo);
    }

    RefCntAutoPtr<IPipelineState> CreateTilePipelineState(const TilePipelineStateCreateInfo& CreateInfo) noexcept(!ThrowOnError)
    {
        return CreateDeviceObject<IPipelineState>("tile pipeline", CreateInfo.PSODesc.Name, &IRenderDevice::CreateTilePipelineState, CreateInfo);
    }

    RefCntAutoPtr<IPipelineState> CreatePipelineState(const GraphicsPipelineStateCreateInfo& CreateInfo) noexcept(!ThrowOnError)
    {
        return CreateGraphicsPipelineState(CreateInfo);
    }
    RefCntAutoPtr<IPipelineState> CreatePipelineState(const ComputePipelineStateCreateInfo& CreateInfo) noexcept(!ThrowOnError)
    {
        return CreateComputePipelineState(CreateInfo);
    }
    RefCntAutoPtr<IPipelineState> CreatePipelineState(const RayTracingPipelineStateCreateInfo& CreateInfo) noexcept(!ThrowOnError)
    {
        return CreateRayTracingPipelineState(CreateInfo);
    }
    RefCntAutoPtr<IPipelineState> CreatePipelineState(const TilePipelineStateCreateInfo& CreateInfo) noexcept(!ThrowOnError)
    {
        return CreateTilePipelineState(CreateInfo);
    }

    RefCntAutoPtr<IFence> CreateFence(const FenceDesc& Desc) noexcept(!ThrowOnError)
    {
        return CreateDeviceObject<IFence>("fence", Desc.Name, &IRenderDevice::CreateFence, Desc);
    }

    RefCntAutoPtr<IQuery> CreateQuery(const QueryDesc& Desc) noexcept(!ThrowOnError)
    {
        return CreateDeviceObject<IQuery>("query", Desc.Name, &IRenderDevice::CreateQuery, Desc);
    }

    RefCntAutoPtr<IRenderPass> CreateRenderPass(const RenderPassDesc& Desc) noexcept(!ThrowOnError)
    {
        return CreateDeviceObject<IRenderPass>("render pass", Desc.Name, &IRenderDevice::CreateRenderPass, Desc);
    }

    RefCntAutoPtr<IFramebuffer> CreateFramebuffer(const FramebufferDesc& Desc) noexcept(!ThrowOnError)
    {
        return CreateDeviceObject<IFramebuffer>("framebuffer", Desc.Name, &IRenderDevice::CreateFramebuffer, Desc);
    }

    RefCntAutoPtr<IBottomLevelAS> CreateBLAS(const BottomLevelASDesc& Desc) noexcept(!ThrowOnError)
    {
        return CreateDeviceObject<IBottomLevelAS>("bottom-level AS", Desc.Name, &IRenderDevice::CreateBLAS, Desc);
    }

    RefCntAutoPtr<ITopLevelAS> CreateTLAS(const TopLevelASDesc& Desc) noexcept(!ThrowOnError)
    {
        return CreateDeviceObject<ITopLevelAS>("top-level AS", Desc.Name, &IRenderDevice::CreateTLAS, Desc);
    }

    RefCntAutoPtr<IShaderBindingTable> CreateSBT(const ShaderBindingTableDesc& Desc) noexcept(!ThrowOnError)
    {
        return CreateDeviceObject<IShaderBindingTable>("shader binding table", Desc.Name, &IRenderDevice::CreateSBT, Desc);
    }

    RefCntAutoPtr<IPipelineResourceSignature> CreatePipelineResourceSignature(const PipelineResourceSignatureDesc& Desc) noexcept(!ThrowOnError)
    {
        return CreateDeviceObject<IPipelineResourceSignature>("pipeline resource signature", Desc.Name, &IRenderDevice::CreatePipelineResourceSignature, Desc);
    }

    RefCntAutoPtr<IDeviceMemory> CreateDeviceMemory(const DeviceMemoryCreateInfo& CreateInfo) noexcept(!ThrowOnError)
    {
        return CreateDeviceObject<IDeviceMemory>("device memory", CreateInfo.Desc.Name, &IRenderDevice::CreateDeviceMemory, CreateInfo);
    }

    RefCntAutoPtr<IPipelineStateCache> CreatePipelineStateCache(const PipelineStateCacheCreateInfo& CreateInfo) noexcept(!ThrowOnError)
    {
        return CreateDeviceObject<IPipelineStateCache>("PSO cache", CreateInfo.Desc.Name, &IRenderDevice::CreatePipelineStateCache, CreateInfo);
    }

    const RenderDeviceInfo& GetDeviceInfo() const noexcept
    {
        return m_pDevice->GetDeviceInfo();
    }

    const GraphicsAdapterInfo& GetAdapterInfo() const noexcept
    {
        return m_pDevice->GetAdapterInfo();
    }

    const TextureFormatInfo& GetTextureFormatInfo(TEXTURE_FORMAT TexFormat) noexcept
    {
        return m_pDevice->GetTextureFormatInfo(TexFormat);
    }

    const TextureFormatInfoExt& GetTextureFormatInfoExt(TEXTURE_FORMAT TexFormat) noexcept
    {
        return m_pDevice->GetTextureFormatInfoExt(TexFormat);
    }

    SparseTextureFormatInfo GetSparseTextureFormatInfo(TEXTURE_FORMAT     TexFormat,
                                                       RESOURCE_DIMENSION Dimension,
                                                       Uint32             SampleCount) noexcept
    {
        return m_pDevice->GetSparseTextureFormatInfo(TexFormat, Dimension, SampleCount);
    }

    void ReleaseStaleResources(bool ForceRelease = false) noexcept
    {
        return m_pDevice->ReleaseStaleResources(ForceRelease);
    }

    void IdleGPU() noexcept
    {
        return m_pDevice->IdleGPU();
    }

    IEngineFactory* GetEngineFactory() const noexcept
    {
        return m_pDevice->GetEngineFactory();
    }

    IRenderDevice* GetDevice() const noexcept
    {
        return m_pDevice;
    }

    operator IRenderDevice*() const noexcept
    {
        return GetDevice();
    }

    explicit operator bool() const noexcept
    {
        return m_pDevice != nullptr;
    }

private:
    template <typename ObjectType, typename CreateMethodType, typename... CreateArgsType>
    RefCntAutoPtr<ObjectType> CreateDeviceObject(const char* ObjectTypeName, const char* ObjectName, CreateMethodType Create, CreateArgsType&&... Args) noexcept(!ThrowOnError)
    {
        RefCntAutoPtr<ObjectType> pObject;
        (m_pDevice->*Create)(std::forward<CreateArgsType>(Args)..., &pObject);
        if (ThrowOnError)
        {
            if (!pObject)
                LOG_ERROR_AND_THROW("Failed to create ", ObjectTypeName, " '", (ObjectName != nullptr ? ObjectName : "<unnamed>"), "'.");
        }
        return pObject;
    }

private:
    RefCntAutoPtr<IRenderDevice> m_pDevice;
};

} // namespace Diligent
