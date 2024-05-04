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
            var res = new SerializableResource(RbfxTestFramework.Context);
            var rigidBody = new Urho3DNet.CollisionShape(RbfxTestFramework.Context);
            res.Value = rigidBody;
            var fileIdentifier = new FileIdentifier("conf", "SerializableResource.json");
            res.SaveFile(fileIdentifier);
            res.Value = null;
            res.LoadFile(fileIdentifier);
            var rb = res.Value as CollisionShape;
            Assert.NotNull(rb);

            var path = RbfxTestFramework.Context.VirtualFileSystem.GetAbsoluteNameFromIdentifier(fileIdentifier);
            System.IO.File.Delete(path);
        }
    }
}
