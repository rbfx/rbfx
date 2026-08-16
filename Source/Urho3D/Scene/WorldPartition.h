#pragma once

#include <EASTL/unordered_map.h>
#include <EASTL/vector.h>

#include "StreamingCell.h"

namespace Urho3D
{

/// Operation emitted by WorldPartition for an external async scene loader.
enum class StreamingOperationType
{
    Load,
    Unload
};

struct URHO3D_API StreamingOperation
{
    ea::string cellId;
    StreamingOperationType type{StreamingOperationType::Load};
    float distanceSquared{M_INFINITY};
};

/// Grid-independent world partition coordinator with deterministic streaming decisions.
class URHO3D_API WorldPartition
{
public:
    /// Set the observer interest radius in world units.
    void SetStreamingRadius(float radius);
    /// Return the observer interest radius.
    float GetStreamingRadius() const { return streamingRadius_; }
    /// Set the maximum number of simultaneously resident cells.
    void SetMaxLoadedCells(unsigned maxLoadedCells);
    /// Return the resident-cell budget.
    unsigned GetMaxLoadedCells() const { return maxLoadedCells_; }

    /// Add a cell descriptor. IDs and coordinates must be unique.
    bool AddCell(const StreamingCellDescriptor& descriptor, ea::string* error = nullptr);
    /// Remove a cell only when it has no in-flight operation.
    bool RemoveCell(const ea::string& cellId, ea::string* error = nullptr);
    /// Find a mutable cell.
    StreamingCell* GetCell(const ea::string& cellId);
    /// Find a const cell.
    const StreamingCell* GetCell(const ea::string& cellId) const;
    /// Return all registered cell IDs in deterministic order.
    ea::vector<ea::string> GetCellIds() const;

    /// Recompute desired residency and queue load/unload operations.
    unsigned Update(const Vector3& observerPosition);
    /// Explicitly request a named cell to load.
    bool RequestLoad(const ea::string& cellId);
    /// Explicitly request a named cell to unload.
    bool RequestUnload(const ea::string& cellId);
    /// Pop the next operation, ordered by priority and distance.
    bool PopNextOperation(StreamingOperation& operation);
    /// Complete a previously issued operation.
    bool CompleteOperation(const ea::string& cellId, bool success, const ea::string& error = EMPTY_STRING);
    /// Remove all queued operations without changing cell state.
    void ClearPendingOperations() { pendingOperations_.clear(); }

    unsigned GetLoadedCellCount() const;
    unsigned GetLoadingCellCount() const;
    unsigned GetPendingOperationCount() const { return pendingOperations_.size(); }
    const Vector3& GetObserverPosition() const { return observerPosition_; }
    const ea::string& GetLastError() const { return lastError_; }

private:
    bool HasPendingOperation(const ea::string& cellId) const;
    void QueueOperation(StreamingCell& cell, StreamingOperationType type);
    void SortPendingOperations();

    ea::unordered_map<ea::string, StreamingCell> cells_;
    ea::vector<StreamingOperation> pendingOperations_;
    Vector3 observerPosition_;
    float streamingRadius_{100.0f};
    unsigned maxLoadedCells_{64};
    ea::string lastError_;
};

} // namespace Urho3D
