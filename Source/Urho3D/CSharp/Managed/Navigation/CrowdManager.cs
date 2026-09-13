// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

using System;
using System.Runtime.InteropServices;

namespace Urho3DNet
{
    public partial class CrowdManager
    {
        public delegate void CrowdAgentVelocityCallbackDelegate(CrowdAgent agent, float timeStep, ref Vector3 desiredVelocity, ref float desiredSpeed);
        private delegate void CrowdAgentVelocityCallbackFnDelegate(IntPtr agent, float timeStep, IntPtr desiredVelocity, IntPtr desiredSpeed);
        [DllImport(global::Urho3DNet.Urho3DPINVOKE.DllImportModule, EntryPoint = "Urho3D_CrowdManager_SetVelocityCallback")]
        private static extern void Urho3D_CrowdManager_SetVelocityCallback(HandleRef crowdManager, IntPtr callback, IntPtr callbackHandle);

        public void SetVelocityCallback(CrowdAgentVelocityCallbackDelegate shader)
        {
            var eventCallback = new CrowdAgentVelocityCallbackFnDelegate((agent, timeStep, desiredVelocity, desiredSpeed) =>
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
            Urho3D_CrowdManager_SetVelocityCallback(swigCPtr, callback, handle);
        }
    }
}
