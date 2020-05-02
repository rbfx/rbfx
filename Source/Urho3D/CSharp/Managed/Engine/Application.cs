using System;
using System.Runtime.InteropServices;

namespace Urho3DNet
{
    public partial class Application
    {
        public delegate IntPtr UserApplicationFactory(HandleRef context);
        public static Application CreateApplicationFromFactory(Context context, UserApplicationFactory factory)
        {
            return Application.wrap(factory(Context.getCPtr(context)), true);
        }
    }
}
