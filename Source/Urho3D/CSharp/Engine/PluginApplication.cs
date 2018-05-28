

namespace Urho3D
{
    /// Base class for clearing plugins for Editor.
    public class PluginApplication : Object
    {
        /// Construct.
        public PluginApplication(Context context) : base(context) { }
        /// Called when plugin is being loaded. Register all custom components and subscribe to events here.
        public virtual void OnLoad() { }
        /// Called when plugin is being unloaded. Unregister all custom components and unsubscribe from events here.
        public virtual void OnUnload() { }
    }
}
