//
// Copyright (c) 2017-2019 the rbfx project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

using System;
using System.Runtime.InteropServices;

namespace Urho3DNet
{
    [System.Security.SuppressUnmanagedCodeSecurity]
    public partial class Urho3D
    {
        [DllImport("Urho3D", EntryPoint="Urho3D_ParseArguments")]
        public static extern void ParseArguments(int argc,
            [MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.LPStr)]string[] argv);

        unsafe public static void GenerateTangents(float[] vertexData, int vertexBufferSize, short[] indexData, int sizeofIndex, int startIndex, int indexCount, int normalOffset, int textureOffset, int tangentOffset)
        {
            fixed (float* vertexIntPtr = vertexData)
            {
                fixed (short* indexIntPtr = indexData)
                {
                    Urho3D.GenerateTangents((IntPtr)vertexIntPtr,
                                            (uint)vertexBufferSize,
                                            (IntPtr)indexIntPtr,
                                            (uint)sizeofIndex,
                                            (uint)startIndex,
                                            (uint)indexCount,
                                            (uint)normalOffset * sizeof(float),
                                            (uint)textureOffset * sizeof(float),
                                            (uint)(tangentOffset * sizeof(float))
                                            );
                }
            }
        }

        unsafe public static void GenerateTangents(float[] vertexData, int vertexBufferSize, int[] indexData, int sizeofIndex, int startIndex, int indexCount, int normalOffset, int textureOffset, int tangentOffset)
        {
            fixed (float* vertexIntPtr = vertexData)
            {
                fixed (int* indexIntPtr = indexData)
                {
                    Urho3D.GenerateTangents((IntPtr)vertexIntPtr,
                                            (uint)vertexBufferSize,
                                            (IntPtr)indexIntPtr,
                                            (uint)sizeofIndex,
                                            (uint)startIndex,
                                            (uint)indexCount,
                                            (uint)normalOffset * sizeof(float),
                                            (uint)textureOffset * sizeof(float),
                                            (uint)(tangentOffset * sizeof(float))
                                            );
                }
            }
        }
    }
}
