//
// Copyright (c) 2017-2020 the rbfx project.
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

#include "MaterialInspectorUndo.h"

namespace Urho3D
{

UndoTechniqueChanged::UndoTechniqueChanged(const Material* material, unsigned index, const TechniqueEntry* oldEntry, const TechniqueEntry* newEntry)
    : context_(material->GetContext())
    , materialName_(material->GetName())
    , index_(index)
{
    if (oldEntry != nullptr)
    {
        oldValue_.techniqueName_ = oldEntry->original_->GetName();
        oldValue_.qualityLevel_ = oldEntry->qualityLevel_;
        oldValue_.lodDistance_ = oldEntry->lodDistance_;
    }
    if (newEntry != nullptr)
    {
        newValue_.techniqueName_ = newEntry->original_->GetName();
        newValue_.qualityLevel_ = newEntry->qualityLevel_;
        newValue_.lodDistance_ = newEntry->lodDistance_;
    }
}

void UndoTechniqueChanged::RemoveTechnique()
{
    if (auto* material = context_->GetSubsystem<ResourceCache>()->GetResource<Material>(materialName_))
    {
        // Shift techniques back
        for (auto i = index_ + 1; i < material->GetNumTechniques(); i++)
        {
            const auto& entry = material->GetTechniqueEntry(i);
            material->SetTechnique(i - 1, entry.original_.Get(), entry.qualityLevel_, entry.lodDistance_);
        }
        // Remove last one
        material->SetNumTechniques(material->GetNumTechniques() - 1);
    }
}

void UndoTechniqueChanged::AddTechnique(const UndoTechniqueChanged::TechniqueInfo& info)
{
    if (auto* material = context_->GetSubsystem<ResourceCache>()->GetResource<Material>(materialName_))
    {
        if (auto* technique = context_->GetSubsystem<ResourceCache>()->GetResource<Technique>(info.techniqueName_))
        {
            auto index = material->GetNumTechniques();
            material->SetNumTechniques(index + 1);

            // Shift techniques front
            for (auto i = index_ + 1; i < material->GetNumTechniques(); i++)
            {
                const auto& entry = material->GetTechniqueEntry(i - 1);
                material->SetTechnique(i, entry.original_.Get(), entry.qualityLevel_, entry.lodDistance_);
            }
            // Insert new technique
            material->SetTechnique(index_, technique, info.qualityLevel_, info.lodDistance_);
        }
    }
}

void UndoTechniqueChanged::SetTechnique(const UndoTechniqueChanged::TechniqueInfo& info)
{
    if (auto* material = context_->GetSubsystem<ResourceCache>()->GetResource<Material>(materialName_))
    {
        if (auto* technique = context_->GetSubsystem<ResourceCache>()->GetResource<Technique>(info.techniqueName_))
            material->SetTechnique(static_cast<unsigned int>(index_), technique, info.qualityLevel_, info.lodDistance_);
    }
}

bool UndoTechniqueChanged::Undo(Context* context)
{
    if (auto* material = context_->GetSubsystem<ResourceCache>()->GetResource<Material>(materialName_))
    {
        if (oldValue_.techniqueName_.empty() && !newValue_.techniqueName_.empty())
            // Was added
            RemoveTechnique();
        else if (!oldValue_.techniqueName_.empty() && newValue_.techniqueName_.empty())
            // Was removed
            AddTechnique(oldValue_);
        else
            // Was modified
            SetTechnique(oldValue_);

        context_->GetSubsystem<ResourceCache>()->IgnoreResourceReload(material);
        material->SaveFile(context_->GetSubsystem<ResourceCache>()->GetResourceFileName(material->GetName()));
        return true;
    }
    return false;
}

bool UndoTechniqueChanged::Redo(Context* context)
{
    if (auto* material = context_->GetSubsystem<ResourceCache>()->GetResource<Material>(materialName_))
    {
        if (oldValue_.techniqueName_.empty() && !newValue_.techniqueName_.empty())
            // Was added
            AddTechnique(newValue_);
        else if (!oldValue_.techniqueName_.empty() && newValue_.techniqueName_.empty())
            // Was removed
            RemoveTechnique();
        else
            // Was modified
            SetTechnique(newValue_);

        context_->GetSubsystem<ResourceCache>()->IgnoreResourceReload(material);
        material->SaveFile(context_->GetSubsystem<ResourceCache>()->GetResourceFileName(material->GetName()));
        return true;
    }
    return false;
}

UndoShaderParameterChanged::UndoShaderParameterChanged(const Material* material, const ea::string& parameterName, const Variant& oldValue, const Variant& newValue)
    : context_(material->GetContext())
    , materialName_(material->GetName())
    , parameterName_(parameterName)
    , oldValue_(oldValue)
    , newValue_(newValue)
{
}

bool UndoShaderParameterChanged::Undo(Context* context)
{
    if (auto* material = context_->GetSubsystem<ResourceCache>()->GetResource<Material>(materialName_))
    {
        if (oldValue_.IsEmpty() && !newValue_.IsEmpty())
            // Was added
            material->RemoveShaderParameter(parameterName_);
        else
            // Was removed or modified
            material->SetShaderParameter(parameterName_, oldValue_);

        context_->GetSubsystem<ResourceCache>()->IgnoreResourceReload(material);
        material->SaveFile(context_->GetSubsystem<ResourceCache>()->GetResourceFileName(material->GetName()));
        return true;
    }
    return false;
}

bool UndoShaderParameterChanged::Redo(Context* context)
{
    if (auto* material = context_->GetSubsystem<ResourceCache>()->GetResource<Material>(materialName_))
    {
        if (!oldValue_.IsEmpty() && newValue_.IsEmpty())
            // Was removed
            material->RemoveShaderParameter(parameterName_);
        else
            // Was added or modified
            material->SetShaderParameter(parameterName_, newValue_);

        context_->GetSubsystem<ResourceCache>()->IgnoreResourceReload(material);
        material->SaveFile(context_->GetSubsystem<ResourceCache>()->GetResourceFileName(material->GetName()));
        return true;
    }
    return false;
}

}   // namespace Urho3D
