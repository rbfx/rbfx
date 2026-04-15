#include "Urho3D/Scene/WorldOrigin.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/Core/WorkQueue.h"
#include "Urho3D/Scene/Scene.h"

namespace Urho3D
{

WorldOrigin::WorldOrigin(Context* context)
    : LogicComponent(context)
{
    SetUpdateEventMask(USE_UPDATE);
}

WorldOrigin::~WorldOrigin()
{
}

void WorldOrigin::RegisterObject(Context* context)
{
    context->AddFactoryReflection<ClassName>(Category_Scene);

    URHO3D_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Max Distance", unsigned, maxDistance_, MaxDistance, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Step", unsigned, step_, DefaultStep, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Update X", bool, updateX_, true, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Update Y", bool, updateY_, false, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Update Z", bool, updateZ_, true, AM_DEFAULT);
}

void WorldOrigin::Update(float timeStep)
{
    const Vector3 position = node_->GetWorldPosition();
    const float distanceToOrigin =
        ea::max({updateX_ ? Abs(position.x_) : 0, updateY_ ? Abs(position.y_) : 0, updateZ_ ? Abs(position.z_) : 0});
    if (distanceToOrigin > maxDistance_)
    {
        IntVector3 offset;
        offset.x_ = updateX_ ? static_cast<int>(SnapRound(position.x_, static_cast<float>(step_))) : 0;
        offset.y_ = updateY_ ? static_cast<int>(SnapRound(position.y_, static_cast<float>(step_))) : 0;
        offset.z_ = updateZ_ ? static_cast<int>(SnapRound(position.z_, static_cast<float>(step_))) : 0;
        if (offset != IntVector3::ZERO)
        {
            const IntVector3 newWorldOrigin = GetScene()->GetWorldOrigin() + offset;
            auto workQueue = GetSubsystem<WorkQueue>();
            WeakPtr<Scene> weakScene{GetScene()};
            WeakPtr<WorldOrigin> weakSelf{this};
            workQueue->PostDelayedTaskForMainThread([weakScene, weakSelf, newWorldOrigin]
            {
                if (auto scene = weakScene.Lock())
                {
                    const Vector3 oldPosition = weakSelf ? weakSelf->GetNode()->GetWorldPosition() : Vector3::ZERO;

                    scene->UpdateWorldOrigin(newWorldOrigin);

                    if (weakSelf)
                    {
                        const Vector3 newPosition = weakSelf->GetNode()->GetWorldPosition();
                        if (newPosition.Equals(oldPosition))
                        {
                            weakSelf->SetEnabled(false);
                            URHO3D_LOGERROR("WorldOrigin '{}' was not moved from {} during rebase, disabling",
                                weakSelf->GetFullNameDebug(), newPosition.ToString());
                        }
                    }
                }
            });
        }
    }
}

} // namespace Urho3D