// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

using System;
using Urho3DNet;

namespace DemoApplication
{
    [ObjectFactory(Category = "Script Components")]
    class ScriptRotateObject : LogicComponent
    {
        public ScriptRotateObject(Context context) : base(context)
        {
            UpdateEventMask = UpdateEvent.UseUpdate;
        }

        public override void Update(float timeStep)
        {
            var d = new Quaternion(10 * timeStep, 20 * timeStep, 30 * timeStep);
            Node.Rotate(d);
        }
    }
}
