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

public partial class IDeviceContext
{
    public unsafe Span<T> MapBuffer<T>(IBuffer buffer, MapType mapTypes, MapFlags mapFlags) where T : unmanaged
    {
        var data = new Span<byte>(MapBuffer(buffer, mapTypes, mapFlags).ToPointer(), (int)buffer.GetDesc().Size);
        return MemoryMarshal.Cast<byte, T>(data);
    }

    public unsafe void SetVertexBuffers(uint startSlot, IBuffer[] buffers, ulong[] offsets, ResourceStateTransitionMode stateTransitionMode, SetVertexBuffersFlags flags = SetVertexBuffersFlags.None)
    {
        var ppBuffers = stackalloc IntPtr[buffers.Length];
        for (var i = 0; i < buffers.Length; i++)
            ppBuffers[i] = buffers[i]?.NativePointer ?? IntPtr.Zero;

        fixed (ulong* pOffsets = offsets)
            SetVertexBuffers(startSlot, (uint)buffers.Length, ppBuffers, pOffsets, stateTransitionMode, flags);
        GC.KeepAlive(buffers);
    }

    public unsafe void SetRenderTargets(ITextureView[] renderTargetViews, ITextureView depthStencilView, ResourceStateTransitionMode mode)
    {
        var ppRTVs = stackalloc IntPtr[renderTargetViews?.Length ?? 0];
        for (var i = 0; i < renderTargetViews?.Length; i++)
            ppRTVs[i] = renderTargetViews[i]?.NativePointer ?? IntPtr.Zero;
        SetRenderTargets((uint)(renderTargetViews?.Length ?? 0), ppRTVs, depthStencilView, mode);
        GC.KeepAlive(renderTargetViews);
    }

    public unsafe void UpdateBuffer<T>(IBuffer buffer, ulong offset, ReadOnlySpan<T> data, ResourceStateTransitionMode stateTransitionMode) where T : unmanaged
    {
        fixed (T* dataPtr = data)
            UpdateBuffer(buffer, offset, (ulong)(Unsafe.SizeOf<T>() * data.Length), new(dataPtr), stateTransitionMode);
    }

    public unsafe void ExecuteCommandLists(ICommandList[] commandLists)
    {
        var ppCmdLists = stackalloc IntPtr[commandLists.Length];
        for (var i = 0; i < commandLists.Length; i++)
            ppCmdLists[i] = commandLists[i].NativePointer;
        ExecuteCommandLists((uint)commandLists.Length, ppCmdLists);
        GC.KeepAlive(commandLists);
    }
}
