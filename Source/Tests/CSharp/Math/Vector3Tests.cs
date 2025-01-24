using Xunit;

namespace Urho3DNet.Tests
{
    public class Vector3Tests
    {
        [Fact]
        public void Conversion()
        {
            Vector3 value = new Vector3(1, 2,3);
            Assert.Equal(value.ToVector2(), new Vector2(1, 2));
            Assert.Equal(value.ToIntVector2(), new IntVector2(1, 2));
            Assert.Equal(value.ToIntVector3(), new IntVector3(1, 2, 3));
        }

        [Fact]
        public void ParseToString()
        {
            Vector3 value = new Vector3(1.1f, 2.2f, 3.3f);

            var res = Vector3.Parse(value.ToString());

            Assert.Equal(value, res);
        }
    }
}
