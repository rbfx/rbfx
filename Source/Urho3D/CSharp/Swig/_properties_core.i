%typemap(cscode) Urho3D::Context %{
  public $typemap(cstype, eastl::unordered_map<Urho3D::StringHash, Urho3D::Variant> &) EventDataMap {
    get { return GetEventDataMap(); }
  }
  public $typemap(cstype, const eastl::unordered_map<Urho3D::StringHash, Urho3D::Variant> &) GlobalVars {
    get { return GetGlobalVars(); }
  }
  /*public _typemap(cstype, const eastl::unordered_map<Urho3D::StringHash, Urho3D::SharedPtr<Urho3D::Object>> &) Subsystems {
    get { return GetSubsystems(); }
  }
  public _typemap(cstype, const eastl::unordered_map<Urho3D::StringHash, Urho3D::SharedPtr<Urho3D::ObjectFactory>> &) ObjectFactories {
    get { return GetObjectFactories(); }
  }
  public _typemap(cstype, const eastl::unordered_map<eastl::string, eastl::vector<Urho3D::StringHash>> &) ObjectCategories {
    get { return GetObjectCategories(); }
  }*/
  public $typemap(cstype, Urho3D::Object *) EventSender {
    get { return GetEventSender(); }
  }
  /*public _typemap(cstype, Urho3D::EventHandler *) EventHandler {
    get { return GetEventHandler(); }
  }*/
  public $typemap(cstype, const eastl::unordered_map<Urho3D::StringHash, eastl::vector<Urho3D::AttributeInfo>> &) AllAttributes {
    get { return GetAllAttributes(); }
  }
  public $typemap(cstype, Urho3D::Engine *) Engine {
    get { return GetEngine(); }
  }
  public $typemap(cstype, Urho3D::Time *) Time {
    get { return GetTime(); }
  }
  /*public _typemap(cstype, Urho3D::WorkQueue *) WorkQueue {
    get { return GetWorkQueue(); }
  }*/
  public $typemap(cstype, Urho3D::FileSystem *) FileSystem {
    get { return GetFileSystem(); }
  }
  public $typemap(cstype, Urho3D::Log *) Log {
    get { return GetLog(); }
  }
  public $typemap(cstype, Urho3D::ResourceCache *) Cache {
    get { return GetCache(); }
  }
  public $typemap(cstype, Urho3D::Localization *) Localization {
    get { return GetLocalization(); }
  }
  public $typemap(cstype, Urho3D::Network *) Network {
    get { return GetNetwork(); }
  }
  public $typemap(cstype, Urho3D::Input *) Input {
    get { return GetInput(); }
  }
  public $typemap(cstype, Urho3D::Audio *) Audio {
    get { return GetAudio(); }
  }
  public $typemap(cstype, Urho3D::UI *) UI {
    get { return GetUI(); }
  }
  public $typemap(cstype, Urho3D::SystemUI *) SystemUI {
    get { return GetSystemUI(); }
  }
  public $typemap(cstype, Urho3D::Graphics *) Graphics {
    get { return GetGraphics(); }
  }
  public $typemap(cstype, Urho3D::Renderer *) Renderer {
    get { return GetRenderer(); }
  }
%}
%csmethodmodifiers Urho3D::Context::GetEventDataMap "private";
%csmethodmodifiers Urho3D::Context::GetGlobalVars "private";
%csmethodmodifiers Urho3D::Context::GetSubsystems "private";
%csmethodmodifiers Urho3D::Context::GetObjectFactories "private";
%csmethodmodifiers Urho3D::Context::GetObjectCategories "private";
%csmethodmodifiers Urho3D::Context::GetEventSender "private";
%csmethodmodifiers Urho3D::Context::GetEventHandler "private";
%csmethodmodifiers Urho3D::Context::GetAllAttributes "private";
%csmethodmodifiers Urho3D::Context::GetEngine "private";
%csmethodmodifiers Urho3D::Context::GetTime "private";
%csmethodmodifiers Urho3D::Context::GetWorkQueue "private";
%csmethodmodifiers Urho3D::Context::GetFileSystem "private";
%csmethodmodifiers Urho3D::Context::GetLog "private";
%csmethodmodifiers Urho3D::Context::GetCache "private";
%csmethodmodifiers Urho3D::Context::GetLocalization "private";
%csmethodmodifiers Urho3D::Context::GetNetwork "private";
%csmethodmodifiers Urho3D::Context::GetInput "private";
%csmethodmodifiers Urho3D::Context::GetAudio "private";
%csmethodmodifiers Urho3D::Context::GetUI "private";
%csmethodmodifiers Urho3D::Context::GetSystemUI "private";
%csmethodmodifiers Urho3D::Context::GetGraphics "private";
%csmethodmodifiers Urho3D::Context::GetRenderer "private";
%typemap(cscode) Urho3D::TypeInfo %{
  public $typemap(cstype, Urho3D::StringHash) Type {
    get { return GetType(); }
  }
  public $typemap(cstype, const eastl::string &) TypeName {
    get { return GetTypeName(); }
  }
  public $typemap(cstype, const Urho3D::TypeInfo *) BaseTypeInfo {
    get { return GetBaseTypeInfo(); }
  }
%}
%csmethodmodifiers Urho3D::TypeInfo::GetType "private";
%csmethodmodifiers Urho3D::TypeInfo::GetTypeName "private";
%csmethodmodifiers Urho3D::TypeInfo::GetBaseTypeInfo "private";
%typemap(cscode) Urho3D::Object %{
  public $typemap(cstype, eastl::unordered_map<Urho3D::StringHash, Urho3D::Variant> &) EventDataMap {
    get { return GetEventDataMap(); }
  }
  public $typemap(cstype, Urho3D::Context *) Context {
    get { return GetContext(); }
  }
  public $typemap(cstype, const eastl::unordered_map<Urho3D::StringHash, Urho3D::Variant> &) GlobalVars {
    get { return GetGlobalVars(); }
  }
  public $typemap(cstype, Urho3D::Object *) EventSender {
    get { return GetEventSender(); }
  }
  /*public _typemap(cstype, Urho3D::EventHandler *) EventHandler {
    get { return GetEventHandler(); }
  }*/
  public $typemap(cstype, const eastl::string &) Category {
    get { return GetCategory(); }
  }
  public $typemap(cstype, bool) BlockEvents {
    get { return GetBlockEvents(); }
    set { SetBlockEvents(value); }
  }
%}
%csmethodmodifiers Urho3D::Object::GetEventDataMap "private";
%csmethodmodifiers Urho3D::Object::GetContext "private";
%csmethodmodifiers Urho3D::Object::GetGlobalVars "private";
%csmethodmodifiers Urho3D::Object::GetEventSender "private";
%csmethodmodifiers Urho3D::Object::GetEventHandler "private";
%csmethodmodifiers Urho3D::Object::GetCategory "private";
%csmethodmodifiers Urho3D::Object::GetBlockEvents "private";
%csmethodmodifiers Urho3D::Object::SetBlockEvents "private";
%typemap(cscode) Urho3D::ObjectFactory %{
  public $typemap(cstype, Urho3D::Context *) Context {
    get { return GetContext(); }
  }
  public $typemap(cstype, const Urho3D::TypeInfo *) TypeInfo {
    get { return GetTypeInfo(); }
  }
  public $typemap(cstype, Urho3D::StringHash) Type {
    get { return GetType(); }
  }
  public $typemap(cstype, const eastl::string &) TypeName {
    get { return GetTypeName(); }
  }
%}
%csmethodmodifiers Urho3D::ObjectFactory::GetContext "private";
%csmethodmodifiers Urho3D::ObjectFactory::GetTypeInfo "private";
%csmethodmodifiers Urho3D::ObjectFactory::GetType "private";
%csmethodmodifiers Urho3D::ObjectFactory::GetTypeName "private";
%typemap(cscode) Urho3D::EventHandler %{
  public $typemap(cstype, Urho3D::Object *) Receiver {
    get { return GetReceiver(); }
  }
  public $typemap(cstype, Urho3D::Object *) Sender {
    get { return GetSender(); }
  }
  public $typemap(cstype, const Urho3D::StringHash &) EventType {
    get { return GetEventType(); }
  }
  public $typemap(cstype, void *) UserData {
    get { return GetUserData(); }
  }
%}
%csmethodmodifiers Urho3D::EventHandler::GetReceiver "private";
%csmethodmodifiers Urho3D::EventHandler::GetSender "private";
%csmethodmodifiers Urho3D::EventHandler::GetEventType "private";
%csmethodmodifiers Urho3D::EventHandler::GetUserData "private";
%typemap(cscode) Urho3D::PluginModule %{
  public $typemap(cstype, Urho3D::ModuleType) ModuleType {
    get { return GetModuleType(); }
  }
  public $typemap(cstype, const eastl::string &) Path {
    get { return GetPath(); }
  }
%}
%csmethodmodifiers Urho3D::PluginModule::GetModuleType "private";
%csmethodmodifiers Urho3D::PluginModule::GetPath "private";
%typemap(cscode) Urho3D::Spline %{
  public $typemap(cstype, Urho3D::InterpolationMode) InterpolationMode {
    get { return GetInterpolationMode(); }
    set { SetInterpolationMode(value); }
  }
  public $typemap(cstype, const eastl::vector<Urho3D::Variant> &) Knots {
    get { return GetKnots(); }
  }
%}
%csmethodmodifiers Urho3D::Spline::GetInterpolationMode "private";
%csmethodmodifiers Urho3D::Spline::SetInterpolationMode "private";
%csmethodmodifiers Urho3D::Spline::GetKnots "private";
%typemap(cscode) Urho3D::StringHashRegister %{
  public $typemap(cstype, const eastl::unordered_map<Urho3D::StringHash, eastl::string> &) InternalMap {
    get { return GetInternalMap(); }
  }
%}
%csmethodmodifiers Urho3D::StringHashRegister::GetInternalMap "private";
%typemap(cscode) Urho3D::Time %{
  public $typemap(cstype, unsigned int) FrameNumber {
    get { return GetFrameNumber(); }
  }
  public $typemap(cstype, float) TimeStep {
    get { return GetTimeStep(); }
  }
  public $typemap(cstype, unsigned int) TimerPeriod {
    get { return GetTimerPeriod(); }
    set { SetTimerPeriod(value); }
  }
  public $typemap(cstype, float) ElapsedTime {
    get { return GetElapsedTime(); }
  }
  public $typemap(cstype, float) FramesPerSecond {
    get { return GetFramesPerSecond(); }
  }
%}
%csmethodmodifiers Urho3D::Time::GetFrameNumber "private";
%csmethodmodifiers Urho3D::Time::GetTimeStep "private";
%csmethodmodifiers Urho3D::Time::GetTimerPeriod "private";
%csmethodmodifiers Urho3D::Time::SetTimerPeriod "private";
%csmethodmodifiers Urho3D::Time::GetElapsedTime "private";
%csmethodmodifiers Urho3D::Time::GetFramesPerSecond "private";
%typemap(cscode) Urho3D::CustomVariantValue %{
  public $typemap(cstype, const std::type_info &) TypeInfo {
    get { return GetTypeInfo(); }
  }
%}
%csmethodmodifiers Urho3D::CustomVariantValue::GetTypeInfo "private";
%typemap(cscode) Urho3D::Variant %{
  public $typemap(cstype, int) Int {
    get { return GetInt(); }
  }
  public $typemap(cstype, long long) Int64 {
    get { return GetInt64(); }
  }
  public $typemap(cstype, unsigned long long) UInt64 {
    get { return GetUInt64(); }
  }
  public $typemap(cstype, unsigned int) UInt {
    get { return GetUInt(); }
  }
  public $typemap(cstype, Urho3D::StringHash) StringHash {
    get { return GetStringHash(); }
  }
  public $typemap(cstype, bool) Bool {
    get { return GetBool(); }
  }
  public $typemap(cstype, float) Float {
    get { return GetFloat(); }
  }
  public $typemap(cstype, double) Double {
    get { return GetDouble(); }
  }
  public $typemap(cstype, const Urho3D::Vector2 &) Vector2 {
    get { return GetVector2(); }
  }
  public $typemap(cstype, const Urho3D::Vector3 &) Vector3 {
    get { return GetVector3(); }
  }
  public $typemap(cstype, const Urho3D::Vector4 &) Vector4 {
    get { return GetVector4(); }
  }
  public $typemap(cstype, const Urho3D::Quaternion &) Quaternion {
    get { return GetQuaternion(); }
  }
  public $typemap(cstype, const Urho3D::Color &) Color {
    get { return GetColor(); }
  }
  public $typemap(cstype, const eastl::string &) String {
    get { return GetString(); }
  }
  public $typemap(cstype, const eastl::vector<unsigned char> &) Buffer {
    get { return GetBuffer(); }
  }
  /*public _typemap(cstype, Urho3D::VectorBuffer) VectorBuffer {
    get { return GetVectorBuffer(); }
  }*/
  public $typemap(cstype, void *) VoidPtr {
    get { return GetVoidPtr(); }
  }
  public $typemap(cstype, const Urho3D::ResourceRef &) ResourceRef {
    get { return GetResourceRef(); }
  }
  public $typemap(cstype, const Urho3D::ResourceRefList &) ResourceRefList {
    get { return GetResourceRefList(); }
  }
  public $typemap(cstype, const eastl::vector<Urho3D::Variant> &) VariantList { // custom name
    get { return GetVariantVector(); }
  }
  public $typemap(cstype, const eastl::vector<eastl::string> &) StringList {    // custom name
    get { return GetStringVector(); }
  }
  public $typemap(cstype, const eastl::unordered_map<Urho3D::StringHash, Urho3D::Variant> &) VariantMap {
    get { return GetVariantMap(); }
  }
  public $typemap(cstype, const Urho3D::Rect &) Rect {
    get { return GetRect(); }
  }
  public $typemap(cstype, const Urho3D::IntRect &) IntRect {
    get { return GetIntRect(); }
  }
  public $typemap(cstype, const Urho3D::IntVector2 &) IntVector2 {
    get { return GetIntVector2(); }
  }
  public $typemap(cstype, const Urho3D::IntVector3 &) IntVector3 {
    get { return GetIntVector3(); }
  }
  public $typemap(cstype, Urho3D::RefCounted *) Ptr {
    get { return GetPtr(); }
  }
  public $typemap(cstype, const Urho3D::Matrix3 &) Matrix3 {
    get { return GetMatrix3(); }
  }
  public $typemap(cstype, const Urho3D::Matrix3x4 &) Matrix3x4 {
    get { return GetMatrix3x4(); }
  }
  public $typemap(cstype, const Urho3D::Matrix4 &) Matrix4 {
    get { return GetMatrix4(); }
  }
  public $typemap(cstype, Urho3D::VariantType) VariantType {
    get { return GetVariantType(); }
  }
  /*public _typemap(cstype, eastl::vector<unsigned char> *) BufferPtr {
    get { return GetBufferPtr(); }
  }
  public $typemap(cstype, eastl::vector<Urho3D::Variant> *) VariantVectorPtr {
    get { return GetVariantVectorPtr(); }
  }
  public $typemap(cstype, eastl::vector<eastl::string> *) StringVectorPtr {
    get { return GetStringVectorPtr(); }
  }
  public $typemap(cstype, eastl::unordered_map<Urho3D::StringHash, Urho3D::Variant> *) VariantMapPtr {
    get { return GetVariantMapPtr(); }
  }*/
%}
%csmethodmodifiers Urho3D::Variant::GetInt "private";
%csmethodmodifiers Urho3D::Variant::GetInt64 "private";
%csmethodmodifiers Urho3D::Variant::GetUInt64 "private";
%csmethodmodifiers Urho3D::Variant::GetUInt "private";
%csmethodmodifiers Urho3D::Variant::GetStringHash "private";
%csmethodmodifiers Urho3D::Variant::GetBool "private";
%csmethodmodifiers Urho3D::Variant::GetFloat "private";
%csmethodmodifiers Urho3D::Variant::GetDouble "private";
%csmethodmodifiers Urho3D::Variant::GetVector2 "private";
%csmethodmodifiers Urho3D::Variant::GetVector3 "private";
%csmethodmodifiers Urho3D::Variant::GetVector4 "private";
%csmethodmodifiers Urho3D::Variant::GetQuaternion "private";
%csmethodmodifiers Urho3D::Variant::GetColor "private";
%csmethodmodifiers Urho3D::Variant::GetString "private";
%csmethodmodifiers Urho3D::Variant::GetBuffer "private";
%csmethodmodifiers Urho3D::Variant::GetVectorBuffer "private";
%csmethodmodifiers Urho3D::Variant::GetVoidPtr "private";
%csmethodmodifiers Urho3D::Variant::GetResourceRef "private";
%csmethodmodifiers Urho3D::Variant::GetResourceRefList "private";
%csmethodmodifiers Urho3D::Variant::GetVariantVector "private";
%csmethodmodifiers Urho3D::Variant::GetStringVector "private";
%csmethodmodifiers Urho3D::Variant::GetVariantMap "private";
%csmethodmodifiers Urho3D::Variant::GetRect "private";
%csmethodmodifiers Urho3D::Variant::GetIntRect "private";
%csmethodmodifiers Urho3D::Variant::GetIntVector2 "private";
%csmethodmodifiers Urho3D::Variant::GetIntVector3 "private";
%csmethodmodifiers Urho3D::Variant::GetPtr "private";
%csmethodmodifiers Urho3D::Variant::GetMatrix3 "private";
%csmethodmodifiers Urho3D::Variant::GetMatrix3x4 "private";
%csmethodmodifiers Urho3D::Variant::GetMatrix4 "private";
%csmethodmodifiers Urho3D::Variant::GetType "private";
%csmethodmodifiers Urho3D::Variant::GetBufferPtr "private";
%csmethodmodifiers Urho3D::Variant::GetVariantVectorPtr "private";
%csmethodmodifiers Urho3D::Variant::GetStringVectorPtr "private";
%csmethodmodifiers Urho3D::Variant::GetVariantMapPtr "private";
%typemap(cscode) Urho3D::WorkQueue %{
  public $typemap(cstype, Urho3D::SharedPtr<Urho3D::WorkItem>) FreeItem {
    get { return GetFreeItem(); }
  }
  public $typemap(cstype, unsigned int) NumThreads {
    get { return GetNumThreads(); }
  }
  public $typemap(cstype, int) Tolerance {
    get { return GetTolerance(); }
    set { SetTolerance(value); }
  }
  public $typemap(cstype, int) NonThreadedWorkMs {
    get { return GetNonThreadedWorkMs(); }
    set { SetNonThreadedWorkMs(value); }
  }
%}
%csmethodmodifiers Urho3D::WorkQueue::GetFreeItem "private";
%csmethodmodifiers Urho3D::WorkQueue::GetNumThreads "private";
%csmethodmodifiers Urho3D::WorkQueue::GetTolerance "private";
%csmethodmodifiers Urho3D::WorkQueue::SetTolerance "private";
%csmethodmodifiers Urho3D::WorkQueue::GetNonThreadedWorkMs "private";
%csmethodmodifiers Urho3D::WorkQueue::SetNonThreadedWorkMs "private";
