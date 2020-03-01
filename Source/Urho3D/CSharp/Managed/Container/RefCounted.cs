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
            Dispose(_disposing);
        }
        /// <summary>
        /// Returns true if native object pointer is null.
        /// </summary>
        public bool Expired => swigCPtr.Handle == IntPtr.Zero;
        /// <summary>
        /// Returns true if native object pointer is not null.
        /// </summary>
        public bool NotExpired => swigCPtr.Handle != IntPtr.Zero;
        /// <summary>
        /// Free GC handle saved in a native object instance.
        /// </summary>
        internal void FreeGCHandle()
        {
            if (Expired)
                return;

            GCHandle handle = GCHandle.FromIntPtr(GetScriptObject());
            if (handle.IsAllocated)
                handle.Free();
            ResetScriptObject();
        }
    }
}
