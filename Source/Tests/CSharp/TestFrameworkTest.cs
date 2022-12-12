using System;
using System.Threading.Tasks;
using Xunit;

namespace Urho3DNet.Tests
{
    public class TestFrameworkTest
    {
        [Fact]
        public async Task CanThrow()
        {
            await Assert.ThrowsAsync<ArgumentException>(async () =>
                await ApplicationRunner.RunAsync(app => throw new ArgumentException()));
        }
    }
}
