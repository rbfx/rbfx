//
// Copyright (c) 2017-2020 the rbfx project.
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
    public partial class CrowdManager
    {
        public delegate void CrowdAgentVelocityShaderDelegate(CrowdAgent agent, float timeStep, ref Vector3 desiredVelocity, ref float desiredSpeed);
        private delegate void CrowdAgentVelocityShaderFnDelegate(IntPtr agent, float timeStep, IntPtr desiredVelocity, IntPtr desiredSpeed);
        [DllImport(global::Urho3DNet.Urho3DPINVOKE.DllImportModule, EntryPoint = "Urho3D_CrowdManager_SetVelocityShader")]
        private static extern void Urho3D_CrowdManager_SetVelocityShader(HandleRef crowdManager, IntPtr callback, IntPtr callbackHandle);

        public void SetVelocityShader(CrowdAgentVelocityShaderDelegate shader)
        {
            var eventCallback = new CrowdAgentVelocityShaderFnDelegate((agent, timeStep, desiredVelocity, desiredSpeed) =>
            {
                unsafe
                {
                    var pDesiredSpeed = (float*) desiredSpeed.ToPointer();
                    var pDesiredVelocity = (Vector3*) desiredVelocity.ToPointer();
                    shader(CrowdAgent.wrap(agent, true), timeStep, ref *pDesiredVelocity, ref *pDesiredSpeed);
                }
            });
            IntPtr handle = GCHandle.ToIntPtr(GCHandle.Alloc(eventCallback));
            IntPtr callback = Marshal.GetFunctionPointerForDelegate(eventCallback);
            Urho3D_CrowdManager_SetVelocityShader(swigCPtr, callback, handle);
        }
    }
}
