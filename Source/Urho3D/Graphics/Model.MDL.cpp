// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2017-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Precompiled.h"

#include "../Core/Context.h"
#include "../Graphics/Geometry.h"
#include "../Graphics/IndexBuffer.h"
#include "../Graphics/Model.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/VertexBuffer.h"
#include "../IO/Log.h"
#include "../IO/File.h"
#include "../IO/FileSystem.h"
#include "../IO/VirtualFileSystem.h"
#include "../Resource/ResourceCache.h"
#include "../Resource/XMLFile.h"

#include "../DebugNew.h"

namespace Urho3D
{

bool Model::LoadMDL(Deserializer& source)
{
    // Check ID
    ea::string fileID = source.ReadFileID();
    if (fileID != "UMDL" && fileID != "UMD2" && fileID != "UMD3")
    {
        URHO3D_LOGERROR(source.GetName() + " is not a valid model file");
        return false;
    }

    // Read version
    const unsigned version = fileID == "UMD3" ? source.ReadUInt() : legacyVersion;
    const bool hasVertexDeclarations = (fileID != "UMDL");

    unsigned memoryUse = sizeof(Model);
    bool async = GetAsyncLoadState() == ASYNC_LOADING;

    // Read vertex buffers
    unsigned numVertexBuffers = source.ReadUInt();
    vertexBuffers_.reserve(numVertexBuffers);
    morphRangeStarts_.resize(numVertexBuffers);
    morphRangeCounts_.resize(numVertexBuffers);
    loadVBData_.resize(numVertexBuffers);
    for (unsigned i = 0; i < numVertexBuffers; ++i)
    {
        VertexBufferDesc& desc = loadVBData_[i];

        desc.vertexCount_ = source.ReadUInt();
        if (!hasVertexDeclarations)
        {
            unsigned elementMask = source.ReadUInt();
            desc.vertexElements_ = VertexBuffer::GetElements(elementMask);
        }
        else
        {
            desc.vertexElements_.clear();
            unsigned numElements = source.ReadUInt();
            for (unsigned j = 0; j < numElements; ++j)
            {
                unsigned elementDesc = source.ReadUInt();
                auto type = (VertexElementType)(elementDesc & 0xffu);
                auto semantic = (VertexElementSemantic)((elementDesc >> 8u) & 0xffu);
                auto index = (unsigned char)((elementDesc >> 16u) & 0xffu);
                desc.vertexElements_.push_back(VertexElement(type, semantic, index));
            }
        }

        morphRangeStarts_[i] = source.ReadUInt();
        morphRangeCounts_[i] = source.ReadUInt();

        SharedPtr<VertexBuffer> buffer(MakeShared<VertexBuffer>(context_));
        unsigned vertexSize = VertexBuffer::GetVertexSize(desc.vertexElements_);
        desc.dataSize_ = desc.vertexCount_ * vertexSize;

        buffer->SetDebugName(Format("Model '{}' Vertex Buffer #{}", GetName(), i));

        // Prepare vertex buffer data to be uploaded during EndLoad()
        if (async)
        {
            desc.data_ = new unsigned char[desc.dataSize_];
            source.Read(desc.data_.get(), desc.dataSize_);
        }
        else
        {
            // If not async loading, use locking to avoid extra allocation & copy
            desc.data_.reset(); // Make sure no previous data
            buffer->SetShadowed(true);
            buffer->SetSize(desc.vertexCount_, desc.vertexElements_);
            void* dest = buffer->Map();
            source.Read(dest, desc.vertexCount_ * vertexSize);
            buffer->Unmap();
        }

        memoryUse += sizeof(VertexBuffer) + desc.vertexCount_ * vertexSize;
        vertexBuffers_.push_back(buffer);
    }

    // Read index buffers
    unsigned numIndexBuffers = source.ReadUInt();
    indexBuffers_.reserve(numIndexBuffers);
    loadIBData_.resize(numIndexBuffers);
    for (unsigned i = 0; i < numIndexBuffers; ++i)
    {
        unsigned indexCount = source.ReadUInt();
        unsigned indexSize = source.ReadUInt();

        SharedPtr<IndexBuffer> buffer(MakeShared<IndexBuffer>(context_));
        buffer->SetDebugName(Format("Model '{}' Index Buffer #{}", GetName(), i));

        // Prepare index buffer data to be uploaded during EndLoad()
        if (async)
        {
            loadIBData_[i].indexCount_ = indexCount;
            loadIBData_[i].indexSize_ = indexSize;
            loadIBData_[i].dataSize_ = indexCount * indexSize;
            loadIBData_[i].data_ = new unsigned char[loadIBData_[i].dataSize_];
            source.Read(loadIBData_[i].data_.get(), loadIBData_[i].dataSize_);
        }
        else
        {
            // If not async loading, use locking to avoid extra allocation & copy
            loadIBData_[i].data_.reset(); // Make sure no previous data
            buffer->SetShadowed(true);
            buffer->SetSize(indexCount, indexSize > sizeof(unsigned short));
            void* dest = buffer->Map();
            source.Read(dest, indexCount * indexSize);
            buffer->Unmap();
        }

        memoryUse += sizeof(IndexBuffer) + indexCount * indexSize;
        indexBuffers_.push_back(buffer);
    }

    // Read geometries
    unsigned numGeometries = source.ReadUInt();
    geometries_.reserve(numGeometries);
    geometryBoneMappings_.reserve(numGeometries);
    geometryCenters_.reserve(numGeometries);
    loadGeometries_.resize(numGeometries);
    for (unsigned i = 0; i < numGeometries; ++i)
    {
        // Read bone mappings
        unsigned boneMappingCount = source.ReadUInt();
        ea::vector<unsigned> boneMapping(boneMappingCount);
        for (unsigned j = 0; j < boneMappingCount; ++j)
            boneMapping[j] = source.ReadUInt();
        geometryBoneMappings_.push_back(boneMapping);

        unsigned numLodLevels = source.ReadUInt();
        ea::vector<SharedPtr<Geometry> > geometryLodLevels;
        geometryLodLevels.reserve(numLodLevels);
        loadGeometries_[i].resize(numLodLevels);

        for (unsigned j = 0; j < numLodLevels; ++j)
        {
            float distance = source.ReadFloat();
            auto type = (PrimitiveType)source.ReadUInt();

            unsigned vbRef = source.ReadUInt();
            unsigned ibRef = source.ReadUInt();
            unsigned indexStart = source.ReadUInt();
            unsigned indexCount = source.ReadUInt();

            if (vbRef >= vertexBuffers_.size())
            {
                URHO3D_LOGERROR("Vertex buffer index out of bounds");
                loadVBData_.clear();
                loadIBData_.clear();
                loadGeometries_.clear();
                return false;
            }
            if (ibRef >= indexBuffers_.size())
            {
                URHO3D_LOGERROR("Index buffer index out of bounds");
                loadVBData_.clear();
                loadIBData_.clear();
                loadGeometries_.clear();
                return false;
            }

            SharedPtr<Geometry> geometry(MakeShared<Geometry>(context_));
            geometry->SetLodDistance(distance);

            // Prepare geometry to be defined during EndLoad()
            loadGeometries_[i][j].type_ = type;
            loadGeometries_[i][j].vbRef_ = vbRef;
            loadGeometries_[i][j].ibRef_ = ibRef;
            loadGeometries_[i][j].indexStart_ = indexStart;
            loadGeometries_[i][j].indexCount_ = indexCount;

            geometryLodLevels.push_back(geometry);
            memoryUse += sizeof(Geometry);
        }

        geometries_.push_back(geometryLodLevels);
    }

    // Read morphs
    unsigned numMorphs = source.ReadUInt();
    morphs_.reserve(numMorphs);
    for (unsigned i = 0; i < numMorphs; ++i)
    {
        ModelMorph newMorph;

        newMorph.name_ = source.ReadString();
        newMorph.nameHash_ = newMorph.name_;
        if (version >= morphWeightVersion)
            newMorph.weight_ = source.ReadFloat();
        unsigned numBuffers = source.ReadUInt();

        for (unsigned j = 0; j < numBuffers; ++j)
        {
            VertexBufferMorph newBuffer;
            unsigned bufferIndex = source.ReadUInt();

            newBuffer.elementMask_ = VertexMaskFlags(source.ReadUInt());
            newBuffer.vertexCount_ = source.ReadUInt();

            // Base size: size of each vertex index
            unsigned vertexSize = sizeof(unsigned);
            // Add size of individual elements
            if (newBuffer.elementMask_ & MASK_POSITION)
                vertexSize += sizeof(Vector3);
            if (newBuffer.elementMask_ & MASK_NORMAL)
                vertexSize += sizeof(Vector3);
            if (newBuffer.elementMask_ & MASK_TANGENT)
                vertexSize += sizeof(Vector3);
            newBuffer.dataSize_ = newBuffer.vertexCount_ * vertexSize;
            newBuffer.morphData_ = new unsigned char[newBuffer.dataSize_];

            source.Read(&newBuffer.morphData_[0], newBuffer.vertexCount_ * vertexSize);

            newMorph.buffers_[bufferIndex] = newBuffer;
            memoryUse += sizeof(VertexBufferMorph) + newBuffer.vertexCount_ * vertexSize;
        }

        morphs_.push_back(newMorph);
        memoryUse += sizeof(ModelMorph);
    }

    // Read skeleton
    skeleton_.Load(source);
    memoryUse += skeleton_.GetNumBones() * sizeof(Bone);

    // Read bounding box
    boundingBox_ = source.ReadBoundingBox();

    // Read geometry centers
    for (unsigned i = 0; i < geometries_.size() && !source.IsEof(); ++i)
        geometryCenters_.push_back(source.ReadVector3());
    while (geometryCenters_.size() < geometries_.size())
        geometryCenters_.push_back(Vector3::ZERO);
    memoryUse += sizeof(Vector3) * geometries_.size();

    SetMemoryUse(memoryUse);
    return true;
}

}
