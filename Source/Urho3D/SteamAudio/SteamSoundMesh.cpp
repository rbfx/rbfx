//
// Copyright (c) 2024-2024 the Urho3D project.
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

#include "../SteamAudio/SteamSoundMesh.h"
#include "../SteamAudio/SteamAudio.h"
#include "../Graphics/Model.h"
#include "../Graphics/Geometry.h"
#include "../Graphics/VertexBuffer.h"
#include "../Graphics/IndexBuffer.h"
#include "../Scene/Node.h"
#include "../Resource/ResourceCache.h"
#include "../Core/Context.h"

#include <phonon.h>

namespace Urho3D
{

SteamSoundMesh::SteamSoundMesh(Context* context) :
    Component(context), mesh_(nullptr), subScene_(nullptr)
{
    audio_ = GetSubsystem<SteamAudio>();

    if (audio_) {
        // Create subscene
        IPLSceneSettings sceneSettings {
            .type = IPL_SCENETYPE_DEFAULT
        };
        iplSceneCreate(audio_->GetPhononContext(), &sceneSettings, &subScene_);

        // Add this sound mesh
        audio_->AddSoundMesh(this);
    }
}

SteamSoundMesh::~SteamSoundMesh()
{
    if (audio_) {
        // Remove subscene and mesh
        iplSceneRelease(&subScene_);
        iplStaticMeshRelease(&mesh_);

        // Remove this sound mesh
        audio_->RemoveSoundMesh(this);
    }
}

void SteamSoundMesh::RegisterObject(Context* context)
{
    context->AddFactoryReflection<SteamSoundMesh>(Category_Audio);

    URHO3D_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Model", GetModel, SetModel, ResourceRef, ResourceRef(Model::GetTypeStatic()), AM_DEFAULT);
}

void SteamSoundMesh::SetModel(const ResourceRef& model)
{
    auto* cache = GetSubsystem<ResourceCache>();
    model_ = cache->GetResource<Model>(model.name_);

    // Do nothing if no audio subsystem
    if (!audio_)
        return;

    // Clear if no model set
    if (!model_) {
        ResetModel();
        return;
    } else if (mesh_) {
        ResetModel();
    }

    // Extract points and indices
    ea::vector<Vector3> allPoints;
    ea::vector<unsigned> allIndices;
    for (const auto& geometry : model_->GetGeometries()) {
        for (const auto& geometry : geometry) {
            for (const auto& vertexBuffer : geometry->GetVertexBuffers()) {
                ea::vector<Vector4> points(vertexBuffer->GetVertexCount());
                vertexBuffer->UnpackVertexData(vertexBuffer->GetShadowData(), vertexBuffer->GetVertexSize(), *vertexBuffer->GetElement(SEM_POSITION), 0, vertexBuffer->GetVertexCount(), points.data(), sizeof(Vector4));
                for (const auto& point : points)
                    allPoints.push_back({point.x_, point.y_, point.z_});
            }
            const auto& indexBuffer = geometry->GetIndexBuffer();
            const auto indices = indexBuffer->GetUnpackedData();
            allIndices.insert(allIndices.begin(), indices.begin(), indices.end());
        }
    }

    // Convert to phonon points and indices
    ea::vector<IPLVector3> phononVertices;
    phononVertices.reserve(allPoints.size());
    ea::vector<IPLTriangle> phononTriangles;
    phononTriangles.reserve(allIndices.size()/3);
    for (const auto& point : allPoints)
        phononVertices.push_back({point.x_, point.y_, point.z_});
    for (unsigned idx = 0; idx < allIndices.size(); idx += 3)
        phononTriangles.push_back({int(allIndices[idx+0]), int(allIndices[idx+1]), int(allIndices[idx+2])});
    ea::vector<IPLint32> phononMaterialIndices(phononTriangles.size(), 0); // All triangles use the same material (for now)

    // Create settings
    IPLStaticMeshSettings staticMeshSettings {};
    staticMeshSettings.numVertices = phononVertices.size();
    staticMeshSettings.numTriangles = phononTriangles.size();
    staticMeshSettings.numMaterials = 1;
    staticMeshSettings.vertices = phononVertices.data();
    staticMeshSettings.triangles = phononTriangles.data();
    staticMeshSettings.materialIndices = phononMaterialIndices.data();
    staticMeshSettings.materials = &material_;

    // Create static mesh
    iplStaticMeshCreate(subScene_, &staticMeshSettings, &mesh_);
    iplStaticMeshAdd(mesh_, subScene_);
    iplSceneCommit(subScene_);
    iplSceneSaveOBJ(subScene_, ("scene-"+ea::to_string(GetNode()->GetID())+".obj").c_str());

    // Create instanced mesh
    IPLInstancedMeshSettings instancedMeshSettings {
        .subScene = subScene_,
        .transform = GetPhononMatrix()
    };
    iplInstancedMeshCreate(audio_->GetScene(), &instancedMeshSettings, &instancedMesh_);
    iplInstancedMeshAdd(instancedMesh_, audio_->GetScene());

    // Mark scene as dirty
    audio_->MarkSceneDirty();
}

ResourceRef SteamSoundMesh::GetModel() const
{
    return GetResourceRef(model_, Model::GetTypeStatic());
}

void SteamSoundMesh::ResetModel()
{
    // Release ressources
    iplInstancedMeshRemove(instancedMesh_, audio_->GetScene());
    iplInstancedMeshRelease(&instancedMesh_);
    iplStaticMeshRemove(mesh_, subScene_);
    iplStaticMeshRelease(&mesh_);
    mesh_ = nullptr;

    // Mark scene as dirty
    audio_->MarkSceneDirty();
}

IPLMatrix4x4 SteamSoundMesh::GetPhononMatrix() const
{
    const Matrix3x4 m = GetNode()->GetWorldTransform();
    return {
        {
            {m.Element(0, 0), m.Element(0, 1), m.Element(0, 2), m.Element(0, 3)},
            {m.Element(1, 0), m.Element(1, 1), m.Element(1, 2), m.Element(1, 3)},
            {m.Element(2, 0), m.Element(2, 1), m.Element(2, 2), m.Element(2, 3)},
            {0.0f,            0.0f,            0.0f,            1.0f           }
        }
    };
}

}
