%include "eastl_map.i"
%include "eastl_unordered_map.i"

%template(VariantMap)                   eastl::unordered_map<Urho3D::StringHash, Urho3D::Variant>;
%template(AttributeMap)                 eastl::unordered_map<Urho3D::StringHash, eastl::vector<Urho3D::AttributeInfo>>;
%template(PackageMap)                   eastl::unordered_map<eastl::string, Urho3D::PackageEntry>;
%template(JSONObject)                   eastl::map<eastl::string, Urho3D::JSONValue>;
%template(ResourceGroupMap)             eastl::unordered_map<Urho3D::StringHash, Urho3D::ResourceGroup>;
%template(ResourceMap)                  eastl::unordered_map<Urho3D::StringHash, eastl::shared_ptr<Urho3D::Resource>>;
%template(PListValueMap)                eastl::unordered_map<eastl::string, Urho3D::PListValue>;
%template(ComponentReplicationStateMap) eastl::unordered_map<unsigned int, Urho3D::ComponentReplicationState>;
%template(NodeReplicationStateMap)      eastl::unordered_map<unsigned int, Urho3D::NodeReplicationState>;
%template(ValueAnimationInfoMap)        eastl::unordered_map<eastl::string, eastl::shared_ptr<Urho3D::ValueAnimationInfo>>;
%template(AnimationTrackMap)            eastl::unordered_map<Urho3D::StringHash, Urho3D::AnimationTrack>;
%template(MaterialShaderParameterMap)   eastl::unordered_map<Urho3D::StringHash, Urho3D::MaterialShaderParameter>;
%template(TextureMap)                   eastl::unordered_map<Urho3D::TextureUnit, eastl::shared_ptr<Urho3D::Texture>>;
%template(AttributeAnimationInfos)      eastl::unordered_map<eastl::string, eastl::shared_ptr<Urho3D::AttributeAnimationInfo>>;
%template(VertexBufferMorphMap)         eastl::unordered_map<unsigned, Urho3D::VertexBufferMorph>;
%template(ShaderParameterMap)           eastl::unordered_map<Urho3D::StringHash, Urho3D::ShaderParameter>;
