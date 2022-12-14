using Xunit;

namespace Urho3DNet.Tests
{
    public class IntVector3Tests
    {
        [Fact]
        public void Conversion()
        {
            IntVector3 value = new IntVector3(1, 2, 3);
            Assert.Equal(value.ToVector2(), new Vector2(1, 2));
            Assert.Equal(value.ToIntVector2(), new IntVector2(1, 2));
            Assert.Equal(value.ToVector3(), new Vector3(1, 2, 3));
        }
    }
}
