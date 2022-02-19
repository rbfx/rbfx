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

#include "../UI/BorderImage.h"

namespace Urho3D
{

class Button;
class Slider;

/// Scroll bar %UI element with forward and back buttons.
class URHO3D_API ScrollBar : public BorderImage
{
    URHO3D_OBJECT(ScrollBar, BorderImage);

public:
    /// Construct.
    explicit ScrollBar(Context* context);
    /// Destruct.
    ~ScrollBar() override;
    /// Register object factory.
    /// @nobind
    static void RegisterObject(Context* context);

    /// Apply attribute changes that can not be applied immediately.
    void ApplyAttributes() override;
    /// React to resize.
    void OnResize(const IntVector2& newSize, const IntVector2& delta) override;
    /// React to editable status change.
    void OnSetEditable() override;

    /// Set orientation type.
    /// @property
    void SetOrientation(Orientation orientation);
    /// Set slider range maximum value (minimum value is always 0).
    /// @property
    void SetRange(float range);
    /// Set slider current value.
    /// @property
    void SetValue(float value);
    /// Change slider current value by a delta.
    void ChangeValue(float delta);
    /// Set button scroll step.
    /// @property
    void SetScrollStep(float step);
    /// Set button step factor, can be used to adjust the step for constant pixel size.
    /// @property
    void SetStepFactor(float factor);
    /// Scroll back one step.
    void StepBack();
    /// Scroll forward one step.
    void StepForward();

    /// Return scrollbar orientation.
    /// @property
    Orientation GetOrientation() const;
    /// Return slider range.
    /// @property
    float GetRange() const;
    /// Return slider current value.
    /// @property
    float GetValue() const;

    /// Return button scroll step.
    /// @property
    float GetScrollStep() const { return scrollStep_; }

    /// Return button step factor.
    /// @property
    float GetStepFactor() const { return stepFactor_; }

    /// Return scroll step multiplied by factor.
    /// @property
    float GetEffectiveScrollStep() const;

    /// Return back button element.
    /// @property
    Button* GetBackButton() const { return backButton_; }

    /// Return forward button element.
    /// @property
    Button* GetForwardButton() const { return forwardButton_; }

    /// Return slider element.
    /// @property
    Slider* GetSlider() const { return slider_; }

protected:
    /// Filter implicit attributes in serialization process.
    bool FilterImplicitAttributes(XMLElement& dest) const override;
    /// Filter implicit attributes in serialization process for internal button.
    bool FilterButtonImplicitAttributes(XMLElement& dest, const ea::string& name) const;

    /// Back button.
    SharedPtr<Button> backButton_;
    /// Forward button.
    SharedPtr<Button> forwardButton_;
    /// Slider.
    SharedPtr<Slider> slider_;
    /// Scroll step.
    float scrollStep_;
    /// Step factor.
    float stepFactor_;
    /// Left button image rect.
    IntRect leftRect_;
    /// Right button image rect.
    IntRect rightRect_;
    /// Up button image rect.
    IntRect upRect_;
    /// Down button image rect.
    IntRect downRect_;

private:
    /// Handle back button pressed.
    void HandleBackButtonPressed(StringHash eventType, VariantMap& eventData);
    /// Handle forward button pressed.
    void HandleForwardButtonPressed(StringHash eventType, VariantMap& eventData);
    /// Handle slider movement.
    void HandleSliderChanged(StringHash eventType, VariantMap& eventData);
    /// Handle slider touch and click on "paging" area.
    void HandleSliderPaged(StringHash eventType, VariantMap& eventData);
};

}
