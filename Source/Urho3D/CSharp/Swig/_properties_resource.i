%typemap(cscode) Urho3D::BackgroundLoader %{
  public $typemap(cstype, unsigned int) NumQueuedResources {
    get { return GetNumQueuedResources(); }
  }
%}
%csmethodmodifiers Urho3D::BackgroundLoader::GetNumQueuedResources "private";
%typemap(cscode) Urho3D::Image %{
  public $typemap(cstype, int) Width {
    get { return GetWidth(); }
  }
  public $typemap(cstype, int) Height {
    get { return GetHeight(); }
  }
  public $typemap(cstype, int) Depth {
    get { return GetDepth(); }
  }
  public $typemap(cstype, unsigned int) Components {
    get { return GetComponents(); }
  }
  public $typemap(cstype, unsigned char *) Data {
    get { return GetData(); }
  }
  public $typemap(cstype, Urho3D::CompressedFormat) CompressedFormat {
    get { return GetCompressedFormat(); }
  }
  public $typemap(cstype, unsigned int) NumCompressedLevels {
    get { return GetNumCompressedLevels(); }
  }
  public $typemap(cstype, Urho3D::SharedPtr<Urho3D::Image>) NextLevel {
    get { return GetNextLevel(); }
  }
  public $typemap(cstype, Urho3D::SharedPtr<Urho3D::Image>) NextSibling {
    get { return GetNextSibling(); }
  }
%}
%csmethodmodifiers Urho3D::Image::GetWidth "private";
%csmethodmodifiers Urho3D::Image::GetHeight "private";
%csmethodmodifiers Urho3D::Image::GetDepth "private";
%csmethodmodifiers Urho3D::Image::GetComponents "private";
%csmethodmodifiers Urho3D::Image::GetData "private";
%csmethodmodifiers Urho3D::Image::GetCompressedFormat "private";
%csmethodmodifiers Urho3D::Image::GetNumCompressedLevels "private";
%csmethodmodifiers Urho3D::Image::GetNextLevel "private";
%csmethodmodifiers Urho3D::Image::GetNextSibling "private";
%typemap(cscode) Urho3D::JSONValue %{
  public $typemap(cstype, Urho3D::JSONValueType) ValueType {
    get { return GetValueType(); }
  }
  public $typemap(cstype, Urho3D::JSONNumberType) NumberType {
    get { return GetNumberType(); }
  }
  public $typemap(cstype, bool) Bool {
    get { return GetBool(); }
  }
  public $typemap(cstype, int) Int {
    get { return GetInt(); }
  }
  public $typemap(cstype, unsigned int) UInt {
    get { return GetUInt(); }
  }
  public $typemap(cstype, float) Float {
    get { return GetFloat(); }
  }
  public $typemap(cstype, double) Double {
    get { return GetDouble(); }
  }
  public $typemap(cstype, const eastl::string &) String {
    get { return GetString(); }
  }
  public $typemap(cstype, const char *) CString {
    get { return GetCString(); }
  }
  public $typemap(cstype, const eastl::vector<Urho3D::JSONValue> &) Array {
    get { return GetArray(); }
  }
  public $typemap(cstype, const eastl::map<eastl::string, Urho3D::JSONValue> &) Object {
    get { return GetObject(); }
  }
  public $typemap(cstype, Urho3D::Variant) Variant {
    get { return GetVariant(); }
  }
  public $typemap(cstype, eastl::unordered_map<Urho3D::StringHash, Urho3D::Variant>) VariantMap {
    get { return GetVariantMap(); }
  }
  public $typemap(cstype, eastl::vector<Urho3D::Variant>) VariantVector {
    get { return GetVariantVector(); }
  }
%}
%csmethodmodifiers Urho3D::JSONValue::GetValueType "private";
%csmethodmodifiers Urho3D::JSONValue::GetNumberType "private";
%csmethodmodifiers Urho3D::JSONValue::GetBool "private";
%csmethodmodifiers Urho3D::JSONValue::GetInt "private";
%csmethodmodifiers Urho3D::JSONValue::GetUInt "private";
%csmethodmodifiers Urho3D::JSONValue::GetFloat "private";
%csmethodmodifiers Urho3D::JSONValue::GetDouble "private";
%csmethodmodifiers Urho3D::JSONValue::GetString "private";
%csmethodmodifiers Urho3D::JSONValue::GetCString "private";
%csmethodmodifiers Urho3D::JSONValue::GetArray "private";
%csmethodmodifiers Urho3D::JSONValue::GetObject "private";
%csmethodmodifiers Urho3D::JSONValue::GetVariant "private";
%csmethodmodifiers Urho3D::JSONValue::GetVariantMap "private";
%csmethodmodifiers Urho3D::JSONValue::GetVariantVector "private";
%typemap(cscode) Urho3D::Localization %{
  public $typemap(cstype, int) NumLanguages {
    get { return GetNumLanguages(); }
  }
%}
%csmethodmodifiers Urho3D::Localization::GetNumLanguages "private";
%typemap(cscode) Urho3D::PListValue %{
  public $typemap(cstype, Urho3D::PListValueType) Type {
    get { return GetValueType(); }
  }
  public $typemap(cstype, int) Int {
    get { return GetInt(); }
    set { SetInt(value); }
  }
  public $typemap(cstype, bool) Bool {
    get { return GetBool(); }
    set { SetBool(value); }
  }
  public $typemap(cstype, float) Float {
    get { return GetFloat(); }
    set { SetFloat(value); }
  }
  public $typemap(cstype, const eastl::string &) String {
    get { return GetString(); }
    set { SetString(value); }
  }
  public $typemap(cstype, Urho3D::IntRect) IntRect {
    get { return GetIntRect(); }
  }
  public $typemap(cstype, Urho3D::IntVector2) IntVector2 {
    get { return GetIntVector2(); }
  }
  public $typemap(cstype, Urho3D::IntVector3) IntVector3 {
    get { return GetIntVector3(); }
  }
  public $typemap(cstype, const eastl::unordered_map<eastl::string, Urho3D::PListValue> &) ValueMap {
    get { return GetValueMap(); }
    set { SetValueMap(value); }
  }
  public $typemap(cstype, const eastl::vector<Urho3D::PListValue> &) ValueVector {
    get { return GetValueVector(); }
    set { SetValueVector(value); }
  }
%}
%csmethodmodifiers Urho3D::PListValue::GetValueType "private";
%csmethodmodifiers Urho3D::PListValue::GetInt "private";
%csmethodmodifiers Urho3D::PListValue::SetInt "private";
%csmethodmodifiers Urho3D::PListValue::GetBool "private";
%csmethodmodifiers Urho3D::PListValue::SetBool "private";
%csmethodmodifiers Urho3D::PListValue::GetFloat "private";
%csmethodmodifiers Urho3D::PListValue::SetFloat "private";
%csmethodmodifiers Urho3D::PListValue::GetString "private";
%csmethodmodifiers Urho3D::PListValue::SetString "private";
%csmethodmodifiers Urho3D::PListValue::GetIntRect "private";
%csmethodmodifiers Urho3D::PListValue::GetIntVector2 "private";
%csmethodmodifiers Urho3D::PListValue::GetIntVector3 "private";
%csmethodmodifiers Urho3D::PListValue::GetValueMap "private";
%csmethodmodifiers Urho3D::PListValue::SetValueMap "private";
%csmethodmodifiers Urho3D::PListValue::GetValueVector "private";
%csmethodmodifiers Urho3D::PListValue::SetValueVector "private";
%typemap(cscode) Urho3D::PListFile %{
  public $typemap(cstype, const eastl::unordered_map<eastl::string, Urho3D::PListValue> &) Root {
    get { return GetRoot(); }
  }
%}
%csmethodmodifiers Urho3D::PListFile::GetRoot "private";
%typemap(cscode) Urho3D::Resource %{
  public $typemap(cstype, const eastl::string &) Name {
    get { return GetName(); }
    set { SetName(value); }
  }
  public $typemap(cstype, Urho3D::StringHash) NameHash {
    get { return GetNameHash(); }
  }
  public $typemap(cstype, unsigned int) MemoryUse {
    get { return GetMemoryUse(); }
    set { SetMemoryUse(value); }
  }
  public $typemap(cstype, unsigned int) UseTimer {
    get { return GetUseTimer(); }
  }
  public $typemap(cstype, Urho3D::AsyncLoadState) AsyncLoadState {
    get { return GetAsyncLoadState(); }
    set { SetAsyncLoadState(value); }
  }
%}
%csmethodmodifiers Urho3D::Resource::GetName "private";
%csmethodmodifiers Urho3D::Resource::SetName "private";
%csmethodmodifiers Urho3D::Resource::GetNameHash "private";
%csmethodmodifiers Urho3D::Resource::GetMemoryUse "private";
%csmethodmodifiers Urho3D::Resource::SetMemoryUse "private";
%csmethodmodifiers Urho3D::Resource::GetUseTimer "private";
%csmethodmodifiers Urho3D::Resource::GetAsyncLoadState "private";
%csmethodmodifiers Urho3D::Resource::SetAsyncLoadState "private";
%typemap(cscode) Urho3D::ResourceCache %{
  public $typemap(cstype, unsigned int) NumBackgroundLoadResources {
    get { return GetNumBackgroundLoadResources(); }
  }
  public $typemap(cstype, const eastl::unordered_map<Urho3D::StringHash, Urho3D::ResourceGroup> &) AllResources {
    get { return GetAllResources(); }
  }
  public $typemap(cstype, const eastl::vector<eastl::string> &) ResourceDirs {
    get { return GetResourceDirs(); }
  }
  public $typemap(cstype, const eastl::vector<Urho3D::SharedPtr<Urho3D::PackageFile>> &) PackageFiles {
    get { return GetPackageFiles(); }
  }
  public $typemap(cstype, unsigned long long) TotalMemoryUse {
    get { return GetTotalMemoryUse(); }
  }
  public $typemap(cstype, bool) AutoReloadResources {
    get { return GetAutoReloadResources(); }
    set { SetAutoReloadResources(value); }
  }
  public $typemap(cstype, bool) ReturnFailedResources {
    get { return GetReturnFailedResources(); }
    set { SetReturnFailedResources(value); }
  }
  public $typemap(cstype, bool) SearchPackagesFirst {
    get { return GetSearchPackagesFirst(); }
    set { SetSearchPackagesFirst(value); }
  }
  public $typemap(cstype, int) FinishBackgroundResourcesMs {
    get { return GetFinishBackgroundResourcesMs(); }
    set { SetFinishBackgroundResourcesMs(value); }
  }
  public $typemap(cstype, unsigned int) NumResourceDirs {
    get { return GetNumResourceDirs(); }
  }
%}
%csmethodmodifiers Urho3D::ResourceCache::GetNumBackgroundLoadResources "private";
%csmethodmodifiers Urho3D::ResourceCache::GetAllResources "private";
%csmethodmodifiers Urho3D::ResourceCache::GetResourceDirs "private";
%csmethodmodifiers Urho3D::ResourceCache::GetPackageFiles "private";
%csmethodmodifiers Urho3D::ResourceCache::GetTotalMemoryUse "private";
%csmethodmodifiers Urho3D::ResourceCache::GetAutoReloadResources "private";
%csmethodmodifiers Urho3D::ResourceCache::SetAutoReloadResources "private";
%csmethodmodifiers Urho3D::ResourceCache::GetReturnFailedResources "private";
%csmethodmodifiers Urho3D::ResourceCache::SetReturnFailedResources "private";
%csmethodmodifiers Urho3D::ResourceCache::GetSearchPackagesFirst "private";
%csmethodmodifiers Urho3D::ResourceCache::SetSearchPackagesFirst "private";
%csmethodmodifiers Urho3D::ResourceCache::GetFinishBackgroundResourcesMs "private";
%csmethodmodifiers Urho3D::ResourceCache::SetFinishBackgroundResourcesMs "private";
%csmethodmodifiers Urho3D::ResourceCache::GetNumResourceDirs "private";
%typemap(cscode) Urho3D::XMLElement %{
  public $typemap(cstype, eastl::string) Name {
    get { return GetName(); }
  }
  public $typemap(cstype, Urho3D::XMLElement) Parent {
    get { return GetParent(); }
  }
  public $typemap(cstype, unsigned int) NumAttributes {
    get { return GetNumAttributes(); }
  }
  public $typemap(cstype, eastl::string) Value {
    get { return GetValue(); }
  }
  public $typemap(cstype, eastl::vector<eastl::string>) AttributeNames {
    get { return GetAttributeNames(); }
  }
  public $typemap(cstype, Urho3D::BoundingBox) BoundingBox {
    get { return GetBoundingBox(); }
  }
  public $typemap(cstype, Urho3D::Variant) Variant {
    get { return GetVariant(); }
  }
  public $typemap(cstype, Urho3D::ResourceRef) ResourceRef {
    get { return GetResourceRef(); }
  }
  public $typemap(cstype, Urho3D::ResourceRefList) ResourceRefList {
    get { return GetResourceRefList(); }
  }
  public $typemap(cstype, eastl::vector<Urho3D::Variant>) VariantVector {
    get { return GetVariantVector(); }
  }
  public $typemap(cstype, eastl::vector<eastl::string>) StringVector {
    get { return GetStringVector(); }
  }
  public $typemap(cstype, eastl::unordered_map<Urho3D::StringHash, Urho3D::Variant>) VariantMap {
    get { return GetVariantMap(); }
  }
  public $typemap(cstype, Urho3D::XMLFile *) File {
    get { return GetFile(); }
  }
  /*public _typemap(cstype, pugi::xml_node_struct *) Node {
    get { return GetNode(); }
  }*/
  public $typemap(cstype, const Urho3D::XPathResultSet *) XPathResultSet {
    get { return GetXPathResultSet(); }
  }
  /*public _typemap(cstype, const pugi::xpath_node *) XPathNode {
    get { return GetXPathNode(); }
  }*/
  public $typemap(cstype, unsigned int) XPathResultIndex {
    get { return GetXPathResultIndex(); }
  }
%}
%csmethodmodifiers Urho3D::XMLElement::GetName "private";
%csmethodmodifiers Urho3D::XMLElement::GetParent "private";
%csmethodmodifiers Urho3D::XMLElement::GetNumAttributes "private";
%csmethodmodifiers Urho3D::XMLElement::GetValue "private";
%csmethodmodifiers Urho3D::XMLElement::GetAttributeNames "private";
%csmethodmodifiers Urho3D::XMLElement::GetBoundingBox "private";
%csmethodmodifiers Urho3D::XMLElement::GetVariant "private";
%csmethodmodifiers Urho3D::XMLElement::GetResourceRef "private";
%csmethodmodifiers Urho3D::XMLElement::GetResourceRefList "private";
%csmethodmodifiers Urho3D::XMLElement::GetVariantVector "private";
%csmethodmodifiers Urho3D::XMLElement::GetStringVector "private";
%csmethodmodifiers Urho3D::XMLElement::GetVariantMap "private";
%csmethodmodifiers Urho3D::XMLElement::GetFile "private";
%csmethodmodifiers Urho3D::XMLElement::GetNode "private";
%csmethodmodifiers Urho3D::XMLElement::GetXPathResultSet "private";
%csmethodmodifiers Urho3D::XMLElement::GetXPathNode "private";
%csmethodmodifiers Urho3D::XMLElement::GetXPathResultIndex "private";
%typemap(cscode) Urho3D::XPathResultSet %{
  /*public _typemap(cstype, pugi::xpath_node_set *) XPathNodeSet {
    get { return GetXPathNodeSet(); }
  }*/
%}
%csmethodmodifiers Urho3D::XPathResultSet::GetXPathNodeSet "private";
%typemap(cscode) Urho3D::XPathQuery %{
  public $typemap(cstype, eastl::string) Query {
    get { return GetQuery(); }
  }
  /*public _typemap(cstype, pugi::xpath_variable_set *) XPathVariableSet {
    get { return GetXPathVariableSet(); }
  }*/
%}
%csmethodmodifiers Urho3D::XPathQuery::GetQuery "private";
%csmethodmodifiers Urho3D::XPathQuery::GetXPathVariableSet "private";
%typemap(cscode) Urho3D::XMLFile %{
  /*public _typemap(cstype, pugi::xml_document *) Document {
    get { return GetDocument(); }
  }*/
%}
%csmethodmodifiers Urho3D::XMLFile::GetDocument "private";
