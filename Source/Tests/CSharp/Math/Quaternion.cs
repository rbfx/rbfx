// Copyright (c) 2023-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
