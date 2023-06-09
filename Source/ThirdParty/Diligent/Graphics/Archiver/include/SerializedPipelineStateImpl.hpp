/*
 *  Copyright 2019-2023 Diligent Graphics LLC
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use  file except in compliance with the License.
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
 *  or consequential damages of any character arising as a result of  License or
 *  out of the use or inability to use the software (including but not limited to damages
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and
 *  all other commercial damages or losses), even if such Contributor has been advised
 *  of the possibility of such damages.
 */

#pragma once

#include <unordered_map>
#include <vector>
#include <array>
#include <string>

#include "SerializationEngineImplTraits.hpp"

#include "SerializedPipelineState.h"
#include "Archiver.h"

#include "ObjectBase.hpp"
#include "PipelineStateBase.hpp"
#include "DeviceObjectArchive.hpp"

namespace Diligent
{

class SerializedResourceSignatureImpl;

class SerializedPipelineStateImpl final : public ObjectBase<ISerializedPipelineState>
{
public:
    using TBase = ObjectBase<ISerializedPipelineState>;

    template <typename PSOCreateInfoType>
    SerializedPipelineStateImpl(IReferenceCounters*             pRefCounters,
                                SerializationDeviceImpl*        pDevice,
                                const PSOCreateInfoType&        CreateInfo,
                                const PipelineStateArchiveInfo& ArchiveInfo);

    ~SerializedPipelineStateImpl() override;

    IMPLEMENT_QUERY_INTERFACE2_IN_PLACE(IID_SerializedPipelineState, IID_PipelineState, TBase)

    virtual const PipelineStateDesc& DILIGENT_CALL_TYPE GetDesc() const override final
    {
        return m_Desc;
    }

    // clang-format off
    UNSUPPORTED_CONST_METHOD(Int32,    GetUniqueID)
    UNSUPPORTED_METHOD      (void,     SetUserData, IObject* pUserData)
    UNSUPPORTED_CONST_METHOD(IObject*, GetUserData)

    UNSUPPORTED_CONST_METHOD(const GraphicsPipelineDesc&,   GetGraphicsPipelineDesc)
    UNSUPPORTED_CONST_METHOD(const RayTracingPipelineDesc&, GetRayTracingPipelineDesc)
    UNSUPPORTED_CONST_METHOD(const TilePipelineDesc&,       GetTilePipelineDesc)

    UNSUPPORTED_METHOD      (void,   BindStaticResources,    SHADER_TYPE ShaderStages, IResourceMapping* pResourceMapping, BIND_SHADER_RESOURCES_FLAGS Flags)
    UNSUPPORTED_CONST_METHOD(Uint32, GetStaticVariableCount, SHADER_TYPE ShaderType)

    UNSUPPORTED_METHOD      (IShaderResourceVariable*, GetStaticVariableByName,  SHADER_TYPE ShaderType, const Char* Name)
    UNSUPPORTED_METHOD      (IShaderResourceVariable*, GetStaticVariableByIndex, SHADER_TYPE ShaderType, Uint32 Index)

    UNSUPPORTED_METHOD      (void, CreateShaderResourceBinding,  IShaderResourceBinding** ppShaderResourceBinding, bool InitStaticResources)
    UNSUPPORTED_CONST_METHOD(void, InitializeStaticSRBResources, IShaderResourceBinding* pShaderResourceBinding)
    UNSUPPORTED_CONST_METHOD(void, CopyStaticResources,          IPipelineState* pPSO)
    UNSUPPORTED_CONST_METHOD(bool, IsCompatibleWith,             const IPipelineState* pPSO)

    UNSUPPORTED_CONST_METHOD(Uint32,                      GetResourceSignatureCount)
    UNSUPPORTED_CONST_METHOD(IPipelineResourceSignature*, GetResourceSignature, Uint32 Index)
    // clang-format on

    virtual Uint32 DILIGENT_CALL_TYPE GetPatchedShaderCount(ARCHIVE_DEVICE_DATA_FLAGS DeviceType) const override final;

    virtual ShaderCreateInfo DILIGENT_CALL_TYPE GetPatchedShaderCreateInfo(
        ARCHIVE_DEVICE_DATA_FLAGS DataType,
        Uint32                    ShaderIndex) const override final;

    using SerializedPSOAuxData = DeviceObjectArchive::SerializedPSOAuxData;
    using DeviceType           = DeviceObjectArchive::DeviceType;
    using TPRSNames            = DeviceObjectArchive::TPRSNames;

    static constexpr auto DeviceDataCount = static_cast<size_t>(DeviceType::Count);

    struct Data
    {
        SerializedPSOAuxData Aux;
        SerializedData       Common;

        struct ShaderInfo
        {
            SerializedData Data;
            size_t         Hash  = 0;
            SHADER_TYPE    Stage = SHADER_TYPE_UNKNOWN;
        };
        std::array<std::vector<ShaderInfo>, DeviceDataCount> Shaders;

        bool DoNotPackSignatures = false;
    };

    const Data& GetData() const { return m_Data; }

    const SerializedData& GetCommonData() const { return m_Data.Common; };


    using RayTracingShaderMapType = std::unordered_map<const IShader*, /*Index in TShaderIndices*/ Uint32>;

    static void ExtractShadersD3D12(const RayTracingPipelineStateCreateInfo& CreateInfo, RayTracingShaderMapType& ShaderMap);
    static void ExtractShadersVk(const RayTracingPipelineStateCreateInfo& CreateInfo, RayTracingShaderMapType& ShaderMap);

    template <typename ShaderStage>
    static void GetRayTracingShaderMap(const std::vector<ShaderStage>& ShaderStages, RayTracingShaderMapType& ShaderMap)
    {
        Uint32 ShaderIndex = 0;
        for (const auto& Stage : ShaderStages)
        {
            for (const auto* pShader : Stage.Serialized)
            {
                if (ShaderMap.emplace(pShader, ShaderIndex).second)
                    ++ShaderIndex;
            }
        }
    }

    IRenderPass* GetRenderPass() const
    {
        return m_pRenderPass;
    }

    using SignaturesVector = std::vector<RefCntAutoPtr<IPipelineResourceSignature>>;
    const SignaturesVector& GetSignatures() { return m_Signatures; }

private:
    template <typename CreateInfoType>
    void PatchShadersVk(const CreateInfoType& CreateInfo) noexcept(false);

    template <typename CreateInfoType>
    void PatchShadersD3D12(const CreateInfoType& CreateInfo) noexcept(false);

    template <typename CreateInfoType>
    void PatchShadersD3D11(const CreateInfoType& CreateInfo) noexcept(false);

    template <typename CreateInfoType>
    void PatchShadersGL(const CreateInfoType& CreateInfo) noexcept(false);

    template <typename CreateInfoType>
    void PatchShadersMtl(const CreateInfoType& CreateInfo, DeviceType DevType, const std::string& DumpDir) noexcept(false);

    // Default signatures in OpenGL are not serialized and require special handling.
    template <typename CreateInfoType>
    void PrepareDefaultSignatureGL(const CreateInfoType& CreateInfo) noexcept(false);


    void SerializeShaderCreateInfo(DeviceType              Type,
                                   const ShaderCreateInfo& CI);


    template <typename PipelineStateImplType, typename SignatureImplType, typename ShaderStagesArrayType, typename... ExtraArgsType>
    void CreateDefaultResourceSignature(DeviceType                   Type,
                                        const PipelineStateDesc&     PSODesc,
                                        SHADER_TYPE                  ActiveShaderStageFlags,
                                        const ShaderStagesArrayType& ShaderStages,
                                        const ExtraArgsType&... ExtraArgs);

protected:
    SerializationDeviceImpl* const m_pSerializationDevice;

    Data m_Data;

    const std::string       m_Name;
    const PipelineStateDesc m_Desc;

    RefCntAutoPtr<IRenderPass>                     m_pRenderPass;
    RefCntAutoPtr<SerializedResourceSignatureImpl> m_pDefaultSignature;
    SignaturesVector                               m_Signatures;
};

#define INSTANTIATE_SERIALIZED_PSO_CTOR(CreateInfoType)                                                             \
    template SerializedPipelineStateImpl::SerializedPipelineStateImpl(IReferenceCounters*             pRefCounters, \
                                                                      SerializationDeviceImpl*        pDevice,      \
                                                                      const CreateInfoType&           CreateInfo,   \
                                                                      const PipelineStateArchiveInfo& ArchiveInfo)
#define DECLARE_SERIALIZED_PSO_CTOR(CreateInfoType) extern INSTANTIATE_SERIALIZED_PSO_CTOR(CreateInfoType)

DECLARE_SERIALIZED_PSO_CTOR(GraphicsPipelineStateCreateInfo);
DECLARE_SERIALIZED_PSO_CTOR(ComputePipelineStateCreateInfo);
DECLARE_SERIALIZED_PSO_CTOR(TilePipelineStateCreateInfo);
DECLARE_SERIALIZED_PSO_CTOR(RayTracingPipelineStateCreateInfo);


#define INSTANTIATE_PATCH_SHADER(MethodName, CreateInfoType, ...) template void SerializedPipelineStateImpl::MethodName<CreateInfoType>(const CreateInfoType& CreateInfo, ##__VA_ARGS__) noexcept(false)
#define DECLARE_PATCH_SHADER(MethodName, CreateInfoType, ...)     extern INSTANTIATE_PATCH_SHADER(MethodName, CreateInfoType, ##__VA_ARGS__)

#define DECLARE_PATCH_METHODS(MethodName, ...)                                        \
    DECLARE_PATCH_SHADER(MethodName, GraphicsPipelineStateCreateInfo, ##__VA_ARGS__); \
    DECLARE_PATCH_SHADER(MethodName, ComputePipelineStateCreateInfo, ##__VA_ARGS__);  \
    DECLARE_PATCH_SHADER(MethodName, TilePipelineStateCreateInfo, ##__VA_ARGS__);     \
    DECLARE_PATCH_SHADER(MethodName, RayTracingPipelineStateCreateInfo, ##__VA_ARGS__);

#define INSTANTIATE_PATCH_SHADER_METHODS(MethodName, ...)                                 \
    INSTANTIATE_PATCH_SHADER(MethodName, GraphicsPipelineStateCreateInfo, ##__VA_ARGS__); \
    INSTANTIATE_PATCH_SHADER(MethodName, ComputePipelineStateCreateInfo, ##__VA_ARGS__);  \
    INSTANTIATE_PATCH_SHADER(MethodName, TilePipelineStateCreateInfo, ##__VA_ARGS__);     \
    INSTANTIATE_PATCH_SHADER(MethodName, RayTracingPipelineStateCreateInfo, ##__VA_ARGS__);

#define INSTANTIATE_PREPARE_DEF_SIGNATURE_GL(CreateInfoType) template void SerializedPipelineStateImpl::PrepareDefaultSignatureGL<CreateInfoType>(const CreateInfoType& CreateInfo) noexcept(false)
#define DECLARE_PREPARE_DEF_SIGNATURE_GL(CreateInfoType)     extern INSTANTIATE_PREPARE_DEF_SIGNATURE_GL(CreateInfoType)
#define DECLARE_PREPARE_DEF_SIGNATURE_GL_METHODS()                     \
    DECLARE_PREPARE_DEF_SIGNATURE_GL(GraphicsPipelineStateCreateInfo); \
    DECLARE_PREPARE_DEF_SIGNATURE_GL(ComputePipelineStateCreateInfo);  \
    DECLARE_PREPARE_DEF_SIGNATURE_GL(TilePipelineStateCreateInfo);     \
    DECLARE_PREPARE_DEF_SIGNATURE_GL(RayTracingPipelineStateCreateInfo);


#if D3D11_SUPPORTED
DECLARE_PATCH_METHODS(PatchShadersD3D11)
#endif

#if D3D12_SUPPORTED
DECLARE_PATCH_METHODS(PatchShadersD3D12)
#endif

#if GL_SUPPORTED
DECLARE_PATCH_METHODS(PatchShadersGL)
DECLARE_PREPARE_DEF_SIGNATURE_GL_METHODS()
#endif

#if VULKAN_SUPPORTED
DECLARE_PATCH_METHODS(PatchShadersVk)
#endif

#if METAL_SUPPORTED
DECLARE_PATCH_METHODS(PatchShadersMtl, DeviceType DevType, const std::string& DumpPath)
#endif

} // namespace Diligent
