namespace Urho3DNet
{
    public partial class Node
    {
        public T CreateComponent<T>(CreateMode mode = CreateMode.Replicated, uint id = 0) where T: Component
        {
            return (T)CreateComponent(typeof(T).Name, mode, id);
        }
    }
}
