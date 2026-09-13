// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Core/Object.h"

namespace Urho3D
{

/// Complete rebuild of navigation mesh.
URHO3D_EVENT(E_NAVIGATION_MESH_REBUILT, NavigationMeshRebuilt)
{
    URHO3D_PARAM(P_NODE, Node); // Node pointer
    URHO3D_PARAM(P_MESH, Mesh); // NavigationMesh pointer
}

/// Mesh tile is added to navigation mesh.
URHO3D_EVENT(E_NAVIGATION_TILE_ADDED, NavigationTileAdded)
{
    URHO3D_PARAM(P_NODE, Node); // Node pointer
    URHO3D_PARAM(P_MESH, Mesh); // NavigationMesh pointer
    URHO3D_PARAM(P_TILE, Tile); // IntVector2
}

/// Mesh tile is removed from navigation mesh.
URHO3D_EVENT(E_NAVIGATION_TILE_REMOVED, NavigationTileRemoved)
{
    URHO3D_PARAM(P_NODE, Node); // Node pointer
    URHO3D_PARAM(P_MESH, Mesh); // NavigationMesh pointer
    URHO3D_PARAM(P_TILE, Tile); // IntVector2
}

/// All mesh tiles are removed from navigation mesh.
URHO3D_EVENT(E_NAVIGATION_ALL_TILES_REMOVED, NavigationAllTilesRemoved)
{
    URHO3D_PARAM(P_NODE, Node); // Node pointer
    URHO3D_PARAM(P_MESH, Mesh); // NavigationMesh pointer
}

/// Crowd agent formation.
URHO3D_EVENT(E_CROWD_AGENT_FORMATION, CrowdAgentFormation)
{
    URHO3D_PARAM(P_NODE, Node); // Node pointer
    URHO3D_PARAM(P_CROWD_AGENT, CrowdAgent); // CrowdAgent pointer
    URHO3D_PARAM(P_INDEX, Index); // unsigned
    URHO3D_PARAM(P_SIZE, Size); // unsigned
    URHO3D_PARAM(P_POSITION, Position); // Vector3 [in/out]
}

/// Crowd agent formation specific to a node.
URHO3D_EVENT(E_CROWD_AGENT_NODE_FORMATION, CrowdAgentNodeFormation)
{
    URHO3D_PARAM(P_NODE, Node); // Node pointer
    URHO3D_PARAM(P_CROWD_AGENT, CrowdAgent); // CrowdAgent pointer
    URHO3D_PARAM(P_INDEX, Index); // unsigned
    URHO3D_PARAM(P_SIZE, Size); // unsigned
    URHO3D_PARAM(P_POSITION, Position); // Vector3 [in/out]
}

/// Crowd agent has been repositioned.
URHO3D_EVENT(E_CROWD_AGENT_REPOSITION, CrowdAgentReposition)
{
    URHO3D_PARAM(P_NODE, Node); // Node pointer
    URHO3D_PARAM(P_CROWD_AGENT, CrowdAgent); // CrowdAgent pointer
    URHO3D_PARAM(P_POSITION, Position); // Vector3
    URHO3D_PARAM(P_VELOCITY, Velocity); // Vector3
    URHO3D_PARAM(P_ARRIVED, Arrived); // bool
    URHO3D_PARAM(P_TIMESTEP, TimeStep); // float
}

/// Crowd agent has been repositioned, specific to a node.
URHO3D_EVENT(E_CROWD_AGENT_NODE_REPOSITION, CrowdAgentNodeReposition)
{
    URHO3D_PARAM(P_NODE, Node); // Node pointer
    URHO3D_PARAM(P_CROWD_AGENT, CrowdAgent); // CrowdAgent pointer
    URHO3D_PARAM(P_POSITION, Position); // Vector3
    URHO3D_PARAM(P_VELOCITY, Velocity); // Vector3
    URHO3D_PARAM(P_ARRIVED, Arrived); // bool
    URHO3D_PARAM(P_TIMESTEP, TimeStep); // float
}

/// Crowd agent's internal state has become invalidated. This is a special case of CrowdAgentStateChanged event.
URHO3D_EVENT(E_CROWD_AGENT_FAILURE, CrowdAgentFailure)
{
    URHO3D_PARAM(P_NODE, Node); // Node pointer
    URHO3D_PARAM(P_CROWD_AGENT, CrowdAgent); // CrowdAgent pointer
    URHO3D_PARAM(P_POSITION, Position); // Vector3
    URHO3D_PARAM(P_VELOCITY, Velocity); // Vector3
    URHO3D_PARAM(P_CROWD_AGENT_STATE, CrowdAgentState); // int
    URHO3D_PARAM(P_CROWD_TARGET_STATE, CrowdTargetState); // int
}

/// Crowd agent's internal state has become invalidated. This is a special case of CrowdAgentStateChanged event.
URHO3D_EVENT(E_CROWD_AGENT_NODE_FAILURE, CrowdAgentNodeFailure)
{
    URHO3D_PARAM(P_NODE, Node); // Node pointer
    URHO3D_PARAM(P_CROWD_AGENT, CrowdAgent); // CrowdAgent pointer
    URHO3D_PARAM(P_POSITION, Position); // Vector3
    URHO3D_PARAM(P_VELOCITY, Velocity); // Vector3
    URHO3D_PARAM(P_CROWD_AGENT_STATE, CrowdAgentState); // int
    URHO3D_PARAM(P_CROWD_TARGET_STATE, CrowdTargetState); // int
}

/// Crowd agent's state has been changed.
URHO3D_EVENT(E_CROWD_AGENT_STATE_CHANGED, CrowdAgentStateChanged)
{
    URHO3D_PARAM(P_NODE, Node); // Node pointer
    URHO3D_PARAM(P_CROWD_AGENT, CrowdAgent); // CrowdAgent pointer
    URHO3D_PARAM(P_POSITION, Position); // Vector3
    URHO3D_PARAM(P_VELOCITY, Velocity); // Vector3
    URHO3D_PARAM(P_CROWD_AGENT_STATE, CrowdAgentState); // int
    URHO3D_PARAM(P_CROWD_TARGET_STATE, CrowdTargetState); // int
}

/// Crowd agent's state has been changed.
URHO3D_EVENT(E_CROWD_AGENT_NODE_STATE_CHANGED, CrowdAgentNodeStateChanged)
{
    URHO3D_PARAM(P_NODE, Node); // Node pointer
    URHO3D_PARAM(P_CROWD_AGENT, CrowdAgent); // CrowdAgent pointer
    URHO3D_PARAM(P_POSITION, Position); // Vector3
    URHO3D_PARAM(P_VELOCITY, Velocity); // Vector3
    URHO3D_PARAM(P_CROWD_AGENT_STATE, CrowdAgentState); // int
    URHO3D_PARAM(P_CROWD_TARGET_STATE, CrowdTargetState); // int
}

/// Addition of obstacle to dynamic navigation mesh.
URHO3D_EVENT(E_NAVIGATION_OBSTACLE_ADDED, NavigationObstacleAdded)
{
    URHO3D_PARAM(P_NODE, Node); // Node pointer
    URHO3D_PARAM(P_OBSTACLE, Obstacle); // Obstacle pointer
    URHO3D_PARAM(P_POSITION, Position); // Vector3
    URHO3D_PARAM(P_RADIUS, Radius); // float
    URHO3D_PARAM(P_HEIGHT, Height); // float
}

/// Removal of obstacle from dynamic navigation mesh.
URHO3D_EVENT(E_NAVIGATION_OBSTACLE_REMOVED, NavigationObstacleRemoved)
{
    URHO3D_PARAM(P_NODE, Node); // Node pointer
    URHO3D_PARAM(P_OBSTACLE, Obstacle); // Obstacle pointer
    URHO3D_PARAM(P_POSITION, Position); // Vector3
    URHO3D_PARAM(P_RADIUS, Radius); // float
    URHO3D_PARAM(P_HEIGHT, Height); // float
}

}
