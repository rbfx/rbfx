%constant unsigned int PackageFragmentSize = Urho3D::PACKAGE_FRAGMENT_SIZE;
%ignore Urho3D::PACKAGE_FRAGMENT_SIZE;
%csconstvalue("0") Urho3D::OPSM_NONE;
%csconstvalue("0") Urho3D::HTTP_INITIALIZING;
%csattribute(Urho3D::Connection, %arg(SLNet::AddressOrGUID), AddressOrGUID, GetAddressOrGUID, SetAddressOrGUID);
%csattribute(Urho3D::Connection, %arg(Urho3D::Scene *), Scene, GetScene, SetScene);
%csattribute(Urho3D::Connection, %arg(Urho3D::Vector3), Position, GetPosition, SetPosition);
%csattribute(Urho3D::Connection, %arg(Urho3D::Quaternion), Rotation, GetRotation, SetRotation);
%csattribute(Urho3D::Connection, %arg(bool), IsClient, IsClient);
%csattribute(Urho3D::Connection, %arg(bool), IsConnected, IsConnected);
%csattribute(Urho3D::Connection, %arg(bool), IsConnectPending, IsConnectPending, SetConnectPending);
%csattribute(Urho3D::Connection, %arg(bool), IsSceneLoaded, IsSceneLoaded);
%csattribute(Urho3D::Connection, %arg(bool), LogStatistics, GetLogStatistics, SetLogStatistics);
%csattribute(Urho3D::Connection, %arg(ea::string), Address, GetAddress);
%csattribute(Urho3D::Connection, %arg(unsigned short), Port, GetPort);
%csattribute(Urho3D::Connection, %arg(float), RoundTripTime, GetRoundTripTime);
%csattribute(Urho3D::Connection, %arg(unsigned int), LastHeardTime, GetLastHeardTime);
%csattribute(Urho3D::Connection, %arg(float), BytesInPerSec, GetBytesInPerSec);
%csattribute(Urho3D::Connection, %arg(float), BytesOutPerSec, GetBytesOutPerSec);
%csattribute(Urho3D::Connection, %arg(int), PacketsInPerSec, GetPacketsInPerSec);
%csattribute(Urho3D::Connection, %arg(int), PacketsOutPerSec, GetPacketsOutPerSec);
%csattribute(Urho3D::Connection, %arg(unsigned int), NumDownloads, GetNumDownloads);
%csattribute(Urho3D::Connection, %arg(ea::string), DownloadName, GetDownloadName);
%csattribute(Urho3D::Connection, %arg(float), DownloadProgress, GetDownloadProgress);
%csattribute(Urho3D::HttpRequest, %arg(ea::string), Url, GetURL);
%csattribute(Urho3D::HttpRequest, %arg(ea::string), Verb, GetVerb);
%csattribute(Urho3D::HttpRequest, %arg(ea::string), Error, GetError);
%csattribute(Urho3D::HttpRequest, %arg(Urho3D::HttpRequestState), State, GetState);
%csattribute(Urho3D::HttpRequest, %arg(unsigned int), AvailableSize, GetAvailableSize);
%csattribute(Urho3D::HttpRequest, %arg(bool), IsOpen, IsOpen);
%csattribute(Urho3D::Network, %arg(ea::string), Guid, GetGUID);
%csattribute(Urho3D::Network, %arg(int), UpdateFps, GetUpdateFps, SetUpdateFps);
%csattribute(Urho3D::Network, %arg(int), SimulatedLatency, GetSimulatedLatency, SetSimulatedLatency);
%csattribute(Urho3D::Network, %arg(float), SimulatedPacketLoss, GetSimulatedPacketLoss, SetSimulatedPacketLoss);
%csattribute(Urho3D::Network, %arg(Urho3D::Connection *), ServerConnection, GetServerConnection);
%csattribute(Urho3D::Network, %arg(ea::vector<SharedPtr<Connection>>), ClientConnections, GetClientConnections);
%csattribute(Urho3D::Network, %arg(bool), IsServerRunning, IsServerRunning);
%csattribute(Urho3D::Network, %arg(ea::string), PackageCacheDir, GetPackageCacheDir, SetPackageCacheDir);
%csattribute(Urho3D::NetworkPriority, %arg(float), BasePriority, GetBasePriority, SetBasePriority);
%csattribute(Urho3D::NetworkPriority, %arg(float), DistanceFactor, GetDistanceFactor, SetDistanceFactor);
%csattribute(Urho3D::NetworkPriority, %arg(float), MinPriority, GetMinPriority, SetMinPriority);
%csattribute(Urho3D::NetworkPriority, %arg(bool), AlwaysUpdateOwner, GetAlwaysUpdateOwner, SetAlwaysUpdateOwner);
%pragma(csharp) moduleimports=%{
public static partial class E
{
    public class ServerConnectedEvent {
        private StringHash _event = new StringHash("ServerConnected");
        public ServerConnectedEvent() { }
        public static implicit operator StringHash(ServerConnectedEvent e) { return e._event; }
    }
    public static ServerConnectedEvent ServerConnected = new ServerConnectedEvent();
    public class ServerDisconnectedEvent {
        private StringHash _event = new StringHash("ServerDisconnected");
        public ServerDisconnectedEvent() { }
        public static implicit operator StringHash(ServerDisconnectedEvent e) { return e._event; }
    }
    public static ServerDisconnectedEvent ServerDisconnected = new ServerDisconnectedEvent();
    public class ConnectFailedEvent {
        private StringHash _event = new StringHash("ConnectFailed");
        public ConnectFailedEvent() { }
        public static implicit operator StringHash(ConnectFailedEvent e) { return e._event; }
    }
    public static ConnectFailedEvent ConnectFailed = new ConnectFailedEvent();
    public class ConnectionInProgressEvent {
        private StringHash _event = new StringHash("ConnectionInProgress");
        public ConnectionInProgressEvent() { }
        public static implicit operator StringHash(ConnectionInProgressEvent e) { return e._event; }
    }
    public static ConnectionInProgressEvent ConnectionInProgress = new ConnectionInProgressEvent();
    public class ClientConnectedEvent {
        private StringHash _event = new StringHash("ClientConnected");
        public StringHash Connection = new StringHash("Connection");
        public ClientConnectedEvent() { }
        public static implicit operator StringHash(ClientConnectedEvent e) { return e._event; }
    }
    public static ClientConnectedEvent ClientConnected = new ClientConnectedEvent();
    public class ClientDisconnectedEvent {
        private StringHash _event = new StringHash("ClientDisconnected");
        public StringHash Connection = new StringHash("Connection");
        public ClientDisconnectedEvent() { }
        public static implicit operator StringHash(ClientDisconnectedEvent e) { return e._event; }
    }
    public static ClientDisconnectedEvent ClientDisconnected = new ClientDisconnectedEvent();
    public class ClientIdentityEvent {
        private StringHash _event = new StringHash("ClientIdentity");
        public StringHash Connection = new StringHash("Connection");
        public StringHash Allow = new StringHash("Allow");
        public ClientIdentityEvent() { }
        public static implicit operator StringHash(ClientIdentityEvent e) { return e._event; }
    }
    public static ClientIdentityEvent ClientIdentity = new ClientIdentityEvent();
    public class ClientSceneLoadedEvent {
        private StringHash _event = new StringHash("ClientSceneLoaded");
        public StringHash Connection = new StringHash("Connection");
        public ClientSceneLoadedEvent() { }
        public static implicit operator StringHash(ClientSceneLoadedEvent e) { return e._event; }
    }
    public static ClientSceneLoadedEvent ClientSceneLoaded = new ClientSceneLoadedEvent();
    public class NetworkMessageEvent {
        private StringHash _event = new StringHash("NetworkMessage");
        public StringHash Connection = new StringHash("Connection");
        public StringHash MessageID = new StringHash("MessageID");
        public StringHash Data = new StringHash("Data");
        public NetworkMessageEvent() { }
        public static implicit operator StringHash(NetworkMessageEvent e) { return e._event; }
    }
    public static NetworkMessageEvent NetworkMessage = new NetworkMessageEvent();
    public class NetworkUpdateEvent {
        private StringHash _event = new StringHash("NetworkUpdate");
        public NetworkUpdateEvent() { }
        public static implicit operator StringHash(NetworkUpdateEvent e) { return e._event; }
    }
    public static NetworkUpdateEvent NetworkUpdate = new NetworkUpdateEvent();
    public class NetworkUpdateSentEvent {
        private StringHash _event = new StringHash("NetworkUpdateSent");
        public NetworkUpdateSentEvent() { }
        public static implicit operator StringHash(NetworkUpdateSentEvent e) { return e._event; }
    }
    public static NetworkUpdateSentEvent NetworkUpdateSent = new NetworkUpdateSentEvent();
    public class NetworkSceneLoadFailedEvent {
        private StringHash _event = new StringHash("NetworkSceneLoadFailed");
        public StringHash Connection = new StringHash("Connection");
        public NetworkSceneLoadFailedEvent() { }
        public static implicit operator StringHash(NetworkSceneLoadFailedEvent e) { return e._event; }
    }
    public static NetworkSceneLoadFailedEvent NetworkSceneLoadFailed = new NetworkSceneLoadFailedEvent();
    public class RemoteEventDataEvent {
        private StringHash _event = new StringHash("RemoteEventData");
        public StringHash Connection = new StringHash("Connection");
        public RemoteEventDataEvent() { }
        public static implicit operator StringHash(RemoteEventDataEvent e) { return e._event; }
    }
    public static RemoteEventDataEvent RemoteEventData = new RemoteEventDataEvent();
    public class NetworkBannedEvent {
        private StringHash _event = new StringHash("NetworkBanned");
        public NetworkBannedEvent() { }
        public static implicit operator StringHash(NetworkBannedEvent e) { return e._event; }
    }
    public static NetworkBannedEvent NetworkBanned = new NetworkBannedEvent();
    public class NetworkInvalidPasswordEvent {
        private StringHash _event = new StringHash("NetworkInvalidPassword");
        public NetworkInvalidPasswordEvent() { }
        public static implicit operator StringHash(NetworkInvalidPasswordEvent e) { return e._event; }
    }
    public static NetworkInvalidPasswordEvent NetworkInvalidPassword = new NetworkInvalidPasswordEvent();
    public class NetworkHostDiscoveredEvent {
        private StringHash _event = new StringHash("NetworkHostDiscovered");
        public StringHash Address = new StringHash("Address");
        public StringHash Port = new StringHash("Port");
        public StringHash Beacon = new StringHash("Beacon");
        public NetworkHostDiscoveredEvent() { }
        public static implicit operator StringHash(NetworkHostDiscoveredEvent e) { return e._event; }
    }
    public static NetworkHostDiscoveredEvent NetworkHostDiscovered = new NetworkHostDiscoveredEvent();
    public class NetworkNatPunchtroughSucceededEvent {
        private StringHash _event = new StringHash("NetworkNatPunchtroughSucceeded");
        public StringHash Address = new StringHash("Address");
        public StringHash Port = new StringHash("Port");
        public NetworkNatPunchtroughSucceededEvent() { }
        public static implicit operator StringHash(NetworkNatPunchtroughSucceededEvent e) { return e._event; }
    }
    public static NetworkNatPunchtroughSucceededEvent NetworkNatPunchtroughSucceeded = new NetworkNatPunchtroughSucceededEvent();
    public class NetworkNatPunchtroughFailedEvent {
        private StringHash _event = new StringHash("NetworkNatPunchtroughFailed");
        public StringHash Address = new StringHash("Address");
        public StringHash Port = new StringHash("Port");
        public NetworkNatPunchtroughFailedEvent() { }
        public static implicit operator StringHash(NetworkNatPunchtroughFailedEvent e) { return e._event; }
    }
    public static NetworkNatPunchtroughFailedEvent NetworkNatPunchtroughFailed = new NetworkNatPunchtroughFailedEvent();
    public class NetworkNatMasterConnectionFailedEvent {
        private StringHash _event = new StringHash("NetworkNatMasterConnectionFailed");
        public NetworkNatMasterConnectionFailedEvent() { }
        public static implicit operator StringHash(NetworkNatMasterConnectionFailedEvent e) { return e._event; }
    }
    public static NetworkNatMasterConnectionFailedEvent NetworkNatMasterConnectionFailed = new NetworkNatMasterConnectionFailedEvent();
    public class NetworkNatMasterConnectionSucceededEvent {
        private StringHash _event = new StringHash("NetworkNatMasterConnectionSucceeded");
        public NetworkNatMasterConnectionSucceededEvent() { }
        public static implicit operator StringHash(NetworkNatMasterConnectionSucceededEvent e) { return e._event; }
    }
    public static NetworkNatMasterConnectionSucceededEvent NetworkNatMasterConnectionSucceeded = new NetworkNatMasterConnectionSucceededEvent();
    public class NetworkNatMasterDisconnectedEvent {
        private StringHash _event = new StringHash("NetworkNatMasterDisconnected");
        public NetworkNatMasterDisconnectedEvent() { }
        public static implicit operator StringHash(NetworkNatMasterDisconnectedEvent e) { return e._event; }
    }
    public static NetworkNatMasterDisconnectedEvent NetworkNatMasterDisconnected = new NetworkNatMasterDisconnectedEvent();
} %}
