//
// Copyright (c) 2008-2022 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "../Precompiled.h"

#include <EASTL/sort.h>

#include "../Core/Context.h"
#include "../Core/CoreEvents.h"
#include "../Core/Profiler.h"
#include "../Core/Thread.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/Material.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/Technique.h"
#include "../Graphics/Texture2D.h"
#include "../Graphics/Texture2DArray.h"
#include "../Graphics/Texture3D.h"
#include "../Graphics/TextureCube.h"
#include "../IO/FileSystem.h"
#include "../IO/Log.h"
#include "../IO/VectorBuffer.h"
#include "../Resource/ResourceCache.h"
#include "../Resource/ResourceEvents.h"
#include "../Resource/XMLFile.h"
#include "../RenderPipeline/ShaderConsts.h"
#include "../Scene/Scene.h"
#include "../Scene/SceneEvents.h"
#include "../Scene/ValueAnimation.h"

#include "../DebugNew.h"

namespace Urho3D
{

extern const char* wrapModeNames[];

ea::string ParseTextureUnitName(ea::string name)
{
    static const ea::vector<ea::string> unitToName = {
        "diffuse",
        "normal",
        "specular",
        "emissive",
        "environment",
        "volume",
        "custom1",
        "custom2",
        "lightramp",
        "lightshape",
        "shadowmap",
        "faceselect",
        "indirection",
        "depth",
        "light",
        "zone",
    };

    static const ea::unordered_map<ea::string, ea::string> nameToUniform = {
        {"diffuse", ShaderResources::Albedo},
        {"diff", ShaderResources::Albedo},
        {"albedo", ShaderResources::Albedo},
        {"normal", ShaderResources::Normal},
        {"norm", ShaderResources::Normal},
        {"specular", ShaderResources::Properties},
        {"spec", ShaderResources::Properties},
        {"emissive", ShaderResources::Emission},
        {"environment", ShaderResources::Reflection0},
        {"env", ShaderResources::Reflection0},
        {"depth", ShaderResources::DepthBuffer},
        {"zone", ShaderResources::Reflection1},
        {"volume", "Volume"},
        {"custom1", "Custom1"},
        {"custom2", "Custom2"},
        {"faceselect", "FaceSelect"},
        {"indirection", "Indirection"},
        {"light", "LightBuffer"},
        // These units are not supported in practice
        {"lightramp", ShaderResources::LightRamp},
        {"lightshape", ShaderResources::LightShape},
        {"shadowmap", ShaderResources::ShadowMap},
    };

    name.make_lower();
    name.trim();

    if (name.length() < 3)
    {
        const int textureUnit = ToInt(name);
        if (textureUnit >= 0 && textureUnit < unitToName.size())
            name = unitToName[textureUnit];
    }

    const auto iter = nameToUniform.find(name);
    if (iter != nameToUniform.end())
        return iter->second;

    return name;
}

StringHash ParseTextureTypeName(const ea::string& name)
{
    ea::string lowerCaseName = name;
    lowerCaseName.make_lower();
    lowerCaseName.trim();

    if (lowerCaseName == "texture")
        return Texture2D::GetTypeStatic();
    else if (lowerCaseName == "cubemap")
        return TextureCube::GetTypeStatic();
    else if (lowerCaseName == "texture3d")
        return Texture3D::GetTypeStatic();
    else if (lowerCaseName == "texturearray")
        return Texture2DArray::GetTypeStatic();

    return nullptr;
}

StringHash ParseTextureTypeXml(ResourceCache* cache, const ea::string& filename)
{
    StringHash type;
    if (!cache)
        return type;

    auto texXml = cache->GetTempResource<XMLFile>(filename, false);
    if (texXml)
        type = ParseTextureTypeName(texXml->GetRoot().GetName());
    return type;
}

static const TechniqueEntry noEntry;

TechniqueEntry::TechniqueEntry() noexcept :
    qualityLevel_(QUALITY_LOW),
    lodDistance_(0.0f)
{
}

TechniqueEntry::TechniqueEntry(Technique* tech, MaterialQuality qualityLevel, float lodDistance) noexcept :
    technique_(tech),
    original_(tech),
    qualityLevel_(qualityLevel),
    lodDistance_(lodDistance)
{
}

bool TechniqueEntry::operator<(const TechniqueEntry& rhs) const
{
    if (lodDistance_ != rhs.lodDistance_)
        return lodDistance_ > rhs.lodDistance_;
    else
        return qualityLevel_ > rhs.qualityLevel_;
}

bool TechniqueEntry::operator ==(const TechniqueEntry& rhs) const
{
    return technique_ == rhs.technique_
        && qualityLevel_ == rhs.qualityLevel_
        && lodDistance_ == rhs.lodDistance_;
}

ShaderParameterAnimationInfo::ShaderParameterAnimationInfo(Material* material, const ea::string& name, ValueAnimation* attributeAnimation,
    WrapMode wrapMode, float speed) :
    ValueAnimationInfo(material, attributeAnimation, wrapMode, speed),
    name_(name)
{
}

ShaderParameterAnimationInfo::ShaderParameterAnimationInfo(const ShaderParameterAnimationInfo& other) = default;

ShaderParameterAnimationInfo::~ShaderParameterAnimationInfo() = default;

void ShaderParameterAnimationInfo::ApplyValue(const Variant& newValue)
{
    static_cast<Material*>(target_.Get())->SetShaderParameter(name_, newValue);
}

Material::Material(Context* context) :
    Resource(context)
{
    ResetToDefaults();
}

Material::~Material() = default;

void Material::RegisterObject(Context* context)
{
    context->AddFactoryReflection<Material>();
}

SharedPtr<Material> Material::CreateBaseMaterial(Context* context,
    const ea::string& shaderName, const ea::string& vsDefines, const ea::string& psDefines)
{
    auto technique = MakeShared<Technique>(context);
    Pass* pass = technique->CreatePass("base");
    pass->SetVertexShader(shaderName);
    pass->SetVertexShaderDefines(vsDefines);
    pass->SetPixelShader(shaderName);
    pass->SetPixelShaderDefines(psDefines);

    auto material = MakeShared<Material>(context);
    material->SetTechnique(0, technique);

    return material;
}

bool Material::BeginLoad(Deserializer& source)
{
    // In headless mode, do not actually load the material, just return success
    auto* graphics = GetSubsystem<Graphics>();
    if (!graphics)
        return true;

    const InternalResourceFormat format = PeekResourceFormat(source);

    if (format == InternalResourceFormat::Xml)
    {
        if (BeginLoadXML(source))
            return true;
    }

    // All loading failed
    loadXMLFile_.Reset();
    ResetToDefaults();
    return false;
}

bool Material::EndLoad()
{
    // In headless mode, do not actually load the material, just return success
    auto* graphics = GetSubsystem<Graphics>();
    if (!graphics)
        return true;

    bool success = false;
    if (loadXMLFile_)
    {
        // If async loading, get the techniques / textures which should be ready now
        XMLElement rootElem = loadXMLFile_->GetRoot();
        success = Load(rootElem);
    }

    loadXMLFile_.Reset();
    return success;
}

bool Material::BeginLoadXML(Deserializer& source)
{
    ResetToDefaults();
    loadXMLFile_ = MakeShared<XMLFile>(context_);
    if (loadXMLFile_->Load(source))
    {
        // If async loading, scan the XML content beforehand for technique & texture resources
        // and request them to also be loaded. Can not do anything else at this point
        if (GetAsyncLoadState() == ASYNC_LOADING)
        {
            auto* cache = GetSubsystem<ResourceCache>();
            XMLElement rootElem = loadXMLFile_->GetRoot();
            XMLElement techniqueElem = rootElem.GetChild("technique");
            while (techniqueElem)
            {
                cache->BackgroundLoadResource<Technique>(techniqueElem.GetAttribute("name"), true, this);
                techniqueElem = techniqueElem.GetNext("technique");
            }

            XMLElement textureElem = rootElem.GetChild("texture");
            while (textureElem)
            {
                ea::string resourceName = textureElem.GetAttribute("name");
                // Detect cube maps and arrays by file extension: they are defined by an XML file
                if (GetExtension(resourceName) == ".xml")
                {
                    StringHash type = ParseTextureTypeXml(cache, resourceName);
                    if (!type && textureElem.HasAttribute("type"))
                        type = textureElem.GetAttribute("type");

                    if (type == Texture3D::GetTypeStatic())
                        cache->BackgroundLoadResource<Texture3D>(resourceName, true, this);
                    else if (type == Texture2DArray::GetTypeStatic())
                        cache->BackgroundLoadResource<Texture2DArray>(resourceName, true, this);
                    else
                        cache->BackgroundLoadResource<TextureCube>(resourceName, true, this);
                }
                else
                    cache->BackgroundLoadResource<Texture2D>(resourceName, true, this);
                textureElem = textureElem.GetNext("texture");
            }
        }

        return true;
    }

    return false;
}

bool Material::Save(Serializer& dest) const
{
    SharedPtr<XMLFile> xml(MakeShared<XMLFile>(context_));
    XMLElement materialElem = xml->CreateRoot("material");

    Save(materialElem);
    return xml->Save(dest);
}

bool Material::Load(const XMLElement& source)
{
    ResetToDefaults();

    if (source.IsNull())
    {
        URHO3D_LOGERROR("Can not load material from null XML element");
        return false;
    }

    auto* cache = GetSubsystem<ResourceCache>();

    XMLElement shaderElem = source.GetChild("shader");
    if (shaderElem)
    {
        vertexShaderDefines_ = shaderElem.GetAttribute("vsdefines");
        pixelShaderDefines_ = shaderElem.GetAttribute("psdefines");
    }

    XMLElement techniqueElem = source.GetChild("technique");
    techniques_.clear();

    while (techniqueElem)
    {
        auto* tech = cache->GetResource<Technique>(techniqueElem.GetAttribute("name"));
        if (tech)
        {
            TechniqueEntry newTechnique;
            newTechnique.technique_ = newTechnique.original_ = tech;
            if (techniqueElem.HasAttribute("quality"))
                newTechnique.qualityLevel_ = (MaterialQuality)techniqueElem.GetInt("quality");
            if (techniqueElem.HasAttribute("loddistance"))
                newTechnique.lodDistance_ = techniqueElem.GetFloat("loddistance");
            techniques_.push_back(newTechnique);
        }

        techniqueElem = techniqueElem.GetNext("technique");
    }

    SortTechniques();
    ApplyShaderDefines();

    XMLElement textureElem = source.GetChild("texture");
    while (textureElem)
    {
        ea::string slotName = textureElem.GetAttribute("slot");
        if (slotName.empty() && textureElem.HasAttribute("unit"))
            slotName = ParseTextureUnitName(textureElem.GetAttribute("unit"));

        const ea::string resourceName = textureElem.GetAttribute("name");
        // Detect cube maps and arrays by file extension: they are defined by an XML file
        if (GetExtension(resourceName) == ".xml")
        {
            StringHash type = ParseTextureTypeXml(cache, resourceName);
            if (!type && textureElem.HasAttribute("type"))
                type = textureElem.GetAttribute("type");

            if (type == Texture3D::GetTypeStatic())
                SetTextureInternal(slotName, cache->GetResource<Texture3D>(resourceName));
            else if (type == Texture2DArray::GetTypeStatic())
                SetTextureInternal(slotName, cache->GetResource<Texture2DArray>(resourceName));
            else
                SetTextureInternal(slotName, cache->GetResource<TextureCube>(resourceName));
        }
        else
            SetTextureInternal(slotName, cache->GetResource<Texture2D>(resourceName));

        textureElem = textureElem.GetNext("texture");
    }
    RefreshTextureEventSubscriptions();

    batchedParameterUpdate_ = true;
    XMLElement parameterElem = source.GetChild("parameter");
    while (parameterElem)
    {
        ea::string name = parameterElem.GetAttribute("name");
        if (!parameterElem.HasAttribute("type"))
            SetShaderParameter(name, ParseShaderParameterValue(parameterElem.GetAttribute("value")));
        else
            SetShaderParameter(name, Variant(parameterElem.GetAttribute("type"), parameterElem.GetAttribute("value")));
        parameterElem = parameterElem.GetNext("parameter");
    }
    batchedParameterUpdate_ = false;

    XMLElement parameterAnimationElem = source.GetChild("parameteranimation");
    while (parameterAnimationElem)
    {
        ea::string name = parameterAnimationElem.GetAttribute("name");
        SharedPtr<ValueAnimation> animation(MakeShared<ValueAnimation>(context_));
        if (!animation->LoadXML(parameterAnimationElem))
        {
            URHO3D_LOGERROR("Could not load parameter animation");
            return false;
        }

        ea::string wrapModeString = parameterAnimationElem.GetAttribute("wrapmode");
        WrapMode wrapMode = WM_LOOP;
        for (int i = 0; i <= WM_CLAMP; ++i)
        {
            if (wrapModeString == wrapModeNames[i])
            {
                wrapMode = (WrapMode)i;
                break;
            }
        }

        float speed = parameterAnimationElem.GetFloat("speed");
        SetShaderParameterAnimation(name, animation, wrapMode, speed);

        parameterAnimationElem = parameterAnimationElem.GetNext("parameteranimation");
    }

    XMLElement cullElem = source.GetChild("cull");
    if (cullElem)
        SetCullMode((CullMode)GetStringListIndex(cullElem.GetAttribute("value").c_str(), cullModeNames, CULL_CCW));

    XMLElement shadowCullElem = source.GetChild("shadowcull");
    if (shadowCullElem)
        SetShadowCullMode((CullMode)GetStringListIndex(shadowCullElem.GetAttribute("value").c_str(), cullModeNames, CULL_CCW));

    XMLElement fillElem = source.GetChild("fill");
    if (fillElem)
        SetFillMode((FillMode)GetStringListIndex(fillElem.GetAttribute("value").c_str(), fillModeNames, FILL_SOLID));

    XMLElement depthBiasElem = source.GetChild("depthbias");
    if (depthBiasElem)
        SetDepthBias(BiasParameters(depthBiasElem.GetFloat("constant"), depthBiasElem.GetFloat("slopescaled"), depthBiasElem.GetFloat("normaloffset")));

    XMLElement alphaToCoverageElem = source.GetChild("alphatocoverage");
    if (alphaToCoverageElem)
        SetAlphaToCoverage(alphaToCoverageElem.GetBool("enable"));

    XMLElement lineAntiAliasElem = source.GetChild("lineantialias");
    if (lineAntiAliasElem)
        SetLineAntiAlias(lineAntiAliasElem.GetBool("enable"));

    XMLElement renderOrderElem = source.GetChild("renderorder");
    if (renderOrderElem)
        SetRenderOrder((unsigned char)renderOrderElem.GetUInt("value"));

    XMLElement occlusionElem = source.GetChild("occlusion");
    if (occlusionElem)
        SetOcclusion(occlusionElem.GetBool("enable"));

    RefreshShaderParameterHash();
    RefreshMemoryUse();
    return true;
}

bool Material::Save(XMLElement& dest) const
{
    if (dest.IsNull())
    {
        URHO3D_LOGERROR("Can not save material to null XML element");
        return false;
    }

    // Write techniques
    for (unsigned i = 0; i < techniques_.size(); ++i)
    {
        const TechniqueEntry& entry = techniques_[i];
        if (!entry.technique_)
            continue;

        XMLElement techniqueElem = dest.CreateChild("technique");
        techniqueElem.SetString("name", entry.technique_->GetName());
        techniqueElem.SetInt("quality", entry.qualityLevel_);
        techniqueElem.SetFloat("loddistance", entry.lodDistance_);
    }

    // Write texture units
    auto textures = textures_.values();
    const auto isLess = [](const MaterialTexture& lhs, const MaterialTexture& rhs) { return lhs.name_ < rhs.name_; };
    ea::sort(textures.begin(), textures.end(), isLess);
    for (const MaterialTexture& texture : textures)
    {
        if (texture.value_)
        {
            XMLElement textureElem = dest.CreateChild("texture");
            textureElem.SetString("slot", texture.name_);
            textureElem.SetString("name", texture.value_->GetName());
        }
    }

    // Write shader compile defines
    if (!vertexShaderDefines_.empty() || !pixelShaderDefines_.empty())
    {
        XMLElement shaderElem = dest.CreateChild("shader");
        if (!vertexShaderDefines_.empty())
            shaderElem.SetString("vsdefines", vertexShaderDefines_);
        if (!pixelShaderDefines_.empty())
            shaderElem.SetString("psdefines", pixelShaderDefines_);
    }

    // Write shader parameters
    for (auto j = shaderParameters_.begin();
         j != shaderParameters_.end(); ++j)
    {
        XMLElement parameterElem = dest.CreateChild("parameter");
        parameterElem.SetString("name", j->second.name_);
        if (!parameterElem.SetVectorVariant("value", j->second.value_))
        {
            if (j->second.value_.GetType() != VAR_BUFFER && j->second.value_.GetType() != VAR_VOIDPTR)
            {
                parameterElem.SetAttribute("type", j->second.value_.GetTypeName());
                parameterElem.SetAttribute("value", j->second.value_.ToString());
            }
        }
    }

    // Write shader parameter animations
    for (auto j = shaderParameterAnimationInfos_.begin();
         j != shaderParameterAnimationInfos_.end(); ++j)
    {
        ShaderParameterAnimationInfo* info = j->second;
        XMLElement parameterAnimationElem = dest.CreateChild("parameteranimation");
        parameterAnimationElem.SetString("name", info->GetName());
        if (!info->GetAnimation()->SaveXML(parameterAnimationElem))
            return false;

        parameterAnimationElem.SetAttribute("wrapmode", wrapModeNames[info->GetWrapMode()]);
        parameterAnimationElem.SetFloat("speed", info->GetSpeed());
    }

    // Write culling modes
    XMLElement cullElem = dest.CreateChild("cull");
    cullElem.SetString("value", cullModeNames[cullMode_]);

    XMLElement shadowCullElem = dest.CreateChild("shadowcull");
    shadowCullElem.SetString("value", cullModeNames[shadowCullMode_]);

    // Write fill mode
    XMLElement fillElem = dest.CreateChild("fill");
    fillElem.SetString("value", fillModeNames[fillMode_]);

    // Write depth bias
    XMLElement depthBiasElem = dest.CreateChild("depthbias");
    depthBiasElem.SetFloat("constant", depthBias_.constantBias_);
    depthBiasElem.SetFloat("slopescaled", depthBias_.slopeScaledBias_);
    depthBiasElem.SetFloat("normaloffset", depthBias_.normalOffset_);

    // Write alpha-to-coverage
    XMLElement alphaToCoverageElem = dest.CreateChild("alphatocoverage");
    alphaToCoverageElem.SetBool("enable", alphaToCoverage_);

    // Write line anti-alias
    XMLElement lineAntiAliasElem = dest.CreateChild("lineantialias");
    lineAntiAliasElem.SetBool("enable", lineAntiAlias_);

    // Write render order
    XMLElement renderOrderElem = dest.CreateChild("renderorder");
    renderOrderElem.SetUInt("value", renderOrder_);

    // Write occlusion
    XMLElement occlusionElem = dest.CreateChild("occlusion");
    occlusionElem.SetBool("enable", occlusion_);

    return true;
}

void Material::SetNumTechniques(unsigned num)
{
    if (!num)
        return;

    techniques_.resize(num);
    RefreshMemoryUse();
}

void Material::SetTechnique(unsigned index, Technique* tech, MaterialQuality qualityLevel, float lodDistance)
{
    if (index >= techniques_.size())
        return;

    techniques_[index] = TechniqueEntry(tech, qualityLevel, lodDistance);
    ApplyShaderDefines(index);
}

void Material::SetTechniques(const ea::vector<TechniqueEntry>& techniques)
{
    techniques_ = techniques;
    SortTechniques();
    ApplyShaderDefines();
}

void Material::SetVertexShaderDefines(const ea::string& defines)
{
    if (defines != vertexShaderDefines_)
    {
        vertexShaderDefines_ = defines;
        ApplyShaderDefines();
        MarkPipelineStateHashDirty();
    }
}

void Material::SetPixelShaderDefines(const ea::string& defines)
{
    if (defines != pixelShaderDefines_)
    {
        pixelShaderDefines_ = defines;
        ApplyShaderDefines();
        MarkPipelineStateHashDirty();
    }
}

void Material::SetShaderParameter(const ea::string& name, const Variant& value, bool isCustom)
{
    MaterialShaderParameter newParam;
    newParam.name_ = name;
    newParam.value_ = value;
    newParam.isCustom_ = isCustom;

    StringHash nameHash(name);
    shaderParameters_[nameHash] = newParam;

    if (nameHash == PSP_MATSPECCOLOR)
    {
        VariantType type = value.GetType();
        const bool oldSpecular = specular_;
        if (type == VAR_VECTOR3)
        {
            const Vector3& vec = value.GetVector3();
            specular_ = vec.x_ > 0.0f || vec.y_ > 0.0f || vec.z_ > 0.0f;
        }
        else if (type == VAR_VECTOR4)
        {
            const Vector4& vec = value.GetVector4();
            specular_ = vec.x_ > 0.0f || vec.y_ > 0.0f || vec.z_ > 0.0f;
        }
        if (oldSpecular != specular_)
            MarkPipelineStateHashDirty();
    }

    if (!batchedParameterUpdate_)
    {
        RefreshShaderParameterHash();
        RefreshMemoryUse();
    }
}

void Material::SetShaderParameterAnimation(const ea::string& name, ValueAnimation* animation, WrapMode wrapMode, float speed)
{
    ShaderParameterAnimationInfo* info = GetShaderParameterAnimationInfo(name);

    if (animation)
    {
        if (info && info->GetAnimation() == animation)
        {
            info->SetWrapMode(wrapMode);
            info->SetSpeed(speed);
            return;
        }

        if (shaderParameters_.find(name) == shaderParameters_.end())
        {
            URHO3D_LOGERROR(GetName() + " has no shader parameter: " + name);
            return;
        }

        StringHash nameHash(name);
        shaderParameterAnimationInfos_[nameHash] = new ShaderParameterAnimationInfo(this, name, animation, wrapMode, speed);
        UpdateEventSubscription();
    }
    else
    {
        if (info)
        {
            StringHash nameHash(name);
            shaderParameterAnimationInfos_.erase(nameHash);
            UpdateEventSubscription();
        }
    }
}

void Material::SetShaderParameterAnimationWrapMode(const ea::string& name, WrapMode wrapMode)
{
    ShaderParameterAnimationInfo* info = GetShaderParameterAnimationInfo(name);
    if (info)
        info->SetWrapMode(wrapMode);
}

void Material::SetShaderParameterAnimationSpeed(const ea::string& name, float speed)
{
    ShaderParameterAnimationInfo* info = GetShaderParameterAnimationInfo(name);
    if (info)
        info->SetSpeed(speed);
}

void Material::SetTexture(ea::string_view name, Texture* texture)
{
    SetTextureInternal(name, texture);
    RefreshTextureEventSubscriptions();
}

void Material::SetTextureInternal(ea::string_view name, Texture* texture)
{
    const auto iter = textures_.find(StringHash{name});
    if (iter != textures_.end())
    {
        if (texture)
            iter->second.value_ = texture;
        else
            textures_.erase(iter);
    }
    else if (texture)
    {
        textures_.emplace(StringHash{name}, MaterialTexture{ea::string{name}, SharedPtr<Texture>{texture}});
    }
}

void Material::SetUVTransform(const Vector2& offset, float rotation, const Vector2& repeat)
{
    Matrix3x4 transform(Matrix3x4::IDENTITY);
    transform.m00_ = repeat.x_;
    transform.m11_ = repeat.y_;

    Matrix3x4 rotationMatrix(Matrix3x4::IDENTITY);
    rotationMatrix.m00_ = Cos(rotation);
    rotationMatrix.m01_ = Sin(rotation);
    rotationMatrix.m10_ = -rotationMatrix.m01_;
    rotationMatrix.m11_ = rotationMatrix.m00_;
    rotationMatrix.m03_ = 0.5f - 0.5f * (rotationMatrix.m00_ + rotationMatrix.m01_);
    rotationMatrix.m13_ = 0.5f - 0.5f * (rotationMatrix.m10_ + rotationMatrix.m11_);

    transform = transform * rotationMatrix;

    Matrix3x4 offsetMatrix = Matrix3x4::IDENTITY;
    offsetMatrix.m03_ = offset.x_;
    offsetMatrix.m13_ = offset.y_;

    transform = offsetMatrix * transform;

    SetShaderParameter("UOffset", Vector4(transform.m00_, transform.m01_, transform.m02_, transform.m03_));
    SetShaderParameter("VOffset", Vector4(transform.m10_, transform.m11_, transform.m12_, transform.m13_));
}

void Material::SetUVTransform(const Vector2& offset, float rotation, float repeat)
{
    SetUVTransform(offset, rotation, Vector2(repeat, repeat));
}

void Material::SetCullMode(CullMode mode)
{
    cullMode_ = mode;
    MarkPipelineStateHashDirty();
}

void Material::SetShadowCullMode(CullMode mode)
{
    shadowCullMode_ = mode;
    MarkPipelineStateHashDirty();
}

void Material::SetFillMode(FillMode mode)
{
    fillMode_ = mode;
    MarkPipelineStateHashDirty();
}

void Material::SetDepthBias(const BiasParameters& parameters)
{
    depthBias_ = parameters;
    depthBias_.Validate();
    MarkPipelineStateHashDirty();
}

void Material::SetAlphaToCoverage(bool enable)
{
    alphaToCoverage_ = enable;
    MarkPipelineStateHashDirty();
}

void Material::SetLineAntiAlias(bool enable)
{
    lineAntiAlias_ = enable;
}

void Material::SetRenderOrder(unsigned char order)
{
    renderOrder_ = order;
}

void Material::SetOcclusion(bool enable)
{
    occlusion_ = enable;
}

void Material::SetScene(Scene* scene)
{
    UnsubscribeFromEvent(E_UPDATE);
    UnsubscribeFromEvent(E_ATTRIBUTEANIMATIONUPDATE);
    subscribed_ = false;
    scene_ = scene;
    UpdateEventSubscription();
}

void Material::RemoveShaderParameter(const ea::string& name)
{
    StringHash nameHash(name);
    shaderParameters_.erase(nameHash);

    if (nameHash == PSP_MATSPECCOLOR)
    {
        if (specular_)
            MarkPipelineStateHashDirty();
        specular_ = false;
    }

    RefreshShaderParameterHash();
    RefreshMemoryUse();
}

void Material::ReleaseShaders()
{
    for (unsigned i = 0; i < techniques_.size(); ++i)
    {
        Technique* tech = techniques_[i].technique_;
        if (tech)
            tech->ReleaseShaders();
    }
}

SharedPtr<Material> Material::Clone(const ea::string& cloneName) const
{
    SharedPtr<Material> ret(MakeShared<Material>(context_));
    ret->CopyFrom(this);
    ret->SetName(cloneName);
    return ret;
}

void Material::CopyFrom(const Material* material)
{
    this->SetName(material->GetName());
    techniques_ = material->techniques_;
    vertexShaderDefines_ = material->vertexShaderDefines_;
    pixelShaderDefines_ = material->pixelShaderDefines_;
    shaderParameters_ = material->shaderParameters_;
    shaderParameterHash_ = material->shaderParameterHash_;
    textures_ = material->textures_;
    depthBias_ = material->depthBias_;
    alphaToCoverage_ = material->alphaToCoverage_;
    lineAntiAlias_ = material->lineAntiAlias_;
    occlusion_ = material->occlusion_;
    specular_ = material->specular_;
    cullMode_ = material->cullMode_;
    shadowCullMode_ = material->shadowCullMode_;
    fillMode_ = material->fillMode_;
    renderOrder_ = material->renderOrder_;
    RefreshMemoryUse();
    RefreshTextureEventSubscriptions();
}

void Material::SortTechniques()
{
    ea::sort(techniques_.begin(), techniques_.end());
}

void Material::MarkForAuxView(unsigned frameNumber)
{
    auxViewFrameNumber_.store(frameNumber, std::memory_order_relaxed);
}

const TechniqueEntry& Material::GetTechniqueEntry(unsigned index) const
{
    return index < techniques_.size() ? techniques_[index] : noEntry;
}

Technique* Material::GetTechnique(unsigned index) const
{
    return index < techniques_.size() ? techniques_[index].technique_ : nullptr;
}

Technique* Material::FindTechnique(Drawable* drawable, MaterialQuality materialQuality) const
{
    const ea::vector<TechniqueEntry>& techniques = GetTechniques();

    // If only one technique, no choice
    if (techniques.size() == 1)
        return techniques[0].technique_;

    // TODO: Consider optimizing this loop
    const float lodDistance = drawable->GetLodDistance();
    for (unsigned i = 0; i < techniques.size(); ++i)
    {
        const TechniqueEntry& entry = techniques[i];
        Technique* tech = entry.technique_;

        if (!tech || materialQuality < entry.qualityLevel_)
            continue;
        if (lodDistance >= entry.lodDistance_)
            return tech;
    }

    // If no suitable technique found, fallback to the last
    return techniques.size() ? techniques.back().technique_ : nullptr;
}

Pass* Material::GetPass(unsigned index, const ea::string& passName) const
{
    Technique* tech = index < techniques_.size() ? techniques_[index].technique_ : nullptr;
    return tech ? tech->GetPass(passName) : nullptr;
}

Pass* Material::GetDefaultPass() const
{
    static const unsigned basePassIndex = Technique::GetPassIndex("base");
    if (Technique* tech = GetTechnique(0))
        return tech->GetPass(basePassIndex);
    return nullptr;
}

Texture* Material::GetTexture(StringHash nameHash) const
{
    const auto iter = textures_.find(nameHash);
    return iter != textures_.end() ? iter->second.value_ : nullptr;
}

const Variant& Material::GetShaderParameter(const ea::string& name) const
{
    auto i = shaderParameters_.find(name);
    return i != shaderParameters_.end() ? i->second.value_ : Variant::EMPTY;
}

ValueAnimation* Material::GetShaderParameterAnimation(const ea::string& name) const
{
    ShaderParameterAnimationInfo* info = GetShaderParameterAnimationInfo(name);
    return info == nullptr ? nullptr : info->GetAnimation();
}

WrapMode Material::GetShaderParameterAnimationWrapMode(const ea::string& name) const
{
    ShaderParameterAnimationInfo* info = GetShaderParameterAnimationInfo(name);
    return info == nullptr ? WM_LOOP : info->GetWrapMode();
}

float Material::GetShaderParameterAnimationSpeed(const ea::string& name) const
{
    ShaderParameterAnimationInfo* info = GetShaderParameterAnimationInfo(name);
    return info == nullptr ? 0 : info->GetSpeed();
}

Scene* Material::GetScene() const
{
    return scene_;
}

Variant Material::ParseShaderParameterValue(const ea::string& value)
{
    ea::string valueTrimmed = value.trimmed();
    if (valueTrimmed.length() && IsAlpha((unsigned)valueTrimmed[0]))
        return Variant(ToBool(valueTrimmed));
    else
        return ToVectorVariant(valueTrimmed);
}

void Material::ResetToDefaults()
{
    // Needs to be a no-op when async loading, as this does a GetResource() which is not allowed from worker threads
    if (!Thread::IsMainThread())
        return;

    vertexShaderDefines_.clear();
    pixelShaderDefines_.clear();

    SetNumTechniques(1);
    auto* renderer = GetSubsystem<Renderer>();
    SetTechnique(0, renderer ? renderer->GetDefaultTechnique() :
        GetSubsystem<ResourceCache>()->GetResource<Technique>("Techniques/NoTexture.xml"));

    textures_.clear();
    RefreshTextureEventSubscriptions();

    batchedParameterUpdate_ = true;
    shaderParameters_.clear();
    shaderParameterAnimationInfos_.clear();
    SetShaderParameter("UOffset", Vector4(1.0f, 0.0f, 0.0f, 0.0f));
    SetShaderParameter("VOffset", Vector4(0.0f, 1.0f, 0.0f, 0.0f));
    SetShaderParameter("MatDiffColor", Vector4::ONE);
    SetShaderParameter("MatEmissiveColor", Vector3::ZERO);
    SetShaderParameter("MatEnvMapColor", Vector3::ONE);
    SetShaderParameter("MatSpecColor", Vector4(0.0f, 0.0f, 0.0f, 1.0f));
    SetShaderParameter("Roughness", 1.0f);
    SetShaderParameter("Metallic", 1.0f);
    SetShaderParameter("DielectricReflectance", 0.5f);
    SetShaderParameter("NormalScale", 1.0f);
    SetShaderParameter("AlphaCutoff", 0.5f);
    batchedParameterUpdate_ = false;

    cullMode_ = CULL_CCW;
    shadowCullMode_ = CULL_CCW;
    fillMode_ = FILL_SOLID;
    depthBias_ = BiasParameters(0.0f, 0.0f);
    renderOrder_ = DEFAULT_RENDER_ORDER;
    occlusion_ = true;

    UpdateEventSubscription();
    RefreshShaderParameterHash();
    RefreshMemoryUse();
}

void Material::RefreshShaderParameterHash()
{
    VectorBuffer temp;
    for (auto i = shaderParameters_.begin();
         i != shaderParameters_.end(); ++i)
    {
        temp.WriteStringHash(i->first);
        temp.WriteVariant(i->second.value_);
    }

    shaderParameterHash_ = 0;
    const unsigned char* data = temp.GetData();
    unsigned dataSize = temp.GetSize();
    for (unsigned i = 0; i < dataSize; ++i)
        shaderParameterHash_ = SDBMHash(shaderParameterHash_, data[i]);
}

void Material::RefreshMemoryUse()
{
    unsigned memoryUse = sizeof(Material);

    memoryUse += techniques_.size() * sizeof(TechniqueEntry);
    memoryUse += textures_.size() * sizeof(SharedPtr<Texture>);
    memoryUse += shaderParameters_.size() * sizeof(MaterialShaderParameter);

    SetMemoryUse(memoryUse);
}

ShaderParameterAnimationInfo* Material::GetShaderParameterAnimationInfo(const ea::string& name) const
{
    StringHash nameHash(name);
    auto i = shaderParameterAnimationInfos_.find(
        nameHash);
    if (i == shaderParameterAnimationInfos_.end())
        return nullptr;
    return i->second;
}

void Material::UpdateEventSubscription()
{
    if (shaderParameterAnimationInfos_.size() && !subscribed_)
    {
        if (scene_)
            SubscribeToEvent(scene_, E_ATTRIBUTEANIMATIONUPDATE, URHO3D_HANDLER(Material, HandleAttributeAnimationUpdate));
        else
            SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(Material, HandleAttributeAnimationUpdate));
        subscribed_ = true;
    }
    else if (subscribed_ && shaderParameterAnimationInfos_.empty())
    {
        UnsubscribeFromEvent(E_UPDATE);
        UnsubscribeFromEvent(E_ATTRIBUTEANIMATIONUPDATE);
        subscribed_ = false;
    }
}

void Material::HandleAttributeAnimationUpdate(StringHash eventType, VariantMap& eventData)
{
    // Timestep parameter is same no matter what event is being listened to
    float timeStep = eventData[Update::P_TIMESTEP].GetFloat();

    // Keep weak pointer to self to check for destruction caused by event handling
    WeakPtr<Object> self(this);

    ea::vector<ea::string> finishedNames;
    for (auto i = shaderParameterAnimationInfos_.begin();
         i != shaderParameterAnimationInfos_.end(); ++i)
    {
        bool finished = i->second->Update(timeStep);
        // If self deleted as a result of an event sent during animation playback, nothing more to do
        if (self.Expired())
            return;

        if (finished)
            finishedNames.push_back(i->second->GetName());
    }

    // Remove finished animations
    for (unsigned i = 0; i < finishedNames.size(); ++i)
        SetShaderParameterAnimation(finishedNames[i], nullptr);
}

void Material::ApplyShaderDefines(unsigned index)
{
    if (index == M_MAX_UNSIGNED)
    {
        for (unsigned i = 0; i < techniques_.size(); ++i)
            ApplyShaderDefines(i);
        return;
    }

    if (index >= techniques_.size() || !techniques_[index].original_)
        return;

    if (vertexShaderDefines_.empty() && pixelShaderDefines_.empty())
        techniques_[index].technique_ = techniques_[index].original_;
    else
        techniques_[index].technique_ = techniques_[index].original_->CloneWithDefines(vertexShaderDefines_, pixelShaderDefines_);
}

void Material::RefreshTextureEventSubscriptions()
{
    UnsubscribeFromEvent(E_RELOADFINISHED);
    const auto onReload = [this] { MarkPipelineStateHashDirty(); };
    for (const auto& item : textures_)
        SubscribeToEvent(item.second.value_, E_RELOADFINISHED, onReload);
    MarkPipelineStateHashDirty();
}

unsigned Material::RecalculatePipelineStateHash() const
{
    unsigned hash = 0;
    CombineHash(hash, MakeHash(vertexShaderDefines_));
    CombineHash(hash, MakeHash(pixelShaderDefines_));
    CombineHash(hash, cullMode_);
    CombineHash(hash, shadowCullMode_);
    CombineHash(hash, fillMode_);
    CombineHash(hash, MakeHash(depthBias_.constantBias_));
    CombineHash(hash, MakeHash(depthBias_.slopeScaledBias_));
    CombineHash(hash, alphaToCoverage_);
    CombineHash(hash, specular_);
    for (const auto& [nameHash, info] : textures_)
    {
        CombineHash(hash, nameHash.Value());
        CombineHash(hash, info.value_->GetSRGB());
        CombineHash(hash, info.value_->GetLinear());
        CombineHash(hash, info.value_->GetSamplerStateDesc().ToHash());
    }

    return hash;
}

}
