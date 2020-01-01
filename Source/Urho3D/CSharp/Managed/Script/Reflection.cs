//
// Copyright (c) 2017-2020 the rbfx project.
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
using System.Reflection;

namespace Urho3DNet.CSharp
{
    public static class ReflectionExtensions
    {
        public static IEnumerable<Tuple<Type, T>> GetTypesWithAttribute<T>(this Assembly assembly)
        {
            foreach(var type in assembly.GetTypes())
            {
                var attributes = type.GetCustomAttributes(typeof(T), true);
                foreach (var attribute in attributes)
                {
                    yield return new Tuple<Type, T>(type, (T) attribute);
                }
            }
        }

        public static IEnumerable<Tuple<MethodInfo, T>> GetMethodsWithAttribute<T>(this Assembly assembly)
        {
            foreach(var type in assembly.GetTypes())
            {
                foreach (var method in type.GetMethods())
                {
                    var attributes = method.GetCustomAttributes(typeof(T), true);
                    foreach (var attribute in attributes)
                    {
                        yield return new Tuple<MethodInfo, T>(method, (T) attribute);
                    }
                }
            }
        }
    }
}
