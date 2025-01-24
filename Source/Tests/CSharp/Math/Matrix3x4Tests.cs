// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

using Xunit;

namespace Urho3DNet.Tests
{
    public class Matrix3x4Tests
    {
        [Fact]
        public void ParseToString()
        {
            Matrix3x4 value = new Matrix3x4(1.1f, 2.2f, 3.3f, 4.4f,
                5.1f, 6.2f, 7.3f, 8.4f,
                9.1f, 12.2f, 11.3f, 12.4f);

            var res = Matrix3x4.Parse(value.ToString());

            Assert.Equal(value, res);
        }
    }
}
