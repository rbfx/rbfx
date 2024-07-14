// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

using System;
using System.Threading.Tasks;
using Xunit;

namespace Urho3DNet.Tests
{
    public partial class ObjectsTests
    {
        [Fact]
        public async Task GetTypeName_WorksFromBaseType()
        {
            await RbfxTestFramework.Context.ToMainThreadAsync();

            Object obj = new Camera(RbfxTestFramework.Context);

            Assert.Equal("Camera", obj.GetTypeName());
        }

        [Fact]
        public async Task GetTypeHash_WorksFromBaseType()
        {
            await RbfxTestFramework.Context.ToMainThreadAsync();

            Object obj = new Camera(RbfxTestFramework.Context);

            Assert.Equal(new StringHash("Camera"), obj.GetTypeHash());
        }

        [Fact]
        public async Task IsInstanceOf_WorksFromBaseType()
        {
            await RbfxTestFramework.Context.ToMainThreadAsync();

            Object obj = new Camera(RbfxTestFramework.Context);

            Assert.True(obj.IsInstanceOf(nameof(Camera)));
            Assert.True(obj.IsInstanceOf(nameof(Object)));
            Assert.False(obj.IsInstanceOf(nameof(Viewport)));
        }

        [Fact]
        public async Task IsInstanceOf_WorksFromNativeCode()
        {
            await RbfxTestFramework.Context.ToMainThreadAsync();

            Node node = new Node(RbfxTestFramework.Context);
            var solver = node.CreateComponent<MyChildComponent>();

            var res = node.GetDerivedComponent<MyParentComponent>();
            Assert.Equal(solver, res);
        }

        [ObjectFactory]
        public partial class MyChildComponent : MyParentComponent
        {
            public MyChildComponent(IntPtr cPtr, bool cMemoryOwn) : base(cPtr, cMemoryOwn)
            {
            }

            public MyChildComponent(Context context) : base(context)
            {
            }
        }
    }

    /// <summary>
    /// Parent class deliberately left as a second class in the file to test if code generation will detect it correctly.
    /// </summary>
    [ObjectFactory]
    public partial class MyParentComponent : LogicComponent
    {
        public MyParentComponent(IntPtr cPtr, bool cMemoryOwn) : base(cPtr, cMemoryOwn)
        {
        }

        public MyParentComponent(Context context) : base(context)
        {
        }
    }
}
