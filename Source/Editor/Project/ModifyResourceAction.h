// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Core/CommonEditorActions.h"

namespace Urho3D
{

class Project;

class ModifyResourceAction : public EditorAction
{
public:
    explicit ModifyResourceAction(Project* project);
    void AddResource(Resource* resource);

    void DisableAutoComplete();
    void SaveOnComplete();

    /// Implement EditorAction.
    /// @{
    bool IsComplete() const override { return !newData_.empty(); }
    void Complete(bool force) override;
    void Redo() const override;
    void Undo() const override;
    bool MergeWith(const EditorAction& other) override;
    /// @}

private:
    struct ResourceData
    {
        StringHash resourceType_;
        ea::string fileName_;
        SharedByteVector bytes_;
    };

    void ApplyResourceData(const ea::string& resourceName, const ResourceData& data) const;

    WeakPtr<Project> project_;
    Context* context_{};

    bool autoComplete_{true};
    bool saveOnComplete_{};

    ea::unordered_map<ea::string, ResourceData> oldData_;
    ea::unordered_map<ea::string, ResourceData> newData_;

    mutable ea::function<void()> callback_;
};

}