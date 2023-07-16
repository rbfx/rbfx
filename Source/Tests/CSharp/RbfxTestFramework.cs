//
// Copyright (c) 2022-2023 the rbfx project.
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
using System.Runtime.CompilerServices;
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
        /// <summary>
        /// Static reference to the RbfxTestFramework instance.
        /// The test framework going to create one.
        /// </summary>
        private static RbfxTestFramework _instance = null!;

        /// <summary>
        /// Context of the application.
        /// </summary>
        private SharedPtr<Context> _context = (Context)null!;

        /// <summary>
        /// Current WorkQueue subsystem.
        /// </summary>
        private WorkQueue _workQueue;

        /// <summary>
        /// Ticking application.
        /// </summary>
        private SharedPtr<SimpleHeadlessApplication> _app = (SimpleHeadlessApplication)null!;

        /// <summary>
        /// Application main thread.
        /// </summary>
        private System.Threading.Thread _thread;

        /// <summary>
        /// ManualResetEvent to synchronise with application creation.
        /// </summary>
        private readonly ManualResetEvent _initLock = new ManualResetEvent(false);

        /// <summary>
        /// Create and run application instance.
        /// </summary>
        /// <param name="messageSink"></param>
        public RbfxTestFramework(IMessageSink messageSink)
            : base(messageSink)
        {
            _instance = this;

            // Run the application main thread.
            _thread = new System.Threading.Thread(RunApp);
            _thread.Start();

            // Wait for application to run.
            _initLock.WaitOne();
        }

        /// <summary>
        /// Current Urho3D context.
        /// </summary>
        public static Context Context => _instance._context;

        /// <summary>
        /// Current Urho3D application.
        /// </summary>
        public static SimpleHeadlessApplication Application => _instance._app;

        /// <summary>
        /// Await this to continue execution in the application main thread.
        /// </summary>
        /// <returns>Main thread awaitable.</returns>
        public static ConfiguredTaskAwaitable<bool> ToMainThreadAsync(ITestOutputHelper testOutputHelper = null)
        {
            var tcs = new TaskCompletionSource<bool>();
            _instance._workQueue.PostTaskForMainThread((threadId, queue) =>
            {
                _instance._app.Ptr.TestOutput = testOutputHelper;
                tcs.TrySetResult(true);
            });
            return tcs.Task.ConfigureAwait(false);
        }

        /// <summary>
        /// Run application main loop in a task.
        /// </summary>
        private void RunApp()
        {
            bool isSet = false;
            try
            {
                _context = new Context();
                _app = new SimpleHeadlessApplication(_context);
                _workQueue = Context.GetSubsystem<WorkQueue>();
                // Signal about application creation.
                _initLock.Set();
                isSet = true;
                _app.Ptr.Run();
            }
            finally
            {
                // Make sure that the constructor can continue to run.
                if (!isSet)
                    _initLock.Set();
                _workQueue.Dispose();
                _app.Dispose();
                _context.Dispose();
            }
        }

        /// <summary>
        /// Dispose application.
        /// </summary>

        public new void Dispose()
        {
            _context.Ptr.Engine.Exit();
            try
            {
                _thread.Join();
            }
            catch (Exception ex)
            {
                Debug.WriteLine(ex);
            }
            base.Dispose();
        }
    }
}
