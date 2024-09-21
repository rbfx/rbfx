using Xunit;

namespace Urho3DNet.Tests
{
    class CStringTestHelperOverride : Urho3DNet.CStringTestHelper
    {
        public override string testVirtualMethod(string p)
        {
            return "o: " + p;
        }
    }

    class EaStringViewTestHelperOverride : Urho3DNet.EaStringViewTestHelper
    {
        public override string testVirtualMethod(string p)
        {
            return "o: " + p;
        }

        public override string testVirtualMethodConstRef(string p)
        {
            return "o: " + p;
        }
    }

    class StdStringViewTestHelperOverride : Urho3DNet.StdStringViewTestHelper
    {
        public override string testVirtualMethod(string p)
        {
            return "o: " + p;
        }

        public override string testVirtualMethodConstRef(string p)
        {
            return "o: " + p;
        }
    }

    class EaStringTestHelperOverride : Urho3DNet.EaStringTestHelper
    {
        public override string testVirtualMethod(string p)
        {
            return "o: " + p;
        }

        public override string testVirtualMethodConstRef(string p)
        {
            return "o: " + p;
        }
    }

    class StdStringTestHelperOverride : Urho3DNet.StdStringTestHelper
    {
        public override string testVirtualMethod(string p)
        {
            return "o: " + p;
        }

        public override string testVirtualMethodConstRef(string p)
        {
            return "o: " + p;
        }
    }

    public class StringsTest
    {
        [Fact]
        public void CStringsTest()
        {
            var helper = new Urho3DNet.CStringTestHelper();
            var helperVirt = new CStringTestHelperOverride();
            Assert.Equal("p: The \u00C6olean Harp", helper.testMethod("The \u00C6olean Harp"));
            Assert.Equal("v: The \u00C6olean Harp", helper.testVirtualMethod("The \u00C6olean Harp"));
            Assert.Equal("o: The \u00C6olean Harp", helperVirt.testVirtualMethod("The \u00C6olean Harp"));
            Assert.Equal("value", helper.Value);
        }

        [Fact]
        public void EaStringViewTest()
        {
            var helper = new Urho3DNet.EaStringViewTestHelper();
            var helperVirt = new EaStringViewTestHelperOverride();
            Assert.Equal("p: The \u00C6olean Harp", helper.testMethod("The \u00C6olean Harp"));
            Assert.Equal("v: The \u00C6olean Harp", helper.testVirtualMethod("The \u00C6olean Harp"));
            Assert.Equal("p: The \u00C6olean Harp", helper.testMethodConstRef("The \u00C6olean Harp"));
            Assert.Equal("v: The \u00C6olean Harp", helper.testVirtualMethodConstRef("The \u00C6olean Harp"));
            Assert.Equal("o: The \u00C6olean Harp", helperVirt.testVirtualMethod("The \u00C6olean Harp"));
            Assert.Equal("o: The \u00C6olean Harp", helperVirt.testVirtualMethodConstRef("The \u00C6olean Harp"));
            Assert.True(helperVirt.testVirtuals());
            Assert.Equal("value", helper.Value);
            Assert.Equal("value", helper.ValueConstRef);
        }

        [Fact]
        public void StdStringViewTest()
        {
            var helper = new Urho3DNet.StdStringViewTestHelper();
            var helperVirt = new StdStringViewTestHelperOverride();
            Assert.Equal("p: The \u00C6olean Harp", helper.testMethod("The \u00C6olean Harp"));
            Assert.Equal("v: The \u00C6olean Harp", helper.testVirtualMethod("The \u00C6olean Harp"));
            Assert.Equal("p: The \u00C6olean Harp", helper.testMethodConstRef("The \u00C6olean Harp"));
            Assert.Equal("v: The \u00C6olean Harp", helper.testVirtualMethodConstRef("The \u00C6olean Harp"));
            Assert.Equal("o: The \u00C6olean Harp", helperVirt.testVirtualMethod("The \u00C6olean Harp"));
            Assert.Equal("o: The \u00C6olean Harp", helperVirt.testVirtualMethodConstRef("The \u00C6olean Harp"));
            Assert.True(helperVirt.testVirtuals());
            Assert.Equal("value", helper.Value);
            Assert.Equal("value", helper.ValueConstRef);
        }

        [Fact]
        public void EaStringTest()
        {
            var helper = new Urho3DNet.EaStringTestHelper();
            var helperVirt = new EaStringTestHelperOverride();
            Assert.Equal("p: The \u00C6olean Harp", helper.testMethod("The \u00C6olean Harp"));
            Assert.Equal("v: The \u00C6olean Harp", helper.testVirtualMethod("The \u00C6olean Harp"));
            Assert.Equal("p: The \u00C6olean Harp", helper.testMethodConstRef("The \u00C6olean Harp"));
            Assert.Equal("v: The \u00C6olean Harp", helper.testVirtualMethodConstRef("The \u00C6olean Harp"));
            Assert.Equal("o: The \u00C6olean Harp", helperVirt.testVirtualMethod("The \u00C6olean Harp"));
            Assert.Equal("o: The \u00C6olean Harp", helperVirt.testVirtualMethodConstRef("The \u00C6olean Harp"));
            Assert.True(helperVirt.testVirtuals());
            Assert.Equal("value", helper.Value);
            Assert.Equal("value", helper.ValueRef);
            Assert.Equal("value", helper.ValueConstRef);
            helper.Value = "The modified \u00C6olean Harp";
            Assert.Equal("The modified \u00C6olean Harp", helper.Value);
            Assert.Equal("The modified \u00C6olean Harp", helper.ValueRef);
            Assert.Equal("The modified \u00C6olean Harp", helper.ValueConstRef);
            helper.ValueRef = "The modified \u00C6olean Harp";
            Assert.Equal("The modified \u00C6olean Harp", helper.Value);
            Assert.Equal("The modified \u00C6olean Harp", helper.ValueRef);
            Assert.Equal("The modified \u00C6olean Harp", helper.ValueConstRef);

            string str = "The \u00C6olean Harp";
            helper.doubleStringRef(ref str);
            Assert.Equal("The \u00C6olean Harp+The \u00C6olean Harp", str);
            helper.doubleStringPtr(ref str);
            Assert.Equal("The \u00C6olean Harp+The \u00C6olean Harp+The \u00C6olean Harp+The \u00C6olean Harp", str);
        }

        [Fact]
        public void StdStringTest()
        {
            var helper = new Urho3DNet.StdStringTestHelper();
            var helperVirt = new StdStringTestHelperOverride();
            Assert.Equal("p: The \u00C6olean Harp", helper.testMethod("The \u00C6olean Harp"));
            Assert.Equal("v: The \u00C6olean Harp", helper.testVirtualMethod("The \u00C6olean Harp"));
            Assert.Equal("p: The \u00C6olean Harp", helper.testMethodConstRef("The \u00C6olean Harp"));
            Assert.Equal("v: The \u00C6olean Harp", helper.testVirtualMethodConstRef("The \u00C6olean Harp"));
            Assert.Equal("o: The \u00C6olean Harp", helperVirt.testVirtualMethod("The \u00C6olean Harp"));
            Assert.Equal("o: The \u00C6olean Harp", helperVirt.testVirtualMethodConstRef("The \u00C6olean Harp"));
            Assert.True(helperVirt.testVirtuals());
            Assert.Equal("value", helper.Value);
            Assert.Equal("value", helper.ValueRef);
            Assert.Equal("value", helper.ValueConstRef);
            helper.Value = "The modified \u00C6olean Harp";
            Assert.Equal("The modified \u00C6olean Harp", helper.Value);
            Assert.Equal("The modified \u00C6olean Harp", helper.ValueRef);
            Assert.Equal("The modified \u00C6olean Harp", helper.ValueConstRef);
            helper.ValueRef = "The modified \u00C6olean Harp";
            Assert.Equal("The modified \u00C6olean Harp", helper.Value);
            Assert.Equal("The modified \u00C6olean Harp", helper.ValueRef);
            Assert.Equal("The modified \u00C6olean Harp", helper.ValueConstRef);

            string str = "The \u00C6olean Harp";
            helper.doubleStringRef(ref str);
            Assert.Equal("The \u00C6olean Harp+The \u00C6olean Harp", str);
            helper.doubleStringPtr(ref str);
            Assert.Equal("The \u00C6olean Harp+The \u00C6olean Harp+The \u00C6olean Harp+The \u00C6olean Harp", str);
        }
    }
}
