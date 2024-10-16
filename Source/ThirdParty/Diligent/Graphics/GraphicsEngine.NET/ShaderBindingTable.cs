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

namespace Diligent;

public partial class IShaderBindingTable
{
    public unsafe void BindRayGenShader<T>(string shaderGroupName, ReadOnlySpan<T> data) where T : unmanaged
    {
        fixed (T* dataPtr = data)
            BindRayGenShader(shaderGroupName, new IntPtr(dataPtr), (uint)(Unsafe.SizeOf<T>() * data.Length));
    }

    public void BindRayGenShader<T>(string shaderGroupName, T[] data) where T : unmanaged
    {
        BindRayGenShader(shaderGroupName, new ReadOnlySpan<T>(data));
    }

    public unsafe void BindMissShader<T>(string shaderGroupName, uint missIndex, ReadOnlySpan<T> data) where T : unmanaged
    {
        fixed (T* dataPtr = data)
            BindMissShader(shaderGroupName, missIndex, new IntPtr(dataPtr), (uint)(Unsafe.SizeOf<T>() * data.Length));
    }

    public void BindMissShader<T>(string shaderGroupName, uint missIndex, T[] data) where T : unmanaged
    {
        BindMissShader(shaderGroupName, missIndex, new ReadOnlySpan<T>(data));
    }

    public unsafe void BindHitGroupForGeometry<T>(ITopLevelAS tlas, string instanceName, string geometryName, uint rayOffsetInHitGroupIndex, string shaderGroupName, ReadOnlySpan<T> data) where T : unmanaged
    {
        fixed (T* dataPtr = data)
            BindHitGroupForGeometry(tlas, instanceName, geometryName, rayOffsetInHitGroupIndex, shaderGroupName, new IntPtr(dataPtr), (uint)(Unsafe.SizeOf<T>() * data.Length));
    }

    public void BindHitGroupForGeometry<T>(ITopLevelAS tlas, string instanceName, string geometryName, uint rayOffsetInHitGroupIndex, string shaderGroupName, T[] data) where T : unmanaged
    {
        BindHitGroupForGeometry(tlas, instanceName, geometryName, rayOffsetInHitGroupIndex, shaderGroupName, new ReadOnlySpan<T>(data));
    }

    public unsafe void BindHitGroupByIndex<T>(uint bindingIndex, string shaderGroupName, ReadOnlySpan<T> data) where T : unmanaged
    {
        fixed (T* dataPtr = data)
            BindHitGroupByIndex(bindingIndex, shaderGroupName, new IntPtr(dataPtr), (uint)(Unsafe.SizeOf<T>() * data.Length));
    }

    public void BindHitGroupByIndex<T>(uint bindingIndex, string shaderGroupName, T[] data) where T : unmanaged
    {
        BindHitGroupByIndex(bindingIndex, shaderGroupName, new ReadOnlySpan<T>(data));
    }

    public unsafe void BindHitGroupForInstance<T>(ITopLevelAS tlas, string instanceName, uint rayOffsetInHitGroupIndex, string shaderGroupName, ReadOnlySpan<T> data) where T : unmanaged
    {
        fixed (T* dataPtr = data)
            BindHitGroupForInstance(tlas, instanceName, rayOffsetInHitGroupIndex, shaderGroupName, new IntPtr(dataPtr), (uint)(Unsafe.SizeOf<T>() * data.Length));
    }

    public void BindHitGroupForInstance<T>(ITopLevelAS tlas, string instanceName, uint rayOffsetInHitGroupIndex, string shaderGroupName, T[] data) where T : unmanaged
    {
        BindHitGroupForInstance(tlas, instanceName, rayOffsetInHitGroupIndex, shaderGroupName, new ReadOnlySpan<T>(data));
    }

    public unsafe void BindHitGroupForTLAS<T>(ITopLevelAS tlas, uint rayOffsetInHitGroupIndex, string shaderGroupName, ReadOnlySpan<T> data) where T : unmanaged
    {
        fixed (T* dataPtr = data)
            BindHitGroupForTLAS(tlas, rayOffsetInHitGroupIndex, shaderGroupName, new IntPtr(dataPtr), (uint)(Unsafe.SizeOf<T>() * data.Length));
    }

    public void BindHitGroupForTLAS<T>(ITopLevelAS tlas, uint rayOffsetInHitGroupIndex, string shaderGroupName, T[] data) where T : unmanaged
    {
        BindHitGroupForTLAS(tlas, rayOffsetInHitGroupIndex, shaderGroupName, new ReadOnlySpan<T>(data));
    }

    public unsafe void BindCallableShader<T>(string shaderGroupName, uint callableIndex, ReadOnlySpan<T> data) where T : unmanaged
    {
        fixed (T* dataPtr = data)
            BindCallableShader(shaderGroupName, callableIndex, new IntPtr(dataPtr), (uint)(Unsafe.SizeOf<T>() * data.Length));
    }

    public void BindCallableShader<T>(string shaderGroupName, uint callableIndex, T[] data) where T : unmanaged
    {
        BindCallableShader(shaderGroupName, callableIndex, new ReadOnlySpan<T>(data));
    }
}
