using System;
using System.Diagnostics;
using System.Reflection;
using Urho3D.CSharp;


namespace Urho3D
{

public partial class Node
{
    public T CreateComponent<T>(CreateMode mode = CreateMode.REPLICATED, uint id = 0) where T : Component
    {
        var componentInstance = Urho3D__Node__CreateComponent_Urho3D__StringHash_Urho3D__CreateMode_unsigned_int_(NativeInstance, StringHash.Calculate(typeof(T).Name), mode, id);
        if (componentInstance == IntPtr.Zero)
            return default(T);
        return InstanceCache.GetOrAdd(componentInstance, ptr => (T) Activator.CreateInstance(typeof(T),
            BindingFlags.NonPublic | BindingFlags.Instance,
            null, new object[] {ptr, false}, null));
    }

    public T GetComponent<T>(bool recursive = false) where T: Component
    {
        var componentInstance = Urho3D__Node__GetComponent_Urho3D__StringHash_bool__const(NativeInstance, StringHash.Calculate(typeof(T).Name), recursive);
        if (componentInstance == IntPtr.Zero)
            return default(T);
        return InstanceCache.GetOrAdd(componentInstance, ptr => (T)Activator.CreateInstance(typeof(T),
            BindingFlags.NonPublic | BindingFlags.Instance, null, new object[] { ptr, false }, null));
    }
}

}
