// Copyright (c) 2026-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

using Xunit;

namespace Urho3DNet.Tests
{
    public class SimpleBindingsTest
    {
        [Fact]
        public void BasicBindings()
        {
            Assert.Equal(42, TestBindings.TestBindingsFunc());
        }
    }
}
