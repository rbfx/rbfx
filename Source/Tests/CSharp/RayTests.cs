using Xunit;

namespace Urho3DNet.Tests
{
    public class RayTests
    {
        [Fact]
        public void HitDistance()
        {
            var ray = new Ray(new Vector3(0, 2, 0), new Vector3(0, -1, 0));
            var plane = new Plane(new Vector3(0, 1, 0), new Vector3(0, -1, 0));
            var distance = ray.HitDistance(plane);
            Assert.Equal(3.0f, distance, 1e-6f);
        }
    }
}
