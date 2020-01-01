//
// Copyright (c) 2017-2020 the rbfx project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

using System.IO;

namespace Urho3DNet
{
    public static class FileSystemExtensions
    {
        public static void CopyAll(this DirectoryInfo source, DirectoryInfo destination)
        {
            if (!source.Exists)
                return;

            if (!destination.Exists)
                destination = Directory.CreateDirectory(destination.FullName);

            foreach (var fileInfo in source.GetFiles())
                System.IO.File.Copy(fileInfo.FullName, Path.Combine(destination.FullName, fileInfo.Name), true);

            foreach (var directoryInfo in source.GetDirectories())
                CopyAll(directoryInfo, new DirectoryInfo(Path.Combine(destination.FullName, directoryInfo.Name)));
        }

        public static void DeleteAll(this DirectoryInfo target)
        {
            if (!target.Exists)
                return;

            foreach (var fileInfo in target.GetFiles())
            {
                fileInfo.Delete();
            }

            foreach (var directoryInfo in target.GetDirectories())
            {
                directoryInfo.DeleteAll();
            }

            target.Delete();
        }
    }
}
