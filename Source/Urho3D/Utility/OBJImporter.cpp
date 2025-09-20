// Copyright (c) 2024-2024 the rbfx project.
// Copyright(c) 2016 Robert Smith (https://github.com/Bly7/OBJ-Loader)
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include <Urho3D/Utility/OBJImporter.h>
#include <Urho3D/IO/Deserializer.h>

#include "Urho3D/Graphics/VertexBuffer.h"
#include "Urho3D/Graphics/IndexBuffer.h"
#include "Urho3D/Core/Context.h"
#include "Urho3D/IO/FileSystem.h"
#include "Urho3D/IO/Log.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "Urho3D/Graphics/Geometry.h"

#include <tinyobjloader/tiny_obj_loader.h>

#include <EASTL/shared_array.h>

namespace Urho3D
{
namespace
{
class BinaryFileAdapter : public std::streambuf
{
    const unsigned BUFFER_SIZE = 1024;

    Deserializer& source_;
    ea::shared_array<char> buffer_;

public:
    BinaryFileAdapter(Deserializer& source)
        : source_(source)
        , buffer_(new char[BUFFER_SIZE])
    {
    }
    ~BinaryFileAdapter() {}

    int underflow()
    {
        if (this->gptr() == this->egptr())
        {
            auto size = source_.Read(this->buffer_.get(), BUFFER_SIZE);
            this->setg(this->buffer_.get(), this->buffer_.get(), this->buffer_.get() + size);
        }
        return this->gptr() == this->egptr() ? std::char_traits<char>::eof()
                                             : std::char_traits<char>::to_int_type(*this->gptr());
    }
};

struct Vertex
{
    Vector3 Position;
    Vector3 Normal;
    Vector2 TexCoord;
};

bool LoadFile(const ea::string& fileName, const ea::string& mtlFileName, tinyobj::attrib_t* attrib, std::vector<tinyobj::shape_t>* shapes,
    std::vector<tinyobj::material_t>* materials, std::string* warn, std::string* err)
{
    std::ifstream obj_ifs;
    obj_ifs.open(fileName.c_str());

    if (!mtlFileName.empty())
    {
        std::ifstream mtl_ifs;
        mtl_ifs.open(mtlFileName.c_str());
        tinyobj::MaterialStreamReader mtl_ss(mtl_ifs);
        return tinyobj::LoadObj(attrib, shapes, materials, warn, err, &obj_ifs, &mtl_ss, true, false);
    }
    {
        std::stringbuf mtl_buf("");
        std::istream mtl_ifs(&mtl_buf);
        tinyobj::MaterialStreamReader mtl_ss(mtl_ifs);
        return tinyobj::LoadObj(attrib, shapes, materials, warn, err, &obj_ifs, &mtl_ss, true, false);
    }
}

SharedPtr<Material> ConvertMaterial(Context* context, const tinyobj::material_t& material)
{
    auto mat{MakeShared<Material>(context)};
    mat->SetName(GetSanitizedName(material.name.c_str()) + ".material");

    mat->SetShaderParameter("MatDiffColor", Vector4{material.diffuse[0], material.diffuse[1], material.diffuse[2], 1.0f});
    mat->SetShaderParameter("MatSpecColor", Vector4{material.specular[0], material.specular[1], material.specular[2], 1.0f});
    mat->SetShaderParameter("Roughness", material.roughness);
    mat->SetShaderParameter("Metallic", material.metallic);

    return mat;
}

SharedPtr<Model> ConvertModel(
    Context* context, const ea::string name, const tinyobj::attrib_t& attrib, const std::vector<tinyobj::shape_t>& shapes, unsigned expectedNumberOfMaterials)
{
    auto model = MakeShared<Model>(context);
    model->SetName(name);

    ea::unordered_map<IntVector3, unsigned> vertexMap;

    ea::vector<Vertex> vertices;
    vertices.reserve(attrib.vertices.size());

    ea::vector<ea::vector<unsigned>> indices;
    indices.resize(expectedNumberOfMaterials+1);
    unsigned totalNumberOfIndices{0};

    auto boundingBox = BoundingBox{};
    for (auto& shape : shapes)
    {
        // Because "triangulate" setting is on the .indices guaranteed to have triangles.
        for (size_t indexIndex = 0; indexIndex < shape.mesh.indices.size(); ++indexIndex)
        {
            auto& index = shape.mesh.indices[indexIndex];
            IntVector3 key{index.vertex_index, index.normal_index, index.texcoord_index};
            unsigned vertexIndex{0};
            auto it = vertexMap.find(key);
            if (it == vertexMap.end())
            {
                // Element not present, add it to the map
                vertexIndex = vertices.size();
                vertexMap[key] = vertexIndex;
                auto i = IntVector3(key.x_ * 3, key.y_ * 3, key.z_ * 2);
                const Vector3 pos{key.x_ >= 0
                        ? Vector3(attrib.vertices[i.x_ + 0], attrib.vertices[i.x_ + 1], attrib.vertices[i.x_ + 2])
                        : Vector3::ZERO};
                const Vector3 vn{key.y_ >= 0
                        ? Vector3(attrib.normals[i.y_ + 0], attrib.normals[i.y_ + 1], attrib.normals[i.y_ + 2])
                        : Vector3::UP};
                const Vector2 uv{
                    key.z_ >= 0 ? Vector2(attrib.texcoords[i.z_ + 0], attrib.texcoords[i.z_ + 1]) : Vector2::ZERO};
                vertices.push_back({pos, vn, uv});
                boundingBox.Merge(pos);
            }
            else
            {
                vertexIndex = it->second;
            }
            auto materialIndex = shape.mesh.material_ids[indexIndex / 3];
            if (materialIndex < 0)
            {
                materialIndex = static_cast<int>(expectedNumberOfMaterials);
            }
            indices[materialIndex].push_back(vertexIndex);
            ++totalNumberOfIndices;
        }
    }

    // Populate vertex buffer
    {
        ea::vector<SharedPtr<VertexBuffer>> buffers;
        ea::vector<unsigned> morphRangeStarts;
        ea::vector<unsigned> morphRangeCounts;

        VertexBufferDesc desc;
        desc.vertexElements_.push_back(
            VertexElement(VertexElementType::TYPE_VECTOR3, VertexElementSemantic::SEM_POSITION, 0));
        desc.vertexElements_.push_back(
            VertexElement(VertexElementType::TYPE_VECTOR3, VertexElementSemantic::SEM_NORMAL, 0));
        desc.vertexElements_.push_back(
            VertexElement(VertexElementType::TYPE_VECTOR2, VertexElementSemantic::SEM_TEXCOORD, 0));

        desc.vertexCount_ = vertices.size();

        const SharedPtr<VertexBuffer> buffer{MakeShared<VertexBuffer>(context)};
        const unsigned vertexSize = VertexBuffer::GetVertexSize(desc.vertexElements_);
        assert(sizeof(Vertex) == vertexSize);
        desc.dataSize_ = desc.vertexCount_ * vertexSize;
        buffer->SetDebugName(Format("Model '{}' Vertex Buffer #{}", name, 0));

        desc.data_.reset(); // Make sure no previous data
        buffer->SetShadowed(true);
        buffer->SetSize(desc.vertexCount_, desc.vertexElements_);
        if (desc.vertexCount_ > 0)
        {
            void* dest = buffer->Map();
            memcpy(dest, vertices.data(), desc.dataSize_);
            buffer->Unmap();
        }
        buffers.push_back(buffer);

        model->SetVertexBuffers(buffers, morphRangeStarts, morphRangeCounts);
        model->SetBoundingBox(boundingBox);
    }
    // Populate index buffer
    {
        ea::vector<SharedPtr<IndexBuffer>> buffers;
        SharedPtr<IndexBuffer> buffer(MakeShared<IndexBuffer>(context));
        buffer->SetDebugName(Format("Model '{}' Index Buffer #{}", name, 0));

        buffer->SetShadowed(true);
        buffer->SetSize(totalNumberOfIndices, true);
        char* dest = static_cast<char*>(buffer->Map());
        for (auto& geometryIndices : indices)
        {
            memcpy(dest, geometryIndices.data(), geometryIndices.size() * sizeof(unsigned));
            dest += geometryIndices.size() * sizeof(unsigned);
        }
        buffer->Unmap();

        buffers.push_back(buffer);
        model->SetIndexBuffers(buffers);
    }

    auto numGeometries = expectedNumberOfMaterials;
    if (!indices[expectedNumberOfMaterials].empty())
        ++numGeometries;

    model->SetNumGeometries(numGeometries);
    unsigned indexStart = 0;
    for (unsigned geometryIndex = 0; geometryIndex < numGeometries; ++geometryIndex)
    {
        model->SetNumGeometryLodLevels(geometryIndex, 1);

        auto geometry = MakeShared<Geometry>(context);

        geometry->SetNumVertexBuffers(1);
        geometry->SetVertexBuffer(0, model->GetVertexBuffers()[0]);
        geometry->SetIndexBuffer(model->GetIndexBuffers()[0]);
        geometry->SetDrawRange(TRIANGLE_LIST, indexStart, indices[geometryIndex].size());

        model->SetGeometry(geometryIndex, 0, geometry);

        indexStart += indices[geometryIndex].size();
    }
  
    return model;
}

} // namespace

OBJImporter::OBJImporter(Context* context)
    : BaseClassName(context)
{
}

OBJImporter::~OBJImporter()
{
}

bool OBJImporter::LoadFileToResourceCache(const ea::string& fileName, const ea::string& resourcePrefix)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn;
    std::string err;

    auto mtlFileName = ReplaceExtension(fileName, ".mtl");
    auto mtlExists = context_->GetSubsystem<FileSystem>()->FileExists(mtlFileName);

    auto valid = LoadFile(fileName, mtlExists ? mtlFileName : EMPTY_STRING, &attrib, &shapes, &materials, &warn, &err);

    if (!valid)
    {
        URHO3D_LOGERROR(err.c_str());
        return false;
    }

    for (auto& material : materials)
    {
        auto mat = ConvertMaterial(context_, material);
        mat->SetName(resourcePrefix + mat->GetName());
        materialsToSave_.push_back(mat);
    }

    {
        auto name = resourcePrefix + GetFileName(fileName) + ".mdl";
        auto model = ConvertModel(context_, name, attrib, shapes, materials.size());
        modelsToSave_.push_back(model);
    }

    return true;
}

bool OBJImporter::SaveResources(const ea::string& folderPrefix)
{
    bool result = true;

    for (const auto& model : modelsToSave_)
    {
        result &= SaveResource(model, folderPrefix);
    }
    for (const auto& material : materialsToSave_)
    {
        result &= SaveResource(material, folderPrefix);
    }

    return result;
}

bool OBJImporter::SaveResource(Resource* resource, const ea::string& folderPrefix)
{
    if (!resource)
        return true;

    const ea::string& fileName = folderPrefix + GetFileNameAndExtension(resource->GetName());
    if (fileName.empty())
        throw RuntimeException("Cannot save imported resource");
    resource->SetAbsoluteFileName(fileName);
    return resource->SaveFile(fileName);
}

} // namespace Urho3D
