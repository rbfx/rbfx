// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
