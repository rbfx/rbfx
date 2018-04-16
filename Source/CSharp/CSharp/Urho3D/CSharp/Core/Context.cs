using System;
using System.Collections.Generic;
using System.Reflection;
using System.Runtime.InteropServices;
using CSharp;

namespace Urho3D
{
    public class RegisterFactoryAttribute : System.Attribute
    {
        public string Category { get; set; } = "";
    }

    internal class RefCountedDeleter : Object
    {
        private int _lastDeletionTickCount;
        private const int DeletionInterval = 1000;

        [DllImport(CSharp.Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void CSharp_ReleasePendingRefCounted();

        public RefCountedDeleter(Context context) : base(context)
        {
            SubscribeToEvent(CoreEvents.E_ENDFRAME, args =>
            {
                if (Environment.TickCount - _lastDeletionTickCount >= DeletionInterval)
                {
                    CSharp_ReleasePendingRefCounted();
                    _lastDeletionTickCount = Environment.TickCount;
                }
            });
        }
    }

    public partial class Context
    {
        private readonly Dictionary<uint, Type> _factoryTypes = new Dictionary<uint, Type>();
        private RefCountedDeleter _refCountedDeleter;

        // This method may be overriden in partial class in order to attach extra logic to object constructor
        internal override void SetupInstance(IntPtr instance, bool ownsInstance)
        {
            // Wrapper initialization routines
            NativeInterface.Setup();

            // Set up this instance
            PerformInstanceSetup(instance, ownsInstance);

            // Register factories marked with attributes
            foreach (var assembly in AppDomain.CurrentDomain.GetAssemblies())
            {
                foreach (var pair in assembly.GetTypesWithAttribute<RegisterFactoryAttribute>())
                    RegisterFactory(pair.Item1, pair.Item2.Category);
            }

            // Performs scheduled deletion of RefCounted
            _refCountedDeleter = new RefCountedDeleter(this);
        }

        public void RegisterFactory<T>(string category = "") where T : Object
        {
            RegisterFactory(typeof(T), category);
        }

        public void RegisterFactory(Type type, string category="")
        {
            if (!type.IsSubclassOf(typeof(Object)))
                throw new ArgumentException("Type must be sublass of Object.");

            _factoryTypes[StringHash.Calculate(type.Name)] = type;

            // Find a wrapper base type.
            var baseType = type.BaseType;
            while (baseType != null && baseType.Assembly != typeof(Context).Assembly)
                baseType = baseType.BaseType;

            if (baseType == null)
                throw new InvalidOperationException("This type can not be registered as factory.");

            Urho3D_Context_RegisterFactory(__ToPInvoke(this), type.Name, StringHash.Calculate("Wrappers::" + baseType.Name), category);
        }

        internal IntPtr CreateObject(uint managedType)
        {
            Type type;
            if (!_factoryTypes.TryGetValue(managedType, out type))
                return IntPtr.Zero;
            var managed = (Object)Activator.CreateInstance(type, BindingFlags.Public | BindingFlags.Instance,
                null, new object[] { this }, null);
            return managed.NativeInstance;
        }

        [DllImport(Config.NativeLibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern void Urho3D_Context_RegisterFactory(IntPtr context,
            [param: MarshalAs(UnmanagedType.LPUTF8Str)]
            string typeName, uint baseType, [param: MarshalAs(UnmanagedType.LPUTF8Str)]
            string category);
    }
}
