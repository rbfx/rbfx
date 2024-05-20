// Copyright (c) 2024-2024 the rbfx project.
// Copyright(c) 2016 Robert Smith (https://github.com/Bly7/OBJ-Loader)
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Precompiled.h"

#include <Urho3D/Graphics/Model.h>
#include <Urho3D/IO/File.h>
#include <Urho3D/Resource/BinaryFile.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Graphics/IndexBuffer.h>
#include <Urho3D/Graphics/VertexBuffer.h>
#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/Resource/XMLFile.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobjloader/tiny_obj_loader.h>

#include "../DebugNew.h"

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
    ~BinaryFileAdapter() { }

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
}

bool Model::LoadOBJ(Deserializer& source)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn;
    std::string err;

    BinaryFileAdapter obj_buf(source);
    std::stringbuf mtl_buf("");

    std::istream obj_ifs(&obj_buf);
    std::istream mtl_ifs(&mtl_buf);

    tinyobj::MaterialStreamReader mtl_ss(mtl_ifs);

    auto valid = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, &obj_ifs, &mtl_ss, true, false);

    if (!valid)
    {
        URHO3D_LOGERROR(err.c_str());
        return false;
    }

    ea::unordered_map<IntVector3, unsigned> vertexMap;

    ea::vector<Vertex> vertices;
    vertices.reserve(attrib.vertices.size());

    ea::vector<unsigned> indices;
    unsigned totalNumberOfIndices{0};
    for (auto&shape: shapes)
    {
        totalNumberOfIndices += shape.mesh.indices.size();
    }
    indices.reserve(totalNumberOfIndices);

    boundingBox_ = BoundingBox{};
    for (auto& shape : shapes)
    {
        // Because "triangulate" setting is on the .indices guaranteed to have triangles.
        for (auto& index : shape.mesh.indices)
        {
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
                const Vector2 uv{key.z_ >= 0 ? Vector2(attrib.texcoords[i.z_ + 0], attrib.texcoords[i.z_ + 1])
                        : Vector2::ZERO};
                vertices.push_back({pos, vn, uv});
                boundingBox_.Merge(pos);
            }
            else
            {
                vertexIndex = it->second;
            }
            indices.push_back(vertexIndex);
        }
    }

    unsigned memoryUse = 0;
    
    bool async = GetAsyncLoadState() == ASYNC_LOADING;
    {
        vertexBuffers_.reserve(1);
        loadVBData_.resize(1);

        VertexBufferDesc& desc{loadVBData_[0]};
        desc.vertexElements_.push_back(
            VertexElement(VertexElementType::TYPE_VECTOR3, VertexElementSemantic::SEM_POSITION, 0));
        desc.vertexElements_.push_back(
            VertexElement(VertexElementType::TYPE_VECTOR3, VertexElementSemantic::SEM_NORMAL, 0));
        desc.vertexElements_.push_back(
            VertexElement(VertexElementType::TYPE_VECTOR2, VertexElementSemantic::SEM_TEXCOORD, 0));

        desc.vertexCount_ = vertices.size();

        const SharedPtr<VertexBuffer> buffer(MakeShared<VertexBuffer>(context_));
        const unsigned vertexSize = VertexBuffer::GetVertexSize(desc.vertexElements_);
        assert(sizeof(Vertex) == vertexSize);
        desc.dataSize_ = desc.vertexCount_ * vertexSize;
        buffer->SetDebugName(Format("Model '{}' Vertex Buffer #{}", GetName(), 0));

        // Prepare vertex buffer data to be uploaded during EndLoad()
        if (async)
        {
            desc.data_ = new unsigned char[desc.dataSize_];
            if (desc.vertexCount_ > 0)
            {
                memcpy(&desc.data_[0], vertices.data(), desc.dataSize_);
            }
        }
        else
        {
            // If not async loading, use locking to avoid extra allocation & copy
            desc.data_.reset(); // Make sure no previous data
            buffer->SetShadowed(true);
            buffer->SetSize(desc.vertexCount_, desc.vertexElements_);
            if (desc.vertexCount_ > 0)
            {
                void* dest = buffer->Map();
                memcpy(dest, vertices.data(), desc.dataSize_);
                buffer->Unmap();
            }
        }

        memoryUse += sizeof(VertexBuffer) + desc.vertexCount_ * vertexSize;
        vertexBuffers_.push_back(buffer);
    }
    {
        indexBuffers_.reserve(1);
        loadIBData_.resize(1);

        SharedPtr<IndexBuffer> buffer(MakeShared<IndexBuffer>(context_));
        buffer->SetDebugName(Format("Model '{}' Index Buffer #{}", GetName(), 0));

        // Prepare index buffer data to be uploaded during EndLoad()
        if (async)
        {
            IndexBufferDesc& ibData = loadIBData_[0];
            ibData.indexCount_ = indices.size();
            ibData.indexSize_ = sizeof(unsigned);
            ibData.dataSize_ = ibData.indexCount_ * ibData.indexSize_;
            ibData.data_ = new unsigned char[ibData.dataSize_];
            memcpy(ibData.data_.get(), indices.data(), ibData.dataSize_);
        }
        else
        {
            // If not async loading, use locking to avoid extra allocation & copy
            buffer->SetShadowed(true);
            buffer->SetSize(indices.size(), true);
            void* dest = buffer->Map();
            memcpy(dest, indices.data(), indices.size() * sizeof(unsigned));
            buffer->Unmap();
        }

        memoryUse += sizeof(IndexBuffer) + indices.size() * sizeof(unsigned);
        indexBuffers_.push_back(buffer);
    }
    {
        auto numGeometries = 1;
        geometries_.reserve(numGeometries);
        geometryBoneMappings_.resize(numGeometries);
        geometryCenters_.resize(numGeometries);
        loadGeometries_.resize(numGeometries);

        for (unsigned geometryIndex = 0; geometryIndex < numGeometries; ++geometryIndex)
        {
            SharedPtr<Geometry> geometry(MakeShared<Geometry>(context_));

            ea::vector<SharedPtr<Geometry>> geometryLodLevels;
            geometryLodLevels.reserve(1);

            loadGeometries_[geometryIndex].resize(1);

            // Prepare geometry to be defined during EndLoad()
            loadGeometries_[geometryIndex][0].type_ = TRIANGLE_LIST;
            loadGeometries_[geometryIndex][0].vbRef_ = 0;
            loadGeometries_[geometryIndex][0].ibRef_ = 0;
            loadGeometries_[geometryIndex][0].indexStart_ = 0;
            loadGeometries_[geometryIndex][0].indexCount_ = indices.size();

            geometryCenters_[geometryIndex] = boundingBox_.Center();

            geometryLodLevels.push_back(geometry);
            memoryUse += sizeof(Geometry);

            geometries_.push_back(geometryLodLevels);
        }
    }

    // Read geometry centers
    SetMemoryUse(memoryUse);
    return true;
}

} // namespace Urho3D
