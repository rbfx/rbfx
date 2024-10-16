# Diligent Core [![Tweet](https://img.shields.io/twitter/url/http/shields.io.svg?style=social)](https://twitter.com/intent/tweet?text=An%20easy-to-use%20cross-platform%20graphics%20library%20that%20takes%20full%20advantage%20of%20%23Direct3D12%20and%20%23VulkanAPI&url=https://github.com/DiligentGraphics/DiligentEngine) <img src="media/diligentgraphics-logo.png" height=64 align="right" valign="middle">

Diligent Core is a modern cross-platfrom low-level graphics API that makes the foundation of the [Diligent Engine](https://github.com/DiligentGraphics/DiligentEngine).
The module implements Direct3D11, Direct3D12, OpenGL, OpenGLES, and Vulkan rendering backends (Metal implementation is available for commercial clients),
as well as basic platform-specific utilities. It is self-contained and can be built by its own.
Please refer to the [main repository](https://github.com/DiligentGraphics/DiligentEngine) for information about the supported platforms and features,
build instructions, etc.

| Platform             | Build Status  |
| ---------------------| ------------- |
|<img src="media/windows-logo.png" width=24 valign="middle"> Win32               | [![Build Status](https://github.com/DiligentGraphics/DiligentCore/actions/workflows/build-windows.yml/badge.svg?branch=master)](https://github.com/DiligentGraphics/DiligentCore/actions/workflows/build-windows.yml?query=branch%3Amaster) |
|<img src="media/uwindows-logo.png" width=24 valign="middle"> Universal Windows  | [![Build Status](https://github.com/DiligentGraphics/DiligentCore/actions/workflows/build-windows.yml/badge.svg?branch=master)](https://github.com/DiligentGraphics/DiligentCore/actions/workflows/build-windows.yml?query=branch%3Amaster) |
|<img src="media/linux-logo.png" width=24 valign="middle"> Linux                 | [![Build Status](https://github.com/DiligentGraphics/DiligentCore/actions/workflows/build-linux.yml/badge.svg?branch=master)](https://github.com/DiligentGraphics/DiligentCore/actions/workflows/build-linux.yml?query=branch%3Amaster) |
|<img src="media/android-logo.png" width=24 valign="middle"> Android             | [![Build Status](https://github.com/DiligentGraphics/DiligentCore/actions/workflows/build-android.yml/badge.svg?branch=master)](https://github.com/DiligentGraphics/DiligentCore/actions/workflows/build-android.yml?query=branch%3Amaster) |
|<img src="media/macos-logo.png" width=24 valign="middle"> MacOS                 | [![Build Status](https://github.com/DiligentGraphics/DiligentCore/actions/workflows/build-apple.yml/badge.svg?branch=master)](https://github.com/DiligentGraphics/DiligentCore/actions/workflows/build-apple.yml?query=branch%3Amaster) |
|<img src="media/apple-logo.png" width=24 valign="middle"> iOS                   | [![Build Status](https://github.com/DiligentGraphics/DiligentCore/actions/workflows/build-apple.yml/badge.svg?branch=master)](https://github.com/DiligentGraphics/DiligentCore/actions/workflows/build-apple.yml?query=branch%3Amaster) |
|<img src="media/tvos-logo.png" width=24 valign="middle"> tvOS                   | [![Build Status](https://github.com/DiligentGraphics/DiligentCore/actions/workflows/build-apple.yml/badge.svg?branch=master)](https://github.com/DiligentGraphics/DiligentCore/actions/workflows/build-apple.yml?query=branch%3Amaster) |
|<img src="media/emscripten-logo.png" width=24 valign="middle"> Emscripten       | [![Build Status](https://github.com/DiligentGraphics/DiligentCore/actions/workflows/build-emscripten.yml/badge.svg?branch=master)](https://github.com/DiligentGraphics/DiligentCore/actions/workflows/build-emscripten.yml?query=branch%3Amaster) | 


[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](License.txt)
[![Chat on Discord](https://img.shields.io/discord/730091778081947680?logo=discord)](https://discord.gg/t7HGBK7)
[![Appveyor Build Status](https://ci.appveyor.com/api/projects/status/github/DiligentGraphics/DiligentCore?svg=true)](https://ci.appveyor.com/project/DiligentGraphics/diligentcore)
[![CodeQL Scanning](https://github.com/DiligentGraphics/DiligentCore/actions/workflows/codeql.yml/badge.svg?branch=master)](https://github.com/DiligentGraphics/DiligentCore/actions/workflows/codeql.yml?query=branch%3Amaster)
[![MSVC Analysis](https://github.com/DiligentGraphics/DiligentCore/actions/workflows/msvc_analysis.yml/badge.svg?branch=master)](https://github.com/DiligentGraphics/DiligentCore/actions/workflows/msvc_analysis.yml?query=branch%3Amaster)
[![Lines of code](https://sloc.xyz/github/DiligentGraphics/DiligentCore)](https://github.com/DiligentGraphics/DiligentCore)


# Table of Contents

- [Cloning the Repository](#cloning)
- [API Basics](#api_basics)
  - [Initializing the Engine](#initialization)
    - [Win32](#initialization_win32)
    - [Universal Windows Platform](#initialization_uwp)
    - [Linux](#initialization_linux)
    - [MacOS](#initialization_macos)
    - [Android](#initialization_android)
    - [iOS](#initialization_ios)
    - [Emscripten](#initialization_emscripten)
    - [Destroying the Engine](#initialization_destroying)
  - [Creating Resources](#creating_resources)
  - [Creating Shaders](#creating_shaders)
  - [Initializing Pipeline State](#initializing_pso)
    - [Pipeline Resource Layout](#pipeline_resource_layout)
  - [Binding Shader Resources](#binding_resources)
  - [Setting the Pipeline State and Invoking Draw Command](#draw_command)
- [Low-level API interoperability](#low_level_api_interoperability)
- [NuGet package build instructions](#nuget_build_instructions)
- [License](#license)
- [Contributing](#contributing)
- [Release History](#release_history)


<a name="cloning"></a>
# Cloning the Repository

To get the repository and all submodules, use the following command:

```
git clone --recursive https://github.com/DiligentGraphics/DiligentCore.git
```

To build the module, see 
[build instructions](https://github.com/DiligentGraphics/DiligentEngine/blob/master/README.md#build-and-run-instructions) 
in the master repository.

<a name="api_basics"></a>
# API Basics

<a name="initialization"></a>
## Initializing the Engine

Before you can use any functionality provided by the engine, you need to create a render device, an immediate context and a swap chain.

<a name="initialization_win32"></a>
### Win32
On Win32 platform, you can create OpenGL, Direct3D11, Direct3D12 or Vulkan device as shown below:

```cpp
void InitializeDiligentEngine(HWND NativeWindowHandle)
{
    SwapChainDesc SCDesc;
    // RefCntAutoPtr<IRenderDevice>  m_pDevice;
    // RefCntAutoPtr<IDeviceContext> m_pImmediateContext;
    // RefCntAutoPtr<ISwapChain>     m_pSwapChain;
    switch (m_DeviceType)
    {
        case RENDER_DEVICE_TYPE_D3D11:
        {
            EngineD3D11CreateInfo EngineCI;
#    if ENGINE_DLL
            // Load the dll and import GetEngineFactoryD3D11() function
            auto* GetEngineFactoryD3D11 = LoadGraphicsEngineD3D11();
#    endif
            auto* pFactoryD3D11 = GetEngineFactoryD3D11();
            pFactoryD3D11->CreateDeviceAndContextsD3D11(EngineCI, &m_pDevice, &m_pImmediateContext);
            Win32NativeWindow Window{hWnd};
            pFactoryD3D11->CreateSwapChainD3D11(m_pDevice, m_pImmediateContext, SCDesc,
                                                FullScreenModeDesc{}, Window, &m_pSwapChain);
        }
        break;

        case RENDER_DEVICE_TYPE_D3D12:
        {
#    if ENGINE_DLL
            // Load the dll and import GetEngineFactoryD3D12() function
            auto GetEngineFactoryD3D12 = LoadGraphicsEngineD3D12();
#    endif
            EngineD3D12CreateInfo EngineCI;

            auto* pFactoryD3D12 = GetEngineFactoryD3D12();
            pFactoryD3D12->CreateDeviceAndContextsD3D12(EngineCI, &m_pDevice, &m_pImmediateContext);
            Win32NativeWindow Window{hWnd};
            pFactoryD3D12->CreateSwapChainD3D12(m_pDevice, m_pImmediateContext, SCDesc,
                                                FullScreenModeDesc{}, Window, &m_pSwapChain);
        }
        break;

    case RENDER_DEVICE_TYPE_GL:
    {
#    if EXPLICITLY_LOAD_ENGINE_GL_DLL
        // Load the dll and import GetEngineFactoryOpenGL() function
        auto GetEngineFactoryOpenGL = LoadGraphicsEngineOpenGL();
#    endif
        auto* pFactoryOpenGL = GetEngineFactoryOpenGL();

        EngineGLCreateInfo EngineCI;
        EngineCI.Window.hWnd = hWnd;

        pFactoryOpenGL->CreateDeviceAndSwapChainGL(EngineCI, &m_pDevice, &m_pImmediateContext,
                                                   SCDesc, &m_pSwapChain);
    }
    break;

    case RENDER_DEVICE_TYPE_VULKAN:
    {
#    if EXPLICITLY_LOAD_ENGINE_VK_DLL
        // Load the dll and import GetEngineFactoryVk() function
        auto GetEngineFactoryVk = LoadGraphicsEngineVk();
#    endif
        EngineVkCreateInfo EngineCI;

        auto* pFactoryVk = GetEngineFactoryVk();
        pFactoryVk->CreateDeviceAndContextsVk(EngineCI, &m_pDevice, &m_pImmediateContext);
        Win32NativeWindow Window{hWnd};
        pFactoryVk->CreateSwapChainVk(m_pDevice, m_pImmediateContext, SCDesc, Window, &m_pSwapChain);
    }
    break;

    default:
        std::cerr << "Unknown device type";
    }
}
```

On Windows, the engine can be statically linked to the application or built as a separate DLL. In the first case,
factory functions `GetEngineFactoryOpenGL()`, `GetEngineFactoryD3D11()`, `GetEngineFactoryD3D12()`, and `GetEngineFactoryVk()`
can be called directly. In the second case, you need to load the DLL into the process's address space using `LoadGraphicsEngineOpenGL()`,
`LoadGraphicsEngineD3D11()`, `LoadGraphicsEngineD3D12()`, or `LoadGraphicsEngineVk()` function. Each function loads appropriate
dynamic library and imports the functions required to initialize the engine. You need to include the following headers:

```cpp
#include "EngineFactoryD3D11.h"
#include "EngineFactoryD3D12.h"
#include "EngineFactoryOpenGL.h"
#include "EngineFactoryVk.h"

```

You also need to add the following directories to the include search paths:

* `DiligentCore/Graphics/GraphicsEngineD3D11/interface`
* `DiligentCore/Graphics/GraphicsEngineD3D12/interface`
* `DiligentCore/Graphics/GraphicsEngineOpenGL/interface`
* `DiligentCore/Graphics/GraphicsEngineVulkan/interface`

As an alternative, you may only add the path to the root folder and
then use include paths relative to it.

Enable `Diligent` namespace:

```cpp
using namespace Diligent;
```

`IEngineFactoryD3D11::CreateDeviceAndContextsD3D11()`, `IEngineFactoryD3D12::CreateDeviceAndContextsD3D12()`, and 
`IEngineFactoryVk::CreateDeviceAndContextsVk()` functions can also create a specified number of immediate and deferred contexts,
which can be used for asynchronous rendering and multi-threaded command recording. The contexts may only be created during
the initialization of the engine. The function populates an array of pointers to the contexts, where the immediates contexts go first,
followed by all deferred contexts.

For more details, take a look at 
[Tutorial00_HelloWin32.cpp](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial00_HelloWin32/src/Tutorial00_HelloWin32.cpp) 
file.

<a name="initialization_uwp"></a>
### Universal Windows Platform

On Universal Windows Platform, you can create Direct3D11 or Direct3D12 device. Initialization is performed the same
way as on Win32 Platform. The difference is that you first create the render device and device contexts by
calling `IEngineFactoryD3D11::CreateDeviceAndContextsD3D11()` or `IEngineFactoryD3D12::CreateDeviceAndContextsD3D12()`.
The swap chain is created later by a call to `IEngineFactoryD3D11::CreateSwapChainD3D11()` or `IEngineFactoryD3D12::CreateSwapChainD3D12()`.
Please look at
[SampleAppUWP.cpp](https://github.com/DiligentGraphics/DiligentSamples/blob/master/SampleBase/src/UWP/SampleAppUWP.cpp) 
file for more details.

<a name="initialization_linux"></a>
### Linux

On Linux platform, the engine supports OpenGL and Vulkan backends. Initialization of GL context on Linux is tightly
coupled with window creation. As a result, Diligent Engine does not initialize the context, but
attaches to the one initialized by the app. An example of the engine initialization on Linux can be found in
[Tutorial00_HelloLinux.cpp](https://github.com/DiligentGraphics/DiligentSamples/blob/master/Tutorials/Tutorial00_HelloLinux/src/Tutorial00_HelloLinux.cpp).

<a name="initialization_macos"></a>
### MacOS

On MacOS, Diligent Engine supports OpenGL, Vulkan and Metal backends. Initialization of GL context on MacOS is
performed by the application, and the engine attaches to the context created by the app; see
[GLView.mm](https://github.com/DiligentGraphics/DiligentTools/blob/master/NativeApp/Apple/Source/Classes/OSX/GLView.mm)
for details. Vulkan backend is initialized similar to other platforms. See 
[MetalView.mm](https://github.com/DiligentGraphics/DiligentTools/blob/master/NativeApp/Apple/Source/Classes/OSX/MetalView.mm).

<a name="initialization_android"></a>
### Android

On Android, you can create OpenGLES or Vulkan device. The following code snippet shows an example:

```cpp
auto* pFactoryOpenGL = GetEngineFactoryOpenGL();
EngineGLCreateInfo EngineCI;
EngineCI.Window.pAWindow = NativeWindowHandle;
pFactoryOpenGL->CreateDeviceAndSwapChainGL(
    EngineCI, &m_pDevice, &m_pContext, SCDesc, &m_pSwapChain);
```

If the engine is built as dynamic library, the library needs to be loaded by the native activity. The following code shows one possible way:

```java
static
{
    try{
        System.loadLibrary("GraphicsEngineOpenGL");
    } catch (UnsatisfiedLinkError e) {
        Log.e("native-activity", "Failed to load GraphicsEngineOpenGL library.\n" + e);
    }
}
```

<a name="initialization_ios"></a>
### iOS

iOS implementation supports OpenGLES, Vulkan and Metal backend. Initialization of GL context on iOS is
performed by the application, and the engine attaches to the context initialized by the app; see
[EAGLView.mm](https://github.com/DiligentGraphics/DiligentTools/blob/master/NativeApp/Apple/Source/Classes/iOS/EAGLView.mm)
for details.

<a name="initialization_emscripten"></a>
### Emscripten

On Emscripten, you can create OpenGLES device. The following code snippet shows an example:
```cpp
//You need to pass the id of the canvas to NativeWindow
auto* pFactoryOpenGL = GetEngineFactoryOpenGL();
EngineGLCreateInfo EngineCI = {};
EngineCI.Window = NativeWindow{"#canvas"};
pFactoryOpenGL->CreateDeviceAndSwapChainGL(EngineCI, &m_pDevice, &m_pContext, SCDesc, &m_pSwapChain);
```
If you are using SDL or GLFW with existing context, you can provide null as the native window handle:
`EngineCI.Window = NativeWindow{nullptr}`

<a name="initialization_destroying"></a>
### Destroying the Engine

The engine performs automatic reference counting and shuts down when the last reference to an engine object is released.

<a name="creating_resources"></a>
## Creating Resources

Device resources are created by the render device. The two main resource types are buffers,
which represent linear memory, and textures, which use memory layouts optimized for fast filtering.
To create a buffer, you need to populate `BufferDesc` structure and call `IRenderDevice::CreateBuffer()`.
The following code creates a uniform (constant) buffer:

```cpp
BufferDesc BuffDesc;
BuffDesc.Name           = "Uniform buffer";
BuffDesc.BindFlags      = BIND_UNIFORM_BUFFER;
BuffDesc.Usage          = USAGE_DYNAMIC;
BuffDesc.uiSizeInBytes  = sizeof(ShaderConstants);
BuffDesc.CPUAccessFlags = CPU_ACCESS_WRITE;
m_pDevice->CreateBuffer(BuffDesc, nullptr, &m_pConstantBuffer);
```

Similar, to create a texture, populate `TextureDesc` structure and call `IRenderDevice::CreateTexture()` as in the following example:

```cpp
TextureDesc TexDesc;
TexDesc.Name      = "My texture 2D";
TexDesc.Type      = TEXTURE_TYPE_2D;
TexDesc.Width     = 1024;
TexDesc.Height    = 1024;
TexDesc.Format    = TEX_FORMAT_RGBA8_UNORM;
TexDesc.Usage     = USAGE_DEFAULT;
TexDesc.BindFlags = BIND_SHADER_RESOURCE | BIND_RENDER_TARGET | BIND_UNORDERED_ACCESS;
TexDesc.Name      = "Sample 2D Texture";
m_pRenderDevice->CreateTexture(TexDesc, nullptr, &m_pTestTex);
```

There is only one function `CreateTexture()` that is capable of creating all types of textures. Type, format,
array size and all other parameters are specified by the members of the `TextureDesc` structure.

For every bind flag specified during the texture creation time, the texture object creates a default view.
Default shader resource view addresses the entire texture, default render target and depth stencil views reference
all array slices in the most detailed mip level, and unordered access view references the entire texture. To get a
default view from the texture, use `ITexture::GetDefaultView()` function. Note that this function does not increment
the reference counter of the returned interface. You can create additional texture views using `ITexture::CreateView()`.
Use `IBuffer::CreateView()` to create additional views of a buffer.

<a name="creating_shaders"></a>
## Creating Shaders

To create a shader, populate `ShaderCreateInfo` structure:

```cpp
ShaderCreateInfo ShaderCI;
```

There are three ways to create a shader. The first way is to provide a pointer to the shader source code through 
`ShaderCreateInfo::Source` member. The second way is to provide a file name. The third way is to provide a pointer
to the compiled byte code through `ShaderCreateInfo::ByteCode` member. Graphics Engine is entirely decoupled
from the platform. Since the host file system is platform-dependent, the structure exposes
`ShaderCreateInfo::pShaderSourceStreamFactory` member that is intended to give the engine access to the file system.
If you provided the source file name, you must also provide a non-null pointer to the shader source stream factory.
If the shader source contains any `#include` directives, the source stream factory will also be used to load these
files. The engine provides default implementation for every supported platform that should be sufficient in most cases.
You can however define your own implementation.

An important member is `ShaderCreateInfo::SourceLanguage`. The following are valid values for this member:

* `SHADER_SOURCE_LANGUAGE_DEFAULT` - The shader source format matches the underlying graphics API: HLSL for D3D11 or D3D12 mode, and GLSL for OpenGL, OpenGLES, and Vulkan modes.
* `SHADER_SOURCE_LANGUAGE_HLSL`    - The shader source is in HLSL. For OpenGL and OpenGLES modes, the source code will be 
                                     converted to GLSL. In Vulkan back-end, the code will be compiled to SPIRV directly.
* `SHADER_SOURCE_LANGUAGE_GLSL`    - The shader source is in GLSL.
* `SHADER_SOURCE_LANGUAGE_GLSL_VERBATIM` - The shader source language is GLSL and should be compiled verbatim.
* `SHADER_SOURCE_LANGUAGE_MSL`     - The source language is Metal Shading Language.

Other members of the `ShaderCreateInfo` structure define the shader include search directories, shader macro definitions,
shader entry point and other parameters.

```cpp
ShaderMacroHelper Macros;
Macros.AddShaderMacro("USE_SHADOWS", 1);
Macros.AddShaderMacro("NUM_SHADOW_SAMPLES", 4);
Macros.Finalize();
ShaderCI.Macros = Macros;
```

When everything is ready, call `IRenderDevice::CreateShader()` to create the shader object:

```cpp
ShaderCreateInfo ShaderCI;
ShaderCI.Desc.Name         = "MyPixelShader";
ShaderCI.FilePath          = "MyShaderFile.fx";
ShaderCI.EntryPoint        = "MyPixelShader";
ShaderCI.Desc.ShaderType   = SHADER_TYPE_PIXEL;
ShaderCI.SourceLanguage    = SHADER_SOURCE_LANGUAGE_HLSL;
const auto* SearchDirectories = "shaders;shaders\\inc;";
RefCntAutoPtr<IShaderSourceInputStreamFactory> pShaderSourceFactory;
m_pEngineFactory->CreateDefaultShaderSourceStreamFactory(SearchDirectories, &pShaderSourceFactory);
ShaderCI.pShaderSourceStreamFactory = pShaderSourceFactory;
RefCntAutoPtr<IShader> pShader;
m_pDevice->CreateShader(ShaderCI, &pShader);
```

<a name="initializing_pso"></a>
## Initializing Pipeline State

Diligent Engine follows Direct3D12/Vulkan style to configure the graphics/compute pipeline. One monolithic Pipelines State Object (PSO)
encompasses all required states (all shader stages, input layout description, depth stencil, rasterizer and blend state
descriptions etc.). To create a graphics pipeline state object, define an instance of `GraphicsPipelineStateCreateInfo` structure:

```cpp
GraphicsPipelineStateCreateInfo PSOCreateInfo;
PipelineStateDesc&              PSODesc = PSOCreateInfo.PSODesc;

PSODesc.Name = "My pipeline state";
```

Describe the pipeline specifics such as the number and format of render targets as well as depth-stencil format:

```cpp
// This is a graphics pipeline
PSODesc.PipelineType                            = PIPELINE_TYPE_GRAPHICS;
PSOCreateInfo.GraphicsPipeline.NumRenderTargets = 1;
PSOCreateInfo.GraphicsPipeline.RTVFormats[0]    = TEX_FORMAT_RGBA8_UNORM_SRGB;
PSOCreateInfo.GraphicsPipeline.DSVFormat        = TEX_FORMAT_D32_FLOAT;
```

Initialize depth-stencil state description `DepthStencilStateDesc`. Note that the constructor initializes
the members with default values and you may only set the ones that are different from default.

```cpp
// Init depth-stencil state
DepthStencilStateDesc& DepthStencilDesc = PSOCreateInfo.GraphicsPipeline.DepthStencilDesc;
DepthStencilDesc.DepthEnable            = true;
DepthStencilDesc.DepthWriteEnable       = true;
```

Initialize blend state description `BlendStateDesc`:

```cpp
// Init blend state
BlendStateDesc& BSDesc = PSOCreateInfo.GraphicsPipeline.BlendDesc;
BSDesc.IndependentBlendEnable = False;
auto &RT0 = BSDesc.RenderTargets[0];
RT0.BlendEnable           = True;
RT0.RenderTargetWriteMask = COLOR_MASK_ALL;
RT0.SrcBlend              = BLEND_FACTOR_SRC_ALPHA;
RT0.DestBlend             = BLEND_FACTOR_INV_SRC_ALPHA;
RT0.BlendOp               = BLEND_OPERATION_ADD;
RT0.SrcBlendAlpha         = BLEND_FACTOR_SRC_ALPHA;
RT0.DestBlendAlpha        = BLEND_FACTOR_INV_SRC_ALPHA;
RT0.BlendOpAlpha          = BLEND_OPERATION_ADD;
```

Initialize rasterizer state description `RasterizerStateDesc`:

```cpp
// Init rasterizer state
RasterizerStateDesc& RasterizerDesc = PSOCreateInfo.GraphicsPipeline.RasterizerDesc;
RasterizerDesc.FillMode              = FILL_MODE_SOLID;
RasterizerDesc.CullMode              = CULL_MODE_NONE;
RasterizerDesc.FrontCounterClockwise = True;
RasterizerDesc.ScissorEnable         = True;
RasterizerDesc.AntialiasedLineEnable = False;
```

Initialize input layout description `InputLayoutDesc`:

```cpp
// Define input layout
InputLayoutDesc& Layout = PSOCreateInfo.GraphicsPipeline.InputLayout;
LayoutElement LayoutElems[] =
{
    LayoutElement( 0, 0, 3, VT_FLOAT32, False ),
    LayoutElement( 1, 0, 4, VT_UINT8,   True ),
    LayoutElement( 2, 0, 2, VT_FLOAT32, False ),
};
Layout.LayoutElements = LayoutElems;
Layout.NumElements    = _countof(LayoutElems);
```

Define primitive topology and set shader pointers:

```cpp
// Define shader and primitive topology
PSOCreateInfo.GraphicsPipeline.PrimitiveTopology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
PSOCreateInfo.pVS = m_pVS;
PSOCreateInfo.pPS = m_pPS;
```

<a name="pipeline_resource_layout"></a>
### Pipeline Resource Layout

Pipeline resource layout informs the engine how the application is going to use different shader resource variables.
To allow grouping of resources based on the expected frequency of resource bindings changes, Diligent Engine introduces
classification of shader variables:

* **Static variables** (`SHADER_RESOURCE_VARIABLE_TYPE_STATIC`) are variables that are expected to be set only once.
  They may not be changed once a resource is bound to the variable. Such variables are intended to hold global constants such 
  as camera attributes or global light attributes constant buffers. Note that it is the *resource binding* that may not change,
  while the contents of the resource is allowed to change according to its usage.
* **Mutable variables** (`SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE`) define resources that are expected to change on a per-material frequency.
  Examples may include diffuse textures, normal maps etc. Again updates to the contents of the resource are orthogobal
  to the binding changes.
* **Dynamic variables** (`SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC`) are expected to change frequently and randomly.

To define variable types, prepare an array of `ShaderResourceVariableDesc` structures and
initialize `PSODesc.ResourceLayout.Variables` and `PSODesc.ResourceLayout.NumVariables` members. Also
`PSODesc.ResourceLayout.DefaultVariableType` can be used to set the type that will be used if a variable name is not provided.

```cpp
ShaderResourceVariableDesc ShaderVars[] =
{
    {SHADER_TYPE_PIXEL, "g_StaticTexture",  SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
    {SHADER_TYPE_PIXEL, "g_MutableTexture", SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
    {SHADER_TYPE_PIXEL, "g_DynamicTexture", SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
};
PSODesc.ResourceLayout.Variables           = ShaderVars;
PSODesc.ResourceLayout.NumVariables        = _countof(ShaderVars);
PSODesc.ResourceLayout.DefaultVariableType = SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
```

When creating a pipeline state, textures can be permanently assigned immutable samplers. If an immutable sampler is assigned to a texture,
it will always be used instead of the one initialized in the texture shader resource view. To define immutable samplers,
prepare an array of `ImmutableSamplerDesc` structures and initialize `PSODesc.ResourceLayout.ImmutableSamplers` and
`PSODesc.ResourceLayout.NumImmutableSamplers` members. Notice that immutable samplers can be assigned to a texture variable of any type,
not necessarily static, so that the texture binding can be changed at run-time, while the sampler will stay immutable.
It is highly recommended to use immutable samplers whenever possible.

```cpp
ImmutableSamplerDesc ImtblSampler;
ImtblSampler.ShaderStages   = SHADER_TYPE_PIXEL;
ImtblSampler.Desc.MinFilter = FILTER_TYPE_LINEAR;
ImtblSampler.Desc.MagFilter = FILTER_TYPE_LINEAR;
ImtblSampler.Desc.MipFilter = FILTER_TYPE_LINEAR;
ImtblSampler.TextureName    = "g_MutableTexture";
PSODesc.ResourceLayout.NumImmutableSamplers = 1;
PSODesc.ResourceLayout.ImmutableSamplers    = &ImtblSampler;
```

[This document](https://github.com/DiligentGraphics/DiligentCore/blob/master/doc/TextureSamplers.md) provides a detailed
information about working with texture samplers.

When all required fields of PSO description structure are set, call `IRenderDevice::CreateGraphicsPipelineState()`
to create the PSO object:

```cpp
m_pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &m_pPSO);
```

<a name="binding_resources"></a>
## Binding Shader Resources

As mentioned above, [shader resource binding in Diligent Engine](http://diligentgraphics.com/2016/03/23/resource-binding-model-in-diligent-engine-2-0/)
is based on grouping variables in 3 different groups (static, mutable and dynamic). Static variables are variables that are
expected to be set only once. They may not be changed once a resource is bound to the variable. Such variables are intended
to hold global constants such as camera attributes or global light attributes constant buffers. They are bound directly to the
Pipeline State Object:

```cpp
m_pPSO->GetStaticShaderVariable(SHADER_TYPE_PIXEL, "g_tex2DShadowMap")->Set(pShadowMapSRV);
```

Mutable and dynamic variables are bound via a new object called Shader Resource Binding (SRB), which is created by the pipeline state
(`IPipelineState::CreateShaderResourceBinding()`), or pipeline resource signature in advanced use cases:

```cpp
m_pPSO->CreateShaderResourceBinding(&m_pSRB, true);
```

The second parameter tells the system to initialize internal structures in the SRB object
that reference static variables in the PSO.

Dynamic and mutable resources are then bound through the SRB object:

```cpp
m_pSRB->GetVariable(SHADER_TYPE_PIXEL,  "tex2DDiffuse")->Set(pDiffuseTexSRV);
m_pSRB->GetVariable(SHADER_TYPE_VERTEX, "cbRandomAttribs")->Set(pRandomAttrsCB);
```

The difference between mutable and dynamic resources is that mutable resources can only be set once per instance
of a shader resource binding. Dynamic resources can be set multiple times. It is important to properly set the variable type as
this affects performance. Static and mutable variables are more efficient. Dynamic variables are more expensive
and introduce some run-time overhead.

An alternative way to bind shader resources is to create an `IResourceMapping` interface that maps resource literal names to the
actual resources:

```cpp
ResourceMappingEntry Entries[] =
{
    {"g_Texture", pTexture->GetDefaultView(TEXTURE_VIEW_SHADER_RESOURCE)}
};
ResourceMappingCreateInfo ResMappingCI;
ResMappingCI.pEntries   = Entries;
ResMappingCI.NumEntries = _countof(Entries);
RefCntAutoPtr<IResourceMapping> pResMapping;
pRenderDevice->CreateResourceMapping(ResMappingCI, &pResMapping);
```

The resource mapping can then be used to bind all static resources in a pipeline state (`IPipelineState::BindStaticResources()`):

```cpp
m_pPSO->BindStaticResources(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, pResMapping,
                            BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED);
```

or all mutable and dynamic resources in a shader resource binding (`IShaderResourceBinding::BindResources()`):

```cpp
m_pSRB->BindResources(SHADER_TYPE_VERTEX | SHADER_TYPE_PIXEL, pResMapping,
                      BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED);
```

The last parameter to all `BindResources()` functions defines how resources should be resolved:

* `BIND_SHADER_RESOURCES_UPDATE_STATIC` - Indicates that static variable bindings are to be updated.
* `BIND_SHADER_RESOURCES_UPDATE_MUTABLE` - Indicates that mutable variable bindings are to be updated.
* `BIND_SHADER_RESOURCES_UPDATE_DYNAMIC` -Indicates that dynamic variable bindings are to be updated.
* `BIND_SHADER_RESOURCES_UPDATE_ALL` - Indicates that all variable types (static, mutable and dynamic) are to be updated.
   Note that if none of `BIND_SHADER_RESOURCES_UPDATE_STATIC`, `BIND_SHADER_RESOURCES_UPDATE_MUTABLE`, and 
   `BIND_SHADER_RESOURCES_UPDATE_DYNAMIC` flags are set, all variable types are updated as if `BIND_SHADER_RESOURCES_UPDATE_ALL` was specified.
* `BIND_SHADER_RESOURCES_KEEP_EXISTING` - If this flag is specified, only unresolved bindings will be updated. All existing bindings will keep their original values. If this flag is not specified, every shader variable will be updated if the mapping contains corresponding resource.
* `BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED` - If this flag is specified, all shader bindings are expected be resolved after the call. If this is not the case, an error will be reported.

`BindResources()` may be called several times with different resource mappings to bind resources.
However, it is recommended to use one large resource mapping as the size of the mapping does not affect element search time.

The engine performs run-time checks to verify that correct resources are being bound. For example, if you try to bind
a constant buffer to a shader resource view variable, an error will be output to the debug console.

<a name="draw_command"></a>
## Setting the Pipeline State and Invoking Draw Command

Before any draw command can be invoked, all required vertex and index buffers as well as the pipeline state should
be bound to the device context:

```cpp
// Set render targets before issuing any draw command.
auto* pRTV = m_pSwapChain->GetCurrentBackBufferRTV();
auto* pDSV = m_pSwapChain->GetDepthBufferDSV();
m_pContext->SetRenderTargets(1, &pRTV, pDSV, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

// Clear render target and depth-stencil
const float zero[4] = {0, 0, 0, 0};
m_pContext->ClearRenderTarget(pRTV, ClearColor, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
m_pContext->ClearDepthStencil(pDSV, CLEAR_DEPTH_FLAG, 1.f, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

// Set vertex and index buffers
IBuffer* buffer[] = {m_pVertexBuffer};
Uint32 offsets[] = {0};
m_pContext->SetVertexBuffers(0, 1, buffer, offsets, SET_VERTEX_BUFFERS_FLAG_RESET,
                             RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
m_pContext->SetIndexBuffer(m_pIndexBuffer, 0, RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

m_pContext->SetPipelineState(m_pPSO);
```

All methods that may need to perform resource state transitions take `RESOURCE_STATE_TRANSITION_MODE` enum
as parameter. The enum defines the following modes:

* `RESOURCE_STATE_TRANSITION_MODE_NONE` - Perform no resource state transitions.
* `RESOURCE_STATE_TRANSITION_MODE_TRANSITION` - Transition resources to the states required by the command.
* `RESOURCE_STATE_TRANSITION_MODE_VERIFY` - Do not transition, but verify that states are correct.

The final step is to commit shader resources to the device context. This is accomplished by
the `IDeviceContext::CommitShaderResources()` method:

```cpp
m_pContext->CommitShaderResources(m_pSRB, COMMIT_SHADER_RESOURCES_FLAG_TRANSITION_RESOURCES);
```

If the method is not called, the engine will detect that resources are not committed and output a
debug message. Note that the last parameter tells the system to transition resources to correct states.
If this flag is not specified, the resources must be explicitly transitioned to required states by a call to
`IDeviceContext::TransitionShaderResources()`:

```cpp
m_pContext->TransitionShaderResources(m_pPSO, m_pSRB);
```

Note that the method requires pointer to the pipeline state that created the shader resource binding.

When all required states and resources are bound, `IDeviceContext::DrawIndexed()` can be used to execute a draw
command or `IDeviceContext::DispatchCompute()` can be used to execute a compute command. Note that for a draw command,
a graphics pipeline must be bound, and for a dispatch command, a compute pipeline must be bound. `DrawIndexed()` takes
`DrawIndexedAttribs` structure as an argument, for example:

```cpp
DrawIndexedAttribs attrs;
attrs.IndexType  = VT_UINT16;
attrs.NumIndices = 36;
attrs.Flags      = DRAW_FLAG_VERIFY_STATES;
pContext->DrawIndexed(attrs);
```

`DRAW_FLAG_VERIFY_STATES` flag instructs the engine to verify that vertex and index buffers used by the
draw command are transitioned to proper states.

`DispatchCompute()` takes `DispatchComputeAttribs` structure that defines compute grid dimensions:

```cpp
m_pContext->SetPipelineState(m_pComputePSO);
m_pContext->CommitShaderResources(m_pComputeSRB, COMMIT_SHADER_RESOURCES_FLAG_TRANSITION_RESOURCES);
DispatchComputeAttribs DispatchAttrs{64, 64, 8};
m_pContext->DispatchCompute(DispatchAttrs);
```

You can learn more about the engine API by studying [samples and tutorials](https://github.com/DiligentGraphics/DiligentSamples).


<a name="low_level_api_interoperability"></a>
# Low-level API interoperability

Diligent Engine extensively supports interoperability with underlying low-level APIs. The engine can be initialized
by attaching to existing D3D11/D3D12 device or OpenGL/GLES context and provides access to the underlying native API
objects. Refer to the following pages for more information:

[Direct3D11 Interoperability](https://github.com/DiligentGraphics/DiligentCore/tree/master/Graphics/GraphicsEngineD3D11#interoperability-with-direct3d11)

[Direct3D12 Interoperability](https://github.com/DiligentGraphics/DiligentCore/tree/master/Graphics/GraphicsEngineD3D12#interoperability-with-direct3d12)

[OpenGL/GLES Interoperability](https://github.com/DiligentGraphics/DiligentCore/tree/master/Graphics/GraphicsEngineOpenGL#interoperability-with-openglgles)

[Vulkan Interoperability](https://github.com/DiligentGraphics/DiligentCore/tree/master/Graphics/GraphicsEngineVulkan#interoperability-with-vulkan)


<a name="nuget_build_instructions"></a>
# NuGet Package Build Instructions

Follow the following steps to build the NuGet package:

1. Install the required Python packages


```
python -m pip install -r ./BuildTools/.NET/requirements.txt
```

2. Run the NuGet package build script, for example:


```
python ./BuildTools/.NET/dotnet-build-package.py -c Debug -d ./
```

## Command Line Arguments

|       Argument             |         Description                                                             |   Required          |
|----------------------------|---------------------------------------------------------------------------------|---------------------|
| `-c` (`configuration`)     | Native dynamic libraries build configuration (e.g. Debug, Release, etc.)        |  Yes                |
| `-d` (`root-dir`)          | The path to the root directory of DiligentCore                                  |  Yes                |
| `-s` (`settings`)          | The path to the settings file                                                   |  No                 |
| `dotnet-tests`             | Flag indicating whether to run .NET tests                                       |  No                 |
| `dotnet-publish`           | Flag indicating whether to publish the package to NuGet Gallery                 |  No                 |
| `free-memory`              | Use this argument if you encounter insufficient memory during the build process |  No                 |

You can override the default settings using a settings file 
(check the `default_settings` dictionary in `dotnet-build-package.py`)

<a name="license"></a>
# License

See [Apache 2.0 license](License.txt).

This project has some third-party dependencies, each of which may have independent licensing:

* [Vulkan-Headers](https://github.com/KhronosGroup/Vulkan-Headers): Vulkan Header files and API registry ([Apache License 2.0](https://github.com/DiligentGraphics/Vulkan-Headers/blob/master/LICENSE.md)).
* [SPIRV-Cross](https://github.com/KhronosGroup/SPIRV-Cross): SPIRV parsing and cross-compilation tools ([Apache License 2.0](https://github.com/DiligentGraphics/SPIRV-Cross/blob/master/LICENSE)).
* [SPIRV-Headers](https://github.com/KhronosGroup/SPIRV-Headers): SPIRV header files ([Khronos MIT-like license](https://github.com/DiligentGraphics/SPIRV-Headers/blob/master/LICENSE)).
* [SPIRV-Tools](https://github.com/KhronosGroup/SPIRV-Tools): SPIRV optimization and validation tools ([Apache License 2.0](https://github.com/DiligentGraphics/SPIRV-Tools/blob/master/LICENSE)).
* [glslang](https://github.com/KhronosGroup/glslang): Khronos reference compiler and validator for GLSL, ESSL, and HLSL ([3-Clause BSD License, 2-Clause BSD License, MIT, Apache License 2.0](https://github.com/DiligentGraphics/glslang/blob/master/LICENSE.txt)).
* [glew](http://glew.sourceforge.net/): OpenGL Extension Wrangler Library ([Mesa 3-D graphics library, Khronos MIT-like license](https://github.com/DiligentGraphics/DiligentCore/blob/master/ThirdParty/glew/LICENSE.txt)).
* [volk](https://github.com/zeux/volk): Meta loader for Vulkan API ([Arseny Kapoulkine MIT-like license](https://github.com/DiligentGraphics/volk/blob/master/LICENSE.md)).
* [stb](https://github.com/nothings/stb): stb single-file public domain libraries for C/C++ ([MIT License or public domain](https://github.com/DiligentGraphics/DiligentCore/blob/master/ThirdParty/stb/stb_image_write.h#L1581)).
* [googletest](https://github.com/google/googletest): Google Testing and Mocking Framework ([BSD 3-Clause "New" or "Revised" License](https://github.com/DiligentGraphics/googletest/blob/master/LICENSE)).
* [DirectXShaderCompiler](https://github.com/microsoft/DirectXShaderCompiler): LLVM/Clang-based DirectX Shader Compiler ([LLVM Release License](https://github.com/DiligentGraphics/DiligentCore/blob/master/ThirdParty/DirectXShaderCompiler/LICENSE.TXT)).
* [DXBCChecksum](ThirdParty/GPUOpenShaderUtils): DXBC Checksum computation algorithm by AMD Developer Tools Team ([MIT lincesne](ThirdParty/GPUOpenShaderUtils/License.txt)).
* [xxHash](https://github.com/Cyan4973/xxHash): Extremely fast non-cryptographic hash algorithm ([2-Clause BSD License](https://github.com/DiligentGraphics/xxHash/blob/dev/LICENSE)).

<a name="contributing"></a>
# Contributing

To contribute your code, submit a [Pull Request](https://github.com/DiligentGraphics/DiligentCore/pulls) 
to this repository. **Diligent Engine** is distributed under the [Apache 2.0 license](License.txt) that guarantees 
that content in the **DiligentCore** repository is free of Intellectual Property encumbrances. 
In submitting any content to this repository,
[you license that content under the same terms](https://docs.github.com/en/free-pro-team@latest/github/site-policy/github-terms-of-service#6-contributions-under-repository-license),
and you agree that the content is free of any Intellectual Property claims and you have the right to license it under those terms. 

Diligent Engine uses [clang-format](https://clang.llvm.org/docs/ClangFormat.html) to ensure
consistent source code style throughout the code base. The format is validated by CI
for each commit and pull request, and the build will fail if any code formatting issue is found. Please refer
to [this page](https://github.com/DiligentGraphics/DiligentCore/blob/master/doc/code_formatting.md) for instructions
on how to set up clang-format and automatic code formatting.


<a name="release_history"></a>
# Release History

See [Release History](ReleaseHistory.md)

-------------------

[diligentgraphics.com](http://diligentgraphics.com)

[![Diligent Engine on Twitter](https://github.com/DiligentGraphics/DiligentCore/blob/master/media/twitter.png)](https://twitter.com/diligentengine)
[![Diligent Engine on Facebook](https://github.com/DiligentGraphics/DiligentCore/blob/master/media/facebook.png)](https://www.facebook.com/DiligentGraphics/)
