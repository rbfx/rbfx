namespace Urho3DNet
{
    public partial class ResourceCache
    {
        public T GetResource<T>(string name, bool sendEventOnFailure = true) where T: Resource
        {
            return (T) GetResource(typeof(T).Name, name, sendEventOnFailure);
        }
    }
}
