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

using SharpGen.Runtime;
using System;
using System.Runtime.InteropServices;

namespace Diligent;

[ExcludeFromTypeList]
[Guid("00000000-0000-0000-0000-000000000000"), Vtbl(typeof(ComObjectVtbl))]
public interface IObject : ICallbackable { }


[Guid("00000000-0000-0000-0000-000000000000")]
public partial class ComObject : CppObject, IObject
{
    public ComObject(IntPtr nativePtr) : base(nativePtr)
    {
    }

    public static explicit operator ComObject(IntPtr nativePtr) => nativePtr == IntPtr.Zero ? null : new ComObject(nativePtr);

    public unsafe void QueryInterface(Guid riid, out IntPtr ppvObject)
    {
        fixed (void* ppvObject_ = &ppvObject)
            ((delegate* unmanaged[Cdecl]<IntPtr, void*, void*, void>)this[0U])(NativePointer, &riid, ppvObject_);
    }

    public unsafe uint AddRef()
    {
        return ((delegate* unmanaged[Cdecl]<IntPtr, uint>)this[1U])(NativePointer);
    }

    public unsafe uint Release()
    {
        return ((delegate* unmanaged[Cdecl]<IntPtr, uint>)this[2U])(NativePointer);
    }

    public unsafe IntPtr GetReferenceCounters()
    {
        return ((delegate* unmanaged[Cdecl]<IntPtr, IntPtr>)this[3U])(NativePointer);
    }

    protected override void DisposeCore(IntPtr nativePointer, bool disposing)
    {
        if (disposing || Configuration.EnableReleaseOnFinalizer)
            Release();
    }
}
