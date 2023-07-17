//
// Copyright (c) 2023-2023 the rbfx project.
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

#include "Input.h"
#include "Urho3D/Input/InputConstants.h"
#include "Urho3D/Resource/Resource.h"

#include <EASTL/fixed_vector.h>

namespace Urho3D
{
namespace Detail
{
/// Helper class to translate keyboard keys.
struct URHO3D_API KeyboardKeyMapping
{
    KeyboardKeyMapping();
    /// Construct.
    KeyboardKeyMapping(Scancode scancode);
    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive);

    Scancode scancode_{};
};

/// Helper class to translate controller buttons.
struct URHO3D_API ControllerButtonMapping
{
    ControllerButtonMapping();
    /// Construct.
    ControllerButtonMapping(ControllerButton controllerButton);
    /// Construct.
    ControllerButtonMapping(unsigned buttonIndex);

    /// Get controller button
    ControllerButton GetControllerButton() const { return static_cast<ControllerButton>(button_); }
    /// Get generic button
    unsigned GetGenericButton() const { return button_; }

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive);

    bool controller_{};
    unsigned button_{};
};

/// Helper class to translate controller axis.
struct URHO3D_API ControllerAxisMapping
{
    ControllerAxisMapping();
    ControllerAxisMapping(ControllerAxis controllerAxis, float neutral = 0.0f, float pressed = 1.0f);
    ControllerAxisMapping(unsigned axisIndex, float neutral = 0.0f, float pressed = 1.0f);

    /// Get controller button
    ControllerAxis GetControllerAxis() const { return static_cast<ControllerAxis>(axis_); }
    /// Get generic button
    unsigned GetGenericAxis() const { return axis_; }

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive);
    /// Check if the mapping overlaps with the other one, including matching axis index and controller type.
    bool OverlapsWith(const ControllerAxisMapping& mapping) const;
    /// Translate axis value into 0..1 range. Returns 0 if the value is outside of the range.
    float Translate(float pos, float deadZone) const;

    bool controller_{};
    unsigned axis_{};
    float neutral_;
    float pressed_;
};

/// Helper class to translate controller hat position.
struct URHO3D_API ControllerHatMapping
{
    ControllerHatMapping();
    ControllerHatMapping(unsigned hatPosition);

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive);

    unsigned hatPosition_{};
};

/// Helper class to translate mouse button.
struct URHO3D_API MouseButtonMapping
{
    MouseButtonMapping();
    MouseButtonMapping(unsigned mouseButton);

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive);

    MouseButtonFlags GetMask() const;

    unsigned mouseButton_{};
};
/// Helper class to translate UI button to input value.
struct URHO3D_API ScreenButtonMapping
{
    ScreenButtonMapping();
    ScreenButtonMapping(const ea::string& elementName);

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive);

    ea::string elementName_{};
};


struct URHO3D_API ActionMapping
{
    ea::fixed_vector<KeyboardKeyMapping, 2> keyboardKeys_;
    ea::fixed_vector<MouseButtonMapping, 1> mouseButtons_;
    ea::fixed_vector<ControllerButtonMapping, 2> controllerButtons_;
    ea::fixed_vector<ControllerAxisMapping, 1> controllerAxes_;
    ea::fixed_vector<ControllerHatMapping, 1> controllerHats_;
    ea::fixed_vector<ScreenButtonMapping, 1> screenButtons_;

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive);
    /// Evaluate action state based on current input.
    float Evaluate(Input* input, UI* ui, float deadZone, int ignoreJoystickId) const;
};
}


class URHO3D_API InputMap : public SimpleResource
{
    URHO3D_OBJECT(InputMap, SimpleResource);

public:
    const float DEFAULT_DEADZONE{0.1f};

    /// Map from string to action mapping. Cache string hashes.
    using StringActionMappingMap = ea::unordered_map<ea::string, Detail::ActionMapping, ea::hash<ea::string>, ea::equal_to<ea::string>,
        EASTLAllocatorType, true>;


    /// Construct.
    explicit InputMap(Context* context);
    ~InputMap() override;

    /// Register object factory and attributes.
    static void RegisterObject(Context* context);

    /// Force override of SerializeInBlock.
    void SerializeInBlock(Archive& archive) override;

    /// Set dead zone half-width to mitigate axis drift.
    void SetDeadZone(float deadZone);
    /// Get dead zone half-width.
    float GetDeadZone() const { return deadZone_; }


    /// Map keyboard key to the action.
    void MapKeyboardKey(const ea::string& action, Scancode scancode);

    /// Map controller button to the action.
    void MapControllerButton(const ea::string& action, ControllerButton button);

    /// Map generic joystick button to the action.
    void MapJoystickButton(const ea::string& action, unsigned buttonIndex);

    /// Map controller axis to the action.
    void MapControllerAxis(const ea::string& action, ControllerAxis axis, float neutral, float pressed);

    /// Map generic joystick axis to the action.
    void MapJoystickAxis(const ea::string& action, unsigned axis, float neutral, float pressed);

    /// Map controller hat to the action.
    void MapHat(const ea::string& action, HatPosition hatPosition);

    /// Map mouse button to the action.
    void MapMouseButton(const ea::string& action, MouseButton mouseButton);

    /// Map screen button to the action.
    void MapScreenButton(const ea::string& action, const ea::string& elementName);

    /// Get mapping for the action.
    const Detail::ActionMapping& GetMapping(const ea::string& action) const;
    /// Get mapping for the action.
    const Detail::ActionMapping& GetMappingByHash(StringHash actionHash) const;

    /// Add new metadata variable or overwrite old value.
    void AddMetadata(const ea::string& name, const Variant& value);
    /// Remove metadata variable.
    void RemoveMetadata(const ea::string& name);
    /// Remove all metadata variables.
    void RemoveAllMetadata();
    /// Return metadata variable.
    const Variant& GetMetadata(const ea::string& name) const;
    /// Return whether the resource has metadata.
    bool HasMetadata() const;

    /// Get all action mappings.
    const StringActionMappingMap& GetMappings() const { return actions_; }

    /// Evaluate action state.
    float Evaluate(const ea::string& action);
    /// Evaluate action state by action string hash.
    float EvaluateByHash(StringHash actionHash);

    /// Get scancode names.
    static ea::span<ea::string_view> GetScanCodeNames();
    /// Get controller button names.
    static ea::span<ea::string_view> GetControllerButtonNames();
    /// Get controller directional pad (hat) names.
    static ea::span<ea::string_view> GetControllerHatNames();
    /// Get mouse button names.
    static ea::span<ea::string_view> GetMouseButtonNames();

    /// Load input map from config or resources.
    static SharedPtr<InputMap> Load(Context* context, const ea::string& name);

private:
    Detail::ActionMapping& GetOrAddMapping(const ea::string& action);

    /// All mapped actions.
    StringActionMappingMap actions_;
    /// Extra data: sensitivity, controller axis dead zone, etc.
    StringVariantMap metadata_;
    /// Axis dead zone.
    float deadZone_{0.1f};
    /// Joystick to ignore (SDL gyroscope virtual joystick)
    int ignoreJoystickId_{-1};
};

} // namespace Urho3D
