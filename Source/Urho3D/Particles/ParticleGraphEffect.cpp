//
// Copyright (c) 2021 the rbfx project.
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
