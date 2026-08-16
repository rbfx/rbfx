#pragma once

#include "../Math/Vector2.h"
#include "../Math/Vector3.h"
#include "../Core/StringUtils.h"

namespace Urho3D
{

/// Runtime state of a world-partition cell.
enum class StreamingCellState
{
    Unloaded,
    Loading,
    Loaded,
    Unloading,
    Failed
};

/// Immutable source and spatial metadata used to stream one cell.
struct URHO3D_API StreamingCellDescriptor
{
    ea::string id;
    IntVector2 coordinates;
    Vector3 center;
    float radius{1.0f};
    ea::string scenePath;
    unsigned memoryCost{};
};

/// State machine and runtime metadata for one world-partition cell.
class URHO3D_API StreamingCell
{
public:
    StreamingCell() = default;
    explicit StreamingCell(const StreamingCellDescriptor& descriptor);

    const StreamingCellDescriptor& GetDescriptor() const { return descriptor_; }
    const ea::string& GetId() const { return descriptor_.id; }
    const IntVector2& GetCoordinates() const { return descriptor_.coordinates; }
    StreamingCellState GetState() const { return state_; }
    const ea::string& GetLastError() const { return lastError_; }
    float GetDistanceSquared() const { return distanceSquared_; }
    unsigned GetLoadRevision() const { return loadRevision_; }

    void SetDistanceSquared(float distanceSquared) { distanceSquared_ = distanceSquared; }
    bool BeginLoad();
    bool CompleteLoad(bool success, const ea::string& error = EMPTY_STRING);
    bool BeginUnload();
    bool CompleteUnload(bool success, const ea::string& error = EMPTY_STRING);
    void ResetFailure();

private:
    StreamingCellDescriptor descriptor_;
    StreamingCellState state_{StreamingCellState::Unloaded};
    ea::string lastError_;
    float distanceSquared_{M_INFINITY};
    unsigned loadRevision_{};
};

} // namespace Urho3D
