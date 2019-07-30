%typemap(cscode) Urho3D::Deserializer %{
  public $typemap(cstype, unsigned int) Position {
    get { return GetPosition(); }
  }
  public $typemap(cstype, unsigned int) Size {
    get { return GetSize(); }
  }
%}
//%csmethodmodifiers Urho3D::Deserializer::GetPosition "private";
//%csmethodmodifiers Urho3D::Deserializer::GetSize "private";
%typemap(cscode) Urho3D::File %{
  public $typemap(cstype, Urho3D::FileMode) Mode {
    get { return GetMode(); }
  }
  public $typemap(cstype, void *) Handle {
    get { return GetHandle(); }
  }
%}
%csmethodmodifiers Urho3D::File::GetMode "private";
%csmethodmodifiers Urho3D::File::GetHandle "private";
%typemap(cscode) Urho3D::FileSystem %{
  public $typemap(cstype, eastl::string) CurrentDir {
    get { return GetCurrentDir(); }
  }
  public $typemap(cstype, bool) ExecuteConsoleCommands {
    get { return GetExecuteConsoleCommands(); }
    set { SetExecuteConsoleCommands(value); }
  }
  public $typemap(cstype, eastl::string) ProgramDir {
    get { return GetProgramDir(); }
  }
  public $typemap(cstype, eastl::string) ProgramFileName {
    get { return GetProgramFileName(); }
  }
  public $typemap(cstype, eastl::string) InterpreterFileName {
    get { return GetInterpreterFileName(); }
  }
  public $typemap(cstype, eastl::string) UserDocumentsDir {
    get { return GetUserDocumentsDir(); }
  }
  public $typemap(cstype, eastl::string) TemporaryDir {
    get { return GetTemporaryDir(); }
  }
%}
%csmethodmodifiers Urho3D::FileSystem::GetCurrentDir "private";
%csmethodmodifiers Urho3D::FileSystem::GetExecuteConsoleCommands "private";
%csmethodmodifiers Urho3D::FileSystem::SetExecuteConsoleCommands "private";
%csmethodmodifiers Urho3D::FileSystem::GetProgramDir "private";
%csmethodmodifiers Urho3D::FileSystem::GetProgramFileName "private";
%csmethodmodifiers Urho3D::FileSystem::GetInterpreterFileName "private";
%csmethodmodifiers Urho3D::FileSystem::GetUserDocumentsDir "private";
%csmethodmodifiers Urho3D::FileSystem::GetTemporaryDir "private";
%typemap(cscode) Urho3D::FileWatcher %{
  public $typemap(cstype, const eastl::string &) Path {
    get { return GetPath(); }
  }
  public $typemap(cstype, float) Delay {
    get { return GetDelay(); }
    set { SetDelay(value); }
  }
%}
%csmethodmodifiers Urho3D::FileWatcher::GetPath "private";
%csmethodmodifiers Urho3D::FileWatcher::GetDelay "private";
%csmethodmodifiers Urho3D::FileWatcher::SetDelay "private";
%typemap(cscode) Urho3D::Log %{
  public $typemap(cstype, Urho3D::LogLevel) Level {
    get { return GetLevel(); }
    set { SetLevel(value); }
  }
%}
%csmethodmodifiers Urho3D::Log::GetLevel "private";
%csmethodmodifiers Urho3D::Log::SetLevel "private";
%typemap(cscode) Urho3D::MemoryBuffer %{
  public $typemap(cstype, unsigned char *) Data {
    get { return GetData(); }
  }
%}
%csmethodmodifiers Urho3D::MemoryBuffer::GetData "private";
%typemap(cscode) Urho3D::PackageFile %{
  public $typemap(cstype, const eastl::unordered_map<eastl::string, Urho3D::PackageEntry> &) Entries {
    get { return GetEntries(); }
  }
  public $typemap(cstype, const eastl::string &) Name {
    get { return GetName(); }
  }
  public $typemap(cstype, Urho3D::StringHash) NameHash {
    get { return GetNameHash(); }
  }
  public $typemap(cstype, unsigned int) NumFiles {
    get { return GetNumFiles(); }
  }
  public $typemap(cstype, unsigned int) TotalSize {
    get { return GetTotalSize(); }
  }
  public $typemap(cstype, unsigned int) TotalDataSize {
    get { return GetTotalDataSize(); }
  }
  public $typemap(cstype, unsigned int) Checksum {
    get { return GetChecksum(); }
  }
  public $typemap(cstype, const eastl::vector<eastl::string>) EntryNames {
    get { return GetEntryNames(); }
  }
%}
%csmethodmodifiers Urho3D::PackageFile::GetEntries "private";
%csmethodmodifiers Urho3D::PackageFile::GetName "private";
%csmethodmodifiers Urho3D::PackageFile::GetNameHash "private";
%csmethodmodifiers Urho3D::PackageFile::GetNumFiles "private";
%csmethodmodifiers Urho3D::PackageFile::GetTotalSize "private";
%csmethodmodifiers Urho3D::PackageFile::GetTotalDataSize "private";
%csmethodmodifiers Urho3D::PackageFile::GetChecksum "private";
%csmethodmodifiers Urho3D::PackageFile::GetEntryNames "private";
%typemap(cscode) Urho3D::VectorBuffer %{
  public $typemap(cstype, const unsigned char *) Data {
    get { return GetData(); }
  }
  public $typemap(cstype, unsigned char *) ModifiableData {
    get { return GetModifiableData(); }
  }
  public $typemap(cstype, const eastl::vector<unsigned char> &) Buffer {
    get { return GetBuffer(); }
  }
%}
%csmethodmodifiers Urho3D::VectorBuffer::GetData "private";
%csmethodmodifiers Urho3D::VectorBuffer::GetModifiableData "private";
%csmethodmodifiers Urho3D::VectorBuffer::GetBuffer "private";
