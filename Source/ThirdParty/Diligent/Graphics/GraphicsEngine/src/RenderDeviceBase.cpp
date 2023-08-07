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

#include "RenderDeviceBase.hpp"
#include "Align.hpp"

namespace Diligent
{

DeviceFeatures EnableDeviceFeatures(const DeviceFeatures& SupportedFeatures,
                                    const DeviceFeatures& RequestedFeatures) noexcept(false)
{
    auto GetFeatureState = [](DEVICE_FEATURE_STATE RequestedState, DEVICE_FEATURE_STATE SupportedState, const char* FeatureName) //
    {
        switch (RequestedState)
        {
            case DEVICE_FEATURE_STATE_DISABLED:
                return SupportedState == DEVICE_FEATURE_STATE_ENABLED ?
                    DEVICE_FEATURE_STATE_ENABLED : // the feature is supported by default and can not be disabled
                    DEVICE_FEATURE_STATE_DISABLED;

            case DEVICE_FEATURE_STATE_ENABLED:
            {
                if (SupportedState != DEVICE_FEATURE_STATE_DISABLED)
                    return DEVICE_FEATURE_STATE_ENABLED;
                else
                    LOG_ERROR_AND_THROW(FeatureName, " not supported by this device");
            }

            case DEVICE_FEATURE_STATE_OPTIONAL:
                return SupportedState != DEVICE_FEATURE_STATE_DISABLED ?
                    DEVICE_FEATURE_STATE_ENABLED :
                    DEVICE_FEATURE_STATE_DISABLED;

            default:
                UNEXPECTED("Unexpected feature state");
                return DEVICE_FEATURE_STATE_DISABLED;
        }
    };

    if (SupportedFeatures.SeparablePrograms == DEVICE_FEATURE_STATE_ENABLED &&
        RequestedFeatures.SeparablePrograms == DEVICE_FEATURE_STATE_DISABLED)
    {
        LOG_INFO_MESSAGE("Can not disable SeparablePrograms");
    }

    DeviceFeatures EnabledFeatures;
#define ENABLE_FEATURE(Feature, FeatureName) \
    EnabledFeatures.Feature = GetFeatureState(RequestedFeatures.Feature, SupportedFeatures.Feature, FeatureName)

    // clang-format off
    ENABLE_FEATURE(SeparablePrograms,                 "Separable programs are");
    ENABLE_FEATURE(ShaderResourceQueries,             "Shader resource queries are");
    ENABLE_FEATURE(WireframeFill,                     "Wireframe fill is");
    ENABLE_FEATURE(MultithreadedResourceCreation,     "Multithreaded resource creation is");
    ENABLE_FEATURE(ComputeShaders,                    "Compute shaders are");
    ENABLE_FEATURE(GeometryShaders,                   "Geometry shaders are");
    ENABLE_FEATURE(Tessellation,                      "Tessellation is");
    ENABLE_FEATURE(MeshShaders,                       "Mesh shaders are");
    ENABLE_FEATURE(RayTracing,                        "Ray tracing is");
    ENABLE_FEATURE(BindlessResources,                 "Bindless resources are");
    ENABLE_FEATURE(OcclusionQueries,                  "Occlusion queries are");
    ENABLE_FEATURE(BinaryOcclusionQueries,            "Binary occlusion queries are");
    ENABLE_FEATURE(TimestampQueries,                  "Timestamp queries are");
    ENABLE_FEATURE(PipelineStatisticsQueries,         "Pipeline statistics queries are");
    ENABLE_FEATURE(DurationQueries,                   "Duration queries are");
    ENABLE_FEATURE(DepthBiasClamp,                    "Depth bias clamp is");
    ENABLE_FEATURE(DepthClamp,                        "Depth clamp is");
    ENABLE_FEATURE(IndependentBlend,                  "Independent blend is");
    ENABLE_FEATURE(DualSourceBlend,                   "Dual-source blend is");
    ENABLE_FEATURE(MultiViewport,                     "Multiviewport is");
    ENABLE_FEATURE(TextureCompressionBC,              "BC texture compression is");
    ENABLE_FEATURE(VertexPipelineUAVWritesAndAtomics, "Vertex pipeline UAV writes and atomics are");
    ENABLE_FEATURE(PixelUAVWritesAndAtomics,          "Pixel UAV writes and atomics are");
    ENABLE_FEATURE(TextureUAVExtendedFormats,         "Texture UAV extended formats are");
    ENABLE_FEATURE(ShaderFloat16,                     "16-bit float shader operations are");
    ENABLE_FEATURE(ResourceBuffer16BitAccess,         "16-bit resource buffer access is");
    ENABLE_FEATURE(UniformBuffer16BitAccess,          "16-bit uniform buffer access is");
    ENABLE_FEATURE(ShaderInputOutput16,               "16-bit shader inputs/outputs are");
    ENABLE_FEATURE(ShaderInt8,                        "8-bit int shader operations are");
    ENABLE_FEATURE(ResourceBuffer8BitAccess,          "8-bit resource buffer access is");
    ENABLE_FEATURE(UniformBuffer8BitAccess,           "8-bit uniform buffer access is");
    ENABLE_FEATURE(ShaderResourceRuntimeArray,        "Shader resource runtime array is");
    ENABLE_FEATURE(WaveOp,                            "Wave operations are");
    ENABLE_FEATURE(InstanceDataStepRate,              "Instance data step rate is");
    ENABLE_FEATURE(NativeFence,                       "Native fence is");
    ENABLE_FEATURE(TileShaders,                       "Tile shaders are");
    ENABLE_FEATURE(TransferQueueTimestampQueries,     "Timestamp queries in transfer queues are");
    ENABLE_FEATURE(VariableRateShading,               "Variable shading rate is");
    ENABLE_FEATURE(SparseResources,                   "Sparse resources are");
    ENABLE_FEATURE(SubpassFramebufferFetch,           "Subpass framebuffer fetch is");
    ENABLE_FEATURE(TextureComponentSwizzle,           "Texture component swizzle is");
    // clang-format on
#undef ENABLE_FEATURE

    ASSERT_SIZEOF(Diligent::DeviceFeatures, 41, "Did you add a new feature to DeviceFeatures? Please handle its status here (if necessary).");

    return EnabledFeatures;
}

COMPONENT_TYPE CheckSparseTextureFormatSupport(TEXTURE_FORMAT                  TexFormat,
                                               RESOURCE_DIMENSION              Dimension,
                                               Uint32                          SampleCount,
                                               const SparseResourceProperties& SparseRes) noexcept
{
    switch (Dimension)
    {
        case RESOURCE_DIM_TEX_2D:
        case RESOURCE_DIM_TEX_2D_ARRAY:
        {
            if ((SparseRes.CapFlags & SPARSE_RESOURCE_CAP_FLAG_TEXTURE_2D) == 0)
                return COMPONENT_TYPE_UNDEFINED;

            static_assert(SPARSE_RESOURCE_CAP_FLAG_TEXTURE_4_SAMPLES == SPARSE_RESOURCE_CAP_FLAG_TEXTURE_2_SAMPLES * 2, "Unexpected enum values");
            static_assert(SPARSE_RESOURCE_CAP_FLAG_TEXTURE_8_SAMPLES == SPARSE_RESOURCE_CAP_FLAG_TEXTURE_2_SAMPLES * 4, "Unexpected enum values");
            static_assert(SPARSE_RESOURCE_CAP_FLAG_TEXTURE_16_SAMPLES == SPARSE_RESOURCE_CAP_FLAG_TEXTURE_2_SAMPLES * 8, "Unexpected enum values");
            VERIFY_EXPR(IsPowerOfTwo(SampleCount));
            if (SampleCount >= 2 && (SparseRes.CapFlags & (SPARSE_RESOURCE_CAP_FLAG_TEXTURE_2_SAMPLES * (SampleCount >> 1))) == 0)
                return COMPONENT_TYPE_UNDEFINED;

            break;
        }
        case RESOURCE_DIM_TEX_CUBE:
        case RESOURCE_DIM_TEX_CUBE_ARRAY:
            if ((SparseRes.CapFlags & SPARSE_RESOURCE_CAP_FLAG_TEXTURE_2D) == 0)
                return COMPONENT_TYPE_UNDEFINED;
            break;

        case RESOURCE_DIM_TEX_3D:
            DEV_CHECK_ERR(SampleCount == 1, "Multisampled texture 3D is not supported");
            if ((SparseRes.CapFlags & SPARSE_RESOURCE_CAP_FLAG_TEXTURE_3D) == 0)
                return COMPONENT_TYPE_UNDEFINED;
            break;

        case RESOURCE_DIM_BUFFER:
        case RESOURCE_DIM_TEX_1D:
        case RESOURCE_DIM_TEX_1D_ARRAY:
            DEV_ERROR("Invalid sparse texture resource dimension");
            return COMPONENT_TYPE_UNDEFINED;

        default:
            DEV_ERROR("Unexpected resource dimension");
            return COMPONENT_TYPE_UNDEFINED;
    }

    return GetTextureFormatAttribs(TexFormat).ComponentType;
}

} // namespace Diligent
