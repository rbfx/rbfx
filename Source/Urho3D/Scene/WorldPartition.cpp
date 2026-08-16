#include "../Precompiled.h"

#include "WorldPartition.h"

#include <EASTL/sort.h>

#include "../Core/StringUtils.h"
#include "../DebugNew.h"

namespace Urho3D
{

void WorldPartition::SetStreamingRadius(float radius)
{
    streamingRadius_ = Max(radius, 0.0f);
}

void WorldPartition::SetMaxLoadedCells(unsigned maxLoadedCells)
{
    maxLoadedCells_ = Max(maxLoadedCells, 1u);
}

bool WorldPartition::AddCell(const StreamingCellDescriptor& descriptor, ea::string* error)
{
    if (descriptor.id.empty())
    {
        if (error)
            *error = "World partition cell id cannot be empty.";
        return false;
    }
    if (cells_.find(descriptor.id) != cells_.end())
    {
        if (error)
            *error = Format("World partition cell '{}' already exists.", descriptor.id);
        return false;
    }
    for (const auto& item : cells_)
    {
        if (item.second.GetCoordinates() == descriptor.coordinates)
        {
            if (error)
                *error = Format("World partition coordinates {} {} are already occupied.",
                    descriptor.coordinates.x_, descriptor.coordinates.y_);
            return false;
        }
    }

    cells_[descriptor.id] = StreamingCell(descriptor);
    lastError_.clear();
    return true;
}

bool WorldPartition::RemoveCell(const ea::string& cellId, ea::string* error)
{
    const auto iter = cells_.find(cellId);
    if (iter == cells_.end())
    {
        if (error)
            *error = Format("World partition cell '{}' does not exist.", cellId);
        return false;
    }
    if (iter->second.GetState() == StreamingCellState::Loading ||
        iter->second.GetState() == StreamingCellState::Unloading || HasPendingOperation(cellId))
    {
        if (error)
            *error = Format("World partition cell '{}' has an in-flight operation.", cellId);
        return false;
    }
    cells_.erase(iter);
    return true;
}

StreamingCell* WorldPartition::GetCell(const ea::string& cellId)
{
    const auto iter = cells_.find(cellId);
    return iter != cells_.end() ? &iter->second : nullptr;
}

const StreamingCell* WorldPartition::GetCell(const ea::string& cellId) const
{
    const auto iter = cells_.find(cellId);
    return iter != cells_.end() ? &iter->second : nullptr;
}

ea::vector<ea::string> WorldPartition::GetCellIds() const
{
    ea::vector<ea::string> result;
    result.reserve(cells_.size());
    for (const auto& item : cells_)
        result.push_back(item.first);
    ea::sort(result.begin(), result.end());
    return result;
}

unsigned WorldPartition::GetLoadedCellCount() const
{
    unsigned count = 0;
    for (const auto& item : cells_)
    {
        if (item.second.GetState() == StreamingCellState::Loaded ||
            item.second.GetState() == StreamingCellState::Unloading)
            ++count;
    }
    return count;
}

unsigned WorldPartition::GetLoadingCellCount() const
{
    unsigned count = 0;
    for (const auto& item : cells_)
    {
        if (item.second.GetState() == StreamingCellState::Loading)
            ++count;
    }
    return count;
}

bool WorldPartition::HasPendingOperation(const ea::string& cellId) const
{
    for (const StreamingOperation& operation : pendingOperations_)
    {
        if (operation.cellId == cellId)
            return true;
    }
    return false;
}

void WorldPartition::QueueOperation(StreamingCell& cell, StreamingOperationType type)
{
    if (HasPendingOperation(cell.GetId()))
        return;

    const bool accepted = type == StreamingOperationType::Load ? cell.BeginLoad() : cell.BeginUnload();
    if (!accepted)
        return;

    StreamingOperation operation;
    operation.cellId = cell.GetId();
    operation.type = type;
    operation.distanceSquared = cell.GetDistanceSquared();
    pendingOperations_.push_back(ea::move(operation));
}

void WorldPartition::SortPendingOperations()
{
    ea::sort(pendingOperations_.begin(), pendingOperations_.end(), [](const StreamingOperation& lhs,
        const StreamingOperation& rhs)
    {
        if (lhs.type != rhs.type)
            return lhs.type == StreamingOperationType::Unload;
        if (lhs.distanceSquared != rhs.distanceSquared)
        {
            if (lhs.type == StreamingOperationType::Unload)
                return lhs.distanceSquared > rhs.distanceSquared;
            return lhs.distanceSquared < rhs.distanceSquared;
        }
        return lhs.cellId < rhs.cellId;
    });
}

bool WorldPartition::RequestLoad(const ea::string& cellId)
{
    StreamingCell* cell = GetCell(cellId);
    if (!cell)
    {
        lastError_ = Format("Cannot load unknown world partition cell '{}'.", cellId);
        return false;
    }
    if (cell->GetState() == StreamingCellState::Loaded || cell->GetState() == StreamingCellState::Loading)
        return true;
    if (cell->GetState() == StreamingCellState::Unloading)
    {
        lastError_ = Format("Cannot load cell '{}' while it is unloading.", cellId);
        return false;
    }
    if (cell->GetState() == StreamingCellState::Failed)
        cell->ResetFailure();
    QueueOperation(*cell, StreamingOperationType::Load);
    SortPendingOperations();
    return cell->GetState() == StreamingCellState::Loading;
}

bool WorldPartition::RequestUnload(const ea::string& cellId)
{
    StreamingCell* cell = GetCell(cellId);
    if (!cell)
    {
        lastError_ = Format("Cannot unload unknown world partition cell '{}'.", cellId);
        return false;
    }
    if (cell->GetState() == StreamingCellState::Unloaded)
        return true;
    if (cell->GetState() == StreamingCellState::Unloading)
        return true;
    if (cell->GetState() == StreamingCellState::Loading)
    {
        lastError_ = Format("Cannot unload cell '{}' while it is loading.", cellId);
        return false;
    }
    QueueOperation(*cell, StreamingOperationType::Unload);
    SortPendingOperations();
    return cell->GetState() == StreamingCellState::Unloading;
}

unsigned WorldPartition::Update(const Vector3& observerPosition)
{
    observerPosition_ = observerPosition;
    lastError_.clear();

    const float radiusSquared = streamingRadius_ * streamingRadius_;
    for (auto& item : cells_)
    {
        StreamingCell& cell = item.second;
        const Vector3 delta = cell.GetDescriptor().center - observerPosition_;
        cell.SetDistanceSquared(delta.x_ * delta.x_ + delta.y_ * delta.y_ + delta.z_ * delta.z_);
    }

    const float loadedRadiusSquared = radiusSquared;
    for (auto& item : cells_)
    {
        StreamingCell& cell = item.second;
        if (cell.GetState() == StreamingCellState::Loaded &&
            cell.GetDistanceSquared() > loadedRadiusSquared && !HasPendingOperation(cell.GetId()))
            QueueOperation(cell, StreamingOperationType::Unload);
    }

    unsigned resident = GetLoadedCellCount() + GetLoadingCellCount();
    if (resident > maxLoadedCells_)
    {
        ea::vector<StreamingCell*> candidates;
        for (auto& item : cells_)
        {
            StreamingCell& cell = item.second;
            if (cell.GetState() == StreamingCellState::Loaded && !HasPendingOperation(cell.GetId()))
                candidates.push_back(&cell);
        }
        ea::sort(candidates.begin(), candidates.end(), [](const StreamingCell* lhs, const StreamingCell* rhs)
        {
            if (lhs->GetDistanceSquared() != rhs->GetDistanceSquared())
                return lhs->GetDistanceSquared() > rhs->GetDistanceSquared();
            return lhs->GetId() > rhs->GetId();
        });
        unsigned excess = resident - maxLoadedCells_;
        for (StreamingCell* cell : candidates)
        {
            if (excess == 0)
                break;
            QueueOperation(*cell, StreamingOperationType::Unload);
            --excess;
        }
    }

    resident = GetLoadedCellCount() + GetLoadingCellCount();
    if (resident < maxLoadedCells_)
    {
        ea::vector<StreamingCell*> candidates;
        for (auto& item : cells_)
        {
            StreamingCell& cell = item.second;
            const float cellRadius = Max(cell.GetDescriptor().radius, 0.0f);
            const float activationRadius = streamingRadius_ + cellRadius;
            if (cell.GetState() == StreamingCellState::Unloaded &&
                cell.GetDistanceSquared() <= activationRadius * activationRadius &&
                !HasPendingOperation(cell.GetId()))
                candidates.push_back(&cell);
        }
        ea::sort(candidates.begin(), candidates.end(), [](const StreamingCell* lhs, const StreamingCell* rhs)
        {
            if (lhs->GetDistanceSquared() != rhs->GetDistanceSquared())
                return lhs->GetDistanceSquared() < rhs->GetDistanceSquared();
            return lhs->GetId() < rhs->GetId();
        });
        for (StreamingCell* cell : candidates)
        {
            if (resident >= maxLoadedCells_)
                break;
            QueueOperation(*cell, StreamingOperationType::Load);
            ++resident;
        }
    }

    SortPendingOperations();
    return pendingOperations_.size();
}

bool WorldPartition::PopNextOperation(StreamingOperation& operation)
{
    if (pendingOperations_.empty())
        return false;
    operation = ea::move(pendingOperations_.front());
    pendingOperations_.erase(pendingOperations_.begin());
    return true;
}

bool WorldPartition::CompleteOperation(const ea::string& cellId, bool success, const ea::string& error)
{
    StreamingCell* cell = GetCell(cellId);
    if (!cell)
    {
        lastError_ = Format("Cannot complete unknown world partition cell '{}'.", cellId);
        return false;
    }

    const bool completed = cell->GetState() == StreamingCellState::Loading
        ? cell->CompleteLoad(success, error)
        : cell->CompleteUnload(success, error);
    if (!completed)
    {
        lastError_ = Format("Cell '{}' has no matching in-flight operation.", cellId);
        return false;
    }

    pendingOperations_.erase(ea::remove_if(pendingOperations_.begin(), pendingOperations_.end(),
        [&](const StreamingOperation& operation) { return operation.cellId == cellId; }), pendingOperations_.end());
    return true;
}

} // namespace Urho3D
