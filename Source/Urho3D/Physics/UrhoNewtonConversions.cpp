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
#ifndef _NEWTON_USE_DOUBLE
        return dMatrix(mat4.Transpose().Data());
#else

        Matrix4 tranposed = mat4.Transpose();
        const float* dataPtr = tranposed.Data();
        dFloat data[16];

        for (int i = 0; i < 16; i++)
            data[i] = dataPtr[i];


        return dMatrix(data);

#endif
    }
    dMatrix UrhoToNewton(const Matrix3x4& mat3x4)
    {
#ifndef _NEWTON_USE_DOUBLE
        Matrix4 asMat4 = mat3x4.ToMatrix4();
        return dMatrix(asMat4.Transpose().Data());
#else
        Matrix4 tranposed = mat3x4.ToMatrix4().Transpose();
        const float* dataPtr = tranposed.Data();
        dFloat data[16];

        for (int i = 0; i < 16; i++)
            data[i] = dataPtr[i];


        return dMatrix(data);
#endif
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
#ifndef _NEWTON_USE_DOUBLE
        return Matrix4(&mat[0][0]).Transpose();
#else
        float data[16];
        for (int r = 0; r < 4; r++) {
            for (int c = 0; c < 4; c++) {
                data[r + 4*c] = mat[c][r];
            }
        }

        return Matrix4(data).Transpose();


#endif
    }

    Quaternion NewtonToUrhoQuat(const dQuaternion& quat)
    {
        return Quaternion(quat.m_w, quat.m_x, quat.m_y, quat.m_z);
    }
    Quaternion NewtonToUrhoQuat(const dgQuaternion& quat)
    {
        return  Quaternion(quat.m_w, quat.m_x, quat.m_y, quat.m_z);
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







    void PrintNewtonMatrix(dMatrix mat)
    {
        const int paddingSize = 10;
        for (int row = 0; row < 4; row++) {

            Vector<String> rowStrings;

            for (int col = 0; col < 4; col++) {
                String colSubString = String(mat[row][col]);

                while (colSubString.Length() < paddingSize)
                    colSubString.Append(' ');



                rowStrings += colSubString;

            }


            URHO3D_LOGINFO(String(rowStrings[0]) + " , " + String(rowStrings[1]) + " , " + String(rowStrings[2]) + " , " + String(rowStrings[3]));
        }


        URHO3D_LOGINFO("");

    }

}
