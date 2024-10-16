/*
 *  Copyright 2019-2024 Diligent Graphics LLC
 *  Copyright 2015-2019 Egor Yusov
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

#include "pch.h"

#include "PipelineStateGLImpl.hpp"

#include <unordered_map>

#include "RenderDeviceGLImpl.hpp"
#include "DeviceContextGLImpl.hpp"
#include "ShaderResourceBindingGLImpl.hpp"
#include "GLTypeConversions.hpp"

#include "EngineMemory.h"
#include "Align.hpp"
#include "GraphicsTypesX.hpp"

namespace Diligent
{

constexpr INTERFACE_ID PipelineStateGLImpl::IID_InternalImpl;

static void VerifyResourceMerge(const PipelineStateDesc&                    PSODesc,
                                const ShaderResourcesGL::GLResourceAttribs& ExistingRes,
                                const ShaderResourcesGL::GLResourceAttribs& NewResAttribs)
{
#define LOG_RESOURCE_MERGE_ERROR_AND_THROW(PropertyName)                                                          \
    LOG_ERROR_AND_THROW("Shader variable '", NewResAttribs.Name,                                                  \
                        "' is shared between multiple shaders in pipeline '", (PSODesc.Name ? PSODesc.Name : ""), \
                        "', but its " PropertyName " varies. A variable shared between multiple shaders "         \
                        "must be defined identically in all shaders. Either use separate variables for "          \
                        "different shader stages, change resource name or make sure that " PropertyName " is consistent.");

    if (ExistingRes.ResourceType != NewResAttribs.ResourceType)
        LOG_RESOURCE_MERGE_ERROR_AND_THROW("type");

    if (ExistingRes.ArraySize != NewResAttribs.ArraySize)
        LOG_RESOURCE_MERGE_ERROR_AND_THROW("array size");
#undef LOG_RESOURCE_MERGE_ERROR_AND_THROW
}

PIPELINE_RESOURCE_FLAGS PipelineStateGLImpl::GetSamplerResourceFlag(const TShaderStages& Stages, bool SilenceWarning) const
{
    VERIFY_EXPR(!Stages.empty());

    constexpr auto UndefinedFlag = static_cast<PIPELINE_RESOURCE_FLAGS>(0xFF);

    auto SamplerResourceFlag = UndefinedFlag;
    bool ConflictsFound      = false;
    for (auto& Stage : Stages)
    {
        auto Lang = Stage->GetSourceLanguage();
        auto Flag = Lang == SHADER_SOURCE_LANGUAGE_HLSL ?
            PIPELINE_RESOURCE_FLAG_NONE :
            PIPELINE_RESOURCE_FLAG_COMBINED_SAMPLER;

        if (SamplerResourceFlag == UndefinedFlag)
            SamplerResourceFlag = Flag;
        else if (SamplerResourceFlag != Flag)
            ConflictsFound = true;
    }

    if (ConflictsFound && !SilenceWarning)
    {
        LOG_WARNING_MESSAGE("Pipeline state '", m_Desc.Name,
                            "' uses shaders compiled from different source languages. When separable programs are not available, this "
                            "may result in incorrect pipeline resource flags (NONE vs COMBINED_SAMPLER) determined for textures.");
    }

    return SamplerResourceFlag;
}

PipelineResourceSignatureDescWrapper PipelineStateGLImpl::GetDefaultResourceSignatureDesc(
    const TShaderStages&              ShaderStages,
    const char*                       PSOName,
    const PipelineResourceLayoutDesc& ResourceLayout,
    Uint32                            SRBAllocationGranularity) noexcept(false)
{
    return PipelineResourceSignatureDescWrapper{PSOName, {}, 1};
}

PipelineResourceSignatureDescWrapper PipelineStateGLImpl::GetDefaultSignatureDesc(
    const TShaderStages& ShaderStages,
    SHADER_TYPE          ActiveStages)
{
    const auto& ResourceLayout = m_Desc.ResourceLayout;

    PipelineResourceSignatureDescWrapper SignDesc{m_Desc.Name, ResourceLayout, m_Desc.SRBAllocationGranularity};
    SignDesc.SetCombinedSamplerSuffix(PipelineResourceSignatureDesc{}.CombinedSamplerSuffix);

    std::unordered_map<ShaderResourceHashKey, const ShaderResourcesGL::GLResourceAttribs&, ShaderResourceHashKey::Hasher> UniqueResources;

    const auto HandleResource = [&](const ShaderResourcesGL::GLResourceAttribs& Attribs) //
    {
        const auto VarDesc     = FindPipelineResourceLayoutVariable(ResourceLayout, Attribs.Name, Attribs.ShaderStages, nullptr);
        const auto it_assigned = UniqueResources.emplace(ShaderResourceHashKey{VarDesc.ShaderStages, Attribs.Name}, Attribs);
        if (it_assigned.second)
        {
            const auto Flags = Attribs.ResourceFlags | ShaderVariableFlagsToPipelineResourceFlags(VarDesc.Flags);
            SignDesc.AddResource(VarDesc.ShaderStages, Attribs.Name, Attribs.ArraySize, Attribs.ResourceType, VarDesc.Type, Flags);
        }
        else
        {
            VerifyResourceMerge(m_Desc, it_assigned.first->second, Attribs);
        }
    };

    if (m_IsProgramPipelineSupported)
    {
        for (size_t i = 0; i < ShaderStages.size(); ++i)
        {
            ShaderGLImpl* pShaderGL = ShaderStages[i];
            pShaderGL->GetShaderResources()->ProcessConstResources(HandleResource, HandleResource, HandleResource, HandleResource);
        }
    }
    else
    {
        auto pImmediateCtx = m_pDevice->GetImmediateContext(0);
        VERIFY_EXPR(pImmediateCtx);
        VERIFY_EXPR(m_GLPrograms[0] != nullptr && m_GLPrograms[0]->GetLinkStatus() == GLProgram::LinkStatus::Succeeded);

        if (!m_GLPrograms[0]->GetResources())
        {
            // Load resources first time this program is used
            const auto SamplerResFlag = GetSamplerResourceFlag(ShaderStages, true /*SilenceWarning*/);
            m_GLPrograms[0]->LoadResources(
                ActiveStages,
                SamplerResFlag,
                pImmediateCtx->GetContextState());
        }
        m_GLPrograms[0]->GetResources()->ProcessConstResources(HandleResource, HandleResource, HandleResource, HandleResource);

        if (ResourceLayout.NumImmutableSamplers > 0)
        {
            // Apply each immutable sampler to all shader stages
            SignDesc.ProcessImmutableSamplers(
                [ActiveStages](ImmutableSamplerDesc& SamDesc) //
                {
                    SamDesc.ShaderStages = ActiveStages;
                });
        }
    }

    return SignDesc;
}

void PipelineStateGLImpl::InitResourceLayout(PSO_CREATE_INTERNAL_FLAGS InternalFlags,
                                             const TShaderStages&      ShaderStages,
                                             SHADER_TYPE               ActiveStages)
{
    if (m_UsingImplicitSignature)
    {
        if ((InternalFlags & PSO_CREATE_INTERNAL_FLAG_IMPLICIT_SIGNATURE0) != 0)
        {
            // Release deserialized default signature as it is empty in OpenGL.
            // We need to create a new one from scratch.
            m_Signatures[0].Release();
        }

        const auto SignDesc = GetDefaultSignatureDesc(ShaderStages, ActiveStages);
        // Always initialize default resource signature as internal device object.
        // This is necessary to avoid cyclic references from TexRegionRenderer.
        // This may never be a problem as the PSO keeps the reference to the device if necessary.
        constexpr bool bIsDeviceInternal = true;
        InitDefaultSignature(SignDesc, GetActiveShaderStages(), bIsDeviceInternal);
        VERIFY_EXPR(m_Signatures[0]);
    }

    DeviceContextGLImpl* pImmediateCtx = m_pDevice->GetImmediateContext(0);
    VERIFY_EXPR(pImmediateCtx != nullptr);
    auto& CtxState = pImmediateCtx->GetContextState();

    if (m_IsProgramPipelineSupported)
    {
        for (size_t i = 0; i < ShaderStages.size(); ++i)
        {
            const ShaderGLImpl*                      pShaderGL  = ShaderStages[i];
            std::shared_ptr<const ShaderResourcesGL> pResources = pShaderGL->GetShaderResources();
            m_GLPrograms[i]->SetResources(pResources);
            ValidateShaderResources(std::move(pResources), pShaderGL->GetDesc().Name, pShaderGL->GetDesc().ShaderType);
        }
    }
    else
    {
        VERIFY_EXPR(m_GLPrograms[0] != nullptr && m_GLPrograms[0]->GetLinkStatus() == GLProgram::LinkStatus::Succeeded);

        std::shared_ptr<const ShaderResourcesGL> pResources = m_GLPrograms[0]->GetResources();
        if (!pResources)
        {
            // If resources are not loaded yet for this program, load them now
            const auto SamplerResFlag = GetSamplerResourceFlag(ShaderStages, false /*SilenceWarning*/);

            pResources = m_GLPrograms[0]->LoadResources(
                ActiveStages,
                GetSamplerResourceFlag(ShaderStages, SamplerResFlag),
                CtxState);
        }
        ValidateShaderResources(std::move(pResources), m_Desc.Name, ActiveStages);
    }

    // Apply resource bindings to programs.
    PipelineResourceSignatureGLImpl::TBindings Bindings = {};
    for (Uint32 s = 0; s < m_SignatureCount; ++s)
    {
        const auto& pSignature = m_Signatures[s];
        if (pSignature == nullptr)
            continue;

        m_BaseBindings[s] = Bindings;
        for (Uint32 p = 0; p < m_NumPrograms; ++p)
        {
            // GL programs are keyed by shader IDs and resource signature IDs or resource layout.
            // Consequently, any pipeline that uses a cached program will assign the same bindings.
            m_GLPrograms[p]->ApplyBindings(pSignature, CtxState, Bindings);
        }

        pSignature->ShiftBindings(Bindings);
    }

    const auto& Limits = GetDevice()->GetDeviceLimits();

    if (Bindings[BINDING_RANGE_UNIFORM_BUFFER] > static_cast<Uint32>(Limits.MaxUniformBlocks))
        LOG_ERROR_AND_THROW("The number of bindings in range '", GetBindingRangeName(BINDING_RANGE_UNIFORM_BUFFER), "' is greater than the maximum allowed (", Limits.MaxUniformBlocks, ").");
    if (Bindings[BINDING_RANGE_TEXTURE] > static_cast<Uint32>(Limits.MaxTextureUnits))
        LOG_ERROR_AND_THROW("The number of bindings in range '", GetBindingRangeName(BINDING_RANGE_TEXTURE), "' is greater than the maximum allowed (", Limits.MaxTextureUnits, ").");
    if (Bindings[BINDING_RANGE_STORAGE_BUFFER] > static_cast<Uint32>(Limits.MaxStorageBlock))
        LOG_ERROR_AND_THROW("The number of bindings in range '", GetBindingRangeName(BINDING_RANGE_STORAGE_BUFFER), "' is greater than the maximum allowed (", Limits.MaxStorageBlock, ").");
    if (Bindings[BINDING_RANGE_IMAGE] > static_cast<Uint32>(Limits.MaxImagesUnits))
        LOG_ERROR_AND_THROW("The number of bindings in range '", GetBindingRangeName(BINDING_RANGE_IMAGE), "' is greater than the maximum allowed (", Limits.MaxImagesUnits, ").");
}

class PipelineStateGLImpl::PipelineBuilderBase
{
public:
    PipelineBuilderBase(PipelineStateGLImpl& Pipeline, const PipelineStateCreateInfo& CreateInfo, TShaderStages Shaders) :
        m_Pipeline{Pipeline},
        m_Shaders{std::move(Shaders)},
        m_CreateAsynchronously{(CreateInfo.Flags & PSO_CREATE_FLAG_ASYNCHRONOUS) != 0 && Pipeline.GetDevice()->GetDeviceInfo().Features.AsyncShaderCompilation}
    {}

    virtual ~PipelineBuilderBase() {}
    virtual bool Tick(bool WaitForCompletion) = 0;

protected:
    enum class State
    {
        Default,
        WaitingShaders,
        LinkingPrograms,
        Complete,
        Failed
    };

    PipelineStateGLImpl& m_Pipeline;
    TShaderStages        m_Shaders;
    const bool           m_CreateAsynchronously;
    State                m_State = State::Default;
};

template <typename PSOCreateInfoType, typename PSOCreateInfoTypeX>
class PipelineStateGLImpl::PipelineBuilder : public PipelineBuilderBase
{
public:
    PipelineBuilder(PipelineStateGLImpl& Pipeline, const PSOCreateInfoType& CreateInfo, TShaderStages Shaders) :
        PipelineBuilderBase{Pipeline, CreateInfo, std::move(Shaders)},
        m_CreateInfo{CreateInfo}
    {
        Pipeline.m_Status.store(PIPELINE_STATE_STATUS_COMPILING);
    }

    virtual bool Tick(bool WaitForCompletion) override final
    {
        VERIFY(m_State != State::Complete && m_State != State::Failed, "The shader is already in final state, this method should not be called");

        if (!m_CreateAsynchronously)
            WaitForCompletion = true;

        if (m_State == State::Default)
        {
            HandleDefaultState();
        }

        if (m_State == State::WaitingShaders)
        {
            HandleWaitingShaders(WaitForCompletion);
        }

        if (m_State == State::LinkingPrograms)
        {
            HandleLinkingPrograms(WaitForCompletion);
        }

        if (m_State == State::Complete)
        {
            m_Pipeline.m_Status.store(PIPELINE_STATE_STATUS_READY);
        }
        else if (m_State == State::Failed)
        {
            for (Uint32 i = 0; i < m_Pipeline.m_NumPrograms; ++i)
            {
                m_Pipeline.m_GLPrograms[i].reset();
            }
            m_Pipeline.m_Status.store(PIPELINE_STATE_STATUS_FAILED);
        }

        return m_State == State::Complete || m_State == State::Failed;
    }

private:
    void HandleDefaultState()
    {
        VERIFY_EXPR(m_State == State::Default);
        m_Pipeline.InitInternalObjects<PSOCreateInfoType>(m_CreateInfo, m_Shaders);
        m_State = State::WaitingShaders;
    }

    void HandleWaitingShaders(bool WaitForCompletion)
    {
        VERIFY_EXPR(m_State == State::WaitingShaders);
        bool AllShaderReady = true;
        for (ShaderGLImpl* pShaderGL : m_Shaders)
        {
            SHADER_STATUS Status = pShaderGL->GetStatus(WaitForCompletion);
            switch (Status)
            {
                case SHADER_STATUS_COMPILING:
                    AllShaderReady = false;
                    break;

                case SHADER_STATUS_FAILED:
                    m_State = State::Failed;
                    return;

                case SHADER_STATUS_READY:
                    break;

                default:
                    UNEXPECTED("Unexpected shader status");
            }
        }

        if (AllShaderReady)
        {
            m_State = State::LinkingPrograms;
        }
    }

    void HandleLinkingPrograms(bool WaitForCompletion)
    {
        VERIFY_EXPR(m_State == State::LinkingPrograms);

        // Get active shader stages
        SHADER_TYPE ActiveStages = SHADER_TYPE_UNKNOWN;
        for (const ShaderGLImpl* pShaderGL : m_Shaders)
        {
            const auto ShaderType = pShaderGL->GetDesc().ShaderType;
            VERIFY((ActiveStages & ShaderType) == 0, "Shader stage ", GetShaderTypeLiteralName(ShaderType), " is already active");
            ActiveStages |= ShaderType;
        }

        // Create programs

        // Linking programs may be epxensive, so we cache programs keyed by shader IDs and resource signature IDs or resource layout.
        if (m_Pipeline.m_IsProgramPipelineSupported)
        {
            for (size_t i = 0; i < m_Shaders.size(); ++i)
            {
                if (!m_Pipeline.m_GLPrograms[i])
                {
                    GLProgramCache::GetProgramAttribs ProgAttribs{
                        &m_Shaders[i],
                        1,
                        true, // IsSeparableProgram
                        m_CreateInfo.ResourceSignaturesCount == 0 ? &m_CreateInfo.PSODesc.ResourceLayout : nullptr,
                        m_CreateInfo.ppResourceSignatures,
                        m_CreateInfo.ResourceSignaturesCount,
                    };
                    m_Pipeline.m_GLPrograms[i]  = m_Pipeline.GetDevice()->GetProgramCache().GetProgram(ProgAttribs);
                    m_Pipeline.m_ShaderTypes[i] = m_Shaders[i]->GetDesc().ShaderType;
                }
            }
        }
        else
        {
            if (!m_Pipeline.m_GLPrograms[0])
            {
                GLProgramCache::GetProgramAttribs ProgAttribs{
                    m_Shaders.data(),
                    static_cast<Uint32>(m_Shaders.size()),
                    false, // IsSeparableProgram
                    m_CreateInfo.ResourceSignaturesCount == 0 ? &m_CreateInfo.PSODesc.ResourceLayout : nullptr,
                    m_CreateInfo.ppResourceSignatures,
                    m_CreateInfo.ResourceSignaturesCount,
                };
                m_Pipeline.m_GLPrograms[0]  = m_Pipeline.GetDevice()->GetProgramCache().GetProgram(ProgAttribs);
                m_Pipeline.m_ShaderTypes[0] = ActiveStages;
            }
        }

        // Check program linking status
        for (Uint32 i = 0; i < m_Pipeline.m_NumPrograms; ++i)
        {
            GLProgram::LinkStatus LinkStatus = m_Pipeline.m_GLPrograms[i]->GetLinkStatus(WaitForCompletion);
            if (LinkStatus == GLProgram::LinkStatus::InProgress)
            {
                return;
            }
            else if (LinkStatus == GLProgram::LinkStatus::Failed)
            {
                std::ostringstream ss;
                ss << "Failed to link program " << i << " for pipeline state '" << m_Pipeline.m_Desc.Name << "': " << m_Pipeline.m_GLPrograms[i]->GetInfoLog();
                if (m_CreateAsynchronously)
                {
                    LOG_ERROR_MESSAGE(ss.str());
                }
                else
                {
                    LOG_ERROR_AND_THROW(ss.str());
                }
                m_State = State::Failed;
                return;
            }
        }

        m_Pipeline.InitResourceLayout(GetInternalCreateFlags(m_CreateInfo), m_Shaders, ActiveStages);
        m_State = State::Complete;
    }

private:
    PSOCreateInfoTypeX m_CreateInfo;
};


template <typename PSOCreateInfoType>
void PipelineStateGLImpl::InitInternalObjects(const PSOCreateInfoType& CreateInfo, const TShaderStages& ShaderStages) noexcept(false)
{
    const auto& DeviceInfo = GetDevice()->GetDeviceInfo();
    VERIFY(DeviceInfo.Type != RENDER_DEVICE_TYPE_UNDEFINED, "Device info is not initialized");

    m_IsProgramPipelineSupported = DeviceInfo.Features.SeparablePrograms != DEVICE_FEATURE_STATE_DISABLED;
    m_NumPrograms                = m_IsProgramPipelineSupported ? static_cast<Uint8>(ShaderStages.size()) : 1;

    FixedLinearAllocator MemPool{GetRawAllocator()};

    ReserveSpaceForPipelineDesc(CreateInfo, MemPool);
    MemPool.AddSpace<SharedGLProgramPtr>(m_NumPrograms);
    // m_SignatureCount is initialized by ReserveSpaceForPipelineDesc()
    MemPool.AddSpace<TBindings>(m_SignatureCount);
    MemPool.AddSpace<SHADER_TYPE>(m_NumPrograms);

    MemPool.Reserve();

    InitializePipelineDesc(CreateInfo, MemPool);
    m_GLPrograms   = MemPool.ConstructArray<SharedGLProgramPtr>(m_NumPrograms);
    m_BaseBindings = MemPool.ConstructArray<TBindings>(m_SignatureCount);
    m_ShaderTypes  = MemPool.ConstructArray<SHADER_TYPE>(m_NumPrograms, SHADER_TYPE_UNKNOWN);
}

PipelineStateGLImpl::PipelineStateGLImpl(IReferenceCounters*                    pRefCounters,
                                         RenderDeviceGLImpl*                    pDeviceGL,
                                         const GraphicsPipelineStateCreateInfo& CreateInfo,
                                         bool                                   bIsDeviceInternal) :
    // clang-format off
    TPipelineStateBase
    {
        pRefCounters,
        pDeviceGL,
        CreateInfo,
        bIsDeviceInternal
    }
// clang-format on
{
    try
    {
        TShaderStages Shaders;
        ExtractShaders<ShaderGLImpl>(CreateInfo, Shaders);

        RefCntAutoPtr<ShaderGLImpl> pTempPS;
        if (CreateInfo.pPS == nullptr)
        {
            // Some OpenGL implementations fail if fragment shader is not present, so
            // create a dummy one.
            ShaderCreateInfo ShaderCI;
            ShaderCI.SourceLanguage  = SHADER_SOURCE_LANGUAGE_GLSL;
            ShaderCI.Source          = "void main(){}";
            ShaderCI.Desc.ShaderType = SHADER_TYPE_PIXEL;
            ShaderCI.Desc.Name       = "Dummy fragment shader";
            pDeviceGL->CreateShader(ShaderCI, pTempPS.DblPtr<IShader>(), nullptr);

            Shaders.emplace_back(pTempPS);
        }

        m_Builder = std::make_unique<PipelineBuilder<GraphicsPipelineStateCreateInfo, GraphicsPipelineStateCreateInfoX>>(*this, CreateInfo, std::move(Shaders));

        // Force builder tick
        PIPELINE_STATE_STATUS Status = GetStatus(/*WaitForCompletion = */ (CreateInfo.Flags & PSO_CREATE_FLAG_ASYNCHRONOUS) == 0);
        VERIFY_EXPR(Status == PIPELINE_STATE_STATUS_READY || (CreateInfo.Flags & PSO_CREATE_FLAG_ASYNCHRONOUS) != 0);
    }
    catch (...)
    {
        Destruct();
        throw;
    }
}

PipelineStateGLImpl::PipelineStateGLImpl(IReferenceCounters*                   pRefCounters,
                                         RenderDeviceGLImpl*                   pDeviceGL,
                                         const ComputePipelineStateCreateInfo& CreateInfo,
                                         bool                                  bIsDeviceInternal) :
    // clang-format off
    TPipelineStateBase
    {
        pRefCounters,
        pDeviceGL,
        CreateInfo,
        bIsDeviceInternal
    }
// clang-format on
{
    try
    {
        TShaderStages Shaders;
        ExtractShaders<ShaderGLImpl>(CreateInfo, Shaders);

        m_Builder = std::make_unique<PipelineBuilder<ComputePipelineStateCreateInfo, ComputePipelineStateCreateInfoX>>(*this, CreateInfo, std::move(Shaders));

        // Force builder tick
        PIPELINE_STATE_STATUS Status = GetStatus(/*WaitForCompletion = */ (CreateInfo.Flags & PSO_CREATE_FLAG_ASYNCHRONOUS) == 0);
        VERIFY_EXPR(Status == PIPELINE_STATE_STATUS_READY || (CreateInfo.Flags & PSO_CREATE_FLAG_ASYNCHRONOUS) != 0);
    }
    catch (...)
    {
        Destruct();
        throw;
    }
}

PipelineStateGLImpl::~PipelineStateGLImpl()
{
    Destruct();
}

void PipelineStateGLImpl::Destruct()
{
    GetDevice()->OnDestroyPSO(*this);

    if (m_GLPrograms)
    {
        for (Uint32 i = 0; i < m_NumPrograms; ++i)
        {
            m_GLPrograms[i].~SharedGLProgramPtr();
        }
        m_GLPrograms = nullptr;
    }

    m_ShaderTypes = nullptr;
    m_NumPrograms = 0;

    TPipelineStateBase::Destruct();
}

IMPLEMENT_QUERY_INTERFACE2(PipelineStateGLImpl, IID_PipelineStateGL, IID_InternalImpl, TPipelineStateBase)

PIPELINE_STATE_STATUS PipelineStateGLImpl::GetStatus(bool WaitForCompletion)
{
    if (m_Builder)
    {
        if (m_Builder->Tick(WaitForCompletion))
        {
            m_Builder.reset();
        }
    }
    return m_Status.load();
}

SHADER_TYPE PipelineStateGLImpl::GetShaderStageType(Uint32 Index) const
{
    VERIFY(Index < m_NumPrograms, "Index is out of range");
    return m_ShaderTypes[Index];
}


void PipelineStateGLImpl::CommitProgram(GLContextState& State)
{
    if (m_IsProgramPipelineSupported)
    {
        // WARNING: glUseProgram() overrides glBindProgramPipeline(). That is, if you have a program in use and
        // a program pipeline bound, all rendering will use the program that is in use, not the pipeline programs!
        // So make sure that glUseProgram(0) has been called if pipeline is in use
        State.SetProgram(GLObjectWrappers::GLProgramObj::Null());
        auto& Pipeline = GetGLProgramPipeline(State.GetCurrentGLContext());
        VERIFY(Pipeline != 0, "Program pipeline must not be null");
        State.SetPipeline(Pipeline);
    }
    else
    {
        VERIFY_EXPR(m_GLPrograms != nullptr && m_GLPrograms[0]->GetGLHandle() != 0);
        State.SetProgram(m_GLPrograms[0]->GetGLHandle());
    }
}

GLObjectWrappers::GLPipelineObj& PipelineStateGLImpl::GetGLProgramPipeline(GLContext::NativeGLContextType Context)
{
    Threading::SpinLockGuard Guard{m_ProgPipelineLock};
    for (auto& ctx_pipeline : m_GLProgPipelines)
    {
        if (ctx_pipeline.first == Context)
            return ctx_pipeline.second;
    }

    // Create new program pipeline
    m_GLProgPipelines.emplace_back(Context, true);
    auto&  ctx_pipeline = m_GLProgPipelines.back();
    GLuint Pipeline     = ctx_pipeline.second;
    for (Uint32 i = 0; i < GetNumShaderStages(); ++i)
    {
        auto GLShaderBit = ShaderTypeToGLShaderBit(GetShaderStageType(i));
        // If the program has an active code for each stage mentioned in set flags,
        // then that code will be used by the pipeline. If program is 0, then the given
        // stages are cleared from the pipeline.
        glUseProgramStages(Pipeline, GLShaderBit, m_GLPrograms[i]->GetGLHandle());
        DEV_CHECK_GL_ERROR("glUseProgramStages() failed");
    }

    ctx_pipeline.second.SetName(m_Desc.Name);

    return ctx_pipeline.second;
}

GLuint PipelineStateGLImpl::GetGLProgramHandle(SHADER_TYPE Stage) const
{
    DEV_CHECK_ERR(IsPowerOfTwo(Stage), "Exactly one shader stage must be specified");

    for (size_t i = 0; i < m_NumPrograms; ++i)
    {
        // Note: in case of non-separable programs, m_ShaderTypes[0] contains
        //       all shader stages in the pipeline.
        if ((m_ShaderTypes[i] & Stage) != 0)
            return m_GLPrograms[i]->GetGLHandle();
    }
    return 0;
}

void PipelineStateGLImpl::ValidateShaderResources(std::shared_ptr<const ShaderResourcesGL> pShaderResources, const char* ShaderName, SHADER_TYPE ShaderStages)
{
    const auto HandleResource = [&](const ShaderResourcesGL::GLResourceAttribs& Attribs,
                                    SHADER_RESOURCE_TYPE                        AltResourceType) //
    {
        const auto ResAttribution = GetResourceAttribution(Attribs.Name, ShaderStages);

#ifdef DILIGENT_DEVELOPMENT
        m_ResourceAttibutions.emplace_back(ResAttribution);
#endif

        if (!ResAttribution)
        {
            LOG_ERROR_AND_THROW("Shader '", ShaderName, "' contains resource '", Attribs.Name,
                                "' that is not present in any pipeline resource signature used to create pipeline state '",
                                m_Desc.Name, "'.");
        }

        const auto* const pSignature = ResAttribution.pSignature;
        VERIFY_EXPR(pSignature != nullptr);

        if (ResAttribution.ResourceIndex != ResourceAttribution::InvalidResourceIndex)
        {
            const auto& ResDesc = pSignature->GetResourceDesc(ResAttribution.ResourceIndex);

            // Shader reflection does not contain read-only flag, so image and storage buffer can be UAV or SRV.
            // Texture SRV is the same as input attachment.
            const auto Type = (AltResourceType == ResDesc.ResourceType ? AltResourceType : Attribs.ResourceType);

            ValidatePipelineResourceCompatibility(ResDesc, Type, Attribs.ResourceFlags, Attribs.ArraySize, ShaderName, pSignature->GetDesc().Name);
        }
        else
        {
            UNEXPECTED("Resource index should be valid");
        }
    };

    const auto HandleUB = [&](const ShaderResourcesGL::UniformBufferInfo& Attribs) {
        HandleResource(Attribs, Attribs.ResourceType);
    };

    const auto HandleTexture = [&](const ShaderResourcesGL::TextureInfo& Attribs) {
        const bool IsTexelBuffer = (Attribs.ResourceType != SHADER_RESOURCE_TYPE_TEXTURE_SRV);
        HandleResource(Attribs, IsTexelBuffer ? Attribs.ResourceType : SHADER_RESOURCE_TYPE_INPUT_ATTACHMENT);
    };

    const auto HandleImage = [&](const ShaderResourcesGL::ImageInfo& Attribs) {
        const bool IsImageBuffer = (Attribs.ResourceType != SHADER_RESOURCE_TYPE_TEXTURE_UAV);
        HandleResource(Attribs, IsImageBuffer ? SHADER_RESOURCE_TYPE_BUFFER_SRV : SHADER_RESOURCE_TYPE_TEXTURE_SRV);
    };

    const auto HandleSB = [&](const ShaderResourcesGL::StorageBlockInfo& Attribs) {
        HandleResource(Attribs, SHADER_RESOURCE_TYPE_BUFFER_SRV);
    };

    pShaderResources->ProcessConstResources(HandleUB, HandleTexture, HandleImage, HandleSB);

#ifdef DILIGENT_DEVELOPMENT
    m_ShaderResources.emplace_back(std::move(pShaderResources));
    m_ShaderNames.emplace_back(ShaderName);
#endif
}

#ifdef DILIGENT_DEVELOPMENT
void PipelineStateGLImpl::DvpVerifySRBResources(const ShaderResourceCacheArrayType& ResourceCaches,
                                                const BaseBindingsArrayType&        BaseBindings) const
{
    // Verify base bindings
    const auto SignCount = GetResourceSignatureCount();
    for (Uint32 sign = 0; sign < SignCount; ++sign)
    {
        const auto* pSignature = GetResourceSignature(sign);
        if (pSignature == nullptr || pSignature->GetTotalResourceCount() == 0)
            continue;

        DEV_CHECK_ERR(GetBaseBindings(sign) == BaseBindings[sign],
                      "Bound resources use incorrect base binding indices. This may indicate a bug in resource signature compatibility comparison.");
    }

    using AttribIter = std::vector<ResourceAttribution>::const_iterator;
    struct HandleResourceHelper
    {
        PipelineStateGLImpl const&          PSO;
        const ShaderResourceCacheArrayType& ResourceCaches;
        AttribIter                          attrib_it;
        Uint32&                             shader_ind;

        HandleResourceHelper(const PipelineStateGLImpl&          _PSO,
                             const ShaderResourceCacheArrayType& _ResourceCaches,
                             AttribIter                          _iter,
                             Uint32&                             _ind) :
            PSO{_PSO},
            ResourceCaches{_ResourceCaches},
            attrib_it{_iter},
            shader_ind{_ind}
        {}

        void Validate(const ShaderResourcesGL::GLResourceAttribs& Attribs, RESOURCE_DIMENSION ResDim, bool IsMS)
        {
            if (*attrib_it && !attrib_it->IsImmutableSampler())
            {
                const auto* pResourceCache = ResourceCaches[attrib_it->SignatureIndex];
                DEV_CHECK_ERR(pResourceCache != nullptr, "Resource cache at index ", attrib_it->SignatureIndex, " is null.");
                attrib_it->pSignature->DvpValidateCommittedResource(Attribs, ResDim, IsMS, attrib_it->ResourceIndex, *pResourceCache,
                                                                    PSO.m_ShaderNames[shader_ind].c_str(), PSO.m_Desc.Name);
            }
            ++attrib_it;
        }

        void operator()(const ShaderResourcesGL::StorageBlockInfo& Attribs) { Validate(Attribs, RESOURCE_DIM_BUFFER, false); }
        void operator()(const ShaderResourcesGL::UniformBufferInfo& Attribs) { Validate(Attribs, RESOURCE_DIM_BUFFER, false); }
        void operator()(const ShaderResourcesGL::TextureInfo& Attribs) { Validate(Attribs, Attribs.ResourceDim, Attribs.IsMultisample); }
        void operator()(const ShaderResourcesGL::ImageInfo& Attribs) { Validate(Attribs, Attribs.ResourceDim, Attribs.IsMultisample); }
    };

    Uint32               ShaderInd = 0;
    HandleResourceHelper HandleResource{*this, ResourceCaches, m_ResourceAttibutions.begin(), ShaderInd};

    VERIFY_EXPR(m_ShaderResources.size() == m_ShaderNames.size());
    for (; ShaderInd < m_ShaderResources.size(); ++ShaderInd)
    {
        m_ShaderResources[ShaderInd]->ProcessConstResources(std::ref(HandleResource), std::ref(HandleResource),
                                                            std::ref(HandleResource), std::ref(HandleResource));
    }
    VERIFY_EXPR(HandleResource.attrib_it == m_ResourceAttibutions.end());
}
#endif // DILIGENT_DEVELOPMENT

} // namespace Diligent
