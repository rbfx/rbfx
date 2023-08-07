//
// Copyright (c) 2023-2023 the rbfx project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

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
