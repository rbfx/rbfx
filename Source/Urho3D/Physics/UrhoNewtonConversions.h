#pragma once


class dMatrix;
class dVector;
class dQuaternion;
class dgQuaternion;

class NewtonCollision;
class NewtonWorld;

namespace Urho3D {
    class Matrix4;
    class Matrix3x4;
    class Vector2;
    class Vector3;
    class Vector4;
    class Quaternion;
    class Sphere;
    class BoundingBox;

    ///Conversion Functions From Urho To Newton
    dMatrix UrhoToNewton(const Matrix4& mat);
    dMatrix UrhoToNewton(const Matrix3x4& mat);
    dVector UrhoToNewton(const Vector3& vec4);
    dVector UrhoToNewton(const Vector3& vec3);
    dVector UrhoToNewton(const Vector2& vec2);
    dQuaternion UrhoToNewton(const Quaternion& quat);

    ///Conversion Function From Newton To Urho
    Vector3 NewtonToUrhoVec3(const dVector& vec);
    Vector4 NewtonToUrhoVec4(const dVector& vec);
    Matrix4 NewtonToUrhoMat4(const dMatrix& mat);
    Quaternion NewtonToUrhoQuat(const dQuaternion& quat);
    Quaternion NewtonToUrhoQuat(const dgQuaternion& quat);



    ///shape conversion

    ///return a newton collision from an urho shape - optionally include the translation of the shape in the collision. Remember to NewtonDestroy the NewtonCollision when you are done with it!
    NewtonCollision* UrhoShapeToNewtonCollision(const NewtonWorld* newtonWorld, const Sphere& sphere, bool includeTranslation = true);
    NewtonCollision* UrhoShapeToNewtonCollision(const NewtonWorld* newtonWorld, const BoundingBox& box, bool includeTranslation = true);








    ///Printing Helpers
    void PrintNewtonMatrix(dMatrix mat);











}
