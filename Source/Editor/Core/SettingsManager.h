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

#pragma once

#include <Urho3D/Core/Object.h>

#include <EASTL/map.h>
#include <EASTL/vector.h>
#include <EASTL/unique_ptr.h>

namespace Urho3D
{

class HotkeyManager;

/// Base class of an individual page with settings.
class SettingsPage : public Object
{
    URHO3D_OBJECT(SettingsPage, Object);

public:
    explicit SettingsPage(Context* context);

    /// Return unique name of the page.
    virtual ea::string GetUniqueName() = 0;
    /// Return whether the serialization is needed.
    virtual bool IsSerializable() = 0;
    /// Return whether to show "reset to default" button.
    virtual bool CanResetToDefault() { return false; }

    /// Serialization must be provided for settings page.
    virtual void SerializeInBlock(Archive& archive) override = 0;
    /// Render page with settings.
    virtual void RenderSettings() = 0;
    /// Reset settings to default.
    virtual void ResetToDefaults() {}
    /// Apply hotkeys for the page.
    virtual void ApplyHotkeys(HotkeyManager* hotkeyManager) {}
};

/// Simple settings page that expects struct with methods.
template <class T>
class SimpleSettingsPage : public SettingsPage
{
public:
    using SettingsPage::SettingsPage;

    /// Implement SettingsPage
    /// @{
    ea::string GetUniqueName() override { return values_.GetUniqueName(); }
    bool IsSerializable() override { return true; }
    bool CanResetToDefault() override { return true; }

    void SerializeInBlock(Archive& archive) override { values_.SerializeInBlock(archive); }
    void RenderSettings() override { values_.RenderSettings(); }
    void ResetToDefaults() override { values_ = T{}; }
    void ApplyHotkeys(HotkeyManager* hotkeyManager) override {}
    /// @}

    const T& GetValues() const { return values_; }

private:
    T values_;
};

struct SettingsPageGroup
{
    ea::map<ea::string, SharedPtr<SettingsPage>> pages_;
    ea::map<ea::string, ea::unique_ptr<SettingsPageGroup>> children_;
};

/// Class used to manage and serialize settings.
class SettingsManager : public Object
{
    URHO3D_OBJECT(SettingsManager, Object);

public:
    using PageMap = ea::map<ea::string, SharedPtr<SettingsPage>>;

    explicit SettingsManager(Context* context);

    /// Add new page with settings. Should have unique key.
    void AddPage(SharedPtr<SettingsPage> page);

    /// Serialize
    /// @{
    void SerializeInBlock(Archive& archive) override;
    void LoadFile(const ea::string& fileName);
    void SaveFile(const ea::string& fileName) const;
    /// @}

    /// Find page by key.
    SettingsPage* FindPage(const ea::string& key) const;
    /// Return sorted pages.
    const PageMap& GetSortedPages() const { return sortedPages_; }
    /// Return page tree for rendering.
    const SettingsPageGroup& GetPageTree() const { return rootGroup_; }

private:
    void InsertPageInGroup(SettingsPageGroup& parentGroup, ea::string_view path, const SharedPtr<SettingsPage>& page, ea::string_view section);

    ea::vector<SharedPtr<SettingsPage>> pages_;
    PageMap sortedPages_;
    SettingsPageGroup rootGroup_;
};

}
