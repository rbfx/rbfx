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
using System.Runtime.InteropServices;

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
        public bool IsDisposed => DisposedCounter == 0;
        protected volatile int DisposedCounter = 0;
        internal int NativeObjectSize = 0;

        internal NativeObject(IntPtr instance, bool ownsInstance=false)
        {
            if (instance != IntPtr.Zero)
            {
                NativeInstance = instance;
                OwnsNativeInstance = ownsInstance;
            }
        }

        internal NativeObject()
        {
        }

        ~NativeObject()
        {
            Dispose();
        }

        // Derived types will put object initialization code here. At the time of writing such code sets up virtual
        // method callbacks and nothing more.
        internal virtual void PerformInstanceSetup(IntPtr instance, bool ownsInstance)
        {
            OwnsNativeInstance = ownsInstance;
            NativeInstance = instance;
            InstanceCache.Add(this);
        }

        // This method may be overriden in partial class in order to attach extra logic to object constructor
        internal virtual void SetupInstance(IntPtr instance, bool ownsInstance)
        {
            PerformInstanceSetup(instance, ownsInstance);
        }

        // This method may be overriden in a partial class or a subclass in order to inject logic into object
        // destruction.
        protected virtual void OnDispose()
        {
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
            if (obj.GetType() != this.GetType()) return false;
            return Equals((NativeObject) obj);
        }

        public override int GetHashCode()
        {
            unchecked
            {
                return (NativeInstance.GetHashCode() * 397) ^ DisposedCounter;
            }
        }

        public virtual void Dispose()
        {
        }
    }
}
