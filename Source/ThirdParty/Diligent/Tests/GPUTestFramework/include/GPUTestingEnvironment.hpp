/*
 *  Copyright 2019-2022 Diligent Graphics LLC
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

#pragma once

#include <memory>

#include "TestingEnvironment.hpp"
#include "RenderDevice.h"
#include "DeviceContext.h"
#include "RefCntAutoPtr.hpp"
#include "SwapChain.h"
#include "GraphicsTypesOutputInserters.hpp"
#include "NativeWindow.h"
#if ARCHIVER_SUPPORTED
#    include "ArchiverFactory.h"
#endif
#include "gtest/gtest.h"

namespace Diligent
{

namespace Testing
{

class GPUTestingEnvironment : public TestingEnvironment
{
public:
    static GPUTestingEnvironment* Initialize(int argc, char** argv);

    struct CreateInfo
    {
        RENDER_DEVICE_TYPE deviceType             = RENDER_DEVICE_TYPE_UNDEFINED;
        ADAPTER_TYPE       AdapterType            = ADAPTER_TYPE_UNKNOWN;
        Uint32             AdapterId              = DEFAULT_ADAPTER_ID;
        Uint32             NumDeferredContexts    = 4;
        bool               EnableDeviceSimulation = false;

        DeviceFeatures Features{DEVICE_FEATURE_STATE_OPTIONAL};
    };
    GPUTestingEnvironment(const CreateInfo& CI, const SwapChainDesc& SCDesc);

    ~GPUTestingEnvironment() override;

    // Override this to define how to set up the environment.
    void SetUp() override final;

    // Override this to define how to tear down the environment.
    void TearDown() override final;

    virtual void Reset();

    virtual bool HasDXCompiler() const { return false; }

    // Returns true if RayTracing feature is enabled and compiler can compile HLSL ray tracing shaders.
    virtual bool SupportsRayTracing() const { return false; }

    virtual void GetDXCompilerVersion(Uint32& MajorVersion, Uint32& MinorVersion) const
    {
        MajorVersion = 0;
        MinorVersion = 0;
    }

    void ReleaseResources();

    class ScopedReset
    {
    public:
        ScopedReset() = default;
        ~ScopedReset()
        {
            if (auto* pTheEnvironment = GetInstance())
                pTheEnvironment->Reset();
        }
    };

    class ScopedReleaseResources
    {
    public:
        ScopedReleaseResources() = default;
        ~ScopedReleaseResources()
        {
            if (auto* pTheEnvironment = GetInstance())
                pTheEnvironment->ReleaseResources();
        }
    };

#if ARCHIVER_SUPPORTED
    IArchiverFactory* GetArchiverFactory()
    {
        return m_ArchiverFactory;
    }
#endif

    IRenderDevice* GetDevice()
    {
        return m_pDevice;
    }
    IDeviceContext* GetDeviceContext(size_t ctx = 0)
    {
        VERIFY_EXPR(ctx < m_NumImmediateContexts);
        return m_pDeviceContexts[ctx];
    }
    IDeviceContext* GetDeferredContext(size_t ctx) { return m_pDeviceContexts[m_NumImmediateContexts + ctx]; }
    ISwapChain*     GetSwapChain() { return m_pSwapChain; }
    size_t          GetNumDeferredContexts() const { return m_pDeviceContexts.size() - m_NumImmediateContexts; }
    size_t          GetNumImmediateContexts() const { return m_NumImmediateContexts; }

    static GPUTestingEnvironment* GetInstance() { return ClassPtrCast<GPUTestingEnvironment>(m_pTheEnvironment); }

    RefCntAutoPtr<ITexture> CreateTexture(const char* Name, TEXTURE_FORMAT Fmt, BIND_FLAGS BindFlags, Uint32 Width, Uint32 Height, void* pInitData = nullptr);

    RefCntAutoPtr<ISampler> CreateSampler(const SamplerDesc& Desc);

    void            SetDefaultCompiler(SHADER_COMPILER compiler);
    SHADER_COMPILER GetDefaultCompiler(SHADER_SOURCE_LANGUAGE lang) const;

    ADAPTER_TYPE GetAdapterType() const { return m_AdapterType; }

    bool NeedWARPResourceArrayIndexingBugWorkaround() const
    {
        return m_NeedWARPResourceArrayIndexingBugWorkaround;
    }

protected:
    NativeWindow CreateNativeWindow();

    Uint32 FindAdapter(const std::vector<GraphicsAdapterInfo>& Adapters,
                       ADAPTER_TYPE                            AdapterType,
                       Uint32                                  AdapterId);

    const RENDER_DEVICE_TYPE m_DeviceType;

    ADAPTER_TYPE m_AdapterType = ADAPTER_TYPE_UNKNOWN;

    // Any platform-specific data (e.g. window handle) that should
    // be cleaned-up when the testing environment object is destroyed.
    struct PlatformData
    {
        virtual ~PlatformData() {}
    };
    std::unique_ptr<PlatformData> m_pPlatformData;

    RefCntAutoPtr<IRenderDevice>               m_pDevice;
    std::vector<RefCntAutoPtr<IDeviceContext>> m_pDeviceContexts;
    Uint32                                     m_NumImmediateContexts = 1;
    RefCntAutoPtr<ISwapChain>                  m_pSwapChain;
    SHADER_COMPILER                            m_ShaderCompiler = SHADER_COMPILER_DEFAULT;

    // As of Windows version 2004 (build 19041), there is a bug in D3D12 WARP rasterizer:
    // Shader resource array indexing always references array element 0 when shaders are compiled.
    // A workaround is to use SM5.0 and default shader compiler.
    bool m_NeedWARPResourceArrayIndexingBugWorkaround = false;

#if ARCHIVER_SUPPORTED
    RefCntAutoPtr<IArchiverFactory> m_ArchiverFactory;
#endif
};

} // namespace Testing

} // namespace Diligent
