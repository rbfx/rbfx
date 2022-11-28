using System;

namespace Urho3DNet.Tests
{
    public class SimpleHeadlessApplication : Application
    {
        private readonly Action<SimpleHeadlessApplication> _callback;

        public SimpleHeadlessApplication(Context context, Action<SimpleHeadlessApplication> callback) : base(context)
        {
            _callback = callback;
        }

        public override void Setup()
        {
            base.Setup();

            EngineParameters[Urho3D.EpHeadless] = true;
        }

        public override void Start()
        {
            base.Start();

            _callback(this);

            Context.Engine.Exit();
        }
    }
}
