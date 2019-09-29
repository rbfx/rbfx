namespace Urho3DNet
{
    public partial class Logger
    {
        public void Trace(string message)   { Write(LogLevel.LogTrace, message); }
        public void Debug(string message)   { Write(LogLevel.LogDebug, message); }
        public void Info(string message)    { Write(LogLevel.LogInfo, message); }
        public void Warning(string message) { Write(LogLevel.LogWarning, message); }
        public void Error(string message)   { Write(LogLevel.LogError, message); }
    }
}
