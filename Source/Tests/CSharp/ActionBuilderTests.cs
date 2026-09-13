// Copyright (c) 2022-2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
