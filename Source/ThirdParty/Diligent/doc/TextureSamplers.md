# Working with Texture Samplers

Implementations of texture samplers vary considerably in the native graphics APIs supported by Diligent Engine.
In Direct3D11, Direct3D12 and Metal, textures and samplers are decoupled and there are separate shader objects for each, for example:

```hlsl
Texture2D    g_Texture;
SamplerState g_Sampler;

// ...

float4 Color = g_Texture.Sample(g_Sampler, UV);
```

In OpenGL, samplers are combined with textures into a single shader object:

```glsl
uniform sampler2D g_Texture;

// ...

vec4 Color = texture(g_Texture, UV);
```

Vulkan supports both ways: there are both combined and separate samplers.

There are two ways texture samplers can be handled in Diligent Engine, which are described in detail below.


## Separate Samplers

A more straightforward and clean way to work with texture samplers is to use separate samplers.
This way is native to Direct3D11, Direct3D12 and Metal and is also fully supported by Vulkan.
Separate samplers are only unavailable in OpenGL.

When separate samplers are used, every sampler is just a regular shader variable and is accessed
similar to all other resources. Samplers follow the same conventions regarding variable types 
(static, mutable, dynamic), for example:

```cpp
RefCntAutoPtr<ISampler> pSampler;
pDevice->CreateSampler(SamDesc, &pSampler);

// ...
pPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "g_StaticSampler")->Set(pSampler);
pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_MutableSampler")->Set(pSampler);
```

Separate samplers are natively supported by HLSL and MSL. GLSL also supports separate samplers using
the following syntax (Vulkan only):

```glsl
uniform sampler   uSampler;
uniform texture2D uTexture;

// ...
vec4 Color = texture(sampler2D(uTexture, uSampler), UV);
```

Unless your application must support OpenGL, it is strongly recommended to use separate samplers.

### Immutable Samplers

In many scenarios sampler properties are known and never change. In cases like this it is preferable to use
immutable samplers. Immutable samplers are defined when either a pipeline state is created using the
`ResourceLayout` or when a pipeline resource signature is created, e.g.

```cpp
GraphicsPipelineStateCreateInfo PSOCreateInfo;

auto& PSODesc          = PSOCreateInfo.PSODesc;
auto& GraphicsPipeline = PSOCreateInfo.GraphicsPipeline;
auto& ResourceLayout   = GraphicsPipeline.ResourceLayout;

ImmutableSamplerDesc ImmutableSamplers[] =
    {
        {SHADER_TYPE_PIXEL, "g_WrapSampler",  WrapSamplerDesc},
        {SHADER_TYPE_PIXEL, "g_ClampSampler", ClampSamplerDesc}
    };
ResourceLayout.ImmutableSamplers    = ImmutableSamplers;
ResourceLayout.NumImmutableSamplers = _countof(ImmutableSamplers);

pDevice->CreateGraphicsPipelineState(PSOCreateInfo, &pPSO);
```

```cpp
PipelineResourceSignatureDesc PRSDesc;

ImmutableSamplerDesc ImmutableSamplers[] =
    {
        {SHADER_TYPE_PIXEL, "g_WrapSampler",  WrapSamplerDesc},
        {SHADER_TYPE_PIXEL, "g_ClampSampler", ClampSamplerDesc}
    };
PRSDesc.ImmutableSamplers    = ImmutableSamplers;
PRSDesc.NumImmutableSamplers = _countof(ImmutableSamplers);

pDevice->CreatePipelineResourceSignature(PRSDesc, &pPRS);
```

In Direct3D12 and Vulkan, immutable samplers have major advantages over regular ones: they
are compiled directly into the PSO, generally faster and don't count towards the limit of the total
number of samplers available to the application.
In Direct3D11 and Metal, immutable samplers are emulated, but from the application point of view
their behavior is identical.

It is always recommended to use immutable samplers whenever possible.


## Combined Samplers

Combined samplers are the only way to sample textures in OpenGL. Diligent Engine supports emulation of OpenGL-style
combined samplers in all backends so that an application targeting OpenGL will also run on other backends
without any changes. 

In Direct3D11, Direct3D12 and Metal, combined samplers are emulated through texture + sampler pairs. The name of the sampler
is obtained by adding a predefined suffix to the texture name (see below), e.g.:

```hlsl
Texture2D    g_Texture;
SamplerState g_Texture_sampler;

// ...

float4 Color = g_Texture.Sample(g_Texture_sampler, UV);
```

When combined image samplers are used, samplers are not exposed as shader resources. In particular, even though
the shader above contains `g_Texture_sampler` sampler, it cannot be accessed directly:

```cpp
pPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "g_StaticTexture_sampler") // returns null
pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_MutableTexture_sampler") // returns null
```

Samplers are set in the texture view that is then bound to the texture variable:

```cpp
pTexView->SetSampler(pSampler);
// Sets both texture and samplers
pPSO->GetStaticVariableByName(SHADER_TYPE_PIXEL, "g_StaticTexture")->Set(pTexView);
pSRB->GetVariableByName(SHADER_TYPE_PIXEL, "g_MutableTexture")->Set(pTexView);
```

:warning: When combined image samplers are used, a sampler must be set in the view
          when the view is set to the shader variable.

Note that emulating combined samplers in Direct3D11, Direct3D12 and Metal requires
defining separate sampler object for each texture. The number of available samplers
is usually significantly smaller compared to the number of available textures.

### Initializing Combined Samplers 

To enable the usage of combined image samplers, an application should set the `ShaderCreateInfo::UseCombinedTextureSamplers`
flag and define the `ShaderCreateInfo::CombinedSamplerSuffix`. The suffix is used as described below.

#### Direct3D11, Direct3D12, Metal

The suffix is appended to the name of the texture, which gives the sampler name. Every sampler must be combined with some texture.
It is an error to have a sampler that is not assigned to any texture, for example:

```hlsl
Texture2D    g_Texture;
SamplerState g_Texture_sampler; // Combined with g_Texture

SamplerState g_Sampler; // Error - not combined with any texture
```

#### Vulkan

Vulkan-style GLSL allows defining both native combined image samplers as well as separate samplers and images.
When `UseCombinedTextureSamplers` flag is set to `true`, separate samplers and images must follow the same conventions
as for Direct3D and Metal. Native combined image samplers may be used directly:

```glsl
uniform texture2D uTexture;
uniform sampler   uTexture_sampler; // Combined with uTexture

uniform sampler   uSampler; // Error - not combined with any texture

uniform sampler2D uTexture2; // Native combined sampler
```


#### OpenGL

There are no separate samplers in OpenGL shaders, and `CombinedSamplerSuffix` is ignored.
`UseCombinedTextureSamplers` flag must always be set to `true` when cross-compiling HLSL
shaders to GLSL.


### Immutable Samplers

Immutable samplers are fully supported for combined image samplers with the only difference that
texture names must be specified instead of the sampler names:

```
ImmutableSamplerDesc ImmutableSamplers[] =
    {
        {SHADER_TYPE_PIXEL, "g_Texture1", WrapSamplerDesc},
        {SHADER_TYPE_PIXEL, "g_Texture2", ClampSamplerDesc}
    };
ResourceLayout.ImmutableSamplers    = ImmutableSamplers;
ResourceLayout.NumImmutableSamplers = _countof(ImmutableSamplers);
```


### Pipeline Resource Signatures

Using combined image samplers with pipeline resource signatures is generally similar:
you need to set `UseCombinedTextureSamplers` member of `PipelineResourceSignatureDesc`
struct to `true` and define the `CombinedSamplerSuffix` (or leave its default value `"_sampler"`):

```cpp
PipelineResourceSignatureDesc PRSDesc;

PRSDesc.UseCombinedTextureSamplers = true;

PipelineResourceDesc Resources[]
{
    {SHADER_TYPE_PIXEL, "g_Tex2D_Static", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
    {SHADER_TYPE_PIXEL, "g_Tex2D_Mut",    1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
    {SHADER_TYPE_PIXEL, "g_Tex2D_Dyn",    1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC}
};
PRSDesc.Resources    = Resources;
PRSDesc.NumResources = _countof(Resources);

pDevice->CreatePipelineResourceSignature(PRSDesc, &pPRS);
```

#### PIPELINE_RESOURCE_FLAG_COMBINED_SAMPLER flag

`PIPELINE_RESOURCE_FLAG_COMBINED_SAMPLER` can be used with the `SHADER_RESOURCE_TYPE_TEXTURE_SRV` resource to indicate
that the resource is a *native* combined image sampler. Depending on the backend, this flag results in different behavior,
as described below.

**Direct3D11, Direct3D12 and Metal**

Native combined image samplers are not available in these backends and are emulated. Using
`PIPELINE_RESOURCE_FLAG_COMBINED_SAMPLER` is the same as defining the texture + sampler pair, e.g.

```cpp
PipelineResourceDesc Resources[]
{
    {SHADER_TYPE_PIXEL, "g_Tex", 1, SHADER_RESOURCE_TYPE_TEXTURE_SRV,
        SHADER_RESOURCE_VARIABLE_TYPE_STATIC | PIPELINE_RESOURCE_FLAG_COMBINED_SAMPLER}
};
```

is the same as defining two resources:

```cpp
PipelineResourceDesc Resources[]
{
    {SHADER_TYPE_PIXEL, "g_Tex",         1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
    {SHADER_TYPE_PIXEL, "g_Tex_sampler", 1, SHADER_RESOURCE_TYPE_SAMPLER,     SHADER_RESOURCE_VARIABLE_TYPE_STATIC}
};
```

The sampler inherits the variable type, array size and texture stages from the texture. Note also that
the sampler may be defined explicitly if it is necessary to override some default settings.

Note that in Direct3D and Metal, `PIPELINE_RESOURCE_FLAG_COMBINED_SAMPLER` requires that `UseCombinedTextureSamplers`
is set to `true` and that `CombinedSamplerSuffix` is not null.


**Vulkan**

In Vulkan, `PIPELINE_RESOURCE_FLAG_COMBINED_SAMPLER` defines the native combined image sampler. It may be
used together with separate images and samplers, e.g.:

```cpp
PipelineResourceDesc Resources[]
{
    {SHADER_TYPE_PIXEL, "g_Tex",         1, SHADER_RESOURCE_TYPE_TEXTURE_SRV, SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
    {SHADER_TYPE_PIXEL, "g_Tex_sampler", 1, SHADER_RESOURCE_TYPE_SAMPLER,     SHADER_RESOURCE_VARIABLE_TYPE_STATIC},
    {SHADER_TYPE_PIXEL, "g_Tex2",        1, SHADER_RESOURCE_TYPE_TEXTURE_SRV,
        SHADER_RESOURCE_VARIABLE_TYPE_STATIC | PIPELINE_RESOURCE_FLAG_COMBINED_SAMPLER}
};
```

Note that Vulkan supports both emulated and native combined image samplers. In the example above, the `g_Tex` + 
`g_Tex_sampler` pair may be accessed as single combined sampler object when `UseCombinedTextureSamplers` is set to `true`,
or as individual resources when `UseCombinedTextureSamplers` is set to `false`. Same is true when resources
are defined through the `ResourceLayout` member of the pipeline description.

:warning: It is an error to define emulated combined image sampler using the `PIPELINE_RESOURCE_FLAG_COMBINED_SAMPLER`
          flag in Vulkan. The flag must only be used to indicate native GLSL combined image samplers.

**OpenGL**

In OpenGL, all samplers are combined samplers. However, for consistency with Direct3D and Metal, GLSL samplers
are exposed without the `PIPELINE_RESOURCE_FLAG_COMBINED_SAMPLER` flag when cross-compiling from HLSL.

#### Universal Combined Sampler Description

**HLSL**

HLSL is the universal shading language in Diligent Engine that is supported in all backends and on
all platforms. Use the following rules to define combined texture samplers in a signature that will
work across all backends:

* Always explicitly define the texture + sampler pair in the list of resources
* Do not use the `PIPELINE_RESOURCE_FLAG_COMBINED_SAMPLER` flag as when compiling HLSL to SPIRV,
  HLSL textures are converted to separate images, not to combined samplers.

**GLSL**

* If you use Vulkan dialect of GLSL, define the texture + sampler pairs for the emulated combined
  image samplers.
* Use `PIPELINE_RESOURCE_FLAG_COMBINED_SAMPLER` flag for native combined texture samplers.

**MSL**

Follow the same rules as for HLSL.


## Shader Cross-Compilation Considerations

Diligent Engine supports multiple ways of shader cross-compilation.
Below are some details related to samplers.

### HLSL -> GLSL for OpenGL

The HLSL texture + sampler pair is converted to combined GLSL sampler under the hood.
Note that for consistency with Direct3D and Metal, the texture resource is exposed without the
`PIPELINE_RESOURCE_FLAG_COMBINED_SAMPLER` flag. Note that this flag is used to indicate
native combined image sampler in GLSL and is generally not applicable to HLSL.

### HLSL -> SPIRV for Vulkan

HLSL textures and samplers are converted directly to separate images and separate samplers in
SPIRV. `UseCombinedTextureSamplers` enables emulated combined image samplers similar to Direct3D
and Metal.

### HLSL -> MSL for Metal

HLSL textures and samplers are converted directly to MSL textures and samplers.
`UseCombinedTextureSamplers` enables Direct3D-style emulated combined image samplers.

### GLSL or SPIRV -> MSL for Metal

Native combined image samplers in GLSL are always converted to texture + sampler pair in MSL.
The pairs may then be accessed as separate resources (`UseCombinedTextureSamplers` is `false`)
or as emulated combined image samplers (`UseCombinedTextureSamplers` is `true`).
`PIPELINE_RESOURCE_FLAG_COMBINED_SAMPLER` flag produces the texture + sampler pair that
matches the MSL objects.


## Miscellaneous

* When a pipeline state is created using the default resource signature (i.e. `ppResourceSignatures` member of
  `PipelineStateCreateInfo` is null), whether to use combined samplers is defined by the values of
  `UseCombinedTextureSamplers` member of `ShaderCreateInfo` of the shaders in the pipeline. It is possible
  to mix shaders that use combined samplers with ones that don't, in which case some texture+sampler
  pairs will be accessed as single object, while others will be separate resources. However, if a shader
  defines a combined sampler suffix, it must match any other suffix used by other shaders. It is an
  error to mix shaders that use different suffixes in one pipeline.

* When a pipeline state is created using the explicit resource signatures (i.e. `ppResourceSignatures` of
  `PipelineStateCreateInfo` is not null), the values of `UseCombinedTextureSamplers` member of `ShaderCreateInfo`
  of the shaders in the pipeline are ignored.
