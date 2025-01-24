//
// Copyright (c) 2023-2024 the rbfx project.
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

using Xunit;

namespace Urho3DNet.Tests
{
    public class QuaternionTests
    {
        [Fact]
        public void QuaternionFromEuler()
        {
            Vector3 angles = new Vector3(10, -20, 30);
            Matrix3 matrix = new Matrix3(angles.Y, new Vector3(0, 1, 0))
                             * new Matrix3(angles.X, new Vector3(1, 0, 0))
                             * new Matrix3(angles.Z, new Vector3(0, 0, 1));
            Matrix3 expected = new Quaternion(angles).RotationMatrix;

            Assert.Equal(expected, matrix, Matrix3.ApproximateEqualityComparer);
        }

        [Fact]
        public void QuaternionFromGimbalLockPosition()
        {
            {
                Vector3 expected = new Vector3(90, -10, 0);
                Matrix3 expectedMatrix = new Matrix3(expected.Y, new Vector3(0, 1, 0))
                                         * new Matrix3(expected.X, new Vector3(1, 0, 0));

                Quaternion actualQuaternion =new Quaternion(expected);
                Vector3 actualAngles = actualQuaternion.EulerAngles;
                Matrix3 actualMatrix = actualQuaternion.RotationMatrix;

                Assert.Equal(expectedMatrix, actualMatrix, Matrix3.ApproximateEqualityComparer);
                Assert.Equal(expected, actualAngles, Vector3.ApproximateEqualityComparer);
            }
            {
                Vector3 expected = new Vector3(-90, -10, 0);
                Matrix3 expectedMatrix = new Matrix3(expected.Y, new Vector3(0, 1, 0)) * new Matrix3(expected.X, new Vector3(1, 0, 0));

                Quaternion actualQuaternion = new Quaternion(expected);
                Vector3 actualAngles = actualQuaternion.EulerAngles;
                Matrix3 actualMatrix = actualQuaternion.RotationMatrix;

                Assert.Equal(expectedMatrix, actualMatrix, Matrix3.ApproximateEqualityComparer);
                Assert.Equal(expected, actualAngles, Vector3.ApproximateEqualityComparer);
            }
        }

        [Fact]
        public void ParseToString()
        {
            Quaternion value = new Quaternion(1.1f, 2.2f, 3.3f, 4.4f);

            var res = Quaternion.Parse(value.ToString());

            Assert.Equal(value, res);
        }
    }
}
