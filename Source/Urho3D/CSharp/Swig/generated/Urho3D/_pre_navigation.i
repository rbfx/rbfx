%csconstvalue("0") Urho3D::CA_REQUESTEDTARGET_NONE;
%csconstvalue("0") Urho3D::CA_TARGET_NONE;
%csconstvalue("0") Urho3D::NAVIGATIONQUALITY_LOW;
%csconstvalue("1") Urho3D::NAVIGATIONQUALITY_MEDIUM;
%csconstvalue("2") Urho3D::NAVIGATIONQUALITY_HIGH;
%csconstvalue("0") Urho3D::NAVIGATIONPUSHINESS_LOW;
%csconstvalue("0") Urho3D::NAVMESH_PARTITION_WATERSHED;
%csconstvalue("0") Urho3D::NAVPATHFLAG_NONE;
%csconstvalue("1") Urho3D::NAVPATHFLAG_START;
%csconstvalue("2") Urho3D::NAVPATHFLAG_END;
%csconstvalue("4") Urho3D::NAVPATHFLAG_OFF_MESH;
%csattribute(Urho3D::CrowdManager, %arg(unsigned int), MaxAgents, GetMaxAgents, SetMaxAgents);
%csattribute(Urho3D::CrowdManager, %arg(float), MaxAgentRadius, GetMaxAgentRadius, SetMaxAgentRadius);
%csattribute(Urho3D::CrowdManager, %arg(Urho3D::NavigationMesh *), NavigationMesh, GetNavigationMesh, SetNavigationMesh);
%csattribute(Urho3D::CrowdManager, %arg(unsigned int), NumQueryFilterTypes, GetNumQueryFilterTypes);
%csattribute(Urho3D::CrowdManager, %arg(Urho3D::VariantVector), QueryFilterTypesAttr, GetQueryFilterTypesAttr, SetQueryFilterTypesAttr);
%csattribute(Urho3D::CrowdManager, %arg(unsigned int), NumObstacleAvoidanceTypes, GetNumObstacleAvoidanceTypes);
%csattribute(Urho3D::CrowdManager, %arg(Urho3D::VariantVector), ObstacleAvoidanceTypesAttr, GetObstacleAvoidanceTypesAttr, SetObstacleAvoidanceTypesAttr);
%csattribute(Urho3D::CrowdAgent, %arg(Urho3D::CrowdAgentVelocityCallback), VelocityCallback, GetVelocityCallback, SetVelocityCallback);
%csattribute(Urho3D::CrowdAgent, %arg(Urho3D::CrowdAgentHeightCallback), HeightCallback, GetHeightCallback, SetHeightCallback);
%csattribute(Urho3D::CrowdAgent, %arg(Urho3D::Vector3), Position, GetPosition);
%csattribute(Urho3D::CrowdAgent, %arg(Urho3D::Vector3), DesiredVelocity, GetDesiredVelocity);
%csattribute(Urho3D::CrowdAgent, %arg(Urho3D::Vector3), ActualVelocity, GetActualVelocity);
%csattribute(Urho3D::CrowdAgent, %arg(Urho3D::Vector3), TargetPosition, GetTargetPosition, SetTargetPosition);
%csattribute(Urho3D::CrowdAgent, %arg(Urho3D::Vector3), TargetVelocity, GetTargetVelocity, SetTargetVelocity);
%csattribute(Urho3D::CrowdAgent, %arg(Urho3D::CrowdAgentRequestedTarget), RequestedTargetType, GetRequestedTargetType);
%csattribute(Urho3D::CrowdAgent, %arg(Urho3D::CrowdAgentState), AgentState, GetAgentState);
%csattribute(Urho3D::CrowdAgent, %arg(Urho3D::CrowdAgentTargetState), TargetState, GetTargetState);
%csattribute(Urho3D::CrowdAgent, %arg(bool), UpdateNodePosition, GetUpdateNodePosition, SetUpdateNodePosition);
%csattribute(Urho3D::CrowdAgent, %arg(int), AgentCrowdId, GetAgentCrowdId);
%csattribute(Urho3D::CrowdAgent, %arg(float), MaxAccel, GetMaxAccel, SetMaxAccel);
%csattribute(Urho3D::CrowdAgent, %arg(float), MaxSpeed, GetMaxSpeed, SetMaxSpeed);
%csattribute(Urho3D::CrowdAgent, %arg(float), Radius, GetRadius, SetRadius);
%csattribute(Urho3D::CrowdAgent, %arg(float), Height, GetHeight, SetHeight);
%csattribute(Urho3D::CrowdAgent, %arg(unsigned int), QueryFilterType, GetQueryFilterType, SetQueryFilterType);
%csattribute(Urho3D::CrowdAgent, %arg(unsigned int), ObstacleAvoidanceType, GetObstacleAvoidanceType, SetObstacleAvoidanceType);
%csattribute(Urho3D::CrowdAgent, %arg(Urho3D::NavigationQuality), NavigationQuality, GetNavigationQuality, SetNavigationQuality);
%csattribute(Urho3D::CrowdAgent, %arg(Urho3D::NavigationPushiness), NavigationPushiness, GetNavigationPushiness, SetNavigationPushiness);
%csattribute(Urho3D::CrowdAgent, %arg(bool), IsInCrowd, IsInCrowd);
%csattribute(Urho3D::NavigationMesh, %arg(ea::vector<IntVector2>), AllTileIndices, GetAllTileIndices);
%csattribute(Urho3D::NavigationMesh, %arg(ea::string), MeshName, GetMeshName, SetMeshName);
%csattribute(Urho3D::NavigationMesh, %arg(int), MaxTiles, GetMaxTiles, SetMaxTiles);
%csattribute(Urho3D::NavigationMesh, %arg(int), TileSize, GetTileSize, SetTileSize);
%csattribute(Urho3D::NavigationMesh, %arg(float), CellSize, GetCellSize, SetCellSize);
%csattribute(Urho3D::NavigationMesh, %arg(float), CellHeight, GetCellHeight, SetCellHeight);
%csattribute(Urho3D::NavigationMesh, %arg(Urho3D::Vector2), HeightRange, GetHeightRange, SetHeightRange);
%csattribute(Urho3D::NavigationMesh, %arg(bool), IsHeightRangeValid, IsHeightRangeValid);
%csattribute(Urho3D::NavigationMesh, %arg(float), AgentHeight, GetAgentHeight, SetAgentHeight);
%csattribute(Urho3D::NavigationMesh, %arg(float), AgentRadius, GetAgentRadius, SetAgentRadius);
%csattribute(Urho3D::NavigationMesh, %arg(float), AgentMaxClimb, GetAgentMaxClimb, SetAgentMaxClimb);
%csattribute(Urho3D::NavigationMesh, %arg(float), AgentMaxSlope, GetAgentMaxSlope, SetAgentMaxSlope);
%csattribute(Urho3D::NavigationMesh, %arg(float), RegionMinSize, GetRegionMinSize, SetRegionMinSize);
%csattribute(Urho3D::NavigationMesh, %arg(float), RegionMergeSize, GetRegionMergeSize, SetRegionMergeSize);
%csattribute(Urho3D::NavigationMesh, %arg(float), EdgeMaxLength, GetEdgeMaxLength, SetEdgeMaxLength);
%csattribute(Urho3D::NavigationMesh, %arg(float), EdgeMaxError, GetEdgeMaxError, SetEdgeMaxError);
%csattribute(Urho3D::NavigationMesh, %arg(float), DetailSampleDistance, GetDetailSampleDistance, SetDetailSampleDistance);
%csattribute(Urho3D::NavigationMesh, %arg(float), DetailSampleMaxError, GetDetailSampleMaxError, SetDetailSampleMaxError);
%csattribute(Urho3D::NavigationMesh, %arg(Urho3D::Vector3), Padding, GetPadding, SetPadding);
%csattribute(Urho3D::NavigationMesh, %arg(bool), IsInitialized, IsInitialized);
%csattribute(Urho3D::NavigationMesh, %arg(Urho3D::NavmeshPartitionType), PartitionType, GetPartitionType, SetPartitionType);
%csattribute(Urho3D::NavigationMesh, %arg(bool), DrawOffMeshConnections, GetDrawOffMeshConnections, SetDrawOffMeshConnections);
%csattribute(Urho3D::NavigationMesh, %arg(bool), DrawNavAreas, GetDrawNavAreas, SetDrawNavAreas);
%csattribute(Urho3D::DynamicNavigationMesh, %arg(ea::vector<unsigned char>), NavigationDataAttr, GetNavigationDataAttr, SetNavigationDataAttr);
%csattribute(Urho3D::DynamicNavigationMesh, %arg(unsigned int), MaxObstacles, GetMaxObstacles, SetMaxObstacles);
%csattribute(Urho3D::DynamicNavigationMesh, %arg(unsigned int), MaxLayers, GetMaxLayers, SetMaxLayers);
%csattribute(Urho3D::DynamicNavigationMesh, %arg(bool), DrawObstacles, GetDrawObstacles, SetDrawObstacles);
%csattribute(Urho3D::NavArea, %arg(unsigned int), AreaID, GetAreaID, SetAreaID);
%csattribute(Urho3D::NavArea, %arg(Urho3D::BoundingBox), BoundingBox, GetBoundingBox, SetBoundingBox);
%csattribute(Urho3D::NavArea, %arg(Urho3D::BoundingBox), WorldBoundingBox, GetWorldBoundingBox);
%csattribute(Urho3D::Navigable, %arg(bool), IsRecursive, IsRecursive, SetRecursive);
%csattribute(Urho3D::Obstacle, %arg(float), Height, GetHeight, SetHeight);
%csattribute(Urho3D::Obstacle, %arg(float), Radius, GetRadius, SetRadius);
%csattribute(Urho3D::Obstacle, %arg(unsigned int), ObstacleID, GetObstacleID);
%csattribute(Urho3D::OffMeshConnection, %arg(Urho3D::Node *), EndPoint, GetEndPoint, SetEndPoint);
%csattribute(Urho3D::OffMeshConnection, %arg(float), Radius, GetRadius, SetRadius);
%csattribute(Urho3D::OffMeshConnection, %arg(bool), IsBidirectional, IsBidirectional, SetBidirectional);
%csattribute(Urho3D::OffMeshConnection, %arg(unsigned int), Mask, GetMask, SetMask);
%csattribute(Urho3D::OffMeshConnection, %arg(unsigned int), AreaID, GetAreaID, SetAreaID);
%pragma(csharp) moduleimports=%{
public static partial class E
{
    public class NavigationMeshRebuiltEvent {
        private StringHash _event = new StringHash("NavigationMeshRebuilt");
        public StringHash Node = new StringHash("Node");
        public StringHash Mesh = new StringHash("Mesh");
        public NavigationMeshRebuiltEvent() { }
        public static implicit operator StringHash(NavigationMeshRebuiltEvent e) { return e._event; }
    }
    public static NavigationMeshRebuiltEvent NavigationMeshRebuilt = new NavigationMeshRebuiltEvent();
    public class NavigationAreaRebuiltEvent {
        private StringHash _event = new StringHash("NavigationAreaRebuilt");
        public StringHash Node = new StringHash("Node");
        public StringHash Mesh = new StringHash("Mesh");
        public StringHash BoundsMin = new StringHash("BoundsMin");
        public StringHash BoundsMax = new StringHash("BoundsMax");
        public NavigationAreaRebuiltEvent() { }
        public static implicit operator StringHash(NavigationAreaRebuiltEvent e) { return e._event; }
    }
    public static NavigationAreaRebuiltEvent NavigationAreaRebuilt = new NavigationAreaRebuiltEvent();
    public class NavigationTileAddedEvent {
        private StringHash _event = new StringHash("NavigationTileAdded");
        public StringHash Node = new StringHash("Node");
        public StringHash Mesh = new StringHash("Mesh");
        public StringHash Tile = new StringHash("Tile");
        public NavigationTileAddedEvent() { }
        public static implicit operator StringHash(NavigationTileAddedEvent e) { return e._event; }
    }
    public static NavigationTileAddedEvent NavigationTileAdded = new NavigationTileAddedEvent();
    public class NavigationTileRemovedEvent {
        private StringHash _event = new StringHash("NavigationTileRemoved");
        public StringHash Node = new StringHash("Node");
        public StringHash Mesh = new StringHash("Mesh");
        public StringHash Tile = new StringHash("Tile");
        public NavigationTileRemovedEvent() { }
        public static implicit operator StringHash(NavigationTileRemovedEvent e) { return e._event; }
    }
    public static NavigationTileRemovedEvent NavigationTileRemoved = new NavigationTileRemovedEvent();
    public class NavigationAllTilesRemovedEvent {
        private StringHash _event = new StringHash("NavigationAllTilesRemoved");
        public StringHash Node = new StringHash("Node");
        public StringHash Mesh = new StringHash("Mesh");
        public NavigationAllTilesRemovedEvent() { }
        public static implicit operator StringHash(NavigationAllTilesRemovedEvent e) { return e._event; }
    }
    public static NavigationAllTilesRemovedEvent NavigationAllTilesRemoved = new NavigationAllTilesRemovedEvent();
    public class CrowdAgentFormationEvent {
        private StringHash _event = new StringHash("CrowdAgentFormation");
        public StringHash Node = new StringHash("Node");
        public StringHash CrowdAgent = new StringHash("CrowdAgent");
        public StringHash Index = new StringHash("Index");
        public StringHash Size = new StringHash("Size");
        public StringHash Position = new StringHash("Position");
        public CrowdAgentFormationEvent() { }
        public static implicit operator StringHash(CrowdAgentFormationEvent e) { return e._event; }
    }
    public static CrowdAgentFormationEvent CrowdAgentFormation = new CrowdAgentFormationEvent();
    public class CrowdAgentNodeFormationEvent {
        private StringHash _event = new StringHash("CrowdAgentNodeFormation");
        public StringHash Node = new StringHash("Node");
        public StringHash CrowdAgent = new StringHash("CrowdAgent");
        public StringHash Index = new StringHash("Index");
        public StringHash Size = new StringHash("Size");
        public StringHash Position = new StringHash("Position");
        public CrowdAgentNodeFormationEvent() { }
        public static implicit operator StringHash(CrowdAgentNodeFormationEvent e) { return e._event; }
    }
    public static CrowdAgentNodeFormationEvent CrowdAgentNodeFormation = new CrowdAgentNodeFormationEvent();
    public class CrowdAgentRepositionEvent {
        private StringHash _event = new StringHash("CrowdAgentReposition");
        public StringHash Node = new StringHash("Node");
        public StringHash CrowdAgent = new StringHash("CrowdAgent");
        public StringHash Position = new StringHash("Position");
        public StringHash Velocity = new StringHash("Velocity");
        public StringHash Arrived = new StringHash("Arrived");
        public StringHash TimeStep = new StringHash("TimeStep");
        public CrowdAgentRepositionEvent() { }
        public static implicit operator StringHash(CrowdAgentRepositionEvent e) { return e._event; }
    }
    public static CrowdAgentRepositionEvent CrowdAgentReposition = new CrowdAgentRepositionEvent();
    public class CrowdAgentNodeRepositionEvent {
        private StringHash _event = new StringHash("CrowdAgentNodeReposition");
        public StringHash Node = new StringHash("Node");
        public StringHash CrowdAgent = new StringHash("CrowdAgent");
        public StringHash Position = new StringHash("Position");
        public StringHash Velocity = new StringHash("Velocity");
        public StringHash Arrived = new StringHash("Arrived");
        public StringHash TimeStep = new StringHash("TimeStep");
        public CrowdAgentNodeRepositionEvent() { }
        public static implicit operator StringHash(CrowdAgentNodeRepositionEvent e) { return e._event; }
    }
    public static CrowdAgentNodeRepositionEvent CrowdAgentNodeReposition = new CrowdAgentNodeRepositionEvent();
    public class CrowdAgentFailureEvent {
        private StringHash _event = new StringHash("CrowdAgentFailure");
        public StringHash Node = new StringHash("Node");
        public StringHash CrowdAgent = new StringHash("CrowdAgent");
        public StringHash Position = new StringHash("Position");
        public StringHash Velocity = new StringHash("Velocity");
        public StringHash CrowdAgentState = new StringHash("CrowdAgentState");
        public StringHash CrowdTargetState = new StringHash("CrowdTargetState");
        public CrowdAgentFailureEvent() { }
        public static implicit operator StringHash(CrowdAgentFailureEvent e) { return e._event; }
    }
    public static CrowdAgentFailureEvent CrowdAgentFailure = new CrowdAgentFailureEvent();
    public class CrowdAgentNodeFailureEvent {
        private StringHash _event = new StringHash("CrowdAgentNodeFailure");
        public StringHash Node = new StringHash("Node");
        public StringHash CrowdAgent = new StringHash("CrowdAgent");
        public StringHash Position = new StringHash("Position");
        public StringHash Velocity = new StringHash("Velocity");
        public StringHash CrowdAgentState = new StringHash("CrowdAgentState");
        public StringHash CrowdTargetState = new StringHash("CrowdTargetState");
        public CrowdAgentNodeFailureEvent() { }
        public static implicit operator StringHash(CrowdAgentNodeFailureEvent e) { return e._event; }
    }
    public static CrowdAgentNodeFailureEvent CrowdAgentNodeFailure = new CrowdAgentNodeFailureEvent();
    public class CrowdAgentStateChangedEvent {
        private StringHash _event = new StringHash("CrowdAgentStateChanged");
        public StringHash Node = new StringHash("Node");
        public StringHash CrowdAgent = new StringHash("CrowdAgent");
        public StringHash Position = new StringHash("Position");
        public StringHash Velocity = new StringHash("Velocity");
        public StringHash CrowdAgentState = new StringHash("CrowdAgentState");
        public StringHash CrowdTargetState = new StringHash("CrowdTargetState");
        public CrowdAgentStateChangedEvent() { }
        public static implicit operator StringHash(CrowdAgentStateChangedEvent e) { return e._event; }
    }
    public static CrowdAgentStateChangedEvent CrowdAgentStateChanged = new CrowdAgentStateChangedEvent();
    public class CrowdAgentNodeStateChangedEvent {
        private StringHash _event = new StringHash("CrowdAgentNodeStateChanged");
        public StringHash Node = new StringHash("Node");
        public StringHash CrowdAgent = new StringHash("CrowdAgent");
        public StringHash Position = new StringHash("Position");
        public StringHash Velocity = new StringHash("Velocity");
        public StringHash CrowdAgentState = new StringHash("CrowdAgentState");
        public StringHash CrowdTargetState = new StringHash("CrowdTargetState");
        public CrowdAgentNodeStateChangedEvent() { }
        public static implicit operator StringHash(CrowdAgentNodeStateChangedEvent e) { return e._event; }
    }
    public static CrowdAgentNodeStateChangedEvent CrowdAgentNodeStateChanged = new CrowdAgentNodeStateChangedEvent();
    public class NavigationObstacleAddedEvent {
        private StringHash _event = new StringHash("NavigationObstacleAdded");
        public StringHash Node = new StringHash("Node");
        public StringHash Obstacle = new StringHash("Obstacle");
        public StringHash Position = new StringHash("Position");
        public StringHash Radius = new StringHash("Radius");
        public StringHash Height = new StringHash("Height");
        public NavigationObstacleAddedEvent() { }
        public static implicit operator StringHash(NavigationObstacleAddedEvent e) { return e._event; }
    }
    public static NavigationObstacleAddedEvent NavigationObstacleAdded = new NavigationObstacleAddedEvent();
    public class NavigationObstacleRemovedEvent {
        private StringHash _event = new StringHash("NavigationObstacleRemoved");
        public StringHash Node = new StringHash("Node");
        public StringHash Obstacle = new StringHash("Obstacle");
        public StringHash Position = new StringHash("Position");
        public StringHash Radius = new StringHash("Radius");
        public StringHash Height = new StringHash("Height");
        public NavigationObstacleRemovedEvent() { }
        public static implicit operator StringHash(NavigationObstacleRemovedEvent e) { return e._event; }
    }
    public static NavigationObstacleRemovedEvent NavigationObstacleRemoved = new NavigationObstacleRemovedEvent();
} %}
