// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

using System;
using System.Collections.Generic;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Security;
using System.Threading.Tasks;
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
            Instance = this;

            using (var script = new Script(this))
            {
                RegisterSubsystem(script);
                script.SubscribeToEvent(E.EngineInitialized, (StringHash e, VariantMap args) => {
                    // Register factories marked with attributes
                    foreach (var assembly in AppDomain.CurrentDomain.GetAssemblies())
                    {
                        // Exclude system libraries and UWP HiddenScope assembly.
                        var assemblyName = assembly.GetName().Name;
                        if (!assemblyName.StartsWith("System.") && !assemblyName.StartsWith("Microsoft.Maui.") && assemblyName != "HiddenScope")
                        {
                            RegisterFactories(assembly);
                        }
                    }
                });
            }

            Urho3DRegisterDirectorFactories(swigCPtr);
        }

        public void RegisterFactories(Assembly assembly)
        {
            foreach (var pair in assembly.GetTypesWithAttribute<ObjectFactoryAttribute>())
                AddFactoryReflection(pair.Item1, pair.Item2.Category);
        }

        public void RemoveFactories(Assembly assembly)
        {
            foreach (var pair in assembly.GetTypesWithAttribute<ObjectFactoryAttribute>())
                RemoveReflection(pair.Item1);
        }

        public void AddFactoryReflection<T>(string category = "") where T : Object
        {
            AddFactoryReflection(typeof(T), category);
        }

        public bool IsReflected<T>() where T : Object
        {
            return IsReflected(typeof(T));
        }

        public bool IsReflected(Type type)
        {
            return IsReflected(new StringHash(type));
        }

        public void AddFactoryReflection(Type type, string category="")
        {
            if (!type.IsSubclassOf(typeof(Object)))
                throw new ArgumentException("Type must be subclass of Object.");

            StringHash typeId;
            var getTypeNameStatic = type.GetMethod(nameof(Urho3DNet.Object.GetTypeStatic), BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic);
            if (getTypeNameStatic != null)
            {
                typeId = (StringHash)getTypeNameStatic.Invoke(null, Array.Empty<object>());
            }
            else
            {
                typeId = type.Name;
            }

            _factoryTypes[typeId.Hash] = type;

            // Find a wrapper base type.
            var baseType = type.BaseType;
            while (baseType != null && baseType.Assembly != typeof(Context).Assembly)
                baseType = baseType.BaseType;

            if (baseType == null)
                throw new InvalidOperationException("This type can not be registered as factory.");

            Urho3D_Context_RegisterFactory(getCPtr(this), type.Name, StringHash.Calculate("SwigDirector_" + baseType.Name), category);
            if (type.IsSubclassOf(typeof(Serializable)))
            {
                using (var serializable = (Serializable)Activator.CreateInstance(type, new object[] { Context.Instance }))
                {
                    serializable.RegisterAttributes();
                }
            }
        }

        // Create an object by type. Return pointer to it or null if no factory found.
        public T CreateObject<T>() where T : Object
        {
            return (T)CreateObject(ObjectReflection<T>.TypeId);
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
            return (T) GetSubsystem(ObjectReflection<T>.TypeId);
        }

        public ConfiguredTaskAwaitable<bool> ToMainThreadAsync()
        {
            var tcs = new TaskCompletionSource<bool>();
            var workQueue = GetSubsystem<WorkQueue>();
            workQueue.PostTaskForMainThread((threadId, queue) => tcs.TrySetResult(true));
            return tcs.Task.ConfigureAwait(false);
        }

        public string GetUrhoTypeName(StringHash stringHash)
        {
            var ret = Urho3DPINVOKE.Context_GetTypeName(swigCPtr, stringHash.Hash);
            if (Urho3DPINVOKE.SWIGPendingException.Pending) throw Urho3DPINVOKE.SWIGPendingException.Retrieve();
            return ret;
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
