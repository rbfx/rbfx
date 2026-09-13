// Copyright (c) 2021 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Precompiled.h"

#include "ParticleGraphEffect.h"

#include "../Core/Context.h"
#include "../Core/Thread.h"
#include "../Graphics//Graphics.h"
#include "../IO/ArchiveSerialization.h"
#include "../IO/Deserializer.h"
#include "../IO/FileSystem.h"
#include "../Resource/XMLArchive.h"
#include "../Resource/XMLFile.h"
#include "ParticleGraphLayer.h"

namespace Urho3D
{

ParticleGraphEffect::ParticleGraphEffect(Context* context)
    : Resource(context)
{
}

ParticleGraphEffect::~ParticleGraphEffect() = default;

void ParticleGraphEffect::RegisterObject(Context* context)
{
    context->AddFactoryReflection<ParticleGraphEffect>();
}

void ParticleGraphEffect::SetNumLayers(unsigned numLayers)
{
    while (numLayers < layers_.size())
    {
        layers_.pop_back();
    }
    layers_.reserve(numLayers);
    while (numLayers > layers_.size())
    {
        layers_.push_back(MakeShared<ParticleGraphLayer>(context_));
    }
}

unsigned ParticleGraphEffect::GetNumLayers() const
{
    return static_cast<unsigned>(layers_.size());
}

SharedPtr<ParticleGraphLayer> ParticleGraphEffect::GetLayer(unsigned layerIndex) const
{
    return layers_[layerIndex];
}

bool ParticleGraphEffect::BeginLoad(Deserializer& source)
{
    ea::string extension = GetExtension(source.GetName());

    ResetToDefaults();

    const auto xmlFile = MakeShared<XMLFile>(context_);
    if (!xmlFile->Load(source))
        return false;

    return xmlFile->LoadObject("particleGraphEffect", *this);
}

void ParticleGraphEffect::ResetToDefaults()
{
    // Needs to be a no-op when async loading, as this does a GetResource() which is not allowed from worker threads
    if (!Thread::IsMainThread())
        return;

    layers_.clear();
}

bool ParticleGraphEffect::Save(Serializer& dest) const
{
    const auto xmlFile = MakeShared<XMLFile>(context_);
    xmlFile->SaveObject("particleGraphEffect", *this);
    xmlFile->Save(dest);
    return true;
}

void ParticleGraphEffect::SerializeInBlock(Archive& archive)
{
    const bool loading = archive.IsInput();

    SerializeVectorAsObjects(archive, "layers", layers_, "layer",
        [&](Archive& archive, const char* name, SharedPtr<ParticleGraphLayer>& value)
    {
        if (loading)
            value = MakeShared<ParticleGraphLayer>(context_);
        SerializeValue(archive, name, *value);
    });
}


}
