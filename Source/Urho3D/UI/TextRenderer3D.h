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

/// \file

#pragma once

#include "../Scene/LogicComponent.h"
#include "../UI/Text3D.h"
#include "../UI/Font.h"

#include <EASTL/unordered_map.h>

namespace Urho3D
{

/// Parameters of rendered 3D text.
struct TextParams3D
{
    /// Text.
    ea::string text_;
    /// Font. Default font is used if none.
    SharedPtr<Font> font_;
    /// Font size.
    float fontSize_{};
    /// Face camera mode.
    FaceCameraMode faceCamera_{ FC_ROTATE_XYZ };
    /// Whether to keep fixed screen size.
    bool fixedScreenSize_{ true };
    /// Whether to snap text to pixels.
    bool snapToPixels_{ true };
    /// Horizontal alignment.
    HorizontalAlignment horizontalAlignment_{ HA_CENTER };
    /// Vertical alignment.
    VerticalAlignment verticalAlignment_{ VA_CENTER };
    /// Text alignment.
    HorizontalAlignment textAlignment_{ HA_CENTER };

    /// Precomputed parameter hash.
    unsigned hash_{};

    /// Recalculate parameter hash.
    void RecalculateHash()
    {
        hash_ = 0;
        CombineHash(hash_, MakeHash(text_));
        CombineHash(hash_, MakeHash(font_));
        CombineHash(hash_, MakeHash(fontSize_));
        CombineHash(hash_, faceCamera_);
        CombineHash(hash_, fixedScreenSize_);
        CombineHash(hash_, snapToPixels_);
        CombineHash(hash_, horizontalAlignment_);
        CombineHash(hash_, verticalAlignment_);
        CombineHash(hash_, textAlignment_);
    }

    /// Return precomputed parameter hash.
    unsigned ToHash() const { return hash_; }

    /// Compare to another object.
    bool operator ==(const TextParams3D& rhs) const
    {
        return text_ == rhs.text_
            && font_ == rhs.font_
            && fontSize_ == rhs.fontSize_
            && faceCamera_ == rhs.faceCamera_
            && fixedScreenSize_ == rhs.fixedScreenSize_
            && snapToPixels_ == rhs.snapToPixels_
            && horizontalAlignment_ == rhs.horizontalAlignment_
            && verticalAlignment_ == rhs.verticalAlignment_
            && textAlignment_ == rhs.textAlignment_;
    }
};

/// Utility class providing 3D text rendering API in immediate mode.
/// Text shall be added before PostUpdate event.
class URHO3D_API TextRenderer3D : public LogicComponent
{
    URHO3D_OBJECT(TextRenderer3D, LogicComponent);

public:
    /// Construct new.
    explicit TextRenderer3D(Context* context);

    /// Register object.
    static void RegisterObject(Context* context);

    /// Add new 3D text.
    void AddText3D(const Vector3& position, const Quaternion& rotation,
        const Color& color, const TextParams3D& params);

    /// Called before the first update. At this point all other components of the node should exist. Will also be called if update events are not wanted; in that case the event is immediately unsubscribed afterward.
    void DelayedStart() override;
    /// Called on scene post-update, variable timestep.
    void PostUpdate(float timeStep) override;

    /// Set default font size.
    void SetDefaultFontSize(float fontSize) { defaultFontSize_ = fontSize; }
    /// Return default font size.
    float GetDefaultFontSize() const { return defaultFontSize_; }

    /// Set default font attribute.
    void SetDefaultFontAttr(const ResourceRef& value);
    /// Return default font attribute.
    ResourceRef GetDefaultFontAttr() const;

private:
    /// 3D text requested via interface and not processed yet.
    struct QueuedTextElement
    {
        /// Position in world space.
        Vector3 position_;
        /// Rotation in world space.
        Quaternion rotation_;
        /// Color.
        Color color_;
        /// Text parameters.
        TextParams3D params_;
    };
    /// 3D text rendered in previous frame and already cached.
    struct CachedTextElement
    {
        /// Whether the element is used during the frame.
        bool used_{};
        /// Text3D component that renders text.
        WeakPtr<Text3D> text_;
    };
    /// 3D text cache.
    using TextCache = ea::unordered_multimap<TextParams3D, CachedTextElement>;

    /// Default font.
    SharedPtr<Font> defaultFont_;
    /// Default font size.
    float defaultFontSize_{ DEFAULT_FONT_SIZE };
    /// Node that keeps actual Text3D components.
    WeakPtr<Node> containerNode_;
    /// Text elements added during current frame.
    ea::vector<QueuedTextElement> queuedTextNodes_;
    /// New text elements that aren't cached yet.
    ea::vector<QueuedTextElement> newTextNodes_;
    /// Unused disabled Text3D elements.
    // TODO: Cleanup unused elements?
    ea::vector<WeakPtr<Text3D>> unusedCachedTextNodes_;
    /// Currently rendered nodes.
    TextCache cachedTextNodes_;
};

}
