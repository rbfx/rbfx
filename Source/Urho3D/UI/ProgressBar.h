//
// Copyright (c) 2008-2022 the Urho3D project.
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

#include "BorderImage.h"
#include "../Math/Vector2.h"
#include "Text.h"

namespace Urho3D
{

/// %ProgressBar bar %UI element.
class URHO3D_API ProgressBar : public BorderImage
{
    URHO3D_OBJECT(ProgressBar, BorderImage);

public:
    /// Construct.
    explicit ProgressBar(Context *context);

    /// Destruct.
    ~ProgressBar() override;

    /// Register object factory.
    /// @nobind
    static void RegisterObject(Context *context);

    /// React to resize.
    void OnResize(const IntVector2& newSize, const IntVector2& delta) override;

    /// Set orientation type.
    /// @property
    void SetOrientation(Orientation orientation);

    /// Set ProgressBar range maximum value (minimum value is always 0).
    /// @property
    void SetRange(float range);

    /// Set ProgressBar current value.
    /// @property
    void SetValue(float value);

    /// Change value by a delta.
    void ChangeValue(float delta);

    /// Return orientation type.
    /// @property
    Orientation GetOrientation() const { return orientation_; }

    /// Return ProgressBar range.
    /// @property
    float GetRange() const { return range_; }

    /// Return ProgressBar current value.
    /// @property
    float GetValue() const { return value_; }

    /// Return knob element.
    /// @property
    BorderImage *GetKnob() const { return knob_; }

    /// Sets the loading percent style.
    void SetLoadingPercentStyle(const ea::string &style) { loadingPercentStyle_ = style; }

    /// Returns the loading percent style.
    const ea::string& GetLoadingPercentStyle() const { return loadingPercentStyle_; }

    /// Sets the flag to display the percent text.
    /// @property
    void SetShowPercentText(bool enable);

    /// Returns the flag to display the percent text.
    /// @property
    bool GetShowPercentText() const { return showPercentText_; }

protected:
    /// Filter implicit attributes in serialization process.
    bool FilterImplicitAttributes(XMLElement &dest) const override;

    /// Update ProgressBar knob position & size.
    void UpdateProgressBar();

    /// ProgressBar knob.
    SharedPtr<BorderImage> knob_;
    /// ProgressBar text.
    SharedPtr<Text> loadingText_;
    /// Orientation.
    Orientation orientation_;
    /// ProgressBar text style.
    ea::string loadingPercentStyle_;
    /// ProgressBar range.
    float range_;
    /// ProgressBar current value.
    float value_;
    /// Flag to show the percent text.
    bool showPercentText_;
};

}
