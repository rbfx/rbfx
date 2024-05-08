using Xunit;

namespace Urho3DNet.Tests
{
    public class IntVector2Tests
    {
        [Fact]
        public void Conversion()
        {
            IntVector2 value = new IntVector2(1, 2);
            Assert.Equal(value.ToVector3(), new Vector3(1, 2, 0));
            Assert.Equal(value.ToVector2(), new Vector2(1, 2));
            Assert.Equal(value.ToIntVector3(), new IntVector3(1, 2, 0));
        }

        [Fact]
        public void ParseToString()
        {
            IntVector2 value = new IntVector2(1, 2);

            var res = IntVector2.Parse(value.ToString());

            Assert.Equal(value, res);
        }
    }
}
