// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

using System.Threading.Tasks;
using Xunit;

namespace Urho3DNet.Tests
{
    public class ResourceRefTests
    {
        [Fact]
        public async Task ParseAndToString()
        {
            await RbfxTestFramework.Context.ToMainThreadAsync();

            var originalString = "Material;MyMaterial.mat";
            ResourceRef resourceRef = ResourceRef.Parse(originalString);
            var resultString = resourceRef.ToString(RbfxTestFramework.Context);

            Assert.Equal(originalString, resultString);
        }

        [Fact]
        public void ParseEmptyString()
        {
            ResourceRef resourceRef = ResourceRef.Parse("");

            Assert.Null(resourceRef);
        }

        [Fact]
        public void ParseNullString()
        {
            ResourceRef resourceRef = ResourceRef.Parse(null);

            Assert.Null(resourceRef);
        }
    }
}
