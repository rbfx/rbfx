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
using System.Runtime.InteropServices;
using System.Security;
using Urho3DNet.CSharp;

namespace Urho3DNet
{
    public class ObjectFactoryAttribute : System.Attribute
    {
        public string Category { get; set; } = "";
    }

    public partial class Context
    {
        public static Context Instance { get; private set; }

        private readonly Dictionary<uint, Type> _factoryTypes = new Dictionary<uint, Type>();

        public static void SetRuntimeApi(ScriptRuntimeApi impl)
        {
            Script.GetRuntimeApi()?.Dispose();
            Script.SetRuntimeApi(impl);
        }

        static Context()
        {
            SetRuntimeApi(new ScriptRuntimeApiImpl());
        }

        // This method may be overriden in partial class in order to attach extra logic to object constructor
        internal void OnSetupInstance()
        {
            using (var script = new Script(this))
                RegisterSubsystem(script);

            Urho3DRegisterDirectorFactories(swigCPtr);

            // Register factories marked with attributes
            foreach (var assembly in AppDomain.CurrentDomain.GetAssemblies())
            {
                // Exclude system libraries and UWP HiddenScope assembly.
                var assemblyName = assembly.GetName().Name;
                if (!assemblyName.StartsWith("System.") && assemblyName != "HiddenScope")
                {
                    RegisterFactories(assembly);
                }
            }

            Instance = this;
        }

        public void RegisterFactories(Assembly assembly)
        {
            foreach (var pair in assembly.GetTypesWithAttribute<ObjectFactoryAttribute>())
                RegisterFactory(pair.Item1, pair.Item2.Category);
        }

        public void RemoveFactories(Assembly assembly)
        {
            foreach (var pair in assembly.GetTypesWithAttribute<ObjectFactoryAttribute>())
                RemoveFactory(pair.Item1, pair.Item2.Category);
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

        // Create an object by type. Return pointer to it or null if no factory found.
        public T CreateObject<T>() where T : Object
        {
            return (T)CreateObject(typeof(T).Name);
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

        // Register object attribute.
        public AttributeHandle RegisterAttribute<T>(AttributeInfo attributeInfo) where T : Object
        {
            return RegisterAttribute(new StringHash(typeof(T).Name), attributeInfo);
        }

        // Remove object attribute.
        public void RemoveAttribute<T>(string name) where T : Object
        {
            RemoveAttribute(new StringHash(typeof(T).Name), name);
        }

        // Update object attribute's default value.
        public void UpdateAttributeDefaultValue<T>(string name, Variant defaultValue) where T : Object
        {
            UpdateAttributeDefaultValue(new StringHash(typeof(T).Name), name, defaultValue);
        }

        #region Interop

        [SuppressUnmanagedCodeSecurity]
        [DllImport(global::Urho3DNet.Urho3DPINVOKE.DllImportModule)]
        private static extern void Urho3DRegisterDirectorFactories(HandleRef context);

        [SuppressUnmanagedCodeSecurity]
        [DllImport(global::Urho3DNet.Urho3DPINVOKE.DllImportModule)]
        private static extern void Urho3D_Context_RegisterFactory(HandleRef context,
            [MarshalAs(UnmanagedType.LPStr)]string typeName, uint baseType,
            [MarshalAs(UnmanagedType.LPStr)]string category);

        #endregion
    }
}
