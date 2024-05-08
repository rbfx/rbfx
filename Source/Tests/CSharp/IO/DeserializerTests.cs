// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

using System.IO;
using System.Threading.Tasks;
using Xunit;

namespace Urho3DNet.Tests
{
    public class DeserializerTests
    {
        [Fact]
        public async Task ReadStringData()
        {
            var context = RbfxTestFramework.Context;
            await context.ToMainThreadAsync();

            var path = Path.GetTempFileName();

            System.IO.File.WriteAllText(path, "Bla");
            try
            {
                var fileIdentifier = new FileIdentifier("file", path);
                var file = context.VirtualFileSystem.OpenFile(fileIdentifier, FileMode.FileRead);

                using var binaryFile = new BinaryFile(context);
                binaryFile.BeginLoad(file);
                var txt = binaryFile.Text;

                file.Close();

                Assert.Equal("Bla", txt);
            }
            finally
            {
                System.IO.File.Delete(path);
            }
        }
    }
}
