
# Coordinate System Conventions

While there exist multiple ways to express transformations of a 3D scene (row-vectors vs column-vectors, right-handed systems vs left-handed systems, etc.),
from the point of view of the graphics API the only coordinate system that matters is called Normalized Device Space (NDC). Diligent Engine uses the
Direct3D convention, in which NDC is a half cube [-1, +1] x [-1, +1] x [0, 1]. For the API, it is totally irrelevant which spaces the coordinates traveled
through before they ended up in the NDC. In particular, if you are using the right-handed coordinate system with the camera looking towards the -Z direction,
eventually the combination of transformation should bring the model into the NDC with Z in [0, 1] range.

The NDC naturally maps to the screen: X axis goes left to right, Y axis goes down-up, and Z axis goes into the screen. Culling mode is based on the
winding direction of vertices in the NDC.

Note that when NDC is converted into window coordinates, the Y axis is inverted such that the first byte of the raw texture data will correspond to the
location with coordinates (-1, +1) in NDC. When the texture is presented to the screen, the image is inverted again because the screen origin is in the
top left corner, so the image is shown properly. When rendering to an off-screen textures, however, the image will end up upside-down.

The convention above is the default convention in Direct3D11 and Direct3D12. Vulkan is generally very similar, but it does not by default invert the Y axis when
mapping NDC to Window coordinates. This, however, can be circumvented by using the negative viewport height, thus making Vulkan backend 100% compatible with 
Direct3D11 and Direct3D12.

OpenGL as always is a headache. First of all, at some point somebody decided that symmetry should prevail over common sense, and as a result the default
NDC space in OpenGL is [-1, +1] x [-1, +1] x [-1, +1]. After a bit of web search you can easily find what disastrous consequences this has for the
precision of floating point numbers when using inverse depth buffering.
[0, +1] z range may be optionally enabled by setting `ZeroToOneNDZ` of `EngineGLCreateInfo` struct to true. This feature requires an extension
that is not widely supported on all mobile devices, so an application should query the actual z range after creating the device.
 
The Z range in OpenGL cannot be fixed under the hood, as it needs to be baked into the projection matrix. Diligent's projection functions take a parameter
that indicates if the matrix should be constructed for OpenGL. Note that Direct3D/Vulkan projection matrices will also work in OpenGL, but will use
half of the Z range (likely without any negative consequences for the majority of applications).

The second issue with OpenGL is that there is no way to invert Y axis when mapping NDC to window space. To alleviate this problem,
Diligent's SetViewport method takes the size of the render target. By default these are taken from the currently bound render target.
OpenGL backend uses these to adjust the viewport coordinates similar to Vulkan. This works in most cases (e.g. shadow mapping), however since
the viewport height can't be negative there are few cases when it does not quite work and require special handling (e.g. off-screen rendering).


So the bottom line is that Direct3D-style coordinates will work in majority of the situations in all backends.
In few cases (such as rendering to a texture and then showing this texture on the screen), OpenGL may require special care:
you may need to invert the texture V coordinate. Other backends are always 100% consistent.


Diligent math library uses row-vectors, similar to Direct3D. Matrices should be multiplied in the order the respective transforms are applied, e.g.:

```cpp
float4x4 WorldViewProj = World * View * Proj;
float4   PositionProj  = PositionWorld * WorldViewProj;
```

As it was mentioned in the beginning, applications may use any library of their preference. As long as the math produces the
same coordinates in NDC, the choice of the specific library is irrelevant.
