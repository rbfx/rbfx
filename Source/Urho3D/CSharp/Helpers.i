// Various marshalling helpers

%ignore SafeArray;
%inline %{
    struct SafeArray
    {
        void* data;
        int length;
    };
%}

%pragma(csharp) modulecode=%{
    [global::System.Runtime.InteropServices.StructLayout(global::System.Runtime.InteropServices.LayoutKind.Sequential)]
    internal struct SafeArray
    {
        public global::System.IntPtr data;
        public int length;

        public SafeArray(global::System.IntPtr d, int l)
        {
            data = d;
            length = l;
        }
    }

    public static int strlen(global::System.IntPtr s)
    {
        int len = 0;
        while (global::System.Runtime.InteropServices.Marshal.ReadByte(s, len) != 0)
            len++;
        return len;
    }
%}

%free(void);
