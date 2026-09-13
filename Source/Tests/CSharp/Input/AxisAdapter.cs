// Copyright (c) 2023-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

using Xunit;

namespace Urho3DNet.Tests
{
    public class AxisAdapterTests
    {
        [Fact]
        public void AxisAdapter_SimpleTransform()
        {
            using AxisAdapter adapter = new AxisAdapter();

            Assert.Equal(1.0f, adapter.Transform(1.0f), 1e-6f);
            Assert.Equal(-1.0f, adapter.Transform(-1.0f), 1e-6f);

            adapter.IsInverted = true;

            Assert.Equal(-1.0f, adapter.Transform(1.0f), 1e-6f);
            Assert.Equal(1.0f, adapter.Transform(-1.0f), 1e-6f);
        }
    }
}
