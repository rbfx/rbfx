%csconstvalue("0") Urho3D::HTTP_INITIALIZING;
%csattribute(Urho3D::FilteredUint, %arg(unsigned int), MinValue, GetMinValue);
%csattribute(Urho3D::FilteredUint, %arg(unsigned int), AverageValue, GetAverageValue);
%csattribute(Urho3D::FilteredUint, %arg(unsigned int), MaxValue, GetMaxValue);
%csattribute(Urho3D::FilteredUint, %arg(unsigned int), StabilizedAverageMaxValue, GetStabilizedAverageMaxValue);
%csattribute(Urho3D::FilteredUint, %arg(bool), IsInitialized, IsInitialized);
%csattribute(Urho3D::ClockSynchronizer, %arg(bool), IsReady, IsReady);
%csattribute(Urho3D::ClockSynchronizer, %arg(unsigned int), Ping, GetPing);
%csattribute(Urho3D::ClockSynchronizer, %arg(unsigned int), LocalTimeOfLatestRoundtrip, GetLocalTimeOfLatestRoundtrip);
%csattribute(Urho3D::HttpRequest, %arg(ea::string), Url, GetURL);
%csattribute(Urho3D::HttpRequest, %arg(ea::string), Verb, GetVerb);
%csattribute(Urho3D::HttpRequest, %arg(ea::string), Error, GetError);
%csattribute(Urho3D::HttpRequest, %arg(Urho3D::HttpRequestState), State, GetState);
%csattribute(Urho3D::HttpRequest, %arg(unsigned int), AvailableSize, GetAvailableSize);
%csattribute(Urho3D::HttpRequest, %arg(bool), IsOpen, IsOpen);
%csattribute(Urho3D::Network, %arg(unsigned int), UpdateFps, GetUpdateFps, SetUpdateFps);
%csattribute(Urho3D::Network, %arg(float), UpdateOvertime, GetUpdateOvertime);
%csattribute(Urho3D::Network, %arg(bool), IsUpdateNow, IsUpdateNow);
%pragma(csharp) moduleimports=%{
public static partial class E
{
    public class NetworkInputProcessedEvent {
        private StringHash _event = new StringHash("NetworkInputProcessed");
        public StringHash TimeStep = new StringHash("TimeStep");
        public NetworkInputProcessedEvent() { }
        public static implicit operator StringHash(NetworkInputProcessedEvent e) { return e._event; }
    }
    public static NetworkInputProcessedEvent NetworkInputProcessed = new NetworkInputProcessedEvent();
    public class BeginServerNetworkFrameEvent {
        private StringHash _event = new StringHash("BeginServerNetworkFrame");
        public StringHash Frame = new StringHash("Frame");
        public BeginServerNetworkFrameEvent() { }
        public static implicit operator StringHash(BeginServerNetworkFrameEvent e) { return e._event; }
    }
    public static BeginServerNetworkFrameEvent BeginServerNetworkFrame = new BeginServerNetworkFrameEvent();
    public class EndServerNetworkFrameEvent {
        private StringHash _event = new StringHash("EndServerNetworkFrame");
        public StringHash Frame = new StringHash("Frame");
        public EndServerNetworkFrameEvent() { }
        public static implicit operator StringHash(EndServerNetworkFrameEvent e) { return e._event; }
    }
    public static EndServerNetworkFrameEvent EndServerNetworkFrame = new EndServerNetworkFrameEvent();
    public class BeginClientNetworkFrameEvent {
        private StringHash _event = new StringHash("BeginClientNetworkFrame");
        public StringHash Frame = new StringHash("Frame");
        public BeginClientNetworkFrameEvent() { }
        public static implicit operator StringHash(BeginClientNetworkFrameEvent e) { return e._event; }
    }
    public static BeginClientNetworkFrameEvent BeginClientNetworkFrame = new BeginClientNetworkFrameEvent();
    public class EndClientNetworkFrameEvent {
        private StringHash _event = new StringHash("EndClientNetworkFrame");
        public StringHash Frame = new StringHash("Frame");
        public EndClientNetworkFrameEvent() { }
        public static implicit operator StringHash(EndClientNetworkFrameEvent e) { return e._event; }
    }
    public static EndClientNetworkFrameEvent EndClientNetworkFrame = new EndClientNetworkFrameEvent();
    public class NetworkUpdateEvent {
        private StringHash _event = new StringHash("NetworkUpdate");
        public StringHash IsServer = new StringHash("IsServer");
        public NetworkUpdateEvent() { }
        public static implicit operator StringHash(NetworkUpdateEvent e) { return e._event; }
    }
    public static NetworkUpdateEvent NetworkUpdate = new NetworkUpdateEvent();
    public class NetworkUpdateSentEvent {
        private StringHash _event = new StringHash("NetworkUpdateSent");
        public StringHash IsServer = new StringHash("IsServer");
        public NetworkUpdateSentEvent() { }
        public static implicit operator StringHash(NetworkUpdateSentEvent e) { return e._event; }
    }
    public static NetworkUpdateSentEvent NetworkUpdateSent = new NetworkUpdateSentEvent();
    public class NetworkHostDiscoveredEvent {
        private StringHash _event = new StringHash("NetworkHostDiscovered");
        public StringHash Address = new StringHash("Address");
        public StringHash Port = new StringHash("Port");
        public StringHash Beacon = new StringHash("Beacon");
        public NetworkHostDiscoveredEvent() { }
        public static implicit operator StringHash(NetworkHostDiscoveredEvent e) { return e._event; }
    }
    public static NetworkHostDiscoveredEvent NetworkHostDiscovered = new NetworkHostDiscoveredEvent();
} %}
