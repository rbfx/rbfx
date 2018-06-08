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
using System.Diagnostics;
using System.Reflection;
using Urho3D.CSharp;


namespace Urho3D
{
    public partial class Component
    {
        public T GetComponent<T>() where T : Component
        {
            var componentInstance = Urho3D__Component__GetComponent_Urho3D__StringHash__const(NativeInstance, StringHash.Calculate(typeof(T).Name));
            if (componentInstance == IntPtr.Zero)
                return null;
            return InstanceCache.GetOrAdd(componentInstance, ptr => (T)Activator.CreateInstance(typeof(T),
                BindingFlags.NonPublic | BindingFlags.Instance, null, new object[] { ptr, false }, null));
        }
    }
}
