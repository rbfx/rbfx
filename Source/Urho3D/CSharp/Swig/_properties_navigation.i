%typemap(cscode) Urho3D::CrowdAgent %{
  public $typemap(cstype, Urho3D::Vector3) Position {
    get { return GetPosition(); }
  }
  public $typemap(cstype, Urho3D::Vector3) DesiredVelocity {
    get { return GetDesiredVelocity(); }
  }
  public $typemap(cstype, Urho3D::Vector3) ActualVelocity {
    get { return GetActualVelocity(); }
  }
  public $typemap(cstype, const Urho3D::Vector3 &) TargetPosition {
    get { return GetTargetPosition(); }
    set { SetTargetPosition(value); }
  }
  public $typemap(cstype, const Urho3D::Vector3 &) TargetVelocity {
    get { return GetTargetVelocity(); }
    set { SetTargetVelocity(value); }
  }
  public $typemap(cstype, Urho3D::CrowdAgentRequestedTarget) RequestedTargetType {
    get { return GetRequestedTargetType(); }
  }
  public $typemap(cstype, Urho3D::CrowdAgentState) AgentState {
    get { return GetAgentState(); }
  }
  public $typemap(cstype, Urho3D::CrowdAgentTargetState) TargetState {
    get { return GetTargetState(); }
  }
  public $typemap(cstype, bool) UpdateNodePosition {
    get { return GetUpdateNodePosition(); }
    set { SetUpdateNodePosition(value); }
  }
  public $typemap(cstype, int) AgentCrowdId {
    get { return GetAgentCrowdId(); }
  }
  public $typemap(cstype, float) MaxAccel {
    get { return GetMaxAccel(); }
    set { SetMaxAccel(value); }
  }
  public $typemap(cstype, float) MaxSpeed {
    get { return GetMaxSpeed(); }
    set { SetMaxSpeed(value); }
  }
  public $typemap(cstype, float) Radius {
    get { return GetRadius(); }
    set { SetRadius(value); }
  }
  public $typemap(cstype, float) Height {
    get { return GetHeight(); }
    set { SetHeight(value); }
  }
  public $typemap(cstype, unsigned int) QueryFilterType {
    get { return GetQueryFilterType(); }
    set { SetQueryFilterType(value); }
  }
  public $typemap(cstype, unsigned int) ObstacleAvoidanceType {
    get { return GetObstacleAvoidanceType(); }
    set { SetObstacleAvoidanceType(value); }
  }
  public $typemap(cstype, Urho3D::NavigationQuality) NavigationQuality {
    get { return GetNavigationQuality(); }
    set { SetNavigationQuality(value); }
  }
  public $typemap(cstype, Urho3D::NavigationPushiness) NavigationPushiness {
    get { return GetNavigationPushiness(); }
    set { SetNavigationPushiness(value); }
  }
%}
%csmethodmodifiers Urho3D::CrowdAgent::GetPosition "private";
%csmethodmodifiers Urho3D::CrowdAgent::GetDesiredVelocity "private";
%csmethodmodifiers Urho3D::CrowdAgent::GetActualVelocity "private";
%csmethodmodifiers Urho3D::CrowdAgent::GetTargetPosition "private";
%csmethodmodifiers Urho3D::CrowdAgent::SetTargetPosition "private";
%csmethodmodifiers Urho3D::CrowdAgent::GetTargetVelocity "private";
%csmethodmodifiers Urho3D::CrowdAgent::SetTargetVelocity "private";
%csmethodmodifiers Urho3D::CrowdAgent::GetRequestedTargetType "private";
%csmethodmodifiers Urho3D::CrowdAgent::GetAgentState "private";
%csmethodmodifiers Urho3D::CrowdAgent::GetTargetState "private";
%csmethodmodifiers Urho3D::CrowdAgent::GetUpdateNodePosition "private";
%csmethodmodifiers Urho3D::CrowdAgent::SetUpdateNodePosition "private";
%csmethodmodifiers Urho3D::CrowdAgent::GetAgentCrowdId "private";
%csmethodmodifiers Urho3D::CrowdAgent::GetMaxAccel "private";
%csmethodmodifiers Urho3D::CrowdAgent::SetMaxAccel "private";
%csmethodmodifiers Urho3D::CrowdAgent::GetMaxSpeed "private";
%csmethodmodifiers Urho3D::CrowdAgent::SetMaxSpeed "private";
%csmethodmodifiers Urho3D::CrowdAgent::GetRadius "private";
%csmethodmodifiers Urho3D::CrowdAgent::SetRadius "private";
%csmethodmodifiers Urho3D::CrowdAgent::GetHeight "private";
%csmethodmodifiers Urho3D::CrowdAgent::SetHeight "private";
%csmethodmodifiers Urho3D::CrowdAgent::GetQueryFilterType "private";
%csmethodmodifiers Urho3D::CrowdAgent::SetQueryFilterType "private";
%csmethodmodifiers Urho3D::CrowdAgent::GetObstacleAvoidanceType "private";
%csmethodmodifiers Urho3D::CrowdAgent::SetObstacleAvoidanceType "private";
%csmethodmodifiers Urho3D::CrowdAgent::GetNavigationQuality "private";
%csmethodmodifiers Urho3D::CrowdAgent::SetNavigationQuality "private";
%csmethodmodifiers Urho3D::CrowdAgent::GetNavigationPushiness "private";
%csmethodmodifiers Urho3D::CrowdAgent::SetNavigationPushiness "private";
%typemap(cscode) Urho3D::CrowdManager %{
  public $typemap(cstype, unsigned int) MaxAgents {
    get { return GetMaxAgents(); }
    set { SetMaxAgents(value); }
  }
  public $typemap(cstype, float) MaxAgentRadius {
    get { return GetMaxAgentRadius(); }
    set { SetMaxAgentRadius(value); }
  }
  public $typemap(cstype, Urho3D::NavigationMesh *) NavigationMesh {
    get { return GetNavigationMesh(); }
    set { SetNavigationMesh(value); }
  }
  public $typemap(cstype, unsigned int) NumQueryFilterTypes {
    get { return GetNumQueryFilterTypes(); }
  }
  public $typemap(cstype, eastl::vector<Urho3D::Variant>) QueryFilterTypesAttr {
    get { return GetQueryFilterTypesAttr(); }
  }
  public $typemap(cstype, unsigned int) NumObstacleAvoidanceTypes {
    get { return GetNumObstacleAvoidanceTypes(); }
  }
  public $typemap(cstype, eastl::vector<Urho3D::Variant>) ObstacleAvoidanceTypesAttr {
    get { return GetObstacleAvoidanceTypesAttr(); }
  }
%}
%csmethodmodifiers Urho3D::CrowdManager::GetMaxAgents "private";
%csmethodmodifiers Urho3D::CrowdManager::SetMaxAgents "private";
%csmethodmodifiers Urho3D::CrowdManager::GetMaxAgentRadius "private";
%csmethodmodifiers Urho3D::CrowdManager::SetMaxAgentRadius "private";
%csmethodmodifiers Urho3D::CrowdManager::GetNavigationMesh "private";
%csmethodmodifiers Urho3D::CrowdManager::SetNavigationMesh "private";
%csmethodmodifiers Urho3D::CrowdManager::GetNumQueryFilterTypes "private";
%csmethodmodifiers Urho3D::CrowdManager::GetQueryFilterTypesAttr "private";
%csmethodmodifiers Urho3D::CrowdManager::GetNumObstacleAvoidanceTypes "private";
%csmethodmodifiers Urho3D::CrowdManager::GetObstacleAvoidanceTypesAttr "private";
%typemap(cscode) Urho3D::DynamicNavigationMesh %{
  public $typemap(cstype, unsigned int) MaxObstacles {
    get { return GetMaxObstacles(); }
    set { SetMaxObstacles(value); }
  }
  public $typemap(cstype, unsigned int) MaxLayers {
    get { return GetMaxLayers(); }
    set { SetMaxLayers(value); }
  }
  public $typemap(cstype, bool) DrawObstacles {
    get { return GetDrawObstacles(); }
    set { SetDrawObstacles(value); }
  }
%}
%csmethodmodifiers Urho3D::DynamicNavigationMesh::GetMaxObstacles "private";
%csmethodmodifiers Urho3D::DynamicNavigationMesh::SetMaxObstacles "private";
%csmethodmodifiers Urho3D::DynamicNavigationMesh::GetMaxLayers "private";
%csmethodmodifiers Urho3D::DynamicNavigationMesh::SetMaxLayers "private";
%csmethodmodifiers Urho3D::DynamicNavigationMesh::GetDrawObstacles "private";
%csmethodmodifiers Urho3D::DynamicNavigationMesh::SetDrawObstacles "private";
%typemap(cscode) Urho3D::NavArea %{
  public $typemap(cstype, unsigned int) AreaID {
    get { return GetAreaID(); }
    set { SetAreaID(value); }
  }
  public $typemap(cstype, Urho3D::BoundingBox) BoundingBox {
    get { return GetBoundingBox(); }
  }
  public $typemap(cstype, Urho3D::BoundingBox) WorldBoundingBox {
    get { return GetWorldBoundingBox(); }
  }
%}
%csmethodmodifiers Urho3D::NavArea::GetAreaID "private";
%csmethodmodifiers Urho3D::NavArea::SetAreaID "private";
%csmethodmodifiers Urho3D::NavArea::GetBoundingBox "private";
%csmethodmodifiers Urho3D::NavArea::GetWorldBoundingBox "private";
%typemap(cscode) Urho3D::NavigationMesh %{
  public $typemap(cstype, eastl::string) MeshName {
    get { return GetMeshName(); }
  }
  public $typemap(cstype, int) TileSize {
    get { return GetTileSize(); }
    set { SetTileSize(value); }
  }
  public $typemap(cstype, float) CellSize {
    get { return GetCellSize(); }
    set { SetCellSize(value); }
  }
  public $typemap(cstype, float) CellHeight {
    get { return GetCellHeight(); }
    set { SetCellHeight(value); }
  }
  public $typemap(cstype, float) AgentHeight {
    get { return GetAgentHeight(); }
    set { SetAgentHeight(value); }
  }
  public $typemap(cstype, float) AgentRadius {
    get { return GetAgentRadius(); }
    set { SetAgentRadius(value); }
  }
  public $typemap(cstype, float) AgentMaxClimb {
    get { return GetAgentMaxClimb(); }
    set { SetAgentMaxClimb(value); }
  }
  public $typemap(cstype, float) AgentMaxSlope {
    get { return GetAgentMaxSlope(); }
    set { SetAgentMaxSlope(value); }
  }
  public $typemap(cstype, float) RegionMinSize {
    get { return GetRegionMinSize(); }
    set { SetRegionMinSize(value); }
  }
  public $typemap(cstype, float) RegionMergeSize {
    get { return GetRegionMergeSize(); }
    set { SetRegionMergeSize(value); }
  }
  public $typemap(cstype, float) EdgeMaxLength {
    get { return GetEdgeMaxLength(); }
    set { SetEdgeMaxLength(value); }
  }
  public $typemap(cstype, float) EdgeMaxError {
    get { return GetEdgeMaxError(); }
    set { SetEdgeMaxError(value); }
  }
  public $typemap(cstype, float) DetailSampleDistance {
    get { return GetDetailSampleDistance(); }
    set { SetDetailSampleDistance(value); }
  }
  public $typemap(cstype, float) DetailSampleMaxError {
    get { return GetDetailSampleMaxError(); }
    set { SetDetailSampleMaxError(value); }
  }
  public $typemap(cstype, const Urho3D::Vector3 &) Padding {
    get { return GetPadding(); }
    set { SetPadding(value); }
  }
  public $typemap(cstype, const Urho3D::BoundingBox &) BoundingBox {
    get { return GetBoundingBox(); }
  }
  public $typemap(cstype, Urho3D::BoundingBox) WorldBoundingBox {
    get { return GetWorldBoundingBox(); }
  }
  public $typemap(cstype, Urho3D::IntVector2) NumTiles {
    get { return GetNumTiles(); }
  }
  public $typemap(cstype, Urho3D::NavmeshPartitionType) PartitionType {
    get { return GetPartitionType(); }
    set { SetPartitionType(value); }
  }
  public $typemap(cstype, bool) DrawOffMeshConnections {
    get { return GetDrawOffMeshConnections(); }
    set { SetDrawOffMeshConnections(value); }
  }
  public $typemap(cstype, bool) DrawNavAreas {
    get { return GetDrawNavAreas(); }
    set { SetDrawNavAreas(value); }
  }
%}
%csmethodmodifiers Urho3D::NavigationMesh::GetMeshName "private";
%csmethodmodifiers Urho3D::NavigationMesh::GetTileSize "private";
%csmethodmodifiers Urho3D::NavigationMesh::SetTileSize "private";
%csmethodmodifiers Urho3D::NavigationMesh::GetCellSize "private";
%csmethodmodifiers Urho3D::NavigationMesh::SetCellSize "private";
%csmethodmodifiers Urho3D::NavigationMesh::GetCellHeight "private";
%csmethodmodifiers Urho3D::NavigationMesh::SetCellHeight "private";
%csmethodmodifiers Urho3D::NavigationMesh::GetAgentHeight "private";
%csmethodmodifiers Urho3D::NavigationMesh::SetAgentHeight "private";
%csmethodmodifiers Urho3D::NavigationMesh::GetAgentRadius "private";
%csmethodmodifiers Urho3D::NavigationMesh::SetAgentRadius "private";
%csmethodmodifiers Urho3D::NavigationMesh::GetAgentMaxClimb "private";
%csmethodmodifiers Urho3D::NavigationMesh::SetAgentMaxClimb "private";
%csmethodmodifiers Urho3D::NavigationMesh::GetAgentMaxSlope "private";
%csmethodmodifiers Urho3D::NavigationMesh::SetAgentMaxSlope "private";
%csmethodmodifiers Urho3D::NavigationMesh::GetRegionMinSize "private";
%csmethodmodifiers Urho3D::NavigationMesh::SetRegionMinSize "private";
%csmethodmodifiers Urho3D::NavigationMesh::GetRegionMergeSize "private";
%csmethodmodifiers Urho3D::NavigationMesh::SetRegionMergeSize "private";
%csmethodmodifiers Urho3D::NavigationMesh::GetEdgeMaxLength "private";
%csmethodmodifiers Urho3D::NavigationMesh::SetEdgeMaxLength "private";
%csmethodmodifiers Urho3D::NavigationMesh::GetEdgeMaxError "private";
%csmethodmodifiers Urho3D::NavigationMesh::SetEdgeMaxError "private";
%csmethodmodifiers Urho3D::NavigationMesh::GetDetailSampleDistance "private";
%csmethodmodifiers Urho3D::NavigationMesh::SetDetailSampleDistance "private";
%csmethodmodifiers Urho3D::NavigationMesh::GetDetailSampleMaxError "private";
%csmethodmodifiers Urho3D::NavigationMesh::SetDetailSampleMaxError "private";
%csmethodmodifiers Urho3D::NavigationMesh::GetPadding "private";
%csmethodmodifiers Urho3D::NavigationMesh::SetPadding "private";
%csmethodmodifiers Urho3D::NavigationMesh::GetBoundingBox "private";
%csmethodmodifiers Urho3D::NavigationMesh::GetWorldBoundingBox "private";
%csmethodmodifiers Urho3D::NavigationMesh::GetNumTiles "private";
%csmethodmodifiers Urho3D::NavigationMesh::GetPartitionType "private";
%csmethodmodifiers Urho3D::NavigationMesh::SetPartitionType "private";
%csmethodmodifiers Urho3D::NavigationMesh::GetDrawOffMeshConnections "private";
%csmethodmodifiers Urho3D::NavigationMesh::SetDrawOffMeshConnections "private";
%csmethodmodifiers Urho3D::NavigationMesh::GetDrawNavAreas "private";
%csmethodmodifiers Urho3D::NavigationMesh::SetDrawNavAreas "private";
%typemap(cscode) Urho3D::Obstacle %{
  public $typemap(cstype, float) Height {
    get { return GetHeight(); }
    set { SetHeight(value); }
  }
  public $typemap(cstype, float) Radius {
    get { return GetRadius(); }
    set { SetRadius(value); }
  }
  public $typemap(cstype, unsigned int) ObstacleID {
    get { return GetObstacleID(); }
  }
%}
%csmethodmodifiers Urho3D::Obstacle::GetHeight "private";
%csmethodmodifiers Urho3D::Obstacle::SetHeight "private";
%csmethodmodifiers Urho3D::Obstacle::GetRadius "private";
%csmethodmodifiers Urho3D::Obstacle::SetRadius "private";
%csmethodmodifiers Urho3D::Obstacle::GetObstacleID "private";
%typemap(cscode) Urho3D::OffMeshConnection %{
  public $typemap(cstype, Urho3D::Node *) EndPoint {
    get { return GetEndPoint(); }
    set { SetEndPoint(value); }
  }
  public $typemap(cstype, float) Radius {
    get { return GetRadius(); }
    set { SetRadius(value); }
  }
  public $typemap(cstype, unsigned int) Mask {
    get { return GetMask(); }
    set { SetMask(value); }
  }
  public $typemap(cstype, unsigned int) AreaID {
    get { return GetAreaID(); }
    set { SetAreaID(value); }
  }
%}
%csmethodmodifiers Urho3D::OffMeshConnection::GetEndPoint "private";
%csmethodmodifiers Urho3D::OffMeshConnection::SetEndPoint "private";
%csmethodmodifiers Urho3D::OffMeshConnection::GetRadius "private";
%csmethodmodifiers Urho3D::OffMeshConnection::SetRadius "private";
%csmethodmodifiers Urho3D::OffMeshConnection::GetMask "private";
%csmethodmodifiers Urho3D::OffMeshConnection::SetMask "private";
%csmethodmodifiers Urho3D::OffMeshConnection::GetAreaID "private";
%csmethodmodifiers Urho3D::OffMeshConnection::SetAreaID "private";
