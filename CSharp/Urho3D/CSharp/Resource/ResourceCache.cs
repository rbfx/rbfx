using System;
using System.Reflection;
using CSharp;


namespace Urho3D
{

public partial class ResourceCache
{
    public T GetResource<T>(string name, bool sendEventOnFailure = true) where T : Resource
    {
        var typeHash = new StringHash(typeof(T).Name);
        var componentInstance = Urho3D__ResourceCache__GetResource_Urho3D__StringHash_Urho3D__String_const__bool_(instance_, typeHash.Hash, name, sendEventOnFailure);
        return InstanceCache.GetOrAdd(componentInstance, ptr => {
            Type type = typeof(T);
            return (T)Activator.CreateInstance(type, BindingFlags.NonPublic | BindingFlags.Public | BindingFlags.Instance, null, new object[] { ptr }, null);
        });
    }
}

}
