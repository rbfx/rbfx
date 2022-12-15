//
// Copyright (c) 2022-2022 the rbfx project.
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
using System.Collections.Generic;
using System.Diagnostics;

namespace Urho3DNet
{
    public class SharedPtr<T> : IDisposable, IEquatable<SharedPtr<T>> where T : RefCounted
    {
        /// <summary>
        /// Construct immutable SharedPtr object.
        /// Don't forget to dispose it.
        /// </summary>
        /// <param name="other">Object to store pointer to.</param>
        public SharedPtr(T other)
        {
            if (other?.IsNotExpired == true)
            {
                _value = other;
                _value.AddRef();
            }
            else
                _value = null;
        }

        /// <summary>
        /// SharedPtr finalizer.
        /// Should not be executed on not-disposed SharedPtr.
        /// </summary>
        ~SharedPtr()
        {
            if (_value?.IsNotExpired == true)
                Trace.WriteLine(
                    $"Object of type {typeof(T).FullName} wasn't properly disposed. Please call Dispose on all SharedPtr<{typeof(T).Name}> instances.");
        }

        /// <summary>
        /// Copy SharedPtr.
        /// </summary>
        /// <param name="other">SharedPtr instance to copy</param>
        public SharedPtr(SharedPtr<T> other) : this(other?._value)
        {
        }

        /// <summary>Indicates whether the current object is equal to another object of the same type.</summary>
        /// <param name="other">An object to compare with this object.</param>
        /// <returns>true if the current object is equal to the <paramref name="other">other</paramref> parameter; otherwise, false.</returns>
        public bool Equals(SharedPtr<T> other)
        {
            if (ReferenceEquals(null, other)) return false;
            if (ReferenceEquals(this, other)) return true;
            return EqualityComparer<T>.Default.Equals(_value, other._value);
        }

        /// <summary>Determines whether the specified object is equal to the current object.</summary>
        /// <param name="obj">The object to compare with the current object.</param>
        /// <returns>true if the specified object  is equal to the current object; otherwise, false.</returns>
        public override bool Equals(object obj)
        {
            if (ReferenceEquals(null, obj)) return false;
            if (ReferenceEquals(this, obj)) return true;
            if (obj.GetType() != this.GetType()) return false;
            return Equals((SharedPtr<T>)obj);
        }

        /// <summary>Serves as the default hash function.</summary>
        /// <returns>A hash code for the current object.</returns>
        public override int GetHashCode()
        {
            return EqualityComparer<T>.Default.GetHashCode(_value);
        }

        /// <summary>
        /// Implicitly convert SharedPtr into value.
        /// </summary>
        /// <param name="other"></param>
        public static implicit operator T(SharedPtr<T> @this) => @this._value;

        /// <summary>Indicates whether the two objects are equal.</summary>
        public static bool operator ==(SharedPtr<T> left, SharedPtr<T> right)
        {
            return Equals(left, right);
        }

        /// <summary>Indicates whether the two objects are not equal.</summary>
        public static bool operator !=(SharedPtr<T> left, SharedPtr<T> right)
        {
            return !Equals(left, right);
        }

        /// <summary>
        /// Get the underlying value.
        /// </summary>
        public T Ptr => _value;

        /// <summary>Performs application-defined tasks associated with freeing, releasing, or resetting unmanaged resources.</summary>
        public void Dispose()
        {
            if (_value != null)
            {
                _value.ReleaseRef();
            }
        }

        /// <summary>Returns a string that represents the current object.</summary>
        /// <returns>A string that represents the current object.</returns>
        public override string ToString()
        {
            return $"{_value}";
        }

        private readonly T _value;
    }

    public static class SharedPtr
    {
        /// <summary>
        /// Create an object by type.
        /// </summary>
        public static SharedPtr<T> MakeShared<T>(Context context) where T : Object
        {
            return new SharedPtr<T>(context.CreateObject<T>());
        }
    }
}
