// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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

    public static uint Calculate(string value)
    {
        var bytes = Encoding.UTF8.GetBytes(value);
        uint result = 2166136261u;
        foreach (var b in bytes)
            result = (result * 16777619u) ^ b;
        return result;
    }

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

    public override bool Equals(object obj)
    {
        return obj is StringHash other && Equals(other);
    }

    public override int GetHashCode()
    {
        return (int)Hash;
    }

    public static bool operator ==(StringHash left, StringHash right)
    {
        return left.Equals(right);
    }

    public static bool operator !=(StringHash left, StringHash right)
    {
        return !left.Equals(right);
    }

}

}
