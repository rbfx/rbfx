%typemap(cscode) Urho3D::Connection %{
  /*public _typemap(cstype, const SLNet::AddressOrGUID &) AddressOrGUID {
    get { return GetAddressOrGUID(); }
    set { SetAddressOrGUID(value); }
  }*/
  public $typemap(cstype, Urho3D::Scene *) Scene {
    get { return GetScene(); }
    set { SetScene(value); }
  }
  public $typemap(cstype, const Urho3D::Vector3 &) Position {
    get { return GetPosition(); }
    set { SetPosition(value); }
  }
  public $typemap(cstype, const Urho3D::Quaternion &) Rotation {
    get { return GetRotation(); }
    set { SetRotation(value); }
  }
  public $typemap(cstype, bool) LogStatistics {
    get { return GetLogStatistics(); }
    set { SetLogStatistics(value); }
  }
  public $typemap(cstype, eastl::string) Address {
    get { return GetAddress(); }
  }
  public $typemap(cstype, unsigned short) Port {
    get { return GetPort(); }
  }
  public $typemap(cstype, float) RoundTripTime {
    get { return GetRoundTripTime(); }
  }
  public $typemap(cstype, unsigned int) LastHeardTime {
    get { return GetLastHeardTime(); }
  }
  public $typemap(cstype, float) BytesInPerSec {
    get { return GetBytesInPerSec(); }
  }
  public $typemap(cstype, float) BytesOutPerSec {
    get { return GetBytesOutPerSec(); }
  }
  public $typemap(cstype, int) PacketsInPerSec {
    get { return GetPacketsInPerSec(); }
  }
  public $typemap(cstype, int) PacketsOutPerSec {
    get { return GetPacketsOutPerSec(); }
  }
  public $typemap(cstype, unsigned int) NumDownloads {
    get { return GetNumDownloads(); }
  }
  public $typemap(cstype, const eastl::string &) DownloadName {
    get { return GetDownloadName(); }
  }
  public $typemap(cstype, float) DownloadProgress {
    get { return GetDownloadProgress(); }
  }
%}
%csmethodmodifiers Urho3D::Connection::GetAddressOrGUID "private";
%csmethodmodifiers Urho3D::Connection::SetAddressOrGUID "private";
%csmethodmodifiers Urho3D::Connection::GetScene "private";
%csmethodmodifiers Urho3D::Connection::SetScene "private";
%csmethodmodifiers Urho3D::Connection::GetPosition "private";
%csmethodmodifiers Urho3D::Connection::SetPosition "private";
%csmethodmodifiers Urho3D::Connection::GetRotation "private";
%csmethodmodifiers Urho3D::Connection::SetRotation "private";
%csmethodmodifiers Urho3D::Connection::GetLogStatistics "private";
%csmethodmodifiers Urho3D::Connection::SetLogStatistics "private";
%csmethodmodifiers Urho3D::Connection::GetAddress "private";
%csmethodmodifiers Urho3D::Connection::GetPort "private";
%csmethodmodifiers Urho3D::Connection::GetRoundTripTime "private";
%csmethodmodifiers Urho3D::Connection::GetLastHeardTime "private";
%csmethodmodifiers Urho3D::Connection::GetBytesInPerSec "private";
%csmethodmodifiers Urho3D::Connection::GetBytesOutPerSec "private";
%csmethodmodifiers Urho3D::Connection::GetPacketsInPerSec "private";
%csmethodmodifiers Urho3D::Connection::GetPacketsOutPerSec "private";
%csmethodmodifiers Urho3D::Connection::GetNumDownloads "private";
%csmethodmodifiers Urho3D::Connection::GetDownloadName "private";
%csmethodmodifiers Urho3D::Connection::GetDownloadProgress "private";
%typemap(cscode) Urho3D::HttpRequest %{
  public $typemap(cstype, const eastl::string &) URL {
    get { return GetURL(); }
  }
  public $typemap(cstype, const eastl::string &) Verb {
    get { return GetVerb(); }
  }
  public $typemap(cstype, eastl::string) Error {
    get { return GetError(); }
  }
  public $typemap(cstype, Urho3D::HttpRequestState) State {
    get { return GetState(); }
  }
  public $typemap(cstype, unsigned int) AvailableSize {
    get { return GetAvailableSize(); }
  }
%}
%csmethodmodifiers Urho3D::HttpRequest::GetURL "private";
%csmethodmodifiers Urho3D::HttpRequest::GetVerb "private";
%csmethodmodifiers Urho3D::HttpRequest::GetError "private";
%csmethodmodifiers Urho3D::HttpRequest::GetState "private";
%csmethodmodifiers Urho3D::HttpRequest::GetAvailableSize "private";
%typemap(cscode) Urho3D::Network %{
  public $typemap(cstype, const eastl::string &) GUID {
    get { return GetGUID(); }
  }
  public $typemap(cstype, int) UpdateFps {
    get { return GetUpdateFps(); }
    set { SetUpdateFps(value); }
  }
  public $typemap(cstype, int) SimulatedLatency {
    get { return GetSimulatedLatency(); }
    set { SetSimulatedLatency(value); }
  }
  public $typemap(cstype, float) SimulatedPacketLoss {
    get { return GetSimulatedPacketLoss(); }
    set { SetSimulatedPacketLoss(value); }
  }
  public $typemap(cstype, Urho3D::Connection *) ServerConnection {
    get { return GetServerConnection(); }
  }
  public $typemap(cstype, eastl::vector<Urho3D::SharedPtr<Urho3D::Connection>>) ClientConnections {
    get { return GetClientConnections(); }
  }
  public $typemap(cstype, const eastl::string &) PackageCacheDir {
    get { return GetPackageCacheDir(); }
    set { SetPackageCacheDir(value); }
  }
%}
%csmethodmodifiers Urho3D::Network::GetGUID "private";
%csmethodmodifiers Urho3D::Network::GetUpdateFps "private";
%csmethodmodifiers Urho3D::Network::SetUpdateFps "private";
%csmethodmodifiers Urho3D::Network::GetSimulatedLatency "private";
%csmethodmodifiers Urho3D::Network::SetSimulatedLatency "private";
%csmethodmodifiers Urho3D::Network::GetSimulatedPacketLoss "private";
%csmethodmodifiers Urho3D::Network::SetSimulatedPacketLoss "private";
%csmethodmodifiers Urho3D::Network::GetServerConnection "private";
%csmethodmodifiers Urho3D::Network::GetClientConnections "private";
%csmethodmodifiers Urho3D::Network::GetPackageCacheDir "private";
%csmethodmodifiers Urho3D::Network::SetPackageCacheDir "private";
%typemap(cscode) Urho3D::NetworkPriority %{
  public $typemap(cstype, float) BasePriority {
    get { return GetBasePriority(); }
    set { SetBasePriority(value); }
  }
  public $typemap(cstype, float) DistanceFactor {
    get { return GetDistanceFactor(); }
    set { SetDistanceFactor(value); }
  }
  public $typemap(cstype, float) MinPriority {
    get { return GetMinPriority(); }
    set { SetMinPriority(value); }
  }
  public $typemap(cstype, bool) AlwaysUpdateOwner {
    get { return GetAlwaysUpdateOwner(); }
    set { SetAlwaysUpdateOwner(value); }
  }
%}
%csmethodmodifiers Urho3D::NetworkPriority::GetBasePriority "private";
%csmethodmodifiers Urho3D::NetworkPriority::SetBasePriority "private";
%csmethodmodifiers Urho3D::NetworkPriority::GetDistanceFactor "private";
%csmethodmodifiers Urho3D::NetworkPriority::SetDistanceFactor "private";
%csmethodmodifiers Urho3D::NetworkPriority::GetMinPriority "private";
%csmethodmodifiers Urho3D::NetworkPriority::SetMinPriority "private";
%csmethodmodifiers Urho3D::NetworkPriority::GetAlwaysUpdateOwner "private";
%csmethodmodifiers Urho3D::NetworkPriority::SetAlwaysUpdateOwner "private";
