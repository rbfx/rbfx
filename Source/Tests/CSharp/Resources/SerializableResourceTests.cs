using System.IO;
using System.Threading.Tasks;
using Xunit;

namespace Urho3DNet.Tests
{
    public class SerializableResourceTests
    {
        [Fact]
        public async Task SaveSimpleResource()
        {
            await RbfxTestFramework.Context.ToMainThreadAsync();

            var path = Path.GetTempFileName();
            try
            {

                var res = new SerializableResource(RbfxTestFramework.Context);
                var collisionShape = new Urho3DNet.CollisionShape(RbfxTestFramework.Context);
                res.Value = collisionShape;
                var fileIdentifier = new FileIdentifier("file", path);
                res.SaveFile(fileIdentifier);
                res.Value = null;
                res.LoadFile(fileIdentifier);
                var rb = res.Value as CollisionShape;
                Assert.NotNull(rb);
            }
            finally
            {
                System.IO.File.Delete(path);
            }
        }
    }
}
