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
/// Defines Diligent::ISerializationDevice interface

#include "../../../Common/interface/StringTools.h"
#include "../../GraphicsEngine/interface/RenderDevice.h"
#include "../../GraphicsEngine/interface/Shader.h"
#include "../../GraphicsEngine/interface/RenderPass.h"
#include "../../GraphicsEngine/interface/PipelineResourceSignature.h"
#include "Archiver.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {205BB0B2-0966-4F51-9380-46EE5BCED28B}
static DILIGENT_CONSTEXPR INTERFACE_ID IID_SerializationDevice =
    {0x205bb0b2, 0x966, 0x4f51, {0x93, 0x80, 0x46, 0xee, 0x5b, 0xce, 0xd2, 0x8b}};


#define DILIGENT_INTERFACE_NAME ISerializationDevice
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define ISerializationDeviceInclusiveMethods \
    IRenderDeviceInclusiveMethods;           \
    ISerializationDeviceMethods SerializationDevice

// clang-format off

/// Shader archive info
struct ShaderArchiveInfo
{
    /// Bitset of Diligent::ARCHIVE_DEVICE_DATA_FLAGS.
    /// Specifies for which backends the shader data will be serialized.
    ARCHIVE_DEVICE_DATA_FLAGS DeviceFlags DEFAULT_INITIALIZER(ARCHIVE_DEVICE_DATA_FLAG_NONE);
};
typedef struct ShaderArchiveInfo ShaderArchiveInfo;

/// Pipeline resource signature archive info
struct ResourceSignatureArchiveInfo
{
    /// Bitset of Diligent::ARCHIVE_DEVICE_DATA_FLAGS.
    /// Specifies for which backends the resource signature data will be serialized.
    ARCHIVE_DEVICE_DATA_FLAGS DeviceFlags DEFAULT_INITIALIZER(ARCHIVE_DEVICE_DATA_FLAG_NONE);
};
typedef struct ResourceSignatureArchiveInfo ResourceSignatureArchiveInfo;

/// Pipeline state archive info
struct PipelineStateArchiveInfo
{
    /// Pipeline state archive flags, see Diligent::PSO_ARCHIVE_FLAGS.
    PSO_ARCHIVE_FLAGS PSOFlags DEFAULT_INITIALIZER(PSO_ARCHIVE_FLAG_NONE);

    /// Bitset of Diligent::ARCHIVE_DEVICE_DATA_FLAGS.
    /// Specifies for which backends the pipeline state data will be serialized.
    ARCHIVE_DEVICE_DATA_FLAGS DeviceFlags DEFAULT_INITIALIZER(ARCHIVE_DEVICE_DATA_FLAG_NONE);
};
typedef struct PipelineStateArchiveInfo PipelineStateArchiveInfo;


/// Contains attributes to calculate pipeline resource bindings
struct PipelineResourceBindingAttribs
{
    /// An array of ResourceSignaturesCount shader resource signatures that
    /// define the layout of shader resources in this pipeline state object.
    /// See Diligent::IPipelineResourceSignature.
    IPipelineResourceSignature** ppResourceSignatures      DEFAULT_INITIALIZER(nullptr);

    /// The number of elements in ppResourceSignatures array.
    Uint32                       ResourceSignaturesCount   DEFAULT_INITIALIZER(0);

    /// The number of render targets, only for graphics pipeline.
    /// \note Required for Direct3D11 graphics pipelines that use UAVs.
    Uint32                       NumRenderTargets  DEFAULT_INITIALIZER(0);

    /// The number of vertex buffers, only for graphics pipeline.
    /// \note Required for Metal.
    Uint32                       NumVertexBuffers  DEFAULT_INITIALIZER(0);

    /// Vertex buffer names.
    /// \note Required for Metal.
    Char const* const*           VertexBufferNames DEFAULT_INITIALIZER(nullptr);

    /// Combination of shader stages.
    SHADER_TYPE                  ShaderStages      DEFAULT_INITIALIZER(SHADER_TYPE_UNKNOWN);

    /// Device type for which resource binding will be calculated.
    enum RENDER_DEVICE_TYPE      DeviceType        DEFAULT_INITIALIZER(RENDER_DEVICE_TYPE_UNDEFINED);
};
typedef struct PipelineResourceBindingAttribs PipelineResourceBindingAttribs;

/// Pipeline resource binding
struct PipelineResourceBinding
{
    /// Resource name
    const Char*          Name           DEFAULT_INITIALIZER(nullptr);

    /// Resource type, see Diligent::SHADER_RESOURCE_TYPE.
    SHADER_RESOURCE_TYPE ResourceType   DEFAULT_INITIALIZER(SHADER_RESOURCE_TYPE_UNKNOWN);

    /// Shader resource stages, see Diligent::SHADER_TYPE.
    SHADER_TYPE          ShaderStages   DEFAULT_INITIALIZER(SHADER_TYPE_UNKNOWN);

    /// Shader register space.
    Uint16               Space          DEFAULT_INITIALIZER(0);

    /// Shader register.
    Uint32               Register       DEFAULT_INITIALIZER(0);

    /// Array size
    Uint32               ArraySize      DEFAULT_INITIALIZER(0);
};
typedef struct PipelineResourceBinding PipelineResourceBinding;


/// Serialization device interface
DILIGENT_BEGIN_INTERFACE(ISerializationDevice, IRenderDevice)
{
    /// Creates a serialized shader.

    /// \param [in]  ShaderCI    - Shader create info, see Diligent::ShaderCreateInfo for details.
    /// \param [in]  ArchiveInfo - Shader archive info, see Diligent::ShaderArchiveInfo for details.
    /// \param [out] ppShader    - Address of the memory location where a pointer to the
    ///                            shader interface will be written.
    /// \param [out] ppCompilerOutput - Address of the memory location where a pointer to the
    ///                                 shader compiler output will be written.
    ///                                 If null, the output will be ignored.
    /// \note
    ///     The method is thread-safe and may be called from multiple threads simultaneously.
    VIRTUAL void METHOD(CreateShader)(THIS_
                                      const ShaderCreateInfo REF  ShaderCI,
                                      const ShaderArchiveInfo REF ArchiveInfo,
                                      IShader**                   ppShader,
                                      IDataBlob**                 ppCompilerOutput DEFAULT_VALUE(nullptr)) PURE;

 
    /// Creates a serialized pipeline resource signature.

    /// \param [in]  Desc        - Pipeline resource signature description, see Diligent::PipelineResourceSignatureDesc for details.
    /// \param [in]  ArchiveInfo - Signature archive info, see Diligent::ResourceSignatureArchiveInfo for details.
    /// \param [out] ppShader    - Address of the memory location where a pointer to the serialized
    ///                            pipeline resource signature object will be written.
    ///
    /// \note
    ///     The method is thread-safe and may be called from multiple threads simultaneously.
    VIRTUAL void METHOD(CreatePipelineResourceSignature)(THIS_
                                                         const PipelineResourceSignatureDesc REF Desc,
                                                         const ResourceSignatureArchiveInfo REF  ArchiveInfo,
                                                         IPipelineResourceSignature**            ppSignature) PURE;

    /// Creates a serialized graphics pipeline state.

    /// \param [in]  PSOCreateInfo   - Graphics pipeline state create info, see Diligent::GraphicsPipelineStateCreateInfo for details.
    /// \param [in]  ArchiveInfo     - Pipeline state archive info, see Diligent::PipelineStateArchiveInfo for details.
    /// \param [out] ppPipelineState - Address of the memory location where a pointer to the serialized
    ///                                pipeline state object will be written.
    ///
    /// \note
    ///     All objects that PSOCreateInfo references (shaders, render pass, resource signatures) must be
    ///     serialized objects created by the same serialization device.
    ///
    ///     The method is thread-safe and may be called from multiple threads simultaneously.
    VIRTUAL void METHOD(CreateGraphicsPipelineState)(THIS_
                                                     const GraphicsPipelineStateCreateInfo REF PSOCreateInfo,
                                                     const PipelineStateArchiveInfo REF        ArchiveInfo,
                                                     IPipelineState**                          ppPipelineState) PURE;

    /// Creates a serialized compute pipeline state.

    /// \param [in]  PSOCreateInfo   - Compute pipeline state create info, see Diligent::ComputePipelineStateCreateInfo for details.
    /// \param [in]  ArchiveInfo     - Pipeline state archive info, see Diligent::PipelineStateArchiveInfo for details.
    /// \param [out] ppPipelineState - Address of the memory location where a pointer to the serialized
    ///                                pipeline state object will be written.
    ///
    /// \note
    ///     All objects that PSOCreateInfo references (shaders, resource signatures) must be
    ///     serialized objects created by the same serialization device.
    ///
    ///     The method is thread-safe and may be called from multiple threads simultaneously.
    VIRTUAL void METHOD(CreateComputePipelineState)(THIS_
                                                    const ComputePipelineStateCreateInfo REF PSOCreateInfo,
                                                    const PipelineStateArchiveInfo REF       ArchiveInfo,
                                                    IPipelineState**                         ppPipelineState) PURE;

    /// Creates a serialized ray tracing pipeline state.

    /// \param [in]  PSOCreateInfo   - Ray tracing pipeline state create info, see Diligent::RayTracingPipelineStateCreateInfo for details.
    /// \param [in]  ArchiveInfo     - Pipeline state archive info, see Diligent::PipelineStateArchiveInfo for details.
    /// \param [out] ppPipelineState - Address of the memory location where a pointer to the serialized
    ///                                pipeline state object will be written.
    ///
    /// \note
    ///     All objects that PSOCreateInfo references (shaders, resource signatures) must be
    ///     serialized objects created by the same serialization device.
    ///
    ///     The method is thread-safe and may be called from multiple threads simultaneously.
    VIRTUAL void METHOD(CreateRayTracingPipelineState)(THIS_
                                                       const RayTracingPipelineStateCreateInfo REF PSOCreateInfo,
                                                       const PipelineStateArchiveInfo REF          ArchiveInfo,
                                                       IPipelineState**                            ppPipelineState) PURE;

    /// Creates a serialized tile pipeline state.

    /// \param [in]  PSOCreateInfo   - Tile pipeline state create info, see Diligent::TilePipelineStateCreateInfo for details.
    /// \param [in]  ArchiveInfo     - Pipeline state archive info, see Diligent::PipelineStateArchiveInfo for details.
    /// \param [out] ppPipelineState - Address of the memory location where a pointer to the serialized
    ///                                pipeline state interface will be written.
    ///
    /// \note
    ///     All objects that PSOCreateInfo references (shaders, resource signatures) must be
    ///     serialized objects created by the same serialization device.
    ///
    ///     The method is thread-safe and may be called from multiple threads simultaneously.
    VIRTUAL void METHOD(CreateTilePipelineState)(THIS_
                                                 const TilePipelineStateCreateInfo REF PSOCreateInfo,
                                                 const PipelineStateArchiveInfo REF    ArchiveInfo,
                                                 IPipelineState**                      ppPipelineState) PURE;


    /// Populates an array of pipeline resource bindings.
    VIRTUAL void METHOD(GetPipelineResourceBindings)(THIS_
                                                     const PipelineResourceBindingAttribs REF Attribs,
                                                     Uint32 REF                               NumBindings,
                                                     const PipelineResourceBinding* REF       pBindings) PURE;


    /// Returns a combination of supported device flags, see Diligent::ARCHIVE_DEVICE_DATA_FLAGS.
    VIRTUAL ARCHIVE_DEVICE_DATA_FLAGS METHOD(GetSupportedDeviceFlags)(THIS) CONST PURE;

    /// Adds a optional render device that will be used to initialize device-specific objects that
    /// may be used for rendering (e.g. shaders).
    /// For example, a shader object retrieved with ISerializedShader::GetDeviceShader() will be
    /// suitable for rendering.
    VIRTUAL void METHOD(AddRenderDevice)(THIS_
                                         IRenderDevice* pDevice) PURE;

#if DILIGENT_CPP_INTERFACE
    /// Overloaded alias for CreateGraphicsPipelineState.
    void CreatePipelineState(const GraphicsPipelineStateCreateInfo& CI, const PipelineStateArchiveInfo& ArchiveInfo, IPipelineState** ppPipelineState)
    {
        CreateGraphicsPipelineState(CI, ArchiveInfo, ppPipelineState);
    }
    /// Overloaded alias for CreateComputePipelineState.
    void CreatePipelineState(const ComputePipelineStateCreateInfo& CI, const PipelineStateArchiveInfo& ArchiveInfo, IPipelineState** ppPipelineState)
    {
        CreateComputePipelineState(CI, ArchiveInfo, ppPipelineState);
    }
    /// Overloaded alias for CreateRayTracingPipelineState.
    void CreatePipelineState(const RayTracingPipelineStateCreateInfo& CI, const PipelineStateArchiveInfo& ArchiveInfo, IPipelineState** ppPipelineState)
    {
        CreateRayTracingPipelineState(CI, ArchiveInfo, ppPipelineState);
    }
    /// Overloaded alias for CreateTilePipelineState.
    void CreatePipelineState(const TilePipelineStateCreateInfo& CI, const PipelineStateArchiveInfo& ArchiveInfo, IPipelineState** ppPipelineState)
    {
        CreateTilePipelineState(CI, ArchiveInfo, ppPipelineState);
    }
#endif
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

#    define ISerializationDevice_CreateShader(This, ...)                    CALL_IFACE_METHOD(SerializationDevice, CreateShader,                    This, __VA_ARGS__)
#    define ISerializationDevice_CreatePipelineResourceSignature(This, ...) CALL_IFACE_METHOD(SerializationDevice, CreatePipelineResourceSignature, This, __VA_ARGS__)
#    define ISerializationDevice_CreateGraphicsPipelineState(This, ...)     CALL_IFACE_METHOD(SerializationDevice, CreateGraphicsPipelineState,     This, __VA_ARGS__)
#    define ISerializationDevice_CreateComputePipelineState(This, ...)      CALL_IFACE_METHOD(SerializationDevice, CreateComputePipelineState,      This, __VA_ARGS__)
#    define ISerializationDevice_CreateRayTracingPipelineState(This, ...)   CALL_IFACE_METHOD(SerializationDevice, CreateRayTracingPipelineState,   This, __VA_ARGS__)
#    define ISerializationDevice_CreateTilePipelineState(This, ...)         CALL_IFACE_METHOD(SerializationDevice, CreateTilePipelineState,         This, __VA_ARGS__)
#    define ISerializationDevice_GetPipelineResourceBindings(This, ...)     CALL_IFACE_METHOD(SerializationDevice, GetPipelineResourceBindings,     This, __VA_ARGS__)

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
