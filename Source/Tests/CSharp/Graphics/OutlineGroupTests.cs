// Copyright (c) 2023-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

using System.Threading.Tasks;
using Xunit;

namespace Urho3DNet.Tests
{
    public class OutlineTests
    {
        [Fact]
        public async Task AddAndRemoveDrawable()
        {
            await RbfxTestFramework.Context.ToMainThreadAsync();
            using var outline = SharedPtr.MakeShared<OutlineGroup>(RbfxTestFramework.Context);
            Assert.NotNull(outline.Ptr);
            using var staticModel = SharedPtr.MakeShared<StaticModel>(RbfxTestFramework.Context);
            Assert.False(outline.Ptr.HasDrawable(staticModel));
            Assert.False(outline.Ptr.RemoveDrawable(staticModel));
            Assert.True(outline.Ptr.AddDrawable(staticModel));
            Assert.True(outline.Ptr.HasDrawable(staticModel));
            Assert.False(outline.Ptr.AddDrawable(staticModel));
            Assert.True(outline.Ptr.RemoveDrawable(staticModel));
            Assert.False(outline.Ptr.HasDrawable(staticModel));
        }
    }
}
