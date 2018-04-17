using System;

namespace Urho3D
{
    public partial class Application
    {
        // This method may be overriden in partial class in order to attach extra logic to object constructor
        internal override void SetupInstance(IntPtr instance, bool ownsInstance)
        {
            // Set up this instance
            PerformInstanceSetup(instance, ownsInstance);

            // Initialize inheritable classes
            Context.Urho3DRegisterWrapperFactories(Context.NativeInstance);
        }
    }
}
