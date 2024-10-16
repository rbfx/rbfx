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
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System;

namespace Diligent;

public static unsafe class ComObjectVtbl
{
    public static readonly IntPtr[] Vtbl =
    {
        (IntPtr) (delegate* unmanaged[Cdecl]<IntPtr, Guid*, void*, void>) (&QueryInterfaceImpl),
        (IntPtr) (delegate* unmanaged[Cdecl]<IntPtr, uint>) (&AddRefImpl),
        (IntPtr) (delegate* unmanaged[Cdecl]<IntPtr, uint>) (&ReleaseImpl),
        (IntPtr) (delegate* unmanaged[Cdecl]<IntPtr, IntPtr>) (&GetReferenceCountersImpl)
    };

    [UnmanagedCallersOnly(CallConvs = new[] { typeof(CallConvCdecl) })]
    private static void QueryInterfaceImpl(IntPtr thisObject, Guid* guid, void* output)
    {
        var callback = CppObjectShadow.ToCallback<CallbackBase>(thisObject);

        ref var ppvObject = ref Unsafe.AsRef<IntPtr>(output);
        var result = callback.Find(*guid);
        ppvObject = result;

        if (result == IntPtr.Zero)
            throw new Exception();

        MarshallingHelpers.AddRef(callback);
    }

    [UnmanagedCallersOnly(CallConvs = new[] { typeof(CallConvCdecl) })]
    private static uint AddRefImpl(IntPtr thisObject) => MarshallingHelpers.AddRef(CppObjectShadow.ToCallback<IObject>(thisObject));

    [UnmanagedCallersOnly(CallConvs = new[] { typeof(CallConvCdecl) })]
    private static uint ReleaseImpl(IntPtr thisObject) => MarshallingHelpers.Release(CppObjectShadow.ToCallback<IObject>(thisObject));

    [UnmanagedCallersOnly(CallConvs = new[] { typeof(CallConvCdecl) })]
    private static IntPtr GetReferenceCountersImpl(IntPtr thisObject) => throw new NotImplementedException();
}
