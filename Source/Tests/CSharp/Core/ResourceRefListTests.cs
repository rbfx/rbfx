// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

using System.Threading.Tasks;
using Xunit;

namespace Urho3DNet.Tests
{
    public class ResourceRefListTests
    {
        [Fact]
        public async Task ParseAndToString()
        {
            await RbfxTestFramework.Context.ToMainThreadAsync();

            var originalString = "Material;MyMaterial1.mat;MyMaterial2.mat";
            ResourceRefList resourceRef = ResourceRefList.Parse(originalString);
            var resultString = resourceRef.ToString(RbfxTestFramework.Context);

            Assert.Equal(originalString, resultString);
        }

        [Fact]
        public void ParseEmptyString()
        {
            ResourceRefList resourceRef = ResourceRefList.Parse("");

            Assert.Null(resourceRef);
        }

        [Fact]
        public void ParseNullString()
        {
            ResourceRefList resourceRef = ResourceRefList.Parse(null);

            Assert.Null(resourceRef);
        }
    }
}
