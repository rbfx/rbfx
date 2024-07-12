// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

using System.Threading.Tasks;
using Xunit;

namespace Urho3DNet.Tests
{
    public class ObjectsTests
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
    }
}
