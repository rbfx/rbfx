using System;
using System.Collections;
using System.Collections.Generic;
using System.Threading;
using System.Threading.Tasks;
using Xunit.Abstractions;
using Xunit.Sdk;
using static System.Net.Mime.MediaTypeNames;

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
        private TaskCompletionSource _initializationTask;

        public ApplicationRunner(IMessageSink messageSink)
            : base(messageSink)
        {
            _instance = this;
            _initializationTask = new TaskCompletionSource();
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
                        _initializationTask.SetResult();
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
