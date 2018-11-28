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
using System.Collections.Generic;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Security;
using Urho3DNet.CSharp;

namespace Urho3DNet
{
    public class ObjectFactoryAttribute : System.Attribute
    {
        public string Category { get; set; } = "";
    }

    public class CSharpCallbackHelper
    {
        static private List<object> callbackReferences = new List<object>();

        public static void RegisterCallback(Delegate callback)
        {

        }
    }

    public partial class Context
    {
        public static Context Instance { get; private set; }

        private readonly Dictionary<uint, Type> _factoryTypes = new Dictionary<uint, Type>();

        // This method may be overriden in partial class in order to attach extra logic to object constructor
        internal void OnSetupInstance()
        {
            Urho3DRegisterDirectorFactories(swigCPtr);

            // Register factories marked with attributes
            foreach (var assembly in AppDomain.CurrentDomain.GetAssemblies())
            {
                foreach (var pair in assembly.GetTypesWithAttribute<ObjectFactoryAttribute>())
                    RegisterFactory(pair.Item1, pair.Item2.Category);
            }

            Instance = this;
        }

        internal void OnDispose()
        {
            // In order to allow clean exit we first must release all cached instances because they hold references to
            // native objects and that prevents them being deallocated. If we failed to do that then Context destructor
            // would run before destroctors of other objects (subsystems, etc). Everything that inherits from
            // Urho3D.Object accesses Context on destruction and that would cause a crash.
            Instance = null;

            foreach (var assembly in AppDomain.CurrentDomain.GetAssemblies())
            {
                foreach (var type in assembly.GetTypes())
                {
                    var memberInfo = type.GetField("_instanceCache", BindingFlags.Static | BindingFlags.NonPublic);
                    ((IDisposable) memberInfo?.GetValue(null))?.Dispose();
                }
            }
        }

        public void RegisterFactory<T>(string category = "") where T : Object
        {
            RegisterFactory(typeof(T), category);
        }

        public void RegisterFactory(Type type, string category="")
        {
            if (!type.IsSubclassOf(typeof(Object)))
                throw new ArgumentException("Type must be subclass of Object.");

            _factoryTypes[StringHash.Calculate(type.Name)] = type;

            // Find a wrapper base type.
            var baseType = type.BaseType;
            while (baseType != null && baseType.Assembly != typeof(Context).Assembly)
                baseType = baseType.BaseType;

            if (baseType == null)
                throw new InvalidOperationException("This type can not be registered as factory.");

            Urho3D_Context_RegisterFactory(getCPtr(this), type.Name, StringHash.Calculate("SwigDirector_" + baseType.Name), category);
        }

        internal HandleRef CreateObject(uint managedType)
        {
            Type type;
            if (!_factoryTypes.TryGetValue(managedType, out type))
                return new HandleRef(null, IntPtr.Zero);
            var managed = (Object)Activator.CreateInstance(type, BindingFlags.Public | BindingFlags.Instance,
                null, new object[] { this }, null);
            return Object.getCPtr(managed);
        }

        public T GetSubsystem<T>() where T: Object
        {
            return (T) GetSubsystem(new StringHash(typeof(T).Name));
        }

        #region Interop

        [SuppressUnmanagedCodeSecurity]
        [DllImport("Urho3DCSharp", CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3DRegisterDirectorFactories(HandleRef context);

        [SuppressUnmanagedCodeSecurity]
        [DllImport("Urho3DCSharp", CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_Context_RegisterFactory(HandleRef context,
            [MarshalAs(UnmanagedType.LPStr)]string typeName, uint baseType,
            [MarshalAs(UnmanagedType.LPStr)]string category);

        #endregion
    }
}
