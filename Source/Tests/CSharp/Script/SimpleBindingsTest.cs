using Newtonsoft.Json.Linq;
using System.Threading.Tasks;
using Xunit;
using Xunit.Abstractions;

namespace Urho3DNet.Tests
{
    public class SimpleBindingsTest
    {
        [Fact]
        public async Task BasicBindings()
        {
            Assert.Equal(TestBindings.testBindingsFunc(), 42);
        }
    }
}
