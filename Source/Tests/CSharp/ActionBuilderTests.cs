//
// Copyright (c) 2022-2022 the rbfx project.
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
using Xunit.Abstractions;

namespace Urho3DNet.Tests
{
    public class ActionBuilderTests
    {
        private readonly ITestOutputHelper _output;

        public ActionBuilderTests(ITestOutputHelper output)
        {
            _output = output;
        }

        [Fact]
        public async Task SimpleAction_MoveBy_NodePositionUpdated()
        {
            await RbfxTestFramework.ToMainThreadAsync(_output);
            var startPos = new Vector3(0, 1, 0);
            var moveBy = new Vector3(2, 0, 0);
            var actionManager = new ActionManager(RbfxTestFramework.Context);
            using (SharedPtr<Node> node = new Node(RbfxTestFramework.Context))
            {
                node.Ptr.Position = startPos;
                using (var builder = new ActionBuilder(RbfxTestFramework.Context))
                {
                    builder.MoveBy(0.1f, moveBy).Run(actionManager, node);
                }
                actionManager.Update(0.0f);
                actionManager.Update(0.1f);

                Assert.Equal(startPos + moveBy, node.Ptr.Position);
            }
        }
    }
}
