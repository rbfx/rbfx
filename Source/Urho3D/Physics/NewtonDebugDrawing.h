#pragma once
#include "Newton.h"
#include "../Math/Color.h"
#include "dCustomJoint.h"


class NewtonBody;

namespace Urho3D
{
    class DebugRenderer;
    class Matrix3x4;
    class PhysicsWorld;


    //class enabling native newton debug calls using Urho3D::DebugRenderer.
    class UrhoNewtonDebugDisplay : public dCustomJoint::dDebugDisplay
    {
    public:
        UrhoNewtonDebugDisplay(DebugRenderer* debugRenderer, bool depthTest) : dCustomJoint::dDebugDisplay(dMatrix())
        {
            debugRenderer_ = debugRenderer;
            depthTest_ = depthTest;
            SetScale(0.5f);
        }
        virtual ~UrhoNewtonDebugDisplay() {}

        void SetDrawScale(float scale) { worldScale_ = scale; }

        virtual void SetColor(const dVector& color) override;
        virtual void DrawLine(const dVector& p0, const dVector& p1) override;
    protected:
        float worldScale_ = 1.0f;
        Color currentColor_;
        bool depthTest_ = false;
        DebugRenderer* debugRenderer_ = nullptr;
    };




    struct debugRenderOptions {
        Color color = Color::GRAY;
        DebugRenderer* debug;
        bool depthTest = false;
    };



    void NewtonDebug_BodyDrawCollision(PhysicsWorld* physicsWorld, const NewtonBody* const body, DebugRenderer* debug, bool depthTest = false);
    void NewtonDebug_DrawCollision(NewtonCollision* collision, const Matrix3x4& transform, const Color& color, DebugRenderer* debug, bool depthTest = false);

    void NewtonDebug_ShowGeometryCollisionCallback(void* userData, int vertexCount, const dFloat* const faceVertec, int id);



}
