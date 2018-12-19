#include "UrhoNewtonConversions.h"
#include "dMatrix.h"
#include "dVector.h"
#include "dQuaternion.h"

#include "Math/Matrix4.h"
#include "Math/Matrix3x4.h"
#include "IO/Log.h"
#include "Newton.h"
#include "Math/Sphere.h"
#include "Math/BoundingBox.h"
#include "dgQuaternion.h"



namespace Urho3D {


    dMatrix UrhoToNewton(const Matrix4& mat4)
    {
        //#todo might be faster to just map values and copy..
        return dMatrix(mat4.Transpose().Data());
    }
    dMatrix UrhoToNewton(const Matrix3x4& mat3x4)
    {
        //#todo might be faster to just map values and copy..
        Matrix4 asMat4 = mat3x4.ToMatrix4();
        return dMatrix(asMat4.Transpose().Data());
    }

    dVector UrhoToNewton(const Vector4& vec4)
    {
        return dVector(vec4.x_, vec4.y_, vec4.z_, vec4.w_);
    }

    dVector UrhoToNewton(const Vector3& vec3)
    {
        return dVector(vec3.x_, vec3.y_, vec3.z_);
    }

    dVector UrhoToNewton(const Vector2& vec2)
    {
        return dVector(vec2.x_, vec2.y_, 0.0f);
    }
    dQuaternion UrhoToNewton(const Quaternion& quat)
    {
        return dQuaternion(quat.w_, quat.x_, quat.y_, quat.z_);
    }




    Vector3 NewtonToUrhoVec3(const dVector& vec)
    {
        return Vector3(vec.m_x, vec.m_y, vec.m_z);
    }
    Vector4 NewtonToUrhoVec4(const dVector& vec)
    {
        return Vector4(vec.m_x, vec.m_y, vec.m_z, vec.m_w);
    }


    Matrix4 NewtonToUrhoMat4(const dMatrix& mat)
    {
        return Matrix4(&mat[0][0]).Transpose();
    }

    Quaternion NewtonToUrhoQuat(const dQuaternion& quat)
    {
        return Quaternion(quat.m_q0, quat.m_q1, quat.m_q2, quat.m_q3);
    }
    Quaternion NewtonToUrhoQuat(const dgQuaternion& quat)
    {
        return Quaternion(quat.m_q0, quat.m_q1, quat.m_q2, quat.m_q3);
    }




    
    NewtonCollision* UrhoShapeToNewtonCollision(const NewtonWorld* newtonWorld, const Sphere& sphere, bool includePosition /*= true*/)
    {
        Matrix3x4 mat;
        mat.SetTranslation(sphere.center_);
        dMatrix dMat = UrhoToNewton(mat);

        NewtonCollision* newtonShape;

        if (includePosition) {
            newtonShape = NewtonCreateSphere(newtonWorld, sphere.radius_, 0, &dMat[0][0]);
        }
        else
        {
            newtonShape = NewtonCreateSphere(newtonWorld, sphere.radius_, 0, nullptr);
        }

        return newtonShape;
    }




    NewtonCollision* UrhoShapeToNewtonCollision(const NewtonWorld* newtonWorld, const BoundingBox& box, bool includePosition /*= true*/)
    {
        Matrix3x4 mat;
        mat.SetTranslation(box.Center());
        dMatrix dMat = UrhoToNewton(mat);

        NewtonCollision* newtonShape;

        if (includePosition) {
            newtonShape = NewtonCreateBox(newtonWorld, box.Size().x_, box.Size().y_, box.Size().z_, 0, &dMat[0][0]);
        }
        else
        {
            newtonShape = NewtonCreateBox(newtonWorld, box.Size().x_, box.Size().y_, box.Size().z_, 0, nullptr);
        }

        return newtonShape;
    }







    void PrintNewton(dMatrix mat)
    {
        for (int x = 0; x < 4; x++)
            URHO3D_LOGINFO(String(mat[x][0]) + " , " + String(mat[x][1]) + " , " + String(mat[x][2]) + " , " + String(mat[x][3]));

        URHO3D_LOGINFO("");

    }

}
