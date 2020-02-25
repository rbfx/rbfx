using System;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Threading;

namespace Urho3DNet
{
    public partial class RefCounted
    {
        /// <summary>
        /// Number of references that were acquired from managed runtime.
        /// </summary>
        private int _netRefs = 0;
        /// <summary>
        /// Increment reference count. Operation is atomic.
        /// Converts GCHandle to weak if this is a first .NET reference.
        /// </summary>
        /// <returns>Returns new reference count value</returns>
        public int AddRef()
        {
            if (Interlocked.Increment(ref _netRefs) == 1)
            {
                // Managed side intends to keep a reference to this object. GCHandle is converted to weak in order to
                // allow object to be garbage-collected when there are no native references and last managed reference
                // was lost by mistake. User should still explicitly ReleaseRef() for deterministic object destruction.
                SetScriptObject(GCHandle.ToIntPtr(GCHandle.Alloc(this, GCHandleType.Weak)));
            }
            return _AddRef();
        }
        /// <summary>
        /// Decrement reference count and delete self if no more references. Operation is atomic.
        /// Converts GCHandle to strong if .NET refcount reaches 0.
        /// </summary>
        /// <returns>Returns new reference count value.</returns>
        /// <exception cref="InvalidProgramException">Thrown on attempt to release a reference that was not previously acquired.</exception>
        public int ReleaseRef()
        {
            if (_ReleaseRefNet() < 0)
                throw new InvalidProgramException("Attempt to release a managed reference that was not acquired.");
            return _ReleaseRef();
        }
        /// <summary>
        /// Decrease .NET refcount. .NET refcount will not go below 0 even if this method is called when refcount is 0.
        /// Converts GCHandle to strong if .NET refcount reaches 0.
        /// </summary>
        /// <returns>Returns new .NET refcount.</returns>
        internal int _ReleaseRefNet()
        {
            if (Interlocked.CompareExchange(ref _netRefs, 0, 0) == 0)
                return -1;

            int netRefs = Interlocked.Decrement(ref _netRefs);
            if (netRefs == 0)
            {
                // Managed side no longer intends to keep a reference to this object. GCHandle is converted to strong in
                // order to prevent premature object destruction when native side still holds a reference.
                SetScriptObject(GCHandle.ToIntPtr(GCHandle.Alloc(this, GCHandleType.Normal)));
            }
            return netRefs;
        }
        /// <summary>
        /// Queue object to Script subsystem for it's ReleaseRef() method to be called on main thread.
        /// </summary>
        /// <returns>Expected new refcount.</returns>
        internal int ReleaseRefSafe()
        {
            var script = Context.Instance?.GetSubsystem<Script>();
            if (script != null)
            {
                Interlocked.Decrement(ref _netRefs);
                return script.ReleaseRefOnMainThread(this);
            }
            else
                return ReleaseRef();
        }
        /// <summary>
        /// Heper that calls Dispose(true). This method is called from native code when object is about to be destroyed.
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
    }
}
