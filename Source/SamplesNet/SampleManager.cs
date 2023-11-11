//
// Copyright (c) 2023-2023 the rbfx project.
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

namespace Urho3DNet
{
    public class SampleManager : Application
    {
        /// <summary>
        ///     Safe pointer to debug HUD.
        /// </summary>
        private SharedPtr<DebugHud> _debugHud;


        public SampleManager(Context context) : base(context)
        {
        }

        /// <summary>
        ///     Setup application.
        ///     This method is executed before most of the engine system initialized.
        /// </summary>
        public override void Setup()
        {
            // Set up engine parameters
            EngineParameters[Urho3D.EpFullScreen] = false;
            EngineParameters[Urho3D.EpWindowResizable] = true;
            EngineParameters[Urho3D.EpWindowTitle] = "Samples";
            EngineParameters[Urho3D.EpApplicationName] = "Built-in Samples";
            EngineParameters[Urho3D.EpFrameLimiter] = true;
        }

        /// <summary>
        ///     Start application.
        /// </summary>
        public override void Start()
        {
#if DEBUG
            // Setup Debug HUD when building in Debug configuration.
            _debugHud = Context.Engine.CreateDebugHud();
            _debugHud.Ptr.Mode = DebugHudMode.DebughudShowAll;
#endif
        }

        public override void Stop()
        {
            _debugHud?.Dispose();
            base.Stop();
        }
    }
}
