//
// Copyright (c) 2008-2017 the Urho3D project.
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

#include "../Core/Variant.h"
#include "../Graphics/Texture2D.h"
#include "../SystemUI/ImGui.h"

#include <EASTL/optional.h>

namespace Urho3D
{

namespace Widgets
{

/// Return best size of small square button with one icon.
URHO3D_API float GetSmallButtonSize();
/// Render toolbar button with optional tooltip. May be toggled on.
URHO3D_API bool ToolbarButton(const char* label, const char* tooltip = nullptr, bool active = false);
/// Render a bit of space in toolbar between buttons.
URHO3D_API void ToolbarSeparator();
/// Render a label for next item. Label may be on the left or on the right, depending on flags.
URHO3D_API void ItemLabel(ea::string_view title, const ea::optional<Color>& color = ea::nullopt, bool isLeft = true);
/// Return recommended color for item label.
URHO3D_API Color GetItemLabelColor(bool isUndefined, bool defaultValue);
/// Return recommended color for item background.
URHO3D_API Color GetItemBackgroundColor(bool isUndefined);

/// Underline previously rendered item (usually text).
URHO3D_API void Underline(const Color& color);
/// Render clickable URL text.
URHO3D_API void TextURL(const ea::string& label, const ea::string& url);

/// Fit content into area.
URHO3D_API ImVec2 FitContent(const ImVec2& contentArea, const ImVec2& originalSize);

/// Render Texture2D as ImGui item.
URHO3D_API void Image(Texture2D* texture, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tintCol = ImVec4(1, 1, 1, 1), const ImVec4& borderCol = ImVec4(0, 0, 0, 0));
URHO3D_API void ImageItem(Texture2D* texture, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), const ImVec4& tintCol = ImVec4(1, 1, 1, 1), const ImVec4& borderCol = ImVec4(0, 0, 0, 0));
URHO3D_API bool ImageButton(Texture2D* texture, const ImVec2& size, const ImVec2& uv0 = ImVec2(0, 0), const ImVec2& uv1 = ImVec2(1, 1), int framePadding = -1, const ImVec4 & bgCol = ImVec4(0, 0, 0, 0), const ImVec4 & tintCol = ImVec4(1, 1, 1, 1));

/// Parameters for rendering arbitrary variant value.
struct EditVariantOptions
{
    /// Increment per pixel for scalar scrolls.
    double step_{0.01};
    /// Minimum value (for component).
    double min_{0.0f};
    /// Maximum value (for component).
    double max_{0.0f};
    /// Whether to treat Vector3 and Vector4 as color values.
    bool asColor_{};
    /// Whether to allow resize for dynamically sized containers.
    bool allowResize_{};
    /// Whether to allow element type changes for containers.
    bool allowTypeChange_{};
    /// Whether to treat integer as bitmask.
    bool asBitmask_{};
    /// Whether to extract elements metadata dynamically from the inspected StringVariantMap itself.
    bool dynamicMetadata_{};
    /// Enum values used to convert integer to string.
    const StringVector* intToString_{};
    /// Allowed resource types.
    const StringVector* resourceTypes_{};
    /// Structure array element names.
    const StringVector* sizedStructVectorElements_{};

    EditVariantOptions& AsColor() { asColor_ = true; return *this; }
    EditVariantOptions& AsBitmask() { asBitmask_ = true; return *this; }
    EditVariantOptions& Range(double min, double max) { min_ = min; max_ = max; return *this; }
    EditVariantOptions& Step(double step) { step_ = step; return *this; }
    EditVariantOptions& Enum(const StringVector& values) { intToString_ = &values; return *this; }
    EditVariantOptions& ResourceTypes(const StringVector& types) { resourceTypes_ = &types; return *this; }
    EditVariantOptions& SizedStructVector(const StringVector& names) { sizedStructVectorElements_ = &names; return *this; }
    EditVariantOptions& AllowResize() { allowResize_ = true; return *this; }
    EditVariantOptions& AllowTypeChange() { allowTypeChange_ = true; return *this; }
    EditVariantOptions& DynamicMetadata() { dynamicMetadata_ = true; return *this; }
};

/// Render reference to resource with optional type constraints. If allowed types are not specified, only current type is allowed.
URHO3D_API bool EditResourceRef(StringHash& type, ea::string& name, const StringVector* allowedTypes);

/// Render vector of resource references with optional type constraints. If allowed types are not specified, only current type is allowed.
URHO3D_API bool EditResourceRefList(StringHash& type, StringVector& names, const StringVector* allowedTypes,
    bool resizable, const StringVector* elementNames = nullptr);

/// Render bitmask.
URHO3D_API bool EditBitmask(unsigned& value);

/// Render variant type selector.
URHO3D_API bool EditVariantType(VariantType& value, const char* button = nullptr);

/// Render variant value in most flexible way w/o type selector.
URHO3D_API bool EditVariantValue(Variant& value);

/// Render variant vector with optional type and size constraints.
URHO3D_API bool EditVariantVector(VariantVector& value, bool resizable, bool dynamicTypes, const StringVector* elementNames = nullptr);

/// Render string vector with optional size constraints.
URHO3D_API bool EditStringVector(StringVector& value, bool resizable);

/// Render string variant map optional type and size constraints.
URHO3D_API bool EditStringVariantMap(StringVariantMap& value, bool resizable, bool dynamicTypes, bool dynamicMetadata);

/// Render arbitrary variant value editor.
URHO3D_API bool EditVariant(Variant& var, const EditVariantOptions& options = {});

/// Render ItemLabel with scope guard of the same name.
struct ItemLabelScopeGuard : public IdScopeGuard
{
    ItemLabelScopeGuard(const char* title, const ea::optional<Color>& color = ea::nullopt, bool isLeft = true)
        : IdScopeGuard(title)
    {
        ItemLabel(title, color, isLeft);
    }
};

}

}

// TODO(editor): Remove or move up
namespace ImGui
{

}
