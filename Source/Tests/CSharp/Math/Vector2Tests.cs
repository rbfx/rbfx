using Xunit;

namespace Urho3DNet.Tests
{
    public class Vector2Tests
    {
        [Fact]
        public void Conversion()
        {
            Vector2 value = new Vector2(1,2);
            Assert.Equal(value.ToVector3(), new Vector3(1, 2, 0));
            Assert.Equal(value.ToIntVector2(), new IntVector2(1, 2));
            Assert.Equal(value.ToIntVector3(), new IntVector3(1, 2, 0));
        }

        [Fact]
        public void ParseToString()
        {
            Vector2 value = new Vector2(1.1f, 2.2f);

            var res = Vector2.Parse(value.ToString());

            Assert.Equal(value, res);
        }
    }
}
