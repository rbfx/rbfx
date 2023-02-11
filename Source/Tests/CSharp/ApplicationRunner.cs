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
using System.Threading;
using System.Threading.Tasks;
using Xunit.Abstractions;
using Xunit.Sdk;

[assembly: Xunit.TestFramework("Urho3DNet.Tests.ApplicationRunner", "Urho3DNet.Tests")]

namespace Urho3DNet.Tests
{
    public class ApplicationRunner : XunitTestFramework, IDisposable
    {
        private static ApplicationRunner _instance;
        private readonly Thread _workerThread;
        private Context _context;
        private SimpleHeadlessApplication _application;
        private Engine _engine;
        private TaskCompletionSource<bool> _initializationTask;

        public ApplicationRunner(IMessageSink messageSink)
            : base(messageSink)
        {
            _instance = this;
            _initializationTask = new TaskCompletionSource<bool>();
            _workerThread = new Thread(WorkerThread);
            _workerThread.Start();
        }

        private void WorkerThread()
        {
            try
            {
                using (_context = new Context())
                {
                    _engine = _context.Engine;
                    using (_application = new SimpleHeadlessApplication(_context))
                    {
                        _initializationTask.SetResult(true);
                        _application.Run();
                    }

                    _application = null;
                }

                _context = null;
            }
            finally
            {
                _initializationTask.TrySetCanceled();
            }
        }

        public new void Dispose()
        {
            if (_application != null)
            {
                _ = _application.RunAsync(_ => _.Context.Engine.Exit()).Result;
            }
            _workerThread.Join();
            base.Dispose();
            _instance = null;
        }

        public static async Task RunAsync(Action<Application> action)
        {
            await _instance._initializationTask.Task;
            await _instance._application.RunAsync(action);
        }
    }
}
