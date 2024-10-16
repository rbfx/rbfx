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
using SharpGen.Runtime;

namespace Diligent;

public partial class IEngineFactory
{
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void MessageCallbackDelegate(DebugMessageSeverity severity, string message, string function, string file, int line);

    private MessageCallbackDelegate m_MessageCallbackRef;

    public unsafe GraphicsAdapterInfo[] EnumerateAdapters(Version minVersion)
    {
        uint numAdapters = default;
        EnumerateAdapters(minVersion, &numAdapters, null);
        if (numAdapters > 0)
        {
            var dataPtr = stackalloc GraphicsAdapterInfo.__Native[(int)numAdapters];
            EnumerateAdapters(minVersion, &numAdapters, dataPtr);

            var adapterInfos = new GraphicsAdapterInfo[numAdapters];
            for (uint i = 0; i < numAdapters; i++)
            {
                adapterInfos[i] = new GraphicsAdapterInfo();
                adapterInfos[i].__MarshalFrom(ref dataPtr[i]);
            }
            return adapterInfos;
        }
        return null;
    }

    public void SetMessageCallback(MessageCallbackDelegate callback)
    {
        m_MessageCallbackRef = callback;
        SetMessageCallback(Marshal.GetFunctionPointerForDelegate(m_MessageCallbackRef));
    }

    public IDataBlob CreateDataBlob<T>(T[] data) where T : unmanaged
    {
        return CreateDataBlob(new ReadOnlySpan<T>(data));
    }

    public unsafe IDataBlob CreateDataBlob<T>(ReadOnlySpan<T> data) where T : unmanaged
    {
        fixed (T* dataPtr = data)
            return CreateDataBlob((nuint)(Unsafe.SizeOf<T>() * data.Length), new IntPtr(dataPtr));
    }

    internal unsafe delegate void CreateDeviceAndContextsDelegate(IEngineFactory @this, EngineCreateInfo createInfo, void* device, void* contexts);

    internal unsafe delegate void EnumerateDisplayModesDelegate(IEngineFactory @this, Version version, uint adapterId, uint outputId, TextureFormat format, void* numDisplayModes, void* displayModes);

    internal unsafe void CreateDeviceAndContextsImpl(EngineCreateInfo createInfo, out IRenderDevice renderDevice, out IDeviceContext[] contexts, CreateDeviceAndContextsDelegate callback)
    {
        var contextCount = Math.Max(1, createInfo.ImmediateContextInfo?.Length ?? 0) + (int)createInfo.NumDeferredContexts;
        var renderDeviceNative = IntPtr.Zero;
        var contextsNative = stackalloc IntPtr[contextCount];
        callback(this, createInfo, &renderDeviceNative, contextsNative);
        if (renderDeviceNative != IntPtr.Zero)
        {
            renderDevice = new IRenderDevice(renderDeviceNative);
            contexts = new IDeviceContext[contextCount];
            for (var i = 0; i < contexts.Length; i++)
                contexts[i] = contextsNative[i] != IntPtr.Zero ? new IDeviceContext(contextsNative[i]) : throw new InvalidOperationException();

            GC.KeepAlive(renderDevice);
            foreach (var e in contexts)
                GC.KeepAlive(e);
            return;
        }

        renderDevice = null;
        contexts = null;
    }

    internal unsafe DisplayModeAttribs[] EnumerateDisplayModesImpl(Version minFeatureLevel, uint adapterId, uint outputId, TextureFormat format, EnumerateDisplayModesDelegate callback)
    {
        uint numDisplayModes = default;
        callback(this, minFeatureLevel, adapterId, outputId, format, &numDisplayModes, null);
        if (numDisplayModes > 0)
        {
            var displayModes = new DisplayModeAttribs[(int)numDisplayModes];
            fixed (void* dataPtr = displayModes)
                callback(this, minFeatureLevel, adapterId, outputId, format, &numDisplayModes, dataPtr);
            return displayModes;
        }
        return null;
    }
}

public partial class IEngineFactoryD3D11
{
    public unsafe void CreateDeviceAndContextsD3D11(EngineD3D11CreateInfo createInfo, out IRenderDevice renderDevice, out IDeviceContext[] contexts)
    {
        CreateDeviceAndContextsImpl(createInfo, out renderDevice, out contexts,
            static (@this, createInfo, device, contexts) =>
            {
                ((IEngineFactoryD3D11)@this).CreateDeviceAndContextsD3D11((EngineD3D11CreateInfo)createInfo, device, contexts);
            });
    }

    public unsafe DisplayModeAttribs[] EnumerateDisplayModes(Version minFeatureLevel, uint adapterId, uint outputId, TextureFormat format)
    {
        return EnumerateDisplayModesImpl(minFeatureLevel, adapterId, outputId, format,
            static (@this, version, adapterId, outputId, format, numDisplayModes, displayModes) =>
            {
                ((IEngineFactoryD3D11)@this).EnumerateDisplayModes(version, adapterId, outputId, format, numDisplayModes, displayModes);
            });
    }
}

public partial class IEngineFactoryD3D12
{
    public unsafe void CreateDeviceAndContextsD3D12(EngineD3D12CreateInfo createInfo, out IRenderDevice renderDevice, out IDeviceContext[] contexts)
    {
        CreateDeviceAndContextsImpl(createInfo, out renderDevice, out contexts,
            static (@this, createInfo, device, contexts) =>
            {
                ((IEngineFactoryD3D12)@this).CreateDeviceAndContextsD3D12((EngineD3D12CreateInfo)createInfo, device, contexts);
            });
    }

    public unsafe DisplayModeAttribs[] EnumerateDisplayModes(Version minFeatureLevel, uint adapterId, uint outputId, TextureFormat format)
    {
        return EnumerateDisplayModesImpl(minFeatureLevel, adapterId, outputId, format,
            static (@this, version, adapterId, outputId, format, numDisplayModes, displayModes) =>
            {
                ((IEngineFactoryD3D12)@this).EnumerateDisplayModes(version, adapterId, outputId, format, numDisplayModes, displayModes);
            });
    }
}

public partial class IEngineFactoryVk
{
    public unsafe void CreateDeviceAndContextsVk(EngineVkCreateInfo createInfo, out IRenderDevice renderDevice, out IDeviceContext[] contexts)
    {
        CreateDeviceAndContextsImpl(createInfo, out renderDevice, out contexts,
            static (@this, createInfo, device, contexts) =>
            {
                ((IEngineFactoryVk)@this).CreateDeviceAndContextsVk((EngineVkCreateInfo)createInfo, device, contexts);
            });
    }
}
