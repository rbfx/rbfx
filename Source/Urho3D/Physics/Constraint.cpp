//
// Copyright (c) 2008-2019 the Urho3D project.
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
#include "Constraint.h"
#include "RigidBody.h"
#include "PhysicsWorld.h"
#include "Core/Context.h"
#include "Scene/Component.h"
#include "Graphics/DebugRenderer.h"

#include "Scene/Scene.h"
#include "dCustomFixDistance.h"
#include "Newton.h"
#include "NewtonDebugDrawing.h"
#include "IO/Log.h"
#include "UrhoNewtonConversions.h"
namespace Urho3D {


    const char* solveModeNames[] =
    {
        "SOLVE_MODE_JOINT_DEFAULT",
        "SOLVE_MODE_EXACT",
        "SOLVE_MODE_ITERATIVE",
        "SOLVE_MODE_KINEMATIC_LOOP",
        nullptr
    };


    Constraint::Constraint(Context* context) : Component(context)
    {

    }

    Constraint::~Constraint()
    {
    }

    void Constraint::RegisterObject(Context* context)
    {
        context->RegisterFactory<Constraint>(DEF_PHYSICS_CATEGORY.CString());
        URHO3D_COPY_BASE_ATTRIBUTES(Component);

        URHO3D_ENUM_ACCESSOR_ATTRIBUTE("Solver Iterations", GetSolveMode, SetSolveMode, CONSTRAINT_SOLVE_MODE, solveModeNames, SOLVE_MODE_JOINT_DEFAULT, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Stiffness", GetStiffness, SetStiffness, float, 0.7f, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("ForceCalculationsEnabled", GetEnableForceCalculation, SetEnableForceCalculation, bool, false, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Other Body ID", GetOtherBodyId, SetOtherBodyById, unsigned, 0, AM_DEFAULT | AM_COMPONENTID);


        URHO3D_ATTRIBUTE("Other Body Frame Position", Vector3, otherPosition_, Vector3::ZERO, AM_DEFAULT);
        URHO3D_ATTRIBUTE("Other Body Frame Rotation", Quaternion, otherRotation_, Quaternion::IDENTITY, AM_DEFAULT);
        URHO3D_ATTRIBUTE("Body Frame Position", Vector3, position_, Vector3::ZERO, AM_DEFAULT);
        URHO3D_ATTRIBUTE("Body Frame Rotation", Quaternion, rotation_, Quaternion::IDENTITY, AM_DEFAULT);


    }

    void Constraint::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
    {
        //draw 2 part line from one frame to the other. Black touching own body and graw touching other body.
        if (ownBody_) {
           
                Vector3 midPoint = (GetOtherWorldFrame().Translation() + GetOwnWorldFrame().Translation())*0.5f;
                debug->AddLine(GetOwnWorldFrame().Translation(), midPoint, Color::BLACK, depthTest);
                debug->AddLine(midPoint, GetOtherWorldFrame().Translation(), Color::GRAY, depthTest);
        }


        //draw the frames.
        const float axisLengths = 0.5f;

        float hueOffset = 0.05f;

        Color xAxisC;
        float shiftedColor = Color::RED.Hue() + hueOffset;
        if (shiftedColor > 1.0f)
            shiftedColor -= 1.0f;

        xAxisC.FromHSL(shiftedColor, Color::RED.SaturationHSL(), Color::RED.Lightness());

        Color yAxisC;
        shiftedColor = Color::GREEN.Hue() + hueOffset;
        if (shiftedColor > 1.0f)
            shiftedColor -= 1.0f;

        yAxisC.FromHSL(shiftedColor, Color::GREEN.SaturationHSL(), Color::GREEN.Lightness());

        Color zAxisC;
        shiftedColor = Color::BLUE.Hue() + hueOffset;
        if (shiftedColor > 1.0f)
            shiftedColor -= 1.0f;

        zAxisC.FromHSL(shiftedColor, Color::BLUE.SaturationHSL(), Color::BLUE.Lightness());



        Color xAxisDark = xAxisC.Lerp(Color::BLACK, 0.5f);
        Color yAxisDark = yAxisC.Lerp(Color::BLACK, 0.5f);
        Color zAxisDark = zAxisC.Lerp(Color::BLACK, 0.5f);



        debug->AddFrame(GetOwnWorldFrame(), axisLengths, xAxisC, yAxisC, zAxisC, depthTest);
        debug->AddFrame(GetOtherWorldFrame(), axisLengths, xAxisDark, yAxisDark, zAxisDark, depthTest);


        //draw the special joint stuff given to us by newton
        UrhoNewtonDebugDisplay debugDisplay(debug, depthTest);
        debugDisplay.SetDrawScale(1.0f/physicsWorld_->GetPhysicsScale());
        if (newtonJoint_)
        {
            newtonJoint_->Debug(&debugDisplay);//#todo this sometimes covers up the 2 frames above - maybe alter inside newton instead?
        }

    }

    void Constraint::SetDisableCollision(bool disable)
    {
        enableBodyCollision_ = !disable;
        MarkDirty();
    }

    void Constraint::SetOtherBody(RigidBody* body)
    {
        if (otherBody_ != body && body != ownBody_) {

            if (otherBody_ != nullptr)
                RemoveJointReferenceFromBody(otherBody_);//remove reference from old body


            otherBody_ = body;
            if (body != nullptr) {
                AddJointReferenceToBody(body);
                body->GetNode()->AddListener(this);


                SetOtherWorldPosition(GetOwnWorldFrame().Translation());
                SetOtherWorldRotation(GetOwnWorldFrame().Rotation());
                
            }

            if (body == nullptr)
                otherBodyId_ = 0;
            else
                otherBodyId_ = body->GetID();


            MarkDirty();
        }
    }


    void Constraint::SetOtherBodyById(unsigned bodyId)
    {
        otherBodyId_ = bodyId;
        Component* component = GetScene()->GetComponent(bodyId);
        if(component && component->GetType() == RigidBody::GetTypeStatic())
            SetOtherBody((RigidBody*)GetScene()->GetComponent(bodyId));
        else
        {
            //URHO3D_LOGWARNING("Constraint:: Unable To Find Body With Component Id: " + String(bodyId));
        }
    }

    void Constraint::WakeBodies()
    {
        if (ownBody_)
        {
            ownBody_->Activate();
        }
        if (otherBody_)
        {
            otherBody_->Activate();
        }
    }

    void Constraint::SetWorldPosition(const Vector3& position)
    {
        SetOwnWorldPosition(position);
        SetOtherWorldPosition(position);
    }

    void Constraint::SetWorldRotation(const Quaternion& rotation)
    {
        SetOwnWorldRotation(rotation);
        SetOtherWorldRotation(rotation);
    }
    
    void Constraint::SetPosition(const Vector3& position)
    {
        SetWorldPosition(ownBody_->GetNode()->GetWorldTransform() * position);
    }
    
    void Constraint::SetRotation(const Quaternion& rotation)
    {
        SetWorldRotation(ownBody_->GetNode()->GetWorldRotation() * rotation);
    }

    void Constraint::SetOwnPosition(const Vector3& position)
    {
        position_ = position;
        MarkDirty();
    }


    void Constraint::SetOwnRotation(const Quaternion& rotation)
    {
        rotation_ = rotation;
        MarkDirty();
    }


    void Constraint::SetOwnWorldPosition(const Vector3& worldPosition)
    {
       // Matrix3x4 worldTransform(ownBody_->GetNode()->GetWorldPosition(), ownBody_->GetNode()->GetWorldRotation(), 1.0f);
        position_ = ownBody_->GetWorldTransform().Inverse() *  worldPosition;
        MarkDirty();
    }

    void Constraint::SetOwnWorldRotation(const Quaternion& worldRotation)
    {
        Quaternion worldRot = ownBody_->GetWorldRotation();
        rotation_ = worldRot.Inverse() * worldRotation;
        MarkDirty();
    } 

    void Constraint::SetOtherPosition(const Vector3& position)
    {
        otherPosition_ = position;


        MarkDirty();
    }


    void Constraint::SetOtherRotation(const Quaternion& rotation)
    {
        otherRotation_ = rotation;

        MarkDirty();
    }


    void Constraint::SetOtherWorldPosition(const Vector3& position)
    {
        if (otherBody_) {
            otherPosition_ = otherBody_->GetWorldTransform().Inverse() * position;

            MarkDirty();
        }
    }

    void Constraint::SetOtherWorldRotation(const Quaternion& rotation)
    {
        if (otherBody_) {
            Quaternion worldRot = otherBody_->GetWorldRotation();
            otherRotation_ = worldRot.Inverse() * rotation;

            MarkDirty();
        }
    }

    void Constraint::SetSolveMode(CONSTRAINT_SOLVE_MODE mode)
    {
        if (solveMode_ != mode) {
            solveMode_ = mode;
            applyAllJointParams();
        }
    }

    void Constraint::SetStiffness(float stiffness)
    {
        if (stiffness_ != stiffness) {
            stiffness_ = stiffness;
            applyAllJointParams();
        }
    }

    void Constraint::SetEnableForceCalculation(bool enabled)
    {
        if (enabled != enableForceCalculations_) {
            enableForceCalculations_ = enabled;
            applyAllJointParams();
        }
    }

    bool Constraint::GetEnableForceCalculation() const
    {
        return enableForceCalculations_;
    }

    Vector3 Constraint::GetOwnForce()
    {
        if(newtonJoint_&& enableForceCalculations_)
            return NewtonToUrhoVec3(newtonJoint_->GetForce0());

        return Vector3();
    }

    Vector3 Constraint::GetOtherForce()
    {
        if (newtonJoint_ && enableForceCalculations_)
            return NewtonToUrhoVec3(newtonJoint_->GetForce1());

        return Vector3();
    }

    Vector3 Constraint::GetOwnTorque()
    {
        if (newtonJoint_ && enableForceCalculations_)
            return NewtonToUrhoVec3(newtonJoint_->GetTorque0());

        return Vector3();
    }

    Vector3 Constraint::GetOtherTorque()
    {
        if (newtonJoint_ && enableForceCalculations_)
            return NewtonToUrhoVec3(newtonJoint_->GetTorque1());

        return Vector3();
    }

    NewtonBody* Constraint::GetOwnNewtonBody() const
    {
        return ownBody_->GetNewtonBody();
    }

    NewtonBody* Constraint::GetOtherNewtonBody() const
    {
        if (otherBody_)
            return otherBody_->GetNewtonBody();
        else
            return nullptr;
    }

    Vector3 Constraint::GetOtherPosition() const
    {

       return otherPosition_;

    }

    Quaternion Constraint::GetOtherRotation() const
    {
        return otherRotation_;
    }

    Matrix3x4 Constraint::GetOwnWorldFrame() const
    {

        //return a frame with no scale at the position and rotation in node space
        Matrix3x4 worldFrame = ownBody_->GetWorldTransform() * Matrix3x4(position_, rotation_, 1.0f);

        //the frame could have uniform scale - reconstruct with no scale
        Matrix3x4 worldFrameNoScale = Matrix3x4(worldFrame.Translation(), worldFrame.Rotation(), 1.0f);


        return worldFrameNoScale;

    }

    Matrix3x4 Constraint::GetOtherWorldFrame() const
    {
        if (otherBody_) {

            //return a frame with no scale at the position and rotation in node space.
            Matrix3x4 worldFrame = otherBody_->GetWorldTransform() * Matrix3x4(otherPosition_, otherRotation_, 1.0f);

            //the frame could have uniform scale - reconstruct with no scale
            Matrix3x4 worldFrameNoScale = Matrix3x4(worldFrame.Translation(), worldFrame.Rotation(), 1.0f);


            return worldFrameNoScale;
        }
        else
        {
            return Matrix3x4(otherPosition_, otherRotation_, 1.0f);
        }
    }

    void Constraint::OnSetEnabled()
    {
        MarkDirty();
    }

    void Constraint::reEvalConstraint()
    {
        if (!IsEnabledEffective()) {
            freeInternal();
        }
        else if (ownBody_ && ownBody_->GetNewtonBody()) {
            freeInternal();

            bool goodToBuild = true;

            if (otherBodyId_ > 0 && !otherBody_)
            {
                SetOtherBodyById(otherBodyId_);
            }

            if (otherBody_)
            {
                if (otherBody_->GetEffectiveMass() <= 0.0f && ownBody_->GetEffectiveMass() <= 0.0f)
                    goodToBuild = false;

            }
            else
            {
                if (ownBody_->GetEffectiveMass() <= 0.0f)
                    goodToBuild = false;
            }

            if (goodToBuild) {
                buildConstraint();
                applyAllJointParams();

            }
            else
            {
                URHO3D_LOGWARNING("Contraint must connect to at least 1 Rigid Body with mass greater than 0.");
            }

            if (newtonJoint_ != nullptr) {

            }
        }
        else//we dont have own body so free the joint..
        {
            freeInternal();
        }






        MarkDirty(false);
    }

    void Constraint::buildConstraint()
    {
        /// ovverride in derived classes.
    }


    bool Constraint::applyAllJointParams()
    {
        WakeBodies();

        if (newtonJoint_ == nullptr)
            return false;

        /// extend in derived classes.
        NewtonJointSetCollisionState((NewtonJoint*)newtonJoint_, enableBodyCollision_);
        newtonJoint_->SetStiffness(stiffness_);
        newtonJoint_->SetJointForceCalculation(enableForceCalculations_);

        if(solveMode_ != SOLVE_MODE_JOINT_DEFAULT)
            newtonJoint_->SetSolverModel(solveMode_);

        return true;
    }

    void Constraint::freeInternal()
    {

        if (newtonJoint_ != nullptr) {
            physicsWorld_->addToFreeQueue(newtonJoint_);
            newtonJoint_ = nullptr;
        }
    }



    void Constraint::AddJointReferenceToBody(RigidBody* rigBody)
    {

        if (!rigBody->connectedConstraints_.Contains(this))
            rigBody->connectedConstraints_.Insert(this);

    }


    void Constraint::RemoveJointReferenceFromBody(RigidBody* rigBody)
    {

        if (rigBody->connectedConstraints_.Contains(this))
            rigBody->connectedConstraints_.Erase(this);

    }

    void Constraint::OnNodeSet(Node* node)
    {
        if (node)
        {
            //auto create physics world similar to rigid body.
            physicsWorld_ = node->GetScene()->GetOrCreateComponent<PhysicsWorld>();

            RigidBody* rigBody = node->GetComponent<RigidBody>();
            if (rigBody) {
                ownBody_ = rigBody;
                ownBodyId_ = ownBody_->GetID();
            }
           
            if(physicsWorld_)
                physicsWorld_->addConstraint(this);

            AddJointReferenceToBody(ownBody_);

            node->AddListener(this);
        }
        else
        {
            if (!ownBody_.Expired())
                RemoveJointReferenceFromBody(ownBody_);

            ownBody_ = nullptr;
            if (!physicsWorld_.Expired())
                physicsWorld_->removeConstraint(this);

            freeInternal();

        }
    }

    void Constraint::OnNodeSetEnabled(Node* node)
    {
        MarkDirty();
    }



    Urho3D::Matrix3x4 Constraint::GetOwnNewtonWorldFrame()
    {
        Matrix3x4 newtonWorldFrame = physicsWorld_->SceneToPhysics_Domain(GetOwnWorldFrame());

        //newtonWorldFrame has scaling from the the physics world frame transformation. - reconstruct without scale because joints expect frame with no scaling.
        return Matrix3x4(newtonWorldFrame.Translation(), newtonWorldFrame.Rotation(), 1.0f);
    }

    Urho3D::Matrix3x4 Constraint::GetOtherNewtonWorldFrame()
    {
        Matrix3x4 newtonWorldFrame = physicsWorld_->SceneToPhysics_Domain(GetOtherWorldFrame());

        //newtonWorldFrame has scaling from the the physics world frame transformation. - reconstruct without scale because joints expect frame with no scaling.
        return Matrix3x4(newtonWorldFrame.Translation(), newtonWorldFrame.Rotation(), 1.0f);
    }

    Urho3D::Vector3 Constraint::GetOwnNewtonWorldPin()
    {
        Matrix3x4 newtonWorldFrame = GetOwnNewtonWorldFrame();
        return newtonWorldFrame * Vector3::RIGHT;
    }

    Urho3D::Vector3 Constraint::GetOtherNewtonWorldPin()
    {
        Matrix3x4 newtonWorldFrame = GetOtherNewtonWorldFrame();
        return newtonWorldFrame * Vector3::RIGHT;
    }

}
