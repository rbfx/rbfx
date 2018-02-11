using System;
using System.Text;
using System.Runtime.InteropServices;

namespace Urho3D
{
    internal static class MarshalTools
    {
        [DllImport("Urho3DCSharp", CallingConvention = CallingConvention.Cdecl)]
        internal static extern void Urho3D__Free(IntPtr ptr);

        public static string GetUtf8String(IntPtr ptr)
        {
            var len = 0;
            while (Marshal.ReadByte(ptr, len) != 0)
                len++;
            var buffer = new byte[len];
            Marshal.Copy(ptr, buffer, 0, buffer.Length);
            return Encoding.UTF8.GetString(buffer);
        }

        public static string GetUtf8StringFree(IntPtr ptr)
        {
            string result = GetUtf8String(ptr);
            Urho3D__Free(ptr);
            return result;
        }
    }
}
