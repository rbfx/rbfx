#include <catch2/catch_amalgamated.hpp>

#include <Urho3D/Graphics/LODGroup.h>
#include <Urho3D/Scene/WorldPartition.h>

using namespace Urho3D;

namespace
{
StreamingCellDescriptor MakeCell(const ea::string& id, float x, int gx)
{
    StreamingCellDescriptor descriptor;
    descriptor.id = id;
    descriptor.coordinates = IntVector2(gx, 0);
    descriptor.center = Vector3(x, 0.0f, 0.0f);
    descriptor.radius = 1.0f;
    descriptor.scenePath = id + ".scene";
    return descriptor;
}
}

TEST_CASE("StreamingCell enforces load and unload transitions", "[streaming][scene]")
{
    StreamingCell cell(MakeCell("Cell_A", 0.0f, 0));
    REQUIRE(cell.GetState() == StreamingCellState::Unloaded);
    REQUIRE_FALSE(cell.BeginUnload());
    REQUIRE(cell.BeginLoad());
    REQUIRE_FALSE(cell.BeginLoad());
    REQUIRE(cell.CompleteLoad(true));
    REQUIRE(cell.GetState() == StreamingCellState::Loaded);
    REQUIRE(cell.GetLoadRevision() == 1);
    REQUIRE_FALSE(cell.CompleteLoad(true));
    REQUIRE(cell.BeginUnload());
    REQUIRE(cell.CompleteUnload(true));
    REQUIRE(cell.GetState() == StreamingCellState::Unloaded);

    REQUIRE(cell.BeginLoad());
    REQUIRE(cell.CompleteLoad(false, "corrupt scene"));
    REQUIRE(cell.GetState() == StreamingCellState::Failed);
    REQUIRE(cell.GetLastError() == "corrupt scene");
    cell.ResetFailure();
    REQUIRE(cell.GetState() == StreamingCellState::Unloaded);
}

TEST_CASE("LODGroup selects levels with hysteresis", "[streaming][lod]")
{
    LODGroup group;
    REQUIRE(group.AddLevel({2, M_INFINITY}));
    REQUIRE(group.AddLevel({0, 10.0f}));
    REQUIRE(group.AddLevel({1, 30.0f}));
    group.SetHysteresis(0.1f);

    REQUIRE(group.Select(5.0f) == 0);
    REQUIRE(group.Select(10.5f, 0) == 0);
    REQUIRE(group.Select(11.1f, 0) == 1);
    REQUIRE(group.Select(9.5f, 1) == 1);
    REQUIRE(group.Select(8.9f, 1) == 0);
    REQUIRE(group.Select(100.0f, 0) == 2);
}

TEST_CASE("WorldPartition prioritizes nearby cells and enforces budget", "[streaming][world]")
{
    WorldPartition partition;
    partition.SetStreamingRadius(10.0f);
    partition.SetMaxLoadedCells(2);
    REQUIRE(partition.AddCell(MakeCell("Near_B", 5.0f, 1)));
    REQUIRE(partition.AddCell(MakeCell("Near_A", 1.0f, 0)));
    REQUIRE(partition.AddCell(MakeCell("Far", 100.0f, 2)));
    REQUIRE_FALSE(partition.AddCell(MakeCell("DuplicateCoord", 7.0f, 1)));

    REQUIRE(partition.Update(Vector3::ZERO) == 2);
    StreamingOperation operation;
    REQUIRE(partition.PopNextOperation(operation));
    REQUIRE(operation.type == StreamingOperationType::Load);
    REQUIRE(operation.cellId == "Near_A");
    REQUIRE(partition.CompleteOperation(operation.cellId, true));
    REQUIRE(partition.PopNextOperation(operation));
    REQUIRE(operation.cellId == "Near_B");
    REQUIRE(partition.CompleteOperation(operation.cellId, true));
    REQUIRE(partition.GetLoadedCellCount() == 2);
    REQUIRE(partition.Update(Vector3::ZERO) == 0);

    REQUIRE(partition.Update(Vector3(100.0f, 0.0f, 0.0f)) == 2);
    REQUIRE(partition.PopNextOperation(operation));
    REQUIRE(operation.type == StreamingOperationType::Unload);
    REQUIRE(partition.CompleteOperation(operation.cellId, true));
    REQUIRE(partition.PopNextOperation(operation));
    REQUIRE(operation.type == StreamingOperationType::Unload);
    REQUIRE(partition.CompleteOperation(operation.cellId, true));
    REQUIRE(partition.Update(Vector3(100.0f, 0.0f, 0.0f)) == 1);
    REQUIRE(partition.PopNextOperation(operation));
    REQUIRE(operation.cellId == "Far");
    REQUIRE(partition.CompleteOperation(operation.cellId, true));
    REQUIRE(partition.GetLoadedCellCount() == 1);
}

TEST_CASE("WorldPartition rejects removal during an in-flight operation", "[streaming][validation]")
{
    WorldPartition partition;
    REQUIRE(partition.AddCell(MakeCell("Cell_A", 0.0f, 0)));
    REQUIRE(partition.Update(Vector3::ZERO) == 1);
    ea::string error;
    REQUIRE_FALSE(partition.RemoveCell("Cell_A", &error));
    REQUIRE_FALSE(error.empty());
    StreamingOperation operation;
    REQUIRE(partition.PopNextOperation(operation));
    REQUIRE(partition.CompleteOperation("Cell_A", true));
    REQUIRE(partition.RemoveCell("Cell_A"));
}
