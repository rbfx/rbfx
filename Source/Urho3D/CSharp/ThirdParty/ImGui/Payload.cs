using System;

namespace ImGui
{
    public unsafe struct Payload
    {
        public readonly NativePayload* NativePtr;
        public Payload(NativePayload* ptr)
        {
            NativePtr = ptr;
        }

        public IntPtr Data => (IntPtr)NativePtr->Data;
        public int DataSize => NativePtr->DataSize;
    }
}
