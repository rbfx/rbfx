using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace Urho3DNet
{
    public partial class VertexBuffer
    {
        public bool SetSize(int vertexCount, VertexMask mask, bool isDynamic)
        {
            return SetSize((uint)vertexCount, (uint)mask, isDynamic);
        }

        public bool SetData(float[] vertexData)
        {
            //// Initialize unmanaged memory to hold the array.
            int vertexDatasize = Marshal.SizeOf(vertexData[0]) * vertexData.Length;
            IntPtr vertexDatapnt = Marshal.AllocHGlobal(vertexDatasize);
            return SetData(vertexDatapnt);
        } 
    }
}
