// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

using Xunit;

namespace Urho3DNet.Tests
{
    public class Matrix2Tests
    {
        [Fact]
        public void ParseToString()
        {
            Matrix2 value = new Matrix2(1.1f, 2.2f, 3.3f, 4.4f);

            var res = Matrix2.Parse(value.ToString());

            Assert.Equal(value, res);
        }
    }
}
