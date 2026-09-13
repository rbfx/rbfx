// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

using System;
using System.Runtime.InteropServices;

namespace Urho3DNet
{
    public partial class Microphone
    {
        [DllImport(global::Urho3DNet.Urho3DPINVOKE.DllImportModule, EntryPoint = "Urho3D_Microphone_CopyDataToSpan")]
        private static extern uint Urho3D_Microphone_CopyDataToSpan(HandleRef receiver, IntPtr data, uint length);

        public uint CopyData(Span<short> data)
        {
            unsafe
            {
                fixed (short* pointer = data)
                {
                    return Urho3D_Microphone_CopyDataToSpan(swigCPtr, (IntPtr)pointer, (uint)data.Length);
                }
            }
        }
    }
}
