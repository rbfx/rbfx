using System;
using System.Text;


namespace Urho3D
{

public struct StringHash
{
    private uint value_;
    public uint Hash => value_;

    public StringHash(uint value)
    {
        value_ = value;
    }

    public StringHash(string value)
    {
        value_ = Calculate(value);
    }

    public static implicit operator StringHash(string value)
    {
        return new StringHash(value);
    }

    public static uint Calculate(string value, uint hash=0)
    {
        // Perform the actual hashing as case-insensitive
        var bytes = Encoding.UTF8.GetBytes(value.ToLower());
        foreach (var b in bytes)
            hash = SDBMHash(hash, b);
        return hash;
    }

    public static uint SDBMHash(uint hash, byte c) { return c + (hash << 6) + (hash << 16) - hash; }

    public override string ToString()
    {
        return String.Format("{:08X}", value_);
    }

    internal static StringHash __FromPInvoke(uint source)
    {
        return new StringHash(source);
    }

    internal static uint __ToPInvoke(StringHash source)
    {
        return source.Hash;
    }
}

}
