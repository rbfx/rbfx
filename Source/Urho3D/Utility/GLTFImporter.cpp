//
// Copyright (c) 2008-2020 the Urho3D project.
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

#include "../Core/Context.h"
#include "../Core/Exception.h"
#include "../Graphics/Light.h"
#include "../Graphics/Material.h"
#include "../Graphics/Model.h"
#include "../Graphics/ModelView.h"
#include "../Graphics/Octree.h"
#include "../Graphics/Texture.h"
#include "../Graphics/Texture2D.h"
#include "../Graphics/StaticModel.h"
#include "../IO/FileSystem.h"
#include "../IO/Log.h"
#include "../RenderPipeline/ShaderConsts.h"
#include "../Resource/BinaryFile.h"
#include "../Resource/Image.h"
#include "../Resource/ResourceCache.h"
#include "../Resource/XMLFile.h"
#include "../Scene/Scene.h"
#include "../Utility/GLTFImporter.h"

#include <tiny_gltf.h>

#include <EASTL/optional.h>
#include <EASTL/unordered_set.h>
#include <EASTL/unordered_map.h>

#include <exception>

#include "../DebugNew.h"

namespace Urho3D
{

namespace tg = tinygltf;

namespace
{

template <class T>
struct StaticCaster
{
    template <class U>
    T operator() (U x) const { return static_cast<T>(x); }
};

int GetByteStride(const tg::BufferView& bufferViewObject, int componentType, int type)
{
    const int componentSizeInBytes = tg::GetComponentSizeInBytes(static_cast<uint32_t>(componentType));
    const int numComponents = tg::GetNumComponentsInType(static_cast<uint32_t>(type));
    if (componentSizeInBytes <= 0 || numComponents <= 0)
        return -1;

    return bufferViewObject.byteStride == 0
        ? componentSizeInBytes * numComponents
        : static_cast<int>(bufferViewObject.byteStride);
}

template <class T>
constexpr bool IsFloatAggregate()
{
    return ea::is_same_v<T, Vector2>
        || ea::is_same_v<T, Vector3>
        || ea::is_same_v<T, Vector4>;
}

template <class T>
ea::vector<T> RepackFloatArray(const ea::vector<float>& componentsArray, unsigned numObjects)
{
    static_assert(IsFloatAggregate<T>(), "Cannot repack to this type");

    ea::vector<T> result;
    result.resize(numObjects);

    const unsigned numComponents = sizeof(T) / sizeof(float);
    for (unsigned i = 0; i < numObjects; ++i)
    {
        if ((i + 1) * numComponents >= componentsArray.size())
            break;

        result[i] = T{ &componentsArray[i * numComponents] };
    }

    return result;
}

template <class T, unsigned N, class U>
ea::array<T, N> ToArray(const U& vec)
{
    ea::array<T, N> result{};
    if (vec.size() >= N)
        ea::transform(vec.begin(), vec.begin() + N, result.begin(), StaticCaster<T>{});
    return result;
}

class GLTFImporterContext : public NonCopyable
{
public:
    GLTFImporterContext(Context* context, tg::Model model,
        const ea::string& outputPath, const ea::string& resourceNamePrefix)
        : context_(context)
        , model_(ea::move(model))
        , outputPath_(outputPath)
        , resourceNamePrefix_(resourceNamePrefix)
    {
    }

    ea::string CreateLocalResourceName(const ea::string& nameHint,
        const ea::string& prefix, const ea::string& defaultName, const ea::string& suffix)
    {
        const ea::string body = !nameHint.empty() ? nameHint : defaultName;
        for (unsigned i = 0; i < 1024; ++i)
        {
            const ea::string_view nameFormat = i != 0 ? "{0}{1}_{2}{3}" : "{0}{1}{3}";
            const ea::string localResourceName = Format(nameFormat, prefix, body, i, suffix);
            if (localResourceNames_.contains(localResourceName))
                continue;

            localResourceNames_.emplace(localResourceName);
            return localResourceName;
        }

        // Should never happen
        throw RuntimeException("Cannot assign resource name");
    }

    ea::string CreateResourceName(const ea::string& localResourceName)
    {
        const ea::string resourceName = resourceNamePrefix_ + localResourceName;
        const ea::string absoluteFileName = outputPath_ + localResourceName;
        resourceNameToAbsoluteFileName_[resourceName] = absoluteFileName;
        return resourceName;
    }

    ea::string GetResourceName(const ea::string& nameHint,
        const ea::string& prefix, const ea::string& defaultName, const ea::string& suffix)
    {
        const ea::string localResourceName = CreateLocalResourceName(nameHint, prefix, defaultName, suffix);
        return CreateResourceName(localResourceName);
    }

    const ea::string& GetAbsoluteFileName(const ea::string& resourceName)
    {
        const auto iter = resourceNameToAbsoluteFileName_.find(resourceName);
        return iter != resourceNameToAbsoluteFileName_.end() ? iter->second : EMPTY_STRING;
    }

    void AddToResourceCache(Resource* resource)
    {
        auto cache = context_->GetSubsystem<ResourceCache>();
        cache->AddManualResource(resource);
    }

    void SaveResource(Resource* resource)
    {
        const ea::string& fileName = GetAbsoluteFileName(resource->GetName());
        if (fileName.empty())
            throw RuntimeException("Cannot save imported resource");
        resource->SaveFile(fileName);
    }

    void SaveResource(Scene* scene)
    {
        XMLFile xmlFile(scene->GetContext());
        scene->SaveXML(xmlFile.GetOrCreateRoot("scene"));
        xmlFile.SaveFile(scene->GetFileName());
    }

    const tg::Model& GetModel() const { return model_; }
    Context* GetContext() const { return context_; }

private:
    Context* const context_{};
    const tg::Model model_;
    const ea::string outputPath_;
    const ea::string resourceNamePrefix_;

    ea::unordered_set<ea::string> localResourceNames_;
    ea::unordered_map<ea::string, ea::string> resourceNameToAbsoluteFileName_;
};

class GLTFTextureImporter : public NonCopyable
{
public:
    explicit GLTFTextureImporter(GLTFImporterContext* context)
        : context_(context)
    {
        const tg::Model& model = context->GetModel();
        const unsigned numTextures = model.textures.size();
        textures_.resize(numTextures);
        for (unsigned i = 0; i < numTextures; ++i)
            textures_[i] = ImportTexture(i, model.textures[i]);
    }

    void CookSamplerParams()
    {
        for (ImportedTexture& texture : textures_)
            texture.cookedSamplerParams_ = CookSamplerParams(texture.image_, texture.samplerParams_);
    }

    void SaveResources()
    {
        for (const ImportedTexture& texture : textures_)
        {
            context_->SaveResource(texture.image_);
            if (auto xmlFile = texture.cookedSamplerParams_)
                xmlFile->SaveFile(xmlFile->GetAbsoluteFileName());
        }
    }

    SharedPtr<Texture2D> GetFakeTexture(int textureIndex) const
    {
        if (textureIndex >= textures_.size())
            throw RuntimeException("Invalid texture #{} is referenced", textureIndex);
        return textures_[textureIndex].fakeTexture2D_;
    }

    static bool LoadImageData(tg::Image* image, const int imageIndex, std::string*, std::string*,
        int reqWidth, int reqHeight, const unsigned char* bytes, int size, void*)
    {
        image->as_is = true;
        image->image.resize(size);
        ea::copy_n(bytes, size, image->image.begin());
        return true;
    }

private:
    struct SamplerParams
    {
        TextureFilterMode filterMode_{ FILTER_DEFAULT };
        bool mipmaps_{ true };
        TextureAddressMode wrapU_{ ADDRESS_WRAP };
        TextureAddressMode wrapV_{ ADDRESS_WRAP };
    };

    struct ImportedTexture
    {
        SharedPtr<BinaryFile> image_;
        SharedPtr<Texture2D> fakeTexture2D_;
        SamplerParams samplerParams_;

        SharedPtr<XMLFile> cookedSamplerParams_;
    };

    static TextureFilterMode GetFilterMode(const tg::Sampler& sampler)
    {
        if (sampler.minFilter == -1 || sampler.magFilter == -1)
            return FILTER_DEFAULT;
        else if (sampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST)
        {
            if (sampler.minFilter == TINYGLTF_TEXTURE_FILTER_NEAREST
                || sampler.minFilter == TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST)
                return FILTER_NEAREST;
            else
                return FILTER_NEAREST_ANISOTROPIC;
        }
        else
        {
            if (sampler.minFilter == TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST)
                return FILTER_BILINEAR;
            else
                return FILTER_DEFAULT;
        }
    }

    static bool HasMipmaps(const tg::Sampler& sampler)
    {
        return sampler.minFilter == -1 || sampler.magFilter == -1
            || sampler.minFilter == TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST
            || sampler.minFilter == TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST
            || sampler.minFilter == TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR
            || sampler.minFilter == TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR;
    }

    static TextureAddressMode GetAddressMode(int sourceMode)
    {
        switch (sourceMode)
        {
        case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
            return ADDRESS_CLAMP;

        case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
            return ADDRESS_MIRROR;

        case TINYGLTF_TEXTURE_WRAP_REPEAT:
        default:
            return ADDRESS_WRAP;
        }
    }

    SharedPtr<BinaryFile> ImportImage(unsigned imageIndex, const tg::Image& sourceImage) const
    {
        auto image = MakeShared<BinaryFile>(context_->GetContext());

        if (sourceImage.mimeType == "image/jpeg")
        {
            const ea::string imageName = context_->GetResourceName(
                sourceImage.name.c_str(), "Textures/", "Texture", ".jpg");
            image->SetName(imageName);
        }
        else if (sourceImage.mimeType == "image/png")
        {
            const ea::string imageName = context_->GetResourceName(
                sourceImage.name.c_str(), "Textures/", "Texture", ".png");
            image->SetName(imageName);
        }
        else
        {
            throw RuntimeException("Image #{} '{}' has unknown type '{}'",
                imageIndex, sourceImage.name.c_str(), sourceImage.mimeType.c_str());
        }

        ByteVector imageBytes;
        imageBytes.resize(sourceImage.image.size());
        ea::copy(sourceImage.image.begin(), sourceImage.image.end(), imageBytes.begin());
        image->SetData(imageBytes);
        return image;
    }

    ImportedTexture ImportTexture(unsigned textureIndex, const tg::Texture& sourceTexture) const
    {
        const tg::Model& model = context_->GetModel();
        if (sourceTexture.source < 0 || sourceTexture.source >= model.images.size())
        {
            throw RuntimeException("Invalid source image #{} of texture #{} '{}'",
                sourceTexture.source, textureIndex, sourceTexture.name.c_str());
        }
        if (sourceTexture.sampler >= model.samplers.size())
        {
            throw RuntimeException("Invalid sampler #{} of texture #{} '{}'",
                sourceTexture.sampler, textureIndex, sourceTexture.name.c_str());
        }

        ImportedTexture result;
        result.image_ = ImportImage(sourceTexture.source, model.images[sourceTexture.source]);
        result.fakeTexture2D_ = MakeShared<Texture2D>(context_->GetContext());
        result.fakeTexture2D_->SetName(result.image_->GetName());
        if (sourceTexture.sampler >= 0)
        {
            const tg::Sampler& sourceSampler = model.samplers[sourceTexture.sampler];
            result.samplerParams_.filterMode_ = GetFilterMode(sourceSampler);
            result.samplerParams_.mipmaps_ = HasMipmaps(sourceSampler);
            result.samplerParams_.wrapU_ = GetAddressMode(sourceSampler.wrapS);
            result.samplerParams_.wrapV_ = GetAddressMode(sourceSampler.wrapT);
        }
        return result;
    }

    SharedPtr<XMLFile> CookSamplerParams(const SharedPtr<BinaryFile>& image, const SamplerParams& samplerParams) const
    {
        static const ea::string addressModeNames[] =
        {
            "wrap",
            "mirror",
            "",
            "border"
        };

        static const ea::string filterModeNames[] =
        {
            "nearest",
            "bilinear",
            "trilinear",
            "anisotropic",
            "nearestanisotropic",
            "default"
        };

        auto xmlFile = MakeShared<XMLFile>(context_->GetContext());

        XMLElement rootElement = xmlFile->CreateRoot("texture");

        if (samplerParams.wrapU_ != ADDRESS_WRAP)
        {
            XMLElement childElement = rootElement.CreateChild("address");
            childElement.SetAttribute("coord", "u");
            childElement.SetAttribute("mode", addressModeNames[samplerParams.wrapU_]);
        };

        if (samplerParams.wrapV_ != ADDRESS_WRAP)
        {
            XMLElement childElement = rootElement.CreateChild("address");
            childElement.SetAttribute("coord", "v");
            childElement.SetAttribute("mode", addressModeNames[samplerParams.wrapV_]);
        };

        if (samplerParams.filterMode_ != FILTER_DEFAULT)
        {
            XMLElement childElement = rootElement.CreateChild("filter");
            childElement.SetAttribute("mode", filterModeNames[samplerParams.filterMode_]);
        }

        if (!samplerParams.mipmaps_)
        {
            XMLElement childElement = rootElement.CreateChild("mipmap");
            childElement.SetBool("enable", false);
        }

        // Don't create XML if all parameters are default
        if (!rootElement.GetChild())
            return nullptr;

        const ea::string& imageName = image->GetName();
        xmlFile->SetName(ReplaceExtension(imageName, ".xml"));
        xmlFile->SetAbsoluteFileName(ReplaceExtension(context_->GetAbsoluteFileName(imageName), ".xml"));
        return xmlFile;
    }

    GLTFImporterContext* context_{};
    ea::vector<ImportedTexture> textures_;
};

class GLTFMaterialImporter : public NonCopyable
{
public:
    explicit GLTFMaterialImporter(GLTFImporterContext* context, const GLTFTextureImporter* textureImporter)
        : context_(context)
        , textureImporter_(textureImporter)
    {
        // Materials are imported on-demand
    }

    SharedPtr<Material> GetOrImportMaterial(const tg::Material& sourceMaterial, const ModelVertexFormat& vertexFormat)
    {
        const ImportedMaterialKey key{ &sourceMaterial, GetImportMaterialFlags(vertexFormat) };
        SharedPtr<Material>& material = materials_[key];
        if (!material)
        {
            material = MakeShared<Material>(context_->GetContext());

            const tg::PbrMetallicRoughness& pbr = sourceMaterial.pbrMetallicRoughness;
            const Vector4 baseColor{ ToArray<float, 4>(pbr.baseColorFactor).data() };
            material->SetShaderParameter(ShaderConsts::Material_MatDiffColor, baseColor);
            material->SetShaderParameter(ShaderConsts::Material_Metallic, static_cast<float>(pbr.metallicFactor));
            material->SetShaderParameter(ShaderConsts::Material_Roughness, static_cast<float>(pbr.roughnessFactor));

            if (pbr.baseColorTexture.index >= 0)
            {
                if (pbr.baseColorTexture.texCoord != 0)
                    URHO3D_LOGWARNING("Material '{}' has non-standard UV for diffuse texture");

                const SharedPtr<Texture2D> diffuseTexture = textureImporter_->GetFakeTexture(pbr.baseColorTexture.index);
                material->SetTexture(TU_DIFFUSE, diffuseTexture);
            }

            const ea::string materialName = context_->GetResourceName(
                sourceMaterial.name.c_str(), "Materials/", "Material", ".xml");
            material->SetName(materialName);

            context_->AddToResourceCache(material);
        }
        return material;
    }

    void SaveResources()
    {
        for (const auto& item : materials_)
            context_->SaveResource(item.second);
    }

private:
    enum ImportedMaterialFlag
    {
        HasVertexColors = 1 << 0,
    };

    using ImportedMaterialKey = ea::pair<const tg::Material*, unsigned>;

    unsigned GetImportMaterialFlags(const ModelVertexFormat& vertexFormat) const
    {
        unsigned flags{};
        if (vertexFormat.color_[0] != MAX_VERTEX_ELEMENT_TYPES)
            flags |= HasVertexColors;
        return flags;
    }

    GLTFImporterContext* context_{};
    const GLTFTextureImporter* textureImporter_{};
    ea::unordered_map<ImportedMaterialKey, SharedPtr<Material>> materials_;
};

tg::Model LoadGLTF(const ea::string& fileName)
{
    tg::TinyGLTF loader;
    loader.SetImageLoader(&GLTFTextureImporter::LoadImageData, nullptr);

    std::string errorMessage;
    tg::Model model;
    if (!loader.LoadASCIIFromFile(&model, &errorMessage, nullptr, fileName.c_str()))
        throw RuntimeException("Failed to import GLTF file: {}", errorMessage.c_str());

    return model;
}

}

class GLTFImporter::Impl
{
public:
    explicit Impl(Context* context, const ea::string& fileName,
        const ea::string& outputPath, const ea::string& resourceNamePrefix)
        : context_(context)
        , importerContext_(context, LoadGLTF(fileName), outputPath, resourceNamePrefix)
        , textureImporter_(&importerContext_)
        , materialImporter_(&importerContext_, &textureImporter_)
    {
        // TODO: Remove me
        model_ = importerContext_.GetModel();

        ImportMeshesAndMaterials();
    }

    bool CookResources()
    {
        textureImporter_.CookSamplerParams();

        auto cache = context_->GetSubsystem<ResourceCache>();

        for (ModelView* modelView : meshToModelView_)
        {
            const auto model = modelView ? CookModelResource(modelView) : nullptr;
            meshToModel_.push_back(model);

            if (model)
            {
                cache->AddManualResource(model);
                importedModels_.push_back(model);
            }
        }

        for (const tg::Scene& sourceScene : model_.scenes)
        {
            const auto scene = ImportScene(sourceScene);
            importedScenes_.push_back(scene);
        }

        return true;
    }

    bool SaveResources()
    {
        textureImporter_.SaveResources();
        materialImporter_.SaveResources();

        for (Material* material : importedMaterials_)
            importerContext_.SaveResource(material);

        for (Model* model : importedModels_)
            importerContext_.SaveResource(model);

        for (Scene* scene : importedScenes_)
            importerContext_.SaveResource(scene);

        return true;
    }

private:
    void ImportMeshesAndMaterials()
    {
        meshToSkin_.clear();
        meshToSkin_.resize(model_.meshes.size());

        for (const tg::Node& node : model_.nodes)
        {
            if (node.mesh < 0)
                continue;

            auto& meshSkin = meshToSkin_[node.mesh];

            if (!meshSkin)
            {
                if (node.skin >= 0)
                    meshSkin = node.skin;
            }
            else
            {
                URHO3D_LOGWARNING("Mesh #{} '{}' has multiple assigned skins, skin #{} '{}' is used.",
                    node.mesh, model_.meshes[node.mesh].name.c_str(),
                    *meshSkin, model_.skins[*meshSkin].name.c_str());
            }
        }

        const unsigned numMeshes = model_.meshes.size();
        meshToModelView_.resize(numMeshes);
        meshToMaterials_.resize(numMeshes);
        for (unsigned i = 0; i < numMeshes; ++i)
        {
            auto modelView = ImportModelView(model_.meshes[i]);
            if (modelView)
                meshToMaterials_[i] = modelView->ExportMaterialList();
            meshToModelView_[i] = modelView;
        }
    }

    SharedPtr<ModelView> ImportModelView(const tg::Mesh& sourceMesh)
    {
        const ea::string modelName = importerContext_.GetResourceName(sourceMesh.name.c_str(), "", "Model", ".mdl");

        auto modelView = MakeShared<ModelView>(context_);
        modelView->SetName(modelName);

        auto& geometries = modelView->GetGeometries();

        const unsigned numGeometries = sourceMesh.primitives.size();
        geometries.resize(numGeometries);
        for (unsigned geometryIndex = 0; geometryIndex < numGeometries; ++geometryIndex)
        {
            GeometryView& geometryView = geometries[geometryIndex];
            geometryView.lods_.resize(1);
            GeometryLODView& geometryLODView = geometryView.lods_[0];

            const tg::Primitive& primitive = sourceMesh.primitives[geometryIndex];
            if (primitive.mode != TINYGLTF_MODE_TRIANGLES)
            {
                URHO3D_LOGWARNING("Unsupported geometry type {} in mesh '{}'.", primitive.mode, sourceMesh.name.c_str());
                return nullptr;
            }

            if (primitive.attributes.empty())
            {
                URHO3D_LOGWARNING("No attributes in primitive #{} in mesh '{}'.", geometryIndex, sourceMesh.name.c_str());
                return nullptr;
            }

            const unsigned numVertices = model_.accessors[primitive.attributes.begin()->second].count;

            geometryLODView.indices_ = ReadOptionalAccessor<unsigned>(primitive.indices);
            geometryLODView.vertices_.resize(numVertices);

            for (const auto& attribute : primitive.attributes)
            {
                const tg::Accessor& accessor = model_.accessors[attribute.second];
                if (!ReadVertexData(geometryLODView.vertexFormat_, geometryLODView.vertices_,
                    attribute.first.c_str(), accessor))
                {
                    URHO3D_LOGWARNING("Cannot read primitive #{} in mesh '{}'.", geometryIndex, sourceMesh.name.c_str());
                    return nullptr;
                }
            }

            if (primitive.material >= 0)
            {
                if (auto material = materialImporter_.GetOrImportMaterial(model_.materials[primitive.material], geometryLODView.vertexFormat_))
                    geometryView.material_ = material->GetName();
            }

            // TODO: Get rid of this line
            modelView->SetVertexFormat(geometryLODView.vertexFormat_);
        }
        return modelView;
    }

    SharedPtr<Model> CookModelResource(ModelView* modelView)
    {
        auto model = modelView->ExportModel();
        return model;
    }

    SharedPtr<Scene> ImportScene(const tg::Scene& sourceScene)
    {
        const ea::string sceneName = importerContext_.GetResourceName(sourceScene.name.c_str(), "", "Scene", ".xml");

        auto scene = MakeShared<Scene>(context_);
        scene->SetFileName(importerContext_.GetAbsoluteFileName(sceneName));
        scene->CreateComponent<Octree>();

        for (int nodeIndex : sourceScene.nodes)
        {
            ImportNode(scene, model_.nodes[nodeIndex]);
        }

        if (!scene->GetComponent<Light>(true))
        {
            // Model forward is Z+, make default lighting from top right when looking at forward side of model.
            Node* node = scene->CreateChild("Default Light");
            node->SetDirection({ 1.0f, -2.0f, -1.0f });
            auto light = node->CreateComponent<Light>();
            light->SetLightType(LIGHT_DIRECTIONAL);
        }
        return scene;
    }

    void ExtractTransform(const tg::Node& node, Vector3& translation, Quaternion& rotation, Vector3& scale)
    {
        translation = Vector3::ZERO;
        rotation = Quaternion::IDENTITY;
        scale = Vector3::ONE;

        if (!node.matrix.empty())
        {
            Matrix4 sourceMatrix;
            std::transform(node.matrix.begin(), node.matrix.end(),
                &sourceMatrix.m00_, StaticCaster<float>{});

            const Matrix3x4 transform{ sourceMatrix.Transpose() };
            transform.Decompose(translation, rotation, scale);
        }
        else
        {
            if (!node.translation.empty())
            {
                std::transform(node.translation.begin(), node.translation.end(),
                    &translation.x_, StaticCaster<float>{});
            }
            if (!node.rotation.empty())
            {
                std::transform(node.rotation.begin(), node.rotation.end(),
                    &rotation.x_, StaticCaster<float>{});
            }
            if (!node.scale.empty())
            {
                std::transform(node.scale.begin(), node.scale.end(),
                    &scale.x_, StaticCaster<float>{});
            }
        }
    }

    void ImportNode(Node* parent, const tg::Node& sourceNode)
    {
        auto cache = context_->GetSubsystem<ResourceCache>();

        Node* node = parent->CreateChild(sourceNode.name.c_str());

        Vector3 translation;
        Quaternion rotation;
        Vector3 scale;
        ExtractTransform(sourceNode, translation, rotation, scale);
        node->SetTransform(translation, rotation, scale);

        if (sourceNode.mesh >= 0)
        {
            if (Model* model = meshToModel_[sourceNode.mesh])
            {
                auto staticModel = node->CreateComponent<StaticModel>();
                staticModel->SetModel(model);

                const StringVector& meshMaterials = meshToMaterials_[sourceNode.mesh];
                for (unsigned i = 0; i < meshMaterials.size(); ++i)
                {
                    auto material = cache->GetResource<Material>(meshMaterials[i]);
                    staticModel->SetMaterial(i, material);
                }
            }
        }

        for (int childIndex : sourceNode.children)
        {
            ImportNode(node, model_.nodes[childIndex]);
        }
    }

    template <class T, class U>
    void ReadBufferViewImpl(ea::vector<U>& result,
        int bufferViewIndex, int byteOffset, int componentType, int type, int count) const
    {
        const tg::BufferView& bufferView = model_.bufferViews[bufferViewIndex];
        const tg::Buffer& buffer = model_.buffers[bufferView.buffer];

        const auto* bufferViewData = buffer.data.data() + bufferView.byteOffset + byteOffset;
        const int stride = GetByteStride(bufferView, componentType, type);

        const int numComponents = tg::GetNumComponentsInType(type);
        for (unsigned i = 0; i < count; ++i)
        {
            for (unsigned j = 0; j < numComponents; ++j)
            {
                T elementValue{};
                memcpy(&elementValue, bufferViewData + sizeof(T) * j, sizeof(T));
                result[i * numComponents + j] = static_cast<U>(elementValue);
            }
            bufferViewData += stride;
        }
    }

    template <class T>
    ea::vector<T> ReadBufferView(int bufferViewIndex, int byteOffset, int componentType, int type, int count) const
    {
        ea::vector<T> result;
        if (bufferViewIndex >= 0)
        {
            const int numComponents = tg::GetNumComponentsInType(type);
            result.clear();
            result.resize(count * numComponents);

            switch (componentType)
            {
            case TINYGLTF_COMPONENT_TYPE_BYTE:
                ReadBufferViewImpl<signed char>(result, bufferViewIndex, byteOffset, componentType, type, count);
                break;

            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                ReadBufferViewImpl<unsigned char>(result, bufferViewIndex, byteOffset, componentType, type, count);
                break;

            case TINYGLTF_COMPONENT_TYPE_SHORT:
                ReadBufferViewImpl<short>(result, bufferViewIndex, byteOffset, componentType, type, count);
                break;

            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                ReadBufferViewImpl<unsigned short>(result, bufferViewIndex, byteOffset, componentType, type, count);
                break;

            case TINYGLTF_COMPONENT_TYPE_INT:
                ReadBufferViewImpl<int>(result, bufferViewIndex, byteOffset, componentType, type, count);
                break;

            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                ReadBufferViewImpl<unsigned>(result, bufferViewIndex, byteOffset, componentType, type, count);
                break;

            case TINYGLTF_COMPONENT_TYPE_FLOAT:
                ReadBufferViewImpl<float>(result, bufferViewIndex, byteOffset, componentType, type, count);
                break;

            case TINYGLTF_COMPONENT_TYPE_DOUBLE:
                ReadBufferViewImpl<double>(result, bufferViewIndex, byteOffset, componentType, type, count);
                break;

            default:
                URHO3D_LOGERROR("Unexpected component type");
                break;
            }
        }
        return result;
    }

    template <class T>
    ea::vector<T> ReadAccessor(const tg::Accessor& accessor) const
    {
        ea::vector<T> result;

        const int numComponents = tg::GetNumComponentsInType(accessor.type);
        result.resize(accessor.count * numComponents);

        // Read dense buffer data
        if (accessor.bufferView >= 0)
        {
            auto bufferData = ReadBufferView<T>(accessor.bufferView, accessor.byteOffset,
                accessor.componentType, accessor.type, accessor.count);
            if (!bufferData.empty())
                result = ea::move(bufferData);
        }

        // Read sparse buffer data
        const int numSparseElements = accessor.sparse.count;
        if (accessor.sparse.isSparse && numSparseElements > 0)
        {
            const auto& accessorIndices = accessor.sparse.indices;
            const auto& accessorValues = accessor.sparse.values;

            const auto indices = ReadBufferView<unsigned>(accessorIndices.bufferView, accessorIndices.byteOffset,
                accessorIndices.componentType, TINYGLTF_TYPE_SCALAR, numSparseElements);

            const auto values = ReadBufferView<T>(accessorValues.bufferView, accessorValues.byteOffset,
                accessor.componentType, accessor.type, numSparseElements);

            for (unsigned i = 0; i < indices.size(); ++i)
                ea::copy_n(&values[i * numComponents], numComponents, &result[indices[i] * numComponents]);
        }

        return result;
    }

    template <class T>
    ea::vector<T> ReadOptionalAccessor(int accessorIndex) const
    {
        ea::vector<T> result;
        if (accessorIndex >= 0)
        {
            const tg::Accessor& accessor = model_.accessors[accessorIndex];
            result = ReadAccessor<T>(accessor);
        }
        return result;
    }

    bool ReadVertexData(ModelVertexFormat& vertexFormat, ea::vector<ModelVertex>& vertices,
        const ea::string& semantics, const tg::Accessor& accessor)
    {
        const auto attributeData = ReadAccessor<float>(accessor);
        if (attributeData.empty())
            return false;

        const auto& parsedSemantics = semantics.split('_');
        const ea::string& semanticsName = parsedSemantics[0];
        const unsigned semanticsIndex = parsedSemantics.size() > 1 ? FromString<unsigned>(parsedSemantics[1]) : 0;

        if (semanticsName == "POSITION")
        {
            if (accessor.type != TINYGLTF_TYPE_VEC3)
            {
                URHO3D_LOGERROR("Unexpected type of vertex position");
                return false;
            }

            vertexFormat.position_ = TYPE_VECTOR3;

            const auto positions = RepackFloatArray<Vector3>(attributeData, accessor.count);
            for (unsigned i = 0; i < accessor.count; ++i)
                vertices[i].SetPosition(positions[i]);
        }
        else if (semanticsName == "NORMAL")
        {
            if (accessor.type != TINYGLTF_TYPE_VEC3)
            {
                URHO3D_LOGERROR("Unexpected type of vertex normal");
                return false;
            }

            vertexFormat.normal_ = TYPE_VECTOR3;

            const auto normals = RepackFloatArray<Vector3>(attributeData, accessor.count);
            for (unsigned i = 0; i < accessor.count; ++i)
                vertices[i].SetNormal(normals[i].Normalized());
        }
        else if (semanticsName == "TANGENT")
        {
            if (accessor.type != TINYGLTF_TYPE_VEC4)
            {
                URHO3D_LOGERROR("Unexpected type of vertex tangent");
                return false;
            }

            vertexFormat.tangent_ = TYPE_VECTOR4;

            const auto tangents = RepackFloatArray<Vector4>(attributeData, accessor.count);
            for (unsigned i = 0; i < accessor.count; ++i)
                vertices[i].tangent_ = tangents[i];
        }
        else if (semanticsName == "TEXCOORD" && semanticsIndex < ModelVertex::MaxUVs)
        {
            if (accessor.type != TINYGLTF_TYPE_VEC2)
            {
                URHO3D_LOGERROR("Unexpected type of vertex uv");
                return false;
            }

            vertexFormat.uv_[semanticsIndex] = TYPE_VECTOR2;

            const auto uvs = RepackFloatArray<Vector2>(attributeData, accessor.count);
            for (unsigned i = 0; i < accessor.count; ++i)
                vertices[i].uv_[semanticsIndex] = { uvs[i], Vector2::ZERO };
        }
        else if (semanticsName == "COLOR" && semanticsIndex < ModelVertex::MaxColors)
        {
            if (accessor.type != TINYGLTF_TYPE_VEC3 && accessor.type != TINYGLTF_TYPE_VEC4)
            {
                URHO3D_LOGERROR("Unexpected type of vertex color");
                return false;
            }

            if (accessor.type == TINYGLTF_TYPE_VEC3)
            {
                vertexFormat.color_[semanticsIndex] = TYPE_VECTOR3;

                const auto colors = RepackFloatArray<Vector3>(attributeData, accessor.count);
                for (unsigned i = 0; i < accessor.count; ++i)
                    vertices[i].color_[semanticsIndex] = { colors[i], 1.0f };
            }
            else if (accessor.type == TINYGLTF_TYPE_VEC4)
            {
                vertexFormat.color_[semanticsIndex] = TYPE_VECTOR4;

                const auto colors = RepackFloatArray<Vector4>(attributeData, accessor.count);
                for (unsigned i = 0; i < accessor.count; ++i)
                    vertices[i].color_[semanticsIndex] = colors[i];
            }
        }

        return true;
    }

    GLTFImporterContext importerContext_;
    GLTFTextureImporter textureImporter_;
    GLTFMaterialImporter materialImporter_;

    Context* context_{};

    /// Initialized after loading
    /// @{
    tg::Model model_;

    ea::vector<ea::optional<int>> meshToSkin_;
    ea::vector<SharedPtr<ModelView>> meshToModelView_;
    ea::vector<StringVector> meshToMaterials_;
    ea::vector<SharedPtr<Image>> texturesToImages_;
    ea::vector<SharedPtr<Texture>> texturesToFakeTextures_;
    /// @}

    /// Initialized after cooking
    /// @{
    ea::vector<SharedPtr<Model>> importedModels_;
    ea::vector<SharedPtr<Material>> importedMaterials_;
    ea::vector<SharedPtr<Model>> meshToModel_;
    ea::vector<SharedPtr<Scene>> importedScenes_;
    /// @}
};

GLTFImporter::GLTFImporter(Context* context)
    : Object(context)
{

}

GLTFImporter::~GLTFImporter()
{

}

bool GLTFImporter::LoadFile(const ea::string& fileName,
    const ea::string& outputPath, const ea::string& resourceNamePrefix)
{
    try
    {
        impl_ = ea::make_unique<Impl>(context_, fileName, outputPath, resourceNamePrefix);
        return true;
    }
    catch(const RuntimeException& e)
    {
        URHO3D_LOGERROR("{}", e.what());
        return false;
    }
}

bool GLTFImporter::CookResources()
{
    try
    {
        if (!impl_)
            throw RuntimeException("GLTF file wasn't loaded");

        return impl_->CookResources();
    }
    catch(const RuntimeException& e)
    {
        URHO3D_LOGERROR("{}", e.what());
        return false;
    }
}

bool GLTFImporter::SaveResources()
{
    try
    {
        if (!impl_)
            throw RuntimeException("Imported asserts weren't cooked");

        return impl_->SaveResources();
    }
    catch(const RuntimeException& e)
    {
        URHO3D_LOGERROR("{}", e.what());
        return false;
    }
}

}
