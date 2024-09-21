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
            Assert.True(obj.IsInstanceOf(nameof(Component)));
            Assert.False(obj.IsInstanceOf(nameof(Object)));
            Assert.False(obj.IsInstanceOf(nameof(Viewport)));

            //Assert.Equal(new StringHash[] { "Camera", "Component", "Serializable" }, Camera.TypeHierarchy);
        }

        [Fact]
        public async Task IsInstanceOf_WorksFromNativeCode()
        {
            await RbfxTestFramework.Context.ToMainThreadAsync();

            Node node = new Node(RbfxTestFramework.Context);
            var solver = node.CreateComponent<MyChildComponent>();

            var res = node.FindComponent<MyParentComponent>(ComponentSearchFlag.Self | ComponentSearchFlag.Derived);
            Assert.Equal(solver, res);
        }

        [Fact]
        public async Task GenericTypeClassName()
        {
            await RbfxTestFramework.Context.ToMainThreadAsync();

            Assert.Equal("TestGenericType<Int32>", TestGenericType<System.Int32>.ClassName);
            Assert.Equal("TestGenericType<Int32>", TestGenericType<System.Int32>.GetTypeNameStatic());
            Assert.Equal("TestGenericType<Int32>", (new TestGenericType<System.Int32>(RbfxTestFramework.Context)).GetTypeName());
        }

        [Fact]
        public async Task GenericTypeId()
        {
            await RbfxTestFramework.Context.ToMainThreadAsync();

            Assert.Equal(new StringHash("TestGenericType<Int32>"), TestGenericType<System.Int32>.TypeId);
            Assert.Equal(new StringHash("TestGenericType<Int32>"), TestGenericType<System.Int32>.GetTypeStatic());
            Assert.Equal("TestGenericType<Int32>", (new TestGenericType<System.Int32>(RbfxTestFramework.Context)).GetTypeHash());
        }

        [Fact]
        public async Task GenericBaseClassName()
        {
            await RbfxTestFramework.Context.ToMainThreadAsync();

            Assert.Equal("Object", TestGenericType<System.Int32>.BaseClassName);
        }

        [Fact]
        public async Task GenericIsInstanceOf()
        {
            await RbfxTestFramework.Context.ToMainThreadAsync();

            var instance = new TestGenericType<System.Int32>(RbfxTestFramework.Context);

            Assert.True(instance.IsInstanceOf(TestGenericType<System.Int32>.TypeId));
            Assert.False(instance.IsInstanceOf(Object.TypeId));
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

    /// <summary>
    /// Parent class deliberately left as a second class in the file to test if code generation will detect it correctly.
    /// </summary>
    public partial class TestGenericType<T1> : Object
    {
        public TestGenericType(IntPtr cPtr, bool cMemoryOwn) : base(cPtr, cMemoryOwn)
        {
        }

        public TestGenericType(Context context) : base(context)
        {
        }
    }
}
