using Newtonsoft.Json.Linq;
using System.ComponentModel;
using System.Threading.Tasks;
using Xunit;
using Xunit.Abstractions;

namespace Urho3DNet.Tests
{
    public partial class TypeInfoTest
    {
        [Fact]
        public void WrappedTypeInfo()
        {
            var component = new LogicComponent(RbfxTestFramework.Context);

            // From C#
            Assert.Equal(nameof(LogicComponent), component.GetTypeName());
            Assert.Equal(new StringHash(nameof(LogicComponent)), component.GetTypeHash());

            // From C++
            Assert.Equal(nameof(LogicComponent), TestBindings.GetObjectTypeName(component));
            Assert.Equal(new StringHash(nameof(LogicComponent)), TestBindings.GetObjectTypeHash(component));
        }

        partial class MyResource : Resource
        {
            public MyResource(Context context) : base(context)
            {
            }
        }

        [Fact]
        public void InheritedTypeInfo()
        {
            var resource = new MyResource(RbfxTestFramework.Context);

            // From C#
            Assert.Equal(nameof(MyResource), resource.GetTypeName());
            Assert.Equal(new StringHash(nameof(MyResource)), resource.GetTypeHash());

            // From C++
            Assert.Equal(nameof(MyResource), TestBindings.GetObjectTypeName(resource));
            Assert.Equal(new StringHash(nameof(MyResource)), TestBindings.GetObjectTypeHash(resource));
        }
    }
}
