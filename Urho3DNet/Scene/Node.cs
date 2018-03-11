using System;
using System.Diagnostics;
using System.Reflection;
using CSharp;


namespace Urho3D
{

public partial class Node
{
    public T CreateComponent<T>(CreateMode mode = CreateMode.REPLICATED, uint id = 0) where T : Component
    {
        var componentInstance = Urho3D__Node__CreateComponent_Urho3D__StringHash_Urho3D__CreateMode_unsigned_int_(instance_, StringHash.Calculate(typeof(T).Name), mode, id);
        if (componentInstance == IntPtr.Zero)
        {
            var component = (T)Activator.CreateInstance(typeof(T), BindingFlags.Public | BindingFlags.Instance, null, new object[] { context_ }, null);
            if (mode == CreateMode.REPLICATED && !IsReplicated())
                mode = CreateMode.LOCAL;
            AddComponent(component, id, mode);
            return component;
        }
        return InstanceCache.GetOrAdd(componentInstance, ptr => (T)Activator.CreateInstance(typeof(T), BindingFlags.NonPublic | BindingFlags.Instance, null, new object[] { ptr }, null));
    }

    public T GetComponent<T>(bool recursive = false)
    {
        var componentInstance = Urho3D__Node__GetComponent_Urho3D__StringHash_bool__const(instance_, StringHash.Calculate(typeof(T).Name), recursive);
        if (componentInstance == IntPtr.Zero)
            return default(T);
        return InstanceCache.GetOrAdd(componentInstance, ptr => (T)Activator.CreateInstance(typeof(T), BindingFlags.NonPublic | BindingFlags.Instance, null, new object[] { ptr }, null));
    }
}

}
