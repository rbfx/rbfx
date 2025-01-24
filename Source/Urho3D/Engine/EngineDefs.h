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

#include "../Container/ConstString.h"


namespace Urho3D
{

/// Engine parameters sorted by name.
/// Keep the list sorted when adding new elements.
/// TODO: Rename to CamelCase
/// @{
URHO3D_GLOBAL_CONSTANT(ConstString EP_APPLICATION_NAME{"ApplicationName"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_APPLICATION_PREFERENCES_DIR{"ApplicationPreferencesDir"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_AUTOLOAD_PATHS{"AutoloadPaths"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_BORDERLESS{"Borderless"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_CONFIG_NAME{"ConfigName"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_DISCARD_SHADER_CACHE{"DiscardShaderCache"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_ENGINE_AUTO_LOAD_SCRIPTS{"EngineAutoLoadScripts"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_ENGINE_CLI_PARAMETERS{"EngineCliParameters"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_EXTERNAL_WINDOW{"ExternalWindow"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_FRAME_LIMITER{"FrameLimiter"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_FULL_SCREEN{"FullScreen"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_GPU_DEBUG{"GPUDebug"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_HEADLESS{"Headless"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_LOAD_FONTS{"LoadFonts"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_LOG_LEVEL{"LogLevel"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_LOG_NAME{"LogName"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_LOG_QUIET{"LogQuiet"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_MAIN_PLUGIN{"MainPlugin"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_MONITOR{"Monitor"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_MULTI_SAMPLE{"MultiSample"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_ORGANIZATION_NAME{"OrganizationName"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_ORIENTATIONS{"Orientations"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_PACKAGE_CACHE_DIR{"PackageCacheDir"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_PLUGINS{"Plugins"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_RENAME_PLUGINS{"RenamePlugins"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_REFRESH_RATE{"RefreshRate"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_RESOURCE_PACKAGES{"ResourcePackages"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_RESOURCE_PATHS{"ResourcePaths"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_RESOURCE_PREFIX_PATHS{"ResourcePrefixPaths"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_RESOURCE_ROOT_FILE{"ResourceRootFile"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_SAVE_SHADER_CACHE{"SaveShaderCache"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_SHADER_CACHE_DIR{"ShaderCacheDir"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_SHADER_LOG_SOURCES{"ShaderLogSource"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_SHADER_POLICY{"ShaderPolicy"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_SOUND_BUFFER{"SoundBuffer"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_SOUND_INTERPOLATION{"SoundInterpolation"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_SOUND_MIX_RATE{"SoundMixRate"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_SOUND_MODE{"SoundMode"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_SOUND{"Sound"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_SYSTEMUI_FLAGS{"SystemUIFlags"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_TEXTURE_ANISOTROPY{"TextureAnisotropy"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_TEXTURE_FILTER_MODE{"TextureFilterMode"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_TEXTURE_QUALITY{"TextureQuality"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_TIME_OUT{"TimeOut"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_TOUCH_EMULATION{"TouchEmulation"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_TWEAK_D3D12{"TweakD3D12"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_TWEAK_VULKAN{"TweakVulkan"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_VALIDATE_SHADERS{"ValidateShaders"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_VSYNC{"VSync"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_WINDOW_HEIGHT{"WindowHeight"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_WINDOW_ICON{"WindowIcon"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_WINDOW_MAXIMIZE{"WindowMaximize"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_WINDOW_POSITION_X{"WindowPositionX"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_WINDOW_POSITION_Y{"WindowPositionY"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_WINDOW_RESIZABLE{"WindowResizable"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_WINDOW_TITLE{"WindowTitle"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_WINDOW_WIDTH{"WindowWidth"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_WORKER_THREADS{"WorkerThreads"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_PSO_CACHE{"PsoCacheDir"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_RENDER_BACKEND{"RenderBackend"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_RENDER_ADAPTER_ID{"RenderAdapterId"});
URHO3D_GLOBAL_CONSTANT(ConstString EP_XR{"XR"});
/// @}

/// Global parameters for user code which go as Engine parameters as convenience.
/// @{
URHO3D_GLOBAL_CONSTANT(ConstString Param_SceneName{"SceneName"});
URHO3D_GLOBAL_CONSTANT(ConstString Param_ScenePosition{"ScenePosition"});
URHO3D_GLOBAL_CONSTANT(ConstString Param_SceneRotation{"SceneRotation"});
/// @}

}
