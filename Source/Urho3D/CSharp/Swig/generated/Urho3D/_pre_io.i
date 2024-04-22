%constant char * NullDevice = Urho3D::NULL_DEVICE;
%ignore Urho3D::NULL_DEVICE;
%constant char * Apk = Urho3D::APK;
%ignore Urho3D::APK;
%csconstvalue("0") Urho3D::FILE_READ;
%csconstvalue("1") Urho3D::SCAN_FILES;
%csconstvalue("2") Urho3D::SCAN_DIRS;
%csconstvalue("4") Urho3D::SCAN_HIDDEN;
%csconstvalue("8") Urho3D::SCAN_APPEND;
%csconstvalue("16") Urho3D::SCAN_RECURSIVE;
%typemap(csattributes) Urho3D::ScanFlag "[global::System.Flags]";
using ScanFlags = Urho3D::ScanFlag;
%typemap(ctype) ScanFlags "size_t";
%typemap(out) ScanFlags "$result = (size_t)$1.AsInteger();"
%csattribute(Urho3D::ArchiveBlock, %arg(unsigned int), SizeHint, GetSizeHint);
%csattribute(Urho3D::ArchiveBase, %arg(Urho3D::Context *), Context, GetContext);
%csattribute(Urho3D::ArchiveBase, %arg(ea::string_view), Name, GetName);
%csattribute(Urho3D::ArchiveBase, %arg(unsigned int), Checksum, GetChecksum);
%csattribute(Urho3D::ArchiveBase, %arg(bool), IsEOF, IsEOF);
%csattribute(Urho3D::ArchiveBlockBase, %arg(ea::string_view), Name, GetName);
%csattribute(Urho3D::ArchiveBlockBase, %arg(bool), IsUnorderedAccessSupported, IsUnorderedAccessSupported);
%csattribute(Urho3D::Log, %arg(Urho3D::LogLevel), Level, GetLevel, SetLevel);
%csattribute(Urho3D::Log, %arg(bool), IsQuiet, IsQuiet, SetQuiet);
%csattribute(Urho3D::VectorBuffer, %arg(unsigned char *), Data, GetData);
%csattribute(Urho3D::VectorBuffer, %arg(unsigned char *), ModifiableData, GetModifiableData);
%csattribute(Urho3D::VectorBuffer, %arg(Urho3D::ByteVector), Buffer, GetBuffer);
%csattribute(Urho3D::BinaryOutputArchiveBlock, %arg(Urho3D::Serializer *), Serializer, GetSerializer);
%csattribute(Urho3D::BinaryOutputArchiveBlock, %arg(bool), IsUnorderedAccessSupported, IsUnorderedAccessSupported);
%csattribute(Urho3D::BinaryInputArchiveBlock, %arg(unsigned int), NextElementPosition, GetNextElementPosition);
%csattribute(Urho3D::BinaryInputArchiveBlock, %arg(bool), IsUnorderedAccessSupported, IsUnorderedAccessSupported);
%csattribute(Urho3D::Base64OutputArchive, %arg(ea::string), Base64, GetBase64);
%csattribute(Urho3D::File, %arg(Urho3D::FileMode), Mode, GetMode);
%csattribute(Urho3D::File, %arg(void *), Handle, GetHandle);
%csattribute(Urho3D::File, %arg(bool), IsPackaged, IsPackaged);
%csattribute(Urho3D::FileSystem, %arg(ea::string), CurrentDir, GetCurrentDir, SetCurrentDir);
%csattribute(Urho3D::FileSystem, %arg(bool), ExecuteConsoleCommands, GetExecuteConsoleCommands, SetExecuteConsoleCommands);
%csattribute(Urho3D::FileSystem, %arg(ea::string), ProgramDir, GetProgramDir);
%csattribute(Urho3D::FileSystem, %arg(ea::string), ProgramFileName, GetProgramFileName);
%csattribute(Urho3D::FileSystem, %arg(ea::string), UserDocumentsDir, GetUserDocumentsDir);
%csattribute(Urho3D::FileSystem, %arg(ea::string), TemporaryDir, GetTemporaryDir);
%csattribute(Urho3D::TemporaryDir, %arg(ea::string), Path, GetPath);
%csattribute(Urho3D::FileWatcher, %arg(ea::string), Path, GetPath);
%csattribute(Urho3D::FileWatcher, %arg(float), Delay, GetDelay, SetDelay);
%csattribute(Urho3D::MemoryBuffer, %arg(unsigned char *), Data, GetData);
%csattribute(Urho3D::MemoryBuffer, %arg(bool), IsReadOnly, IsReadOnly);
%csattribute(Urho3D::WatchableMountPoint, %arg(bool), IsWatching, IsWatching, SetWatching);
%csattribute(Urho3D::MountedAliasRoot, %arg(ea::string), Name, GetName);
%csattribute(Urho3D::MountedDirectory, %arg(ea::string), Name, GetName);
%csattribute(Urho3D::MountedDirectory, %arg(ea::string), Directory, GetDirectory);
%csattribute(Urho3D::MountedExternalMemory, %arg(ea::string), Name, GetName);
%csattribute(Urho3D::MountedRoot, %arg(ea::string), Name, GetName);
%csattribute(Urho3D::MultiFileWatcher, %arg(float), Delay, GetDelay, SetDelay);
%csattribute(Urho3D::NamedPipe, %arg(bool), IsServer, IsServer);
%csattribute(Urho3D::PackageFile, %arg(ea::unordered_map<ea::string, PackageEntry>), Entries, GetEntries);
%csattribute(Urho3D::PackageFile, %arg(Urho3D::StringHash), NameHash, GetNameHash);
%csattribute(Urho3D::PackageFile, %arg(unsigned int), NumFiles, GetNumFiles);
%csattribute(Urho3D::PackageFile, %arg(unsigned int), TotalSize, GetTotalSize);
%csattribute(Urho3D::PackageFile, %arg(unsigned int), TotalDataSize, GetTotalDataSize);
%csattribute(Urho3D::PackageFile, %arg(unsigned int), Checksum, GetChecksum);
%csattribute(Urho3D::PackageFile, %arg(bool), IsCompressed, IsCompressed);
%csattribute(Urho3D::PackageFile, %arg(ea::vector<ea::string>), EntryNames, GetEntryNames);
%csattribute(Urho3D::PackageFile, %arg(ea::string), Name, GetName);
%csattribute(Urho3D::VirtualFileSystem, %arg(bool), IsWatching, IsWatching, SetWatching);
%csattribute(Urho3D::MountPointGuard, %arg(Urho3D::MountPoint *), MountPoint, Get);
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
