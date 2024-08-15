using System;

namespace ImGuiNet
{

    [System.Security.SuppressUnmanagedCodeSecurity]
    public unsafe partial class ImGui
    {
        [global::System.Runtime.InteropServices.DllImport(global::ImGuiNet.ImGuiPINVOKE.DllImportModule, EntryPoint = "CSharp_ImGuiNet_InputText")]
        private static extern bool ImGui_InputText(string label, byte* buffer, int bufferSize, int flags);

        public static bool InputText(string label, ref Span<byte> buffer, ImGuiInputTextFlags flags = 0)
        {
            if (buffer.IsEmpty)
                throw new ArgumentException("Buffer cannot be empty.", nameof(buffer));

            fixed (byte* bufferPtr = buffer)
            {
                return ImGui_InputText(label, bufferPtr, buffer.Length, (int)flags);
            }
        }
    }
}
