// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

using System.Threading.Tasks;
using Xunit;
using Xunit.Abstractions;

namespace Urho3DNet.Tests
{
    public partial class RmlUIComponentTests
    {
        private readonly ITestOutputHelper _output;

        public RmlUIComponentTests(ITestOutputHelper output)
        {
            _output = output;
        }

        public partial class CusomUIComponent : RmlUIComponent
        {
            private Variant _variant = new Variant(42);
            private VariantMap _variantMap = new VariantMap { { "int", 42 }, { "str", "text" } };
            private VariantList _variantList = new VariantList { 42, "text" };

            public CusomUIComponent(Context context) : base(context)
            {
            }

            protected override void OnDataModelInitialized()
            {
                BindDataModelVariant("variant", _variant);
                BindDataModelVariantMap("variant_map", _variantMap);
                BindDataModelVariantVector("variant_vector", _variantList);
                BindDataModelProperty("slider_value", GetSliderValue, SetSliderValue);
                BindDataModelProperty("counter", GetCounter, SetCounter);
                BindDataModelProperty("progress", GetProgress, SetProgress);
                BindDataModelEvent("event", OnEvent);
            }

            private void OnEvent(VariantList args)
            {
            }

            private void SetProgress(Variant args)
            {
            }

            private void GetProgress(Variant args)
            {
            }

            private void SetCounter(Variant args)
            {
            }

            private void GetCounter(Variant args)
            {
            }

            private void SetSliderValue(Variant args)
            {
            }

            private void GetSliderValue(Variant args)
            {
            }
        }

        [Fact]
        public async Task RmlUIComponent_OnDataModelInitialized()
        {
            await RbfxTestFramework.ToMainThreadAsync(_output);
            var context = RbfxTestFramework.Context;
            if (!context.IsReflected<CusomUIComponent>())
            {
                context.AddFactoryReflection(typeof(CusomUIComponent));
            }

            using (var node = new Node(context))
            {
                var component = node.CreateComponent<CusomUIComponent>();
                component.SetResource("UI/HelloRmlUI.rml");
            }
        }
    }
}

