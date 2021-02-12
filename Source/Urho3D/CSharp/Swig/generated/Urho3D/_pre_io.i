%constant char * Apk = Urho3D::APK;
%ignore Urho3D::APK;
%constant unsigned int ScanFiles = Urho3D::SCAN_FILES;
%ignore Urho3D::SCAN_FILES;
%constant unsigned int ScanDirs = Urho3D::SCAN_DIRS;
%ignore Urho3D::SCAN_DIRS;
%constant unsigned int ScanHidden = Urho3D::SCAN_HIDDEN;
%ignore Urho3D::SCAN_HIDDEN;
%constant char * NullDevice = Urho3D::NULL_DEVICE;
%ignore Urho3D::NULL_DEVICE;
URHO3D_REFCOUNTED(Urho3D::File);
URHO3D_REFCOUNTED(Urho3D::FileSystem);
URHO3D_REFCOUNTED(Urho3D::FileWatcher);
URHO3D_REFCOUNTED(Urho3D::Log);
URHO3D_REFCOUNTED(Urho3D::MultiFileWatcher);
URHO3D_REFCOUNTED(Urho3D::NamedPipe);
URHO3D_REFCOUNTED(Urho3D::PackageFile);
%csconstvalue("0") Urho3D::FILE_READ;
%csattribute(Urho3D::ArchiveBlock, %arg(unsigned int), SizeHint, GetSizeHint);
%csattribute(Urho3D::ArchiveBase, %arg(Urho3D::Context *), Context, GetContext);
%csattribute(Urho3D::ArchiveBase, %arg(ea::string_view), Name, GetName);
%csattribute(Urho3D::ArchiveBase, %arg(unsigned int), Checksum, GetChecksum);
%csattribute(Urho3D::ArchiveBase, %arg(bool), Eof, IsEOF);
%csattribute(Urho3D::ArchiveBase, %arg(ea::string_view), ErrorString, GetErrorString);
%csattribute(Urho3D::ArchiveBase, %arg(ea::string_view), ErrorStack, GetErrorStack);
%csattribute(Urho3D::BinaryOutputArchiveBlock, %arg(ea::string_view), Name, GetName);
%csattribute(Urho3D::BinaryOutputArchiveBlock, %arg(Urho3D::Serializer *), Serializer, GetSerializer);
%csattribute(Urho3D::BinaryOutputArchive, %arg(ea::string_view), Name, GetName);
%csattribute(Urho3D::BinaryOutputArchive, %arg(unsigned int), Checksum, GetChecksum);
%csattribute(Urho3D::BinaryInputArchiveBlock, %arg(ea::string_view), Name, GetName);
%csattribute(Urho3D::BinaryInputArchiveBlock, %arg(unsigned int), NextElementPosition, GetNextElementPosition);
%csattribute(Urho3D::BinaryInputArchive, %arg(ea::string_view), Name, GetName);
%csattribute(Urho3D::BinaryInputArchive, %arg(unsigned int), Checksum, GetChecksum);
%csattribute(Urho3D::File, %arg(ea::string), AbsoluteName, GetAbsoluteName);
%csattribute(Urho3D::File, %arg(Urho3D::FileMode), Mode, GetMode);
%csattribute(Urho3D::File, %arg(void *), Handle, GetHandle);
%csattribute(Urho3D::File, %arg(bool), Packaged, IsPackaged);
%csattribute(Urho3D::FileSystem, %arg(ea::string), CurrentDir, GetCurrentDir, SetCurrentDir);
%csattribute(Urho3D::FileSystem, %arg(bool), ExecuteConsoleCommands, GetExecuteConsoleCommands, SetExecuteConsoleCommands);
%csattribute(Urho3D::FileSystem, %arg(ea::string), ProgramDir, GetProgramDir);
%csattribute(Urho3D::FileSystem, %arg(ea::string), ProgramFileName, GetProgramFileName);
%csattribute(Urho3D::FileSystem, %arg(ea::string), InterpreterFileName, GetInterpreterFileName);
%csattribute(Urho3D::FileSystem, %arg(ea::string), UserDocumentsDir, GetUserDocumentsDir);
%csattribute(Urho3D::FileSystem, %arg(ea::string), TemporaryDir, GetTemporaryDir);
%csattribute(Urho3D::FileWatcher, %arg(ea::string), Path, GetPath);
%csattribute(Urho3D::FileWatcher, %arg(float), Delay, GetDelay, SetDelay);
%csattribute(Urho3D::Log, %arg(Urho3D::LogLevel), Level, GetLevel, SetLevel);
%csattribute(Urho3D::Log, %arg(bool), Quiet, IsQuiet, SetQuiet);
%csattribute(Urho3D::MemoryBuffer, %arg(unsigned char *), Data, GetData);
%csattribute(Urho3D::MemoryBuffer, %arg(bool), ReadOnly, IsReadOnly);
%csattribute(Urho3D::MultiFileWatcher, %arg(float), Delay, GetDelay, SetDelay);
%csattribute(Urho3D::NamedPipe, %arg(bool), Server, IsServer);
%csattribute(Urho3D::PackageFile, %arg(ea::unordered_map<ea::string, PackageEntry>), Entries, GetEntries);
%csattribute(Urho3D::PackageFile, %arg(ea::string), Name, GetName);
%csattribute(Urho3D::PackageFile, %arg(Urho3D::StringHash), NameHash, GetNameHash);
%csattribute(Urho3D::PackageFile, %arg(unsigned int), NumFiles, GetNumFiles);
%csattribute(Urho3D::PackageFile, %arg(unsigned int), TotalSize, GetTotalSize);
%csattribute(Urho3D::PackageFile, %arg(unsigned int), TotalDataSize, GetTotalDataSize);
%csattribute(Urho3D::PackageFile, %arg(unsigned int), Checksum, GetChecksum);
%csattribute(Urho3D::PackageFile, %arg(bool), Compressed, IsCompressed);
%csattribute(Urho3D::PackageFile, %arg(ea::vector<ea::string>), EntryNames, GetEntryNames);
%pragma(csharp) moduleimports=%{
public static partial class E
{
    public class LogMessageEvent {
        private StringHash _event = new StringHash("LogMessage");
        public StringHash Message = new StringHash("Message");
        public StringHash Logger = new StringHash("Logger");
        public StringHash Level = new StringHash("Level");
        public StringHash Time = new StringHash("Time");
        public LogMessageEvent() { }
        public static implicit operator StringHash(LogMessageEvent e) { return e._event; }
    }
    public static LogMessageEvent LogMessage = new LogMessageEvent();
    public class AsyncExecFinishedEvent {
        private StringHash _event = new StringHash("AsyncExecFinished");
        public StringHash RequestID = new StringHash("RequestID");
        public StringHash ExitCode = new StringHash("ExitCode");
        public AsyncExecFinishedEvent() { }
        public static implicit operator StringHash(AsyncExecFinishedEvent e) { return e._event; }
    }
    public static AsyncExecFinishedEvent AsyncExecFinished = new AsyncExecFinishedEvent();
} %}
