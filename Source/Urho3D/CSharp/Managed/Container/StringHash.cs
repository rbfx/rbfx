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
using System;
using System.Text;
using System.Runtime.InteropServices;


namespace Urho3DNet
{

[Serializable]
[StructLayout(LayoutKind.Sequential)]
public struct StringHash : IEquatable<StringHash>
{
    public uint Hash { get; }

    public StringHash(uint value)
    {
        Hash = value;
    }

    public StringHash(string value)
    {
        Hash = Calculate(value);
    }

    public StringHash(Type type)
    {
        Hash = Calculate(type.Name);
    }

    public static implicit operator StringHash(uint value)
    {
        return new StringHash(value);
    }

    public static implicit operator StringHash(string value)
    {
        return new StringHash(value);
    }

    public static implicit operator StringHash(Type type)
    {
        return new StringHash(type);
    }

    public static uint Calculate(string value, uint hash=0)
    {
        // Perform the actual hashing as case-insensitive
        var bytes = Encoding.UTF8.GetBytes(value);
        foreach (var b in bytes)
            hash = SDBMHash(hash, b);
        return hash;
    }

    public static uint SDBMHash(uint hash, byte c) { return c + (hash << 6) + (hash << 16) - hash; }

    public override string ToString()
    {
        return $"{Hash:X8}";
    }

    internal static StringHash GetManagedInstance(uint source)
    {
        return new StringHash(source);
    }

    internal static uint GetNativeInstance(StringHash source)
    {
        return source.Hash;
    }

    public bool Equals(StringHash other)
    {
            return Hash == other.Hash;
    }
}

}
