using System;
using System.Text;
using System.Runtime.InteropServices;

namespace Urho3D
{
    internal static class MarshalTools
    {
        [DllImport("Urho3DCSharp", CallingConvention = CallingConvention.Cdecl)]
        internal static extern void Urho3D__Free(IntPtr ptr);

        public static string GetUtf8StringFree(IntPtr ptr)
        {
            var len = 0;
            while (Marshal.ReadByte(ptr, len) != 0)
                len++;
            var buffer = new byte[len];
            Marshal.Copy(ptr, buffer, 0, buffer.Length);
            string result = Encoding.UTF8.GetString(buffer);
            Urho3D__Free(ptr);
            return result;
        }
    }
}
