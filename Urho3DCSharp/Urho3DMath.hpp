namespace Urho3DMath
{

struct IntVector2
{
    /// X coordinate.
    int x_;
    /// Y coordinate.
    int y_;
};

struct Vector2
{
    /// X coordinate.
    float x_;
    /// Y coordinate.
    float y_;
};

struct IntVector3
{
    /// X coordinate.
    int x_;
    /// Y coordinate.
    int y_;
    /// Z coordinate.
    int z_;
};

struct Vector3
{
    /// X coordinate.
    float x_;
    /// Y coordinate.
    float y_;
    /// Z coordinate.
    float z_;
};

struct Vector4
{
    /// X coordinate.
    float x_;
    /// Y coordinate.
    float y_;
    /// Z coordinate.
    float z_;
    /// W coordinate.
    float w_;
};

struct Quaternion
{
    /// X coordinate.
    float x_;
    /// Y coordinate.
    float y_;
    /// Z coordinate.
    float z_;
    /// W coordinate.
    float w_;
};

struct Color
{
    /// Red value.
    float r_;
    /// Green value.
    float g_;
    /// Blue value.
    float b_;
    /// Alpha value.
    float a_;
};

struct Rect
{
    /// Minimum vector.
    Vector2 min_;
    /// Maximum vector.
    Vector2 max_;
};

struct IntRect
{
    /// Left coordinate.
    int left_;
    /// Top coordinate.
    int top_;
    /// Right coordinate.
    int right_;
    /// Bottom coordinate.
    int bottom_;
};

struct BoundingBox
{
    /// Minimum vector.
    Vector3 min_;
    float dummyMin_; // This is never used, but exists to pad the min_ value to four floats.
    /// Maximum vector.
    Vector3 max_;
    float dummyMax_; // This is never used, but exists to pad the max_ value to four floats.
};

struct Plane
{
    /// Plane normal.
    Vector3 normal_;
    /// Plane absolute normal.
    Vector3 absNormal_;
    /// Plane constant.
    float d_;
};

struct Matrix2
{
    float m00_;
    float m01_;
    float m10_;
    float m11_;
};

struct Matrix3
{
    float m00_;
    float m01_;
    float m02_;
    float m10_;
    float m11_;
    float m12_;
    float m20_;
    float m21_;
    float m22_;
};

struct Matrix3x4
{
    float m00_;
    float m01_;
    float m02_;
    float m03_;
    float m10_;
    float m11_;
    float m12_;
    float m13_;
    float m20_;
    float m21_;
    float m22_;
    float m23_;
};

struct Matrix4
{
    float m00_;
    float m01_;
    float m02_;
    float m03_;
    float m10_;
    float m11_;
    float m12_;
    float m13_;
    float m20_;
    float m21_;
    float m22_;
    float m23_;
    float m30_;
    float m31_;
    float m32_;
    float m33_;
};

}
