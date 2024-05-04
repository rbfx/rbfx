// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

using Xunit;

namespace Urho3DNet.Tests
{
    public class IntRectTests
    {
        [Fact]
        public void ParseToString()
        {
            IntRect value = new IntRect(1, 2, 3, 4);

            var res = IntRect.Parse(value.ToString());

            Assert.Equal(value, res);
        }
    }
}
