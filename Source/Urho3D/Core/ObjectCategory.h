// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../Container/ConstString.h"

namespace Urho3D
{

/// Categories for built-in components.
/// @{
URHO3D_GLOBAL_CONSTANT(ConstString Category_Audio{"Component/Audio"});
URHO3D_GLOBAL_CONSTANT(ConstString Category_Geometry{"Component/Geometry"});
URHO3D_GLOBAL_CONSTANT(ConstString Category_IK{"Component/Inverse Kinematics"});
URHO3D_GLOBAL_CONSTANT(ConstString Category_Logic{"Component/Logic"});
URHO3D_GLOBAL_CONSTANT(ConstString Category_Navigation{"Component/Navigation"});
URHO3D_GLOBAL_CONSTANT(ConstString Category_Network{"Component/Network"});
URHO3D_GLOBAL_CONSTANT(ConstString Category_Physics{"Component/Physics"});
URHO3D_GLOBAL_CONSTANT(ConstString Category_Physics2D{"Component/Physics2D"});
URHO3D_GLOBAL_CONSTANT(ConstString Category_RmlUI{"Component/RmlUI"});
URHO3D_GLOBAL_CONSTANT(ConstString Category_Scene{"Component/Scene"});
URHO3D_GLOBAL_CONSTANT(ConstString Category_Subsystem{"Component/Subsystem"});
URHO3D_GLOBAL_CONSTANT(ConstString Category_Urho2D{"Component/Urho2D"});
/// @}

/// 'Plugin' and 'User' categories are displayed separately in Editor.
/// - 'Component/Plugin/PluginName' is intended for components registered by plugins.
/// - 'Component/User' is intended for application-specific components.
/// - Components that don't belong to 'Plugin' and 'User' categories are displayed together with built-in components.
/// @{
URHO3D_GLOBAL_CONSTANT(ConstString Category_Plugin{"Component/Plugin"});
URHO3D_GLOBAL_CONSTANT(ConstString Category_User{"Component/User"});
/// @}

/// Category for objects derived from RenderPass.
URHO3D_GLOBAL_CONSTANT(ConstString Category_RenderPass{"RenderPass"});
/// Category for objects derived from AssetTransformer.
URHO3D_GLOBAL_CONSTANT(ConstString Category_Transformer{"Transformer"});
/// Category for objects derived from UIElement.
URHO3D_GLOBAL_CONSTANT(ConstString Category_UI{"UI"});

}
