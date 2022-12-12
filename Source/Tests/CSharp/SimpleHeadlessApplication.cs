using System;
using System.Collections.Generic;
using System.Threading.Tasks;

namespace Urho3DNet.Tests
{
    public class SimpleHeadlessApplication : Application
    {
        private Queue<QueueItem> _tasks = new Queue<QueueItem>();

        public SimpleHeadlessApplication(Context context) : base(context)
        {
        }

        public override void Setup()
        {
            base.Setup();

            EngineParameters[Urho3D.EpHeadless] = true;

        }

        private void HandleUpdate(VariantMap obj)
        {
            for (;;)
            {
                QueueItem item;
                lock (_tasks)
                {
                    if (_tasks.Count == 0)
                    {
                        return;
                    }

                    item = _tasks.Dequeue();
                }

                item.Run(this);
            }
        }

        public override void Start()
        {
            base.Start();
            SubscribeToEvent(E.Update, HandleUpdate);
        }

        public async Task<bool> RunAsync(Action<Application> action)
        {
            var tcs = new QueueItem(action);
            lock (_tasks)
            {
                _tasks.Enqueue(tcs);
            }

            await tcs.RunAsync();
            return true;
        }

        class QueueItem
        {
            private readonly TaskCompletionSource<bool> _tcs;
            private readonly Action<Application> _action;

            public QueueItem(Action<Application> action)
            {
                _tcs = new TaskCompletionSource<bool>();
                _action = action;
            }

            public async Task RunAsync()
            {
                await _tcs.Task;
            }

            public void Run(Application app)
            {
                try
                {
                    _action(app);
                }
                catch (Exception ex)
                {
                    _tcs.TrySetException(ex);
                }

                _tcs.TrySetResult(true);
            }
        }
    }
}
