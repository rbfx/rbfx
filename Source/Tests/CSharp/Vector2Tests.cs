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
    }
}
