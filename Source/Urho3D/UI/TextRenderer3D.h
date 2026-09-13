// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
    /// Depth test mode.
    bool depthTest_{true};
    /// View mask.
    unsigned viewMask_{DEFAULT_VIEWMASK};

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
        CombineHash(hash_, depthTest_);
        CombineHash(hash_, viewMask_);
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
            && textAlignment_ == rhs.textAlignment_
            && depthTest_ == rhs.depthTest_
            && viewMask_ == rhs.viewMask_;
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
