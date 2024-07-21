using Xunit;

namespace Urho3DNet.Tests
{
    public class SimpleBindingsTest
    {
        [Fact]
        public void BasicBindings()
        {
            Assert.Equal(42, TestBindings.TestBindingsFunc());
        }
    }
}
