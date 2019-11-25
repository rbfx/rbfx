using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace Urho3DNet
{
    public partial class IndexBuffer
    { 
        public bool SetData(short[] indexData)
        {
            //// Initialize unmanaged memory to hold the array.
            int indexDatasize = Marshal.SizeOf(indexData[0]) * indexData.Length;
            IntPtr indexDatapnt = Marshal.AllocHGlobal(indexDatasize);
            return SetData(indexDatapnt);
        }
    }
}
