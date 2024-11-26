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

using System;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Diligent;

public partial struct ShaderUnpackInfo
{
    public delegate void ModifyShaderDelegate(ref ShaderDesc desc);

    [UnmanagedCallersOnly(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static unsafe void ModifyShaderDescImpl(IntPtr nativeDesc, IntPtr userData)
    {
        ShaderDesc desc = default;
        desc.__MarshalFrom(ref *(ShaderDesc.__Native*)nativeDesc);
        Marshal.GetDelegateForFunctionPointer<ModifyShaderDelegate>(userData)(ref desc);
        desc.__MarshalTo(ref *(ShaderDesc.__Native*)nativeDesc);
    }
}

public partial struct PipelineStateUnpackInfo
{
    public delegate void ModifyPipelineStateDelegate(PipelineStateCreateInfo createInfo);

    [UnmanagedCallersOnly(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static unsafe void ModifyPipelineStateCreateInfoImpl(IntPtr nativeCI, IntPtr userData)
    {
        // TODO DRY
        switch (((PipelineStateCreateInfo.__Native*)nativeCI)->PSODesc.PipelineType)
        {
            case PipelineType.Mesh:
            case PipelineType.Graphics:
                {
                    var pipelineCI = new GraphicsPipelineStateCreateInfo(ref *(GraphicsPipelineStateCreateInfo.__Native*)nativeCI);
                    Marshal.GetDelegateForFunctionPointer<ModifyPipelineStateDelegate>(userData)(pipelineCI);
                    pipelineCI.__MarshalTo(ref *(GraphicsPipelineStateCreateInfo.__Native*)nativeCI);
                    break;
                }
            case PipelineType.Compute:
                {
                    var pipelineCI = new ComputePipelineStateCreateInfo(ref *(ComputePipelineStateCreateInfo.__Native*)nativeCI);
                    Marshal.GetDelegateForFunctionPointer<ModifyPipelineStateDelegate>(userData)(pipelineCI);
                    pipelineCI.__MarshalTo(ref *(ComputePipelineStateCreateInfo.__Native*)nativeCI);
                    break;
                }
            case PipelineType.RayTracing:
                {
                    var pipelineCI = new RayTracingPipelineStateCreateInfo(ref *(RayTracingPipelineStateCreateInfo.__Native*)nativeCI);
                    Marshal.GetDelegateForFunctionPointer<ModifyPipelineStateDelegate>(userData)(pipelineCI);
                    pipelineCI.__MarshalTo(ref *(RayTracingPipelineStateCreateInfo.__Native*)nativeCI);
                    break;
                }
            case PipelineType.Tile:
                {
                    var pipelineCI = new TilePipelineStateCreateInfo(ref *(TilePipelineStateCreateInfo.__Native*)nativeCI);
                    Marshal.GetDelegateForFunctionPointer<ModifyPipelineStateDelegate>(userData)(pipelineCI);
                    pipelineCI.__MarshalTo(ref *(TilePipelineStateCreateInfo.__Native*)nativeCI);
                    break;
                }
            default:
                throw new InvalidOperationException();
        }
    }
}

public partial struct RenderPassUnpackInfo
{
    public delegate void ModifyRenderPassDelegate(ref RenderPassDesc desc);

    [UnmanagedCallersOnly(CallConvs = new[] { typeof(CallConvCdecl) })]
    internal static unsafe void ModifyRenderPassDescImpl(IntPtr nativeDesc, IntPtr userData)
    {
        var desc = new RenderPassDesc { };
        desc.__MarshalFrom(ref *(RenderPassDesc.__Native*)nativeDesc);
        Marshal.GetDelegateForFunctionPointer<ModifyRenderPassDelegate>(userData)(ref desc);
        desc.__MarshalTo(ref *(RenderPassDesc.__Native*)nativeDesc);
    }
}
