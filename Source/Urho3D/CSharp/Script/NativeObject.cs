//
// Copyright (c) 2018 Rokas Kupstys
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

using System;
using System.Threading;

namespace Urho3D.CSharp
{
    public interface INativeObject : IDisposable
    {
        IntPtr NativeInstance { get; }
        bool IsDisposed { get; }
    }

    public abstract class NativeObject : INativeObject, IEquatable<NativeObject>
    {
        public IntPtr NativeInstance { get; protected set; } = IntPtr.Zero;
        protected bool OwnsNativeInstance { get; set; }
        public bool IsDisposed => _disposedCounter == 0;
        private int _disposedCounter;
        internal int NativeObjectSize = 0;

        internal NativeObject(IntPtr instance, bool ownsInstance=false)
        {
            if (instance != IntPtr.Zero)
            {
                NativeInstance = instance;
                OwnsNativeInstance = ownsInstance;
                OnSetupInstance();
                // Instance is not added to InstanceCache here because this code will run only when invoked from
                // T.GetManagedInstance() method. This method takes care of adding instance to InstanceCache.
            }
        }

        internal NativeObject()
        {
        }

        ~NativeObject()
        {
            if (Interlocked.Increment(ref _disposedCounter) == 1)
                Dispose(false);
        }

        /// <summary>
        /// Partial wrapepr classes should override this method to inject extra logic to wrapper class setup. This
        /// method is called from a constructor therefore only (object) setup tasks not depending on object state should
        /// be performed here. It is guaranteed tat NativeInstance will be set before this method is called.
        /// </summary>
        internal virtual void OnSetupInstance()
        {
        }

        /// <summary>
        /// Wrapper classes will most likely implement new version of this method.
        /// </summary>
        /// <param name="instance"></param>
        /// <param name="ownsInstance"></param>
        internal void SetupInstance(IntPtr instance, bool ownsInstance)
        {
            OwnsNativeInstance = ownsInstance;
            NativeInstance = instance;
            InstanceCache.Add(this);
            OnSetupInstance();
        }

        public bool Equals(NativeObject other)
        {
            if (ReferenceEquals(null, other)) return false;
            if (ReferenceEquals(this, other)) return true;
            return NativeInstance.Equals(other.NativeInstance) && IsDisposed == other.IsDisposed;
        }

        public override bool Equals(object obj)
        {
            if (ReferenceEquals(null, obj)) return false;
            if (ReferenceEquals(this, obj)) return true;
            if (obj.GetType() != GetType()) return false;
            return Equals((NativeObject) obj);
        }

        public override int GetHashCode()
        {
            return NativeInstance.GetHashCode() * 397;
        }

        public void Dispose()
        {
            if (Interlocked.Increment(ref _disposedCounter) != 1)
                return;

            Dispose(true);
            GC.SuppressFinalize(this);
        }

        /// <summary>
        /// This method is used for disposing native resources. Wrapper classes mimic class inheritance of the native
        /// code therefore these wrapper classes must not call base.Dispose(disposing). Doing so would result in
        /// calling wrong destructor or releasing reference mutlple times.
        ///
        /// However user code inheriting from wrapper classes must always call base.Dispose(disposing). Failure to do
        /// so will cause a resource leak because native instance will never be deallocated. Subclasses of RefCounted
        /// or inheritable classes will also prevent deallocation of managed wrapepr object.
        /// </summary>
        /// <param name="disposing">Set to true when method is called from Dispose().</param>
        protected virtual void Dispose(bool disposing)
        {
        }

        /// <summary>
        /// This method may be overriden in a partial class or a subclass in order to inject logic into object
        /// destruction. It is required because wrapper class already overrides Dispose(bool).
        /// </summary>
        /// <param name="disposing">Set to true when method is called from Dispose().</param>
        internal virtual void OnDispose(bool disposing)
        {
        }
    }
}
