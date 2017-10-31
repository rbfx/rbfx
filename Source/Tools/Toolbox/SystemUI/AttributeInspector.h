//
// Copyright (c) 2008-2017 the Urho3D project.
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


#include <array>

#include <Urho3D/Core/Object.h>
#include <Urho3D/Core/Context.h>


namespace Urho3D
{

class AttributeInspector : public Object
{
    URHO3D_OBJECT(AttributeInspector, Object);
public:
    /// Construct.
    explicit AttributeInspector(Context* context);

    /// Render attribute inspector widgets. Attributes are rendered in columns. Be sure to call `ui::Columns()` before calling this method.
    void RenderAttributes(Serializable* item);

protected:
    /// Return a value buffer that will be unique for specified attribute.
    /// \param name a name of attribute.
    /// \param defaultValue a value that will be fileld in if buffer does not exist.
    std::array<char, 0x1000>& GetBuffer(const String& name, const String& defaultValue);
    /// Discard a value buffer of attribute.
    /// \param name a name of attribute whose value buffer is to be discarded.
    void RemoveBuffer(const String& name);
    /// Render value widget of single attribute.
    /// \returns true if value was modified.
    bool RenderSingleAttribute(const AttributeInfo& info, Variant& value);
    /// Automatically creates two columns where first column is as wide as longest label.
    void NextColumn();

    /// A filter value. Attributes whose titles do not contain substring sored in this variable will not be rendered.
    std::array<char, 0x100> filter_;
    /// Buffers used by system ui for editing attribute values.
    HashMap<String, std::array<char, 0x1000>> buffers_;
    /// Last serializable whose attribute list was rendered.
    WeakPtr<Serializable> lastSerializable_;
    /// Name of attribute that was modified on last frame.
    const char* modifiedLastFrame_ = nullptr;
    /// Value of attribute before modifying it started.
    Variant originalValue_;
    /// Max width of attribute label.
    int maxWidth_ = 0;
};

class AttributeInspectorWindow : public AttributeInspector
{
    URHO3D_OBJECT(AttributeInspectorWindow, Object);
public:
    /// Construct.
    explicit AttributeInspectorWindow(Context* context);

    /// Enable or disable rendering of attribute inspector window.
    void SetEnabled(bool enabled);
    /// Returns true if attribute inspector window is enabled.
    bool IsEnabled() const;
    /// Set serializable whose attributes should be rendered.
    void SetSerializable(Serializable* item);
    /// Return serializable whose attributes are being rendered.
    Serializable* GetSerializable() const { return currentSerializable_; }

protected:
    /// Render attribute inspector UI.
    virtual void RenderUi();

    /// Enable or disable rendering of attribute inspector window.
    bool enabled_ = false;
    /// Current Serializable whose attributes are rendered.
    WeakPtr<Serializable> currentSerializable_;
};

class AttributeInspectorDockWindow : public AttributeInspectorWindow
{
    URHO3D_OBJECT(AttributeInspectorDockWindow, Object);
public:
    /// Construct.
    explicit AttributeInspectorDockWindow(Context* context);

    /// Render attribute inspector UI.
    void RenderUi() override;
};

}
