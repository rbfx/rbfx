#!/usr/bin/env python3
#
# Copyright (c) 2025-2025 the rbfx project.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

import os
import sys
import struct
import lz4.block
from typing import List, Dict, Tuple


COMPRESSED_BLOCK_SIZE = 32768

IGNORE_EXTENSIONS = ['.bak', '.rule']


class FileEntry:
    """Represents a file entry in the package."""
    def __init__(self, name: str, offset: int = 0, size: int = 0, checksum: int = 0):
        self.name = name
        self.offset = offset
        self.size = size
        self.checksum = checksum


def sdbm_hash(hash_val: int, byte_val: int) -> int:
    """SDBM hash function."""
    return (byte_val + (hash_val << 6) + (hash_val << 16) - hash_val) & 0xFFFFFFFF


def calculate_checksum(data: bytes, initial: int = 0) -> int:
    """Calculate SDBM checksum for data."""
    checksum = initial
    for byte in data:
        checksum = sdbm_hash(checksum, byte)
    return checksum


def write_string(f, s: str):
    """Write a null-terminated string."""
    encoded = s.encode('utf-8')
    f.write(encoded)
    f.write(b'\0')  # Write null terminator


def read_string(f) -> str:
    """Read a null-terminated string."""
    chars = []
    while True:
        c = f.read(1)
        if not c or c == b'\0':
            break
        chars.append(c)
    return b''.join(chars).decode('utf-8')


def write_uint(f, value: int):
    """Write an unsigned 32-bit integer."""
    f.write(struct.pack('<I', value))


def read_uint(f) -> int:
    """Read an unsigned 32-bit integer."""
    return struct.unpack('<I', f.read(4))[0]


def write_ushort(f, value: int):
    """Write an unsigned 16-bit integer."""
    f.write(struct.pack('<H', value))


def read_ushort(f) -> int:
    """Read an unsigned 16-bit integer."""
    return struct.unpack('<H', f.read(2))[0]


def write_file_id(f, file_id: str):
    """Write a 4-character file ID."""
    f.write(file_id.encode('ascii'))


def read_file_id(f) -> str:
    """Read a 4-character file ID."""
    return f.read(4).decode('ascii')


def add_trailing_slash(path: str) -> str:
    """Add trailing slash to path if not present."""
    return path if path.endswith('/') else path + '/'


def scan_dir_recursive(directory: str) -> List[str]:
    """Recursively scan directory for files."""
    files = []
    for root, dirs, filenames in os.walk(directory):
        files = [f for f in files if not f[0] == '.']
        dirs[:] = [d for d in dirs if not d[0] == '.']

        for filename in filenames:
            full_path = os.path.join(root, filename)
            rel_path = os.path.relpath(full_path, directory)
            # Convert Windows path separators to forward slashes
            rel_path = rel_path.replace(os.sep, '/')
            files.append(rel_path)
    return sorted(files)


def should_ignore_file(filename: str) -> bool:
    """Check if file should be ignored based on extension."""
    ext = os.path.splitext(filename)[1].lower()
    return ext in IGNORE_EXTENSIONS


def process_file(filename: str, root_dir: str) -> FileEntry:
    """Process a file and create its entry."""
    full_path = os.path.join(root_dir, filename)
    size = os.path.getsize(full_path)
    return FileEntry(filename, 0, size, 0)


def write_header(f, num_files: int, checksum: int, compressed: bool):
    """Write package file header."""
    if compressed:
        write_file_id(f, 'ULZ4')
    else:
        write_file_id(f, 'UPAK')
    write_uint(f, num_files)
    write_uint(f, checksum)


def write_package_file(package_name: str, root_dir: str, entries: List[FileEntry],
                       base_path: str, compress: bool, quiet: bool, block_size: int):
    """Write the package file."""
    if not quiet:
        print("Writing package")

    checksum = 0

    with open(package_name, 'wb') as dest:
        # Write header with placeholder checksum
        write_header(dest, len(entries), 0, compress)

        # Write file entries (offsets are placeholders)
        for entry in entries:
            write_string(dest, base_path + entry.name)
            write_uint(dest, entry.offset)
            write_uint(dest, entry.size)
            write_uint(dest, entry.checksum)

        total_data_size = 0

        # Write file data and calculate checksums
        for entry in entries:
            entry.offset = dest.tell()
            file_path = os.path.join(root_dir, entry.name)

            with open(file_path, 'rb') as src:
                data = src.read()

            data_size = len(data)
            total_data_size += data_size

            # Calculate checksums
            entry.checksum = calculate_checksum(data, 0)
            checksum = calculate_checksum(data, checksum)

            if not compress:
                if not quiet:
                    print(f"{entry.name} size {data_size}")
                dest.write(data)
            else:
                # Compress in blocks
                pos = 0
                compressed_start = dest.tell()

                while pos < data_size:
                    unpacked_size = min(block_size, data_size - pos)
                    block = data[pos:pos + unpacked_size]

                    # Compress block using LZ4 HC with level 9 (matches C++ default)
                    compressed = lz4.block.compress(block, mode='high_compression',
                                                   compression=9, store_size=False)
                    packed_size = len(compressed)

                    write_ushort(dest, unpacked_size)
                    write_ushort(dest, packed_size)
                    dest.write(compressed)

                    pos += unpacked_size

                if not quiet:
                    total_packed_bytes = dest.tell() - compressed_start
                    ratio = data_size / total_packed_bytes if total_packed_bytes > 0 else 0
                    print(f"{entry.name}\tin: {data_size}\tout: {total_packed_bytes}\tratio: {ratio:.6f}")

        # Write package size at the end
        current_size = dest.tell()
        write_uint(dest, current_size + 4)

        # Rewrite header with correct values
        dest.seek(0)
        write_header(dest, len(entries), checksum, compress)

        # Rewrite entries with correct offsets and checksums
        for entry in entries:
            write_string(dest, base_path + entry.name)
            write_uint(dest, entry.offset)
            write_uint(dest, entry.size)
            write_uint(dest, entry.checksum)

    if not quiet:
        final_size = os.path.getsize(package_name)
        print(f"Number of files: {len(entries)}")
        print(f"File data size: {total_data_size}")
        print(f"Package size: {final_size}")
        print(f"Checksum: {checksum}")
        print(f"Compressed: {'yes' if compress else 'no'}")


class PackageFile:
    """Read and parse package files."""
    def __init__(self, filename: str):
        self.filename = filename
        self.entries: Dict[str, Tuple[int, int, int]] = {}  # name -> (offset, size, checksum)
        self.num_files = 0
        self.checksum = 0
        self.compressed = False
        self.total_size = 0
        self.total_data_size = 0

        self._load()

    def _load(self):
        """Load package file."""
        with open(self.filename, 'rb') as f:
            # Read header
            file_id = read_file_id(f)
            if file_id == 'ULZ4':
                self.compressed = True
            elif file_id != 'UPAK':
                raise ValueError(f"Invalid package file ID: {file_id}")

            self.num_files = read_uint(f)
            self.checksum = read_uint(f)

            # Read entries
            for _ in range(self.num_files):
                name = read_string(f)
                offset = read_uint(f)
                size = read_uint(f)
                checksum = read_uint(f)
                self.entries[name] = (offset, size, checksum)
                self.total_data_size += size

            # Get total size
            self.total_size = os.path.getsize(self.filename)

    def get_entries_sorted(self) -> List[Tuple[str, int, int, int]]:
        """Get entries sorted by offset."""
        entries_list = [(name, offset, size, checksum)
                       for name, (offset, size, checksum) in self.entries.items()]
        return sorted(entries_list, key=lambda x: x[1])


def output_package_info(package_name: str, mode: str):
    """Output package information."""
    pkg = PackageFile(package_name)

    if mode == 'i':
        print(f"Number of files: {pkg.num_files}")
        print(f"File data size: {pkg.total_data_size}")
        print(f"Package size: {pkg.total_size}")
        print(f"Checksum: {pkg.checksum}")
        print(f"Compressed: {'yes' if pkg.compressed else 'no'}")

    elif mode == 'l' or mode == 'L':
        if mode == 'L' and not pkg.compressed:
            print("Error: -L is applicable for compressed package file only", file=sys.stderr)
            sys.exit(1)

        entries = pkg.get_entries_sorted()
        for i, (name, offset, size, checksum) in enumerate(entries):
            if mode == 'L':
                # Calculate compressed size
                if i + 1 < len(entries):
                    next_offset = entries[i + 1][1]
                else:
                    next_offset = pkg.total_size - 4  # Subtract the final size uint

                compressed_size = next_offset - offset
                ratio = size / compressed_size if compressed_size > 0 else 0
                print(f"{name}\tin: {size}\tout: {compressed_size}\tratio: {ratio:.6f}")
            else:
                print(name)


def main():
    """Main entry point."""
    if len(sys.argv) < 3:
        print("Usage: PackageTool <directory to process> <package name> [basepath] [options]")
        print()
        print("Options:")
        print("-c      Enable package file LZ4 compression")
        print("-q      Enable quiet mode")
        print()
        print("Basepath is an optional prefix that will be added to the file entries.")
        print()
        print("Alternative output usage: PackageTool <output option> <package name>")
        print("Output option:")
        print("-i      Output package file information")
        print("-l      Output file names (including their paths) contained in the package")
        print("-L      Similar to -l but also output compression ratio (compressed package file only)")
        sys.exit(1)

    dir_name = sys.argv[1]
    package_name = sys.argv[2]

    # Check if this is output mode
    is_output_mode = len(dir_name) == 2 and dir_name[0] == '-'

    if is_output_mode:
        output_package_info(package_name, dir_name[1])
        return

    # Parse options
    base_path = ''
    compress = False
    quiet = False

    for i in range(3, len(sys.argv)):
        arg = sys.argv[i]
        if arg[0] != '-':
            base_path = add_trailing_slash(arg)
        else:
            if len(arg) > 1:
                if arg[1] == 'c':
                    compress = True
                elif arg[1] == 'q':
                    quiet = True
                else:
                    print(f"Error: Unrecognized option {arg}", file=sys.stderr)
                    sys.exit(1)

    if not quiet:
        print(f"Scanning directory {dir_name} for files")

    # Get file list recursively
    file_names = scan_dir_recursive(dir_name)

    # Filter out ignored extensions
    file_names = [f for f in file_names if not should_ignore_file(f)]

    if not file_names:
        print("Error: No files found", file=sys.stderr)
        sys.exit(1)

    # Check if package is up to date
    if os.path.exists(package_name):
        package_time = os.path.getmtime(package_name)
        try:
            pkg = PackageFile(package_name)
            if pkg.num_files == len(file_names):
                files_out_of_date = False
                for filename in file_names:
                    file_path = os.path.join(dir_name, filename)
                    if os.path.getmtime(file_path) > package_time:
                        files_out_of_date = True
                        break

                if not files_out_of_date:
                    print(f"Package {package_name} is up to date.")
                    return
        except:
            # If we can't read the package, rebuild it
            pass

    # Process files
    entries = [process_file(filename, dir_name) for filename in file_names]

    # Write package
    write_package_file(package_name, dir_name, entries, base_path, compress,
                      quiet, COMPRESSED_BLOCK_SIZE)


if __name__ == '__main__':
    main()
