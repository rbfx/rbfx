using System;
using System.Diagnostics;
using Xunit.Abstractions;

namespace Urho3DNet.Tests
{
    /// <summary>
    /// Test application.
    /// </summary>
    public partial class SimpleHeadlessApplication : Application
    {
        public SimpleHeadlessApplication(Context context) : base(context)
        {
        }

        /// <summary>
        /// Setup headless application.
        /// </summary>
        public override void Setup()
        {
            base.Setup();
            EngineParameters[Urho3D.EpHeadless] = true;
        }

        /// <summary>
        /// Start headless application.
        /// </summary>
        public override void Start()
        {
            base.Start();
            SubscribeToEvent(E.LogMessage, OnLogMessage);
        }

        /// <summary>
        /// Current test output.
        /// </summary>
        public ITestOutputHelper? TestOutput { get; set; }

        /// <summary>
        /// Handle log messages.
        /// </summary>
        /// <param name="args">Log message arguments.</param>
        /// <exception cref="Exception"></exception>
        private void OnLogMessage(VariantMap args)
        {
            try
            {
                var helper = TestOutput;
                var logLevel = (LogLevel)args[E.LogMessage.Level].Int;
                var message = args[E.LogMessage.Message].String;
                if (helper != null)
                {
                    helper.WriteLine($"{logLevel}: {message}");
                }
                else
                {
                    Trace.WriteLine($"{logLevel}: {message}");
                }
            }
            catch (NullReferenceException)
            {
                //TestOutput may be disposed internally.
            }
        }
    }
}
