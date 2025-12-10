// Copyright (c) 2025-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/RmlUI/RmlUIManager.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/RmlUI/RmlUI.h"
#include "Urho3D/RmlUI/RmlUIComponent.h"
#include "Urho3D/Scene/Scene.h"

#include <EASTL/unordered_map.h>

namespace Urho3D
{

RmlUIManager::RmlUIManager(Context* context)
    : Component(context)
    , rmlUi_(GetSubsystem<RmlUI>())
{
}

RmlUIManager::~RmlUIManager() = default;

void RmlUIManager::RegisterObject(Context* context)
{
    context->AddFactoryReflection<RmlUIManager>();
}

void RmlUIManager::SetDocumentsEnabled(bool enabled)
{
    for (const auto& document : documents_)
    {
        if (document)
            document->SetEnabled(enabled);
    }
}

void RmlUIManager::SetOwner(RmlUI* rmlUi)
{
    if (rmlUi_ == rmlUi)
        return;

    ea::unordered_map<WeakPtr<RmlUIComponent>, bool> wasEnabled;
    for (const auto& document : documents_)
    {
        if (document)
        {
            wasEnabled[document] = document->IsEnabled();
            document->SetEnabled(false);
        }
    }

    rmlUi_ = rmlUi;

    for (const auto& document : documents_)
    {
        if (document)
            document->SetEnabled(wasEnabled[document]);
    }
}

void RmlUIManager::AddDocument(RmlUIComponent* component)
{
    documents_.insert(WeakPtr<RmlUIComponent>{component});
}

void RmlUIManager::RemoveDocument(RmlUIComponent* component)
{
    documents_.erase(WeakPtr<RmlUIComponent>{component});
}

void RmlUIManager::OnSceneSet(Scene* previousScene, Scene* scene)
{
    documents_.clear();
    if (!scene)
        return;

    ea::vector<RmlUIComponent*> documents;
    scene->FindComponents(documents, ComponentSearchFlag::Default | ComponentSearchFlag::Derived);

    for (RmlUIComponent* document : documents)
    {
        AddDocument(document);
        document->ReconnectToManager();
    }
}

} // namespace Urho3D
