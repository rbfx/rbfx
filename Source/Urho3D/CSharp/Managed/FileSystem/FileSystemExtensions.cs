// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
