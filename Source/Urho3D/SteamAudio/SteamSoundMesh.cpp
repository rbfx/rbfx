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
#include "../Scene/Node.h"
#include "../Resource/ResourceCache.h"
#include "../Core/Context.h"

#include <phonon.h>

namespace Urho3D
{

SteamSoundMesh::SteamSoundMesh(Context* context) :
    Component(context)
{
    audio_ = GetSubsystem<SteamAudio>();

    if (audio_)
        // Add this sound mesh
        audio_->AddSoundMesh(this);
}

SteamSoundMesh::~SteamSoundMesh()
{
    if (audio_)
        // Remove this sound mesh
        audio_->RemoveSoundMesh(this);
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

    // Clear if no model set
    if (!model_)
        return;

    // Extract mesh
    for (const auto& geometry : model_->GetGeometries()) {
        for (const auto& geometry : geometry) {
            for (const auto& vertexBuffer : geometry->GetVertexBuffers()) {
                //vertexBuffer->...?
            }
        }
    }
}

ResourceRef SteamSoundMesh::GetModel() const
{
    return GetResourceRef(model_, Model::GetTypeStatic());
}

}
