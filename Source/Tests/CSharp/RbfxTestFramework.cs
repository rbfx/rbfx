//
// Copyright (c) 2022-2022 the rbfx project.
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
using System.Diagnostics;
using System.Threading;
using System.Threading.Tasks;
using Xunit.Abstractions;
using Xunit.Sdk;

[assembly: Xunit.TestFramework("Urho3DNet.Tests.RbfxTestFramework", "Urho3DNet.Tests")]

namespace Urho3DNet.Tests
{
    /// <summary>
    /// Helper class to run the Urho3D Application in a background.
    /// </summary>
    public class RbfxTestFramework : XunitTestFramework, IDisposable
    {
        private static RbfxTestFramework _instance;

        private Context _context;
        private SimpleHeadlessApplication _app;
        private Task<int> _task;
        private ManualResetEvent _initLock = new ManualResetEvent(false);

        /// <summary>
        /// Create and run application instance.
        /// </summary>
        /// <param name="messageSink"></param>
        public RbfxTestFramework(IMessageSink messageSink)
            : base(messageSink)
        {
            _instance = this;
            _task = Task.Factory.StartNew(RunApp, TaskCreationOptions.LongRunning);
            _initLock.WaitOne();
        }

        public static Context Context
        {
            get
            {
                return _instance._context;
            }
        }

        /// <summary>
        /// Run application main loop in a task.
        /// </summary>
        private int RunApp()
        {
            _context = new Context();
            _app = new SimpleHeadlessApplication(_context);
            _initLock.Set();
            return _app.Run();
        }

        /// <summary>
        /// Dispose application.
        /// </summary>

        public new void Dispose()
        {
            _app.ErrorExit();
            try
            {
                int res = _task.Result;
            }
            catch (Exception ex)
            {
                Debug.WriteLine(ex);
            }
            _context?.Dispose();
            base.Dispose();
        }
    }
}
