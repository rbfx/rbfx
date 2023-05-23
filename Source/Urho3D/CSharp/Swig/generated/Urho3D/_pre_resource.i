%constant unsigned int BinaryMagicSize = Urho3D::BinaryMagicSize;
%ignore Urho3D::BinaryMagicSize;
%constant Urho3D::BinaryMagic DefaultBinaryMagic = Urho3D::DefaultBinaryMagic;
%ignore Urho3D::DefaultBinaryMagic;
%ignore Urho3D::COLOR_LUT_SIZE;
%csconst(1) Urho3D::COLOR_LUT_SIZE;
%constant int ColorLutSize = 16;
%constant unsigned int PriorityLast = Urho3D::PRIORITY_LAST;
%ignore Urho3D::PRIORITY_LAST;
%csconstvalue("0") Urho3D::CF_NONE;
%csconstvalue("0") Urho3D::PLVT_NONE;
%csattribute(Urho3D::JSONValue, %arg(Urho3D::JSONValueType), ValueType, GetValueType);
%csattribute(Urho3D::JSONValue, %arg(Urho3D::JSONNumberType), NumberType, GetNumberType);
%csattribute(Urho3D::JSONValue, %arg(bool), IsNull, IsNull);
%csattribute(Urho3D::JSONValue, %arg(bool), IsBool, IsBool);
%csattribute(Urho3D::JSONValue, %arg(bool), IsNumber, IsNumber);
%csattribute(Urho3D::JSONValue, %arg(bool), IsString, IsString);
%csattribute(Urho3D::JSONValue, %arg(bool), IsArray, IsArray);
%csattribute(Urho3D::JSONValue, %arg(bool), IsObject, IsObject);
%csattribute(Urho3D::Resource, %arg(ea::string), Name, GetName, SetName);
%csattribute(Urho3D::Resource, %arg(Urho3D::StringHash), NameHash, GetNameHash);
%csattribute(Urho3D::Resource, %arg(unsigned int), MemoryUse, GetMemoryUse, SetMemoryUse);
%csattribute(Urho3D::Resource, %arg(unsigned int), UseTimer, GetUseTimer);
%csattribute(Urho3D::Resource, %arg(Urho3D::AsyncLoadState), AsyncLoadState, GetAsyncLoadState, SetAsyncLoadState);
%csattribute(Urho3D::Resource, %arg(ea::string), AbsoluteFileName, GetAbsoluteFileName, SetAbsoluteFileName);
%csattribute(Urho3D::ResourceWithMetadata, %arg(Urho3D::StringVector), MetadataKeys, GetMetadataKeys);
%csattribute(Urho3D::Image, %arg(bool), IsCubemap, IsCubemap);
%csattribute(Urho3D::Image, %arg(bool), IsArray, IsArray);
%csattribute(Urho3D::Image, %arg(bool), IsSRGB, IsSRGB);
%csattribute(Urho3D::Image, %arg(int), Width, GetWidth);
%csattribute(Urho3D::Image, %arg(int), Height, GetHeight);
%csattribute(Urho3D::Image, %arg(int), Depth, GetDepth);
%csattribute(Urho3D::Image, %arg(Urho3D::IntVector3), Size, GetSize);
%csattribute(Urho3D::Image, %arg(unsigned int), Components, GetComponents);
%csattribute(Urho3D::Image, %arg(unsigned char *), Data, GetData, SetData);
%csattribute(Urho3D::Image, %arg(bool), IsCompressed, IsCompressed);
%csattribute(Urho3D::Image, %arg(Urho3D::CompressedFormat), CompressedFormat, GetCompressedFormat);
%csattribute(Urho3D::Image, %arg(unsigned int), NumCompressedLevels, GetNumCompressedLevels);
%csattribute(Urho3D::Image, %arg(SharedPtr<Urho3D::Image>), NextLevel, GetNextLevel);
%csattribute(Urho3D::Image, %arg(SharedPtr<Urho3D::Image>), NextSibling, GetNextSibling);
%csattribute(Urho3D::Image, %arg(SharedPtr<Urho3D::Image>), DecompressedImage, GetDecompressedImage);
%csattribute(Urho3D::XMLElement, %arg(bool), IsNull, IsNull);
%csattribute(Urho3D::XMLElement, %arg(ea::string), Name, GetName, SetName);
%csattribute(Urho3D::XMLElement, %arg(Urho3D::XMLElement), Parent, GetParent);
%csattribute(Urho3D::XMLElement, %arg(unsigned int), NumAttributes, GetNumAttributes);
%csattribute(Urho3D::XMLElement, %arg(ea::string), Value, GetValue, SetValue);
%csattribute(Urho3D::XMLElement, %arg(ea::vector<ea::string>), AttributeNames, GetAttributeNames);
%csattribute(Urho3D::XMLElement, %arg(Urho3D::BoundingBox), BoundingBox, GetBoundingBox, SetBoundingBox);
%csattribute(Urho3D::XMLElement, %arg(Urho3D::Variant), Variant, GetVariant, SetVariant);
%csattribute(Urho3D::XMLElement, %arg(Urho3D::ResourceRef), ResourceRef, GetResourceRef, SetResourceRef);
%csattribute(Urho3D::XMLElement, %arg(Urho3D::ResourceRefList), ResourceRefList, GetResourceRefList, SetResourceRefList);
%csattribute(Urho3D::XMLElement, %arg(Urho3D::VariantVector), VariantVector, GetVariantVector, SetVariantVector);
%csattribute(Urho3D::XMLElement, %arg(Urho3D::StringVector), StringVector, GetStringVector, SetStringVector);
%csattribute(Urho3D::XMLElement, %arg(Urho3D::VariantMap), VariantMap, GetVariantMap, SetVariantMap);
%csattribute(Urho3D::XMLElement, %arg(Urho3D::StringVariantMap), StringVariantMap, GetStringVariantMap, SetStringVariantMap);
%csattribute(Urho3D::XMLElement, %arg(Urho3D::XMLFile *), File, GetFile);
%csattribute(Urho3D::XMLElement, %arg(pugi::xml_node_struct *), Node, GetNode);
%csattribute(Urho3D::XMLElement, %arg(Urho3D::XPathResultSet *), XPathResultSet, GetXPathResultSet);
%csattribute(Urho3D::XMLElement, %arg(pugi::xpath_node *), XPathNode, GetXPathNode);
%csattribute(Urho3D::XMLElement, %arg(unsigned int), XPathResultIndex, GetXPathResultIndex);
%csattribute(Urho3D::XPathResultSet, %arg(pugi::xpath_node_set *), XPathNodeSet, GetXPathNodeSet);
%csattribute(Urho3D::XPathQuery, %arg(pugi::xpath_variable_set *), XPathVariableSet, GetXPathVariableSet);
%csattribute(Urho3D::XMLFile, %arg(pugi::xml_document *), Document, GetDocument);
%csattribute(Urho3D::BackgroundLoader, %arg(unsigned int), NumQueuedResources, GetNumQueuedResources);
%csattribute(Urho3D::BinaryFile, %arg(Urho3D::ByteVector), Data, GetData, SetData);
%csattribute(Urho3D::BinaryFile, %arg(ea::string_view), Text, GetText);
%csattribute(Urho3D::Graph, %arg(unsigned int), NumNodes, GetNumNodes);
%csattribute(Urho3D::Graph, %arg(unsigned int), NextNodeId, GetNextNodeId);
%csattribute(Urho3D::GraphPin, %arg(ea::string), Name, GetName);
%csattribute(Urho3D::GraphInPin, %arg(bool), IsConnected, IsConnected);
%csattribute(Urho3D::GraphInPin, %arg(Urho3D::Variant), Value, GetValue, SetValue);
%csattribute(Urho3D::GraphExitPin, %arg(bool), IsConnected, IsConnected);
%csattribute(Urho3D::GraphNodeProperty, %arg(ea::string), Name, GetName);
%csattribute(Urho3D::GraphNodeProperty, %arg(Urho3D::StringHash), NameHash, GetNameHash);
%csattribute(Urho3D::GraphNode, %arg(unsigned int), Id, GetID);
%csattribute(Urho3D::GraphNode, %arg(Urho3D::Graph *), Graph, GetGraph);
%csattribute(Urho3D::GraphNode, %arg(ea::string), Name, GetName, SetName);
%csattribute(Urho3D::GraphNode, %arg(Urho3D::StringHash), NameHash, GetNameHash);
%csattribute(Urho3D::GraphNode, %arg(ea::span<GraphNodeProperty>), Properties, GetProperties);
%csattribute(Urho3D::GraphNode, %arg(unsigned int), NumInputs, GetNumInputs);
%csattribute(Urho3D::GraphNode, %arg(unsigned int), NumOutputs, GetNumOutputs);
%csattribute(Urho3D::GraphNode, %arg(unsigned int), NumEnters, GetNumEnters);
%csattribute(Urho3D::GraphNode, %arg(unsigned int), NumExits, GetNumExits);
%csattribute(Urho3D::GraphNode, %arg(Urho3D::Vector2), PositionHint, GetPositionHint, SetPositionHint);
%csattribute(Urho3D::ImageCube, %arg(ea::vector<SharedPtr<Image>>), Images, GetImages);
%csattribute(Urho3D::ImageCube, %arg(Urho3D::XMLFile *), ParametersXML, GetParametersXML);
%csattribute(Urho3D::ImageCube, %arg(unsigned int), SphericalHarmonicsMipLevel, GetSphericalHarmonicsMipLevel);
%csattribute(Urho3D::ImageCube, %arg(SharedPtr<Urho3D::ImageCube>), DecompressedImage, GetDecompressedImage);
%csattribute(Urho3D::JSONFile, %arg(Urho3D::JSONValue), Root, GetRoot);
%csattribute(Urho3D::JSONOutputArchiveBlock, %arg(bool), IsUnorderedAccessSupported, IsUnorderedAccessSupported);
%csattribute(Urho3D::JSONInputArchiveBlock, %arg(unsigned int), SizeHint, GetSizeHint);
%csattribute(Urho3D::JSONInputArchiveBlock, %arg(bool), IsUnorderedAccessSupported, IsUnorderedAccessSupported);
%csattribute(Urho3D::Localization, %arg(int), NumLanguages, GetNumLanguages);
%csattribute(Urho3D::PListValue, %arg(int), Int, GetInt, SetInt);
%csattribute(Urho3D::PListValue, %arg(bool), Bool, GetBool, SetBool);
%csattribute(Urho3D::PListValue, %arg(float), Float, GetFloat, SetFloat);
%csattribute(Urho3D::PListValue, %arg(ea::string), String, GetString, SetString);
%csattribute(Urho3D::PListValue, %arg(Urho3D::IntRect), IntRect, GetIntRect);
%csattribute(Urho3D::PListValue, %arg(Urho3D::IntVector2), IntVector2, GetIntVector2);
%csattribute(Urho3D::PListValue, %arg(Urho3D::IntVector3), IntVector3, GetIntVector3);
%csattribute(Urho3D::PListValue, %arg(Urho3D::PListValueMap), ValueMap, GetValueMap, SetValueMap);
%csattribute(Urho3D::PListValue, %arg(Urho3D::PListValueVector), ValueVector, GetValueVector, SetValueVector);
%csattribute(Urho3D::PListFile, %arg(Urho3D::PListValueMap), Root, GetRoot);
%csattribute(Urho3D::ResourceCache, %arg(unsigned int), NumBackgroundLoadResources, GetNumBackgroundLoadResources);
%csattribute(Urho3D::ResourceCache, %arg(ea::unordered_map<StringHash, ResourceGroup>), AllResources, GetAllResources);
%csattribute(Urho3D::ResourceCache, %arg(unsigned long long), TotalMemoryUse, GetTotalMemoryUse);
%csattribute(Urho3D::ResourceCache, %arg(bool), ReturnFailedResources, GetReturnFailedResources, SetReturnFailedResources);
%csattribute(Urho3D::ResourceCache, %arg(bool), SearchPackagesFirst, GetSearchPackagesFirst, SetSearchPackagesFirst);
%csattribute(Urho3D::ResourceCache, %arg(int), FinishBackgroundResourcesMs, GetFinishBackgroundResourcesMs, SetFinishBackgroundResourcesMs);
%csattribute(Urho3D::XMLAttributeReference, %arg(Urho3D::XMLElement), Element, GetElement);
%csattribute(Urho3D::XMLAttributeReference, %arg(char *), AttributeName, GetAttributeName);
%csattribute(Urho3D::XMLOutputArchiveBlock, %arg(bool), IsUnorderedAccessSupported, IsUnorderedAccessSupported);
%csattribute(Urho3D::XMLInputArchiveBlock, %arg(bool), IsUnorderedAccessSupported, IsUnorderedAccessSupported);
%pragma(csharp) moduleimports=%{
public static partial class E
{
    public class ReloadStartedEvent {
        private StringHash _event = new StringHash("ReloadStarted");
        public ReloadStartedEvent() { }
        public static implicit operator StringHash(ReloadStartedEvent e) { return e._event; }
    }
    public static ReloadStartedEvent ReloadStarted = new ReloadStartedEvent();
    public class ReloadFinishedEvent {
        private StringHash _event = new StringHash("ReloadFinished");
        public ReloadFinishedEvent() { }
        public static implicit operator StringHash(ReloadFinishedEvent e) { return e._event; }
    }
    public static ReloadFinishedEvent ReloadFinished = new ReloadFinishedEvent();
    public class ReloadFailedEvent {
        private StringHash _event = new StringHash("ReloadFailed");
        public ReloadFailedEvent() { }
        public static implicit operator StringHash(ReloadFailedEvent e) { return e._event; }
    }
    public static ReloadFailedEvent ReloadFailed = new ReloadFailedEvent();
    public class FileChangedEvent {
        private StringHash _event = new StringHash("FileChanged");
        public StringHash FileName = new StringHash("FileName");
        public StringHash ResourceName = new StringHash("ResourceName");
        public FileChangedEvent() { }
        public static implicit operator StringHash(FileChangedEvent e) { return e._event; }
    }
    public static FileChangedEvent FileChanged = new FileChangedEvent();
    public class LoadFailedEvent {
        private StringHash _event = new StringHash("LoadFailed");
        public StringHash ResourceName = new StringHash("ResourceName");
        public LoadFailedEvent() { }
        public static implicit operator StringHash(LoadFailedEvent e) { return e._event; }
    }
    public static LoadFailedEvent LoadFailed = new LoadFailedEvent();
    public class ResourceNotFoundEvent {
        private StringHash _event = new StringHash("ResourceNotFound");
        public StringHash ResourceName = new StringHash("ResourceName");
        public ResourceNotFoundEvent() { }
        public static implicit operator StringHash(ResourceNotFoundEvent e) { return e._event; }
    }
    public static ResourceNotFoundEvent ResourceNotFound = new ResourceNotFoundEvent();
    public class UnknownResourceTypeEvent {
        private StringHash _event = new StringHash("UnknownResourceType");
        public StringHash ResourceType = new StringHash("ResourceType");
        public UnknownResourceTypeEvent() { }
        public static implicit operator StringHash(UnknownResourceTypeEvent e) { return e._event; }
    }
    public static UnknownResourceTypeEvent UnknownResourceType = new UnknownResourceTypeEvent();
    public class ResourceBackgroundLoadedEvent {
        private StringHash _event = new StringHash("ResourceBackgroundLoaded");
        public StringHash ResourceName = new StringHash("ResourceName");
        public StringHash Success = new StringHash("Success");
        public StringHash Resource = new StringHash("Resource");
        public ResourceBackgroundLoadedEvent() { }
        public static implicit operator StringHash(ResourceBackgroundLoadedEvent e) { return e._event; }
    }
    public static ResourceBackgroundLoadedEvent ResourceBackgroundLoaded = new ResourceBackgroundLoadedEvent();
    public class ChangeLanguageEvent {
        private StringHash _event = new StringHash("ChangeLanguage");
        public ChangeLanguageEvent() { }
        public static implicit operator StringHash(ChangeLanguageEvent e) { return e._event; }
    }
    public static ChangeLanguageEvent ChangeLanguage = new ChangeLanguageEvent();
} %}
