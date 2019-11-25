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

        public bool SetData(short[] indexData)
        {
            //// Initialize unmanaged memory to hold the array.
            int indexDatasize = Marshal.SizeOf(indexData[0]) * indexData.Length;
            IntPtr indexDatapnt = Marshal.AllocHGlobal(indexDatasize);
            return SetData(indexDatapnt);
        }
    }
}
