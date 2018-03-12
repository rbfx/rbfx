using System;
using System.Collections.Generic;
using System.Reflection;
using System.Runtime.InteropServices;

namespace Urho3D
{
    public partial class Context
    {
        private readonly Dictionary<uint, Type> factoryTypes_ = new Dictionary<uint, Type>();

        public void RegisterFactory<T>(string category="") where T: Object
        {
            var type = typeof(T);
            factoryTypes_[StringHash.Calculate(type.Name)] = type;

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
            if (!factoryTypes_.TryGetValue(managedType, out type))
                return IntPtr.Zero;
            var managed = (Object)Activator.CreateInstance(type, BindingFlags.Public | BindingFlags.Instance,
                null, new object[] { this }, null);
            return managed.instance_;
        }

        [DllImport("Urho3DCSharp", CallingConvention = CallingConvention.Cdecl)]
        internal static extern void Urho3D_Context_RegisterFactory(IntPtr context,
            [param: MarshalAs(UnmanagedType.LPUTF8Str)]
            string typeName, uint baseType, [param: MarshalAs(UnmanagedType.LPUTF8Str)]
            string category);
    }
}
