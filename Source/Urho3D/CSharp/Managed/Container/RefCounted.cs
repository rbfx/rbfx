// Copyright (c) 2026-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

using System;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Threading;

namespace Urho3DNet
{
    public partial class RefCounted
    {
        /// <summary>
        /// Helper that calls Dispose(true). This method is called from native code when object is about to be destroyed.
        /// </summary>
        internal void DisposeInternal()
        {
            Dispose(true);
        }
        /// <summary>
        /// Returns true if native object pointer is null.
        /// </summary>
        public bool IsExpired => swigCPtr.Handle == IntPtr.Zero;
        /// <summary>
        /// Returns true if native object pointer is not null.
        /// </summary>
        public bool IsNotExpired => swigCPtr.Handle != IntPtr.Zero;
        /// <summary>
        /// Free GC handle saved in a native object instance.
        /// </summary>
        internal void FreeGCHandle()
        {
            if (IsExpired)
                return;

            IntPtr cPtr = GetScriptObject();
            if (cPtr != IntPtr.Zero)
            {
                GCHandle handle = GCHandle.FromIntPtr(cPtr);
                if (handle.IsAllocated)
                    handle.Free();
                ResetScriptObject();
            }
        }
    }
}
