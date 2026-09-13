// Copyright (c) 2026-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

using System.Threading.Tasks;
using Xunit;
using Xunit.Abstractions;

namespace Urho3DNet.Tests
{
    public class LogOutputTests
    {
        private readonly ITestOutputHelper _output;

        public LogOutputTests(ITestOutputHelper output)
        {
            _output = output;
        }

        [Fact]
        public async Task LogError()
        {
            await RbfxTestFramework.ToMainThreadAsync(_output);

            Log.Error("Error Message");
        }

        [Fact]
        public async Task LogInfo()
        {
            await RbfxTestFramework.ToMainThreadAsync(_output);

            Log.Info("Info Message");
        }
    }
}
