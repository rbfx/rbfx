#include <Urho3D/Math/Vector2.h>
#include <Urho3D/Math/Vector3.h>
#include <Urho3D/Math/Vector4.h>
#include <Urho3D/Math/Matrix2.h>
#include <Urho3D/Math/Matrix3.h>
#include <Urho3D/Math/Matrix4.h>
#include <Urho3D/Math/Matrix3x4.h>
#include <Urho3D/Math/Color.h>
#include <Urho3D/Math/Rect.h>
#include <Urho3D/Math/Quaternion.h>
#include <Urho3D/Math/BoundingBox.h>
#include <Urho3D/Math/Plane.h>
#include <type_traits>


namespace Urho3D
{

template<typename T>
struct PODTypes { };

#define ENABLE_POD_TYPE_CONVERTER(T) \
    template<> \
    struct PODTypes<T> \
    { \
        typedef POD##T podType; \
        typedef T cppType; \
    };

struct PODIntVector2
{
    /// X coordinate.
    int x_;
    /// Y coordinate.
    int y_;
};
ENABLE_POD_TYPE_CONVERTER(IntVector2);

struct PODVector2
{
    /// X coordinate.
    float x_;
    /// Y coordinate.
    float y_;
};
ENABLE_POD_TYPE_CONVERTER(Vector2);

struct PODIntVector3
{
    /// X coordinate.
    int x_;
    /// Y coordinate.
    int y_;
    /// Z coordinate.
    int z_;
};
ENABLE_POD_TYPE_CONVERTER(IntVector3);

struct PODVector3
{
    /// X coordinate.
    float x_;
    /// Y coordinate.
    float y_;
    /// Z coordinate.
    float z_;
};
ENABLE_POD_TYPE_CONVERTER(Vector3);

struct PODVector4
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
ENABLE_POD_TYPE_CONVERTER(Vector4);

struct PODQuaternion
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
ENABLE_POD_TYPE_CONVERTER(Quaternion);

struct PODColor
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
ENABLE_POD_TYPE_CONVERTER(Color);

struct PODRect
{
    /// Minimum vector.
    Vector2 min_;
    /// Maximum vector.
    Vector2 max_;
};
ENABLE_POD_TYPE_CONVERTER(Rect);

struct PODIntRect
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
ENABLE_POD_TYPE_CONVERTER(IntRect);

struct PODBoundingBox
{
    /// Minimum vector.
    Vector3 min_;
    float dummyMin_; // This is never used, but exists to pad the min_ value to four floats.
    /// Maximum vector.
    Vector3 max_;
    float dummyMax_; // This is never used, but exists to pad the max_ value to four floats.
};
ENABLE_POD_TYPE_CONVERTER(BoundingBox);

struct PODPlane
{
    /// Plane normal.
    Vector3 normal_;
    /// Plane absolute normal.
    Vector3 absNormal_;
    /// Plane constant.
    float d_;
};
ENABLE_POD_TYPE_CONVERTER(Plane);

struct PODMatrix2
{
    float m00_;
    float m01_;
    float m10_;
    float m11_;
};
ENABLE_POD_TYPE_CONVERTER(Matrix2);

struct PODMatrix3
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
ENABLE_POD_TYPE_CONVERTER(Matrix3);

struct PODMatrix3x4
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
ENABLE_POD_TYPE_CONVERTER(Matrix3x4);

struct PODMatrix4
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
ENABLE_POD_TYPE_CONVERTER(Matrix4);

}

template<typename T>
struct CSharpPODConverter
{
    using CppType=typename PODTypes<T>::cppType;
    using CType=typename PODTypes<T>::podType;

    inline static CType ToCSharp(const CppType& value)
    {
        return force_cast<CType>(value);
    }

    inline static CppType FromCSharp(const CType& value)
    {
        return force_cast<CppType>(value);
    }

    inline static CppType* FromCSharp(CType* value)
    {
        return reinterpret_cast<CppType*>(value);
    }
};
