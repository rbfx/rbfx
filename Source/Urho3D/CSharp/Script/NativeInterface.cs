using System;
using System.Runtime.InteropServices;

namespace Urho3D.CSharp
{
    public static class NativeInterface
    {
        internal static IntPtr CreateObject(IntPtr contextPtr, uint managedType)
        {
            var context = Context.GetManagedInstance(contextPtr, true);
            return context.CreateObject(managedType);
        }

        internal static void Dispose()
        {
            InstanceCache.Dispose();
        }
    }
}
