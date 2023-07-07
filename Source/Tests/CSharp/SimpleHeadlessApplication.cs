using System;
using System.Collections.Generic;
using System.Threading.Tasks;

namespace Urho3DNet.Tests
{
    public class SimpleHeadlessApplication : Application
    {
        public SimpleHeadlessApplication(Context context) : base(context)
        {
        }

        public override void Setup()
        {
            base.Setup();

            EngineParameters[Urho3D.EpHeadless] = true;
        }
    }
}
