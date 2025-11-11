using System;
using System.Diagnostics;
using System.Runtime.InteropServices;

namespace Urho3DNet
{
#if __IOS__
    [global::ObjCRuntime.MonoNativeFunctionWrapper]
#endif
    internal delegate int MainFunctionCallbackDelegate();

    /// <summary>
    /// Attribute to mark the main function of the application. Only relevant for android applications.
    /// Marked function should match MainFunctionCallbackDelegate signature and be static.
    /// </summary>
    [AttributeUsage(AttributeTargets.Method, Inherited = false, AllowMultiple = false)]
    internal class SDLMainAttribute : System.Attribute
    {
        private static MainFunctionCallbackDelegate _mainFunctionCallback;

        [DllImport(global::Urho3DNet.Urho3DPINVOKE.DllImportModule, EntryPoint = "Urho3D_SetMainFunction")]
        private static extern void Urho3D_SetMainFunction(IntPtr callback);

        public static void SetMainFunction(MainFunctionCallbackDelegate callback)
        {
            Debug.Assert(_mainFunctionCallback == null);
            _mainFunctionCallback = callback;
            IntPtr callbackPtr = Marshal.GetFunctionPointerForDelegate(_mainFunctionCallback);
            Urho3D_SetMainFunction(callbackPtr);
        }
    }
}
