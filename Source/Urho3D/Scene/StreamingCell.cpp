#include "../Precompiled.h"

#include "StreamingCell.h"

#include "../DebugNew.h"

namespace Urho3D
{

StreamingCell::StreamingCell(const StreamingCellDescriptor& descriptor)
    : descriptor_(descriptor)
{
    if (descriptor_.radius <= 0.0f)
        descriptor_.radius = 1.0f;
}

bool StreamingCell::BeginLoad()
{
    if (state_ != StreamingCellState::Unloaded && state_ != StreamingCellState::Failed)
        return false;
    state_ = StreamingCellState::Loading;
    lastError_.clear();
    return true;
}

bool StreamingCell::CompleteLoad(bool success, const ea::string& error)
{
    if (state_ != StreamingCellState::Loading)
        return false;
    if (!success)
    {
        state_ = StreamingCellState::Failed;
        lastError_ = error.empty() ? "Streaming cell load failed." : error;
        return true;
    }

    state_ = StreamingCellState::Loaded;
    lastError_.clear();
    ++loadRevision_;
    return true;
}

bool StreamingCell::BeginUnload()
{
    if (state_ != StreamingCellState::Loaded)
        return false;
    state_ = StreamingCellState::Unloading;
    lastError_.clear();
    return true;
}

bool StreamingCell::CompleteUnload(bool success, const ea::string& error)
{
    if (state_ != StreamingCellState::Unloading)
        return false;
    if (!success)
    {
        state_ = StreamingCellState::Failed;
        lastError_ = error.empty() ? "Streaming cell unload failed." : error;
        return true;
    }

    state_ = StreamingCellState::Unloaded;
    lastError_.clear();
    return true;
}

void StreamingCell::ResetFailure()
{
    if (state_ == StreamingCellState::Failed)
    {
        state_ = StreamingCellState::Unloaded;
        lastError_.clear();
    }
}

} // namespace Urho3D
