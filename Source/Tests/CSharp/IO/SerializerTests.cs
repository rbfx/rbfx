// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

using System.IO;
using System.Threading.Tasks;
using Xunit;

namespace Urho3DNet.Tests
{
    public class SerializerTests
    {
        [Fact]
        public async Task WriteStringData()
        {
            await RbfxTestFramework.Context.ToMainThreadAsync();

            var path = Path.GetTempFileName();
            try
            {
                var fileIdentifier = new FileIdentifier("file", path);
                var file = RbfxTestFramework.Context.VirtualFileSystem.OpenFile(fileIdentifier, FileMode.FileWrite);
                Urho3D.WriteStringData(file, "Bla");
                file.Close();

                var txt = System.IO.File.ReadAllText(path);

                Assert.Equal("Bla", txt);
            }
            finally
            {
                System.IO.File.Delete(path);
            }
        }
    }
}
