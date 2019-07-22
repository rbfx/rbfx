namespace Urho3DNet
{
    public partial class RefCounted
    {
        internal void ReleaseRefSafe()
        {
            var script = Context.Instance?.GetSubsystem<Script>();
            if (script != null)
                script.ReleaseRefOnMainThread(this);
            else
                ReleaseRef();
        }
    }
}
