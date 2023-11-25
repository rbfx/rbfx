
# GraphicsEngineOpenGL

Implementation of OpenGL/GLES backend

# Interoperability with OpenGL/GLES

Diligent Engine exposes methods to access internal OpenGL/GLES objects, is able to create diligent engine buffers
and textures from existing GL buffer and texture handles, and can be initialized by attaching to existing GL
context.

## Accessing Native GL objects

Below are some of the methods that provide access to internal GL object handles:

|                       Function             |                              Description                                                                      |
|--------------------------------------------|---------------------------------------------------------------------------------------------------------------|
| `IBufferGL::GetGLBufferHandle`             | returns OpenGL buffer handle                     |
| `IDeviceContextGL::UpdateCurrentGLContext` | attaches to the active GL context in the thread. Returns false if there is no active context, and true otherwise.  If an application uses multiple GL contexts, this method must be called before any other command to let the engine update the active context. |
| `ITextureGL::GetGLTextureHandle`           | returns OpenGL texture handle                    |
| `ITextureGL::GetBindTarget`                | returns OpenGL texture bind target               |
| `IQueryGL::GetGlQueryHandle`               | returns OpenGL query handle                      |
| `ISwapChainGL::GetDefaultFBO`              | returns default framebuffer handle               |


## Creating Diligent Engine Objects from OpenGL Handles

* `void IRenderDeviceGL::CreateTextureFromGLHandle(Uint32 GLHandle, Uint32 GLBindTarget, const TextureDesc& TexDesc, RESOURCE_STATE InitialState, ITexture** ppTexture)` -
    creates a diligent engine texture from OpenGL handle. The method takes OpenGL handle `GLHandle`, texture bind target `GLBindTarget`
    (when null value is provided, the engine automatically selects the target that corresponds to the texture type, e.g. GL_TEXTURE_2D
    for a 2D texture), texture description `TexDesc`, as well as initial state `InitialState`,
    and writes the pointer to the created texture object at the memory address pointed to by `ppTexture`. The engine can automatically
    set texture width, height, depth, mip levels count, and format, but the remaining field of `TexDesc` structure must be populated by
    the application. Note that diligent engine texture object does not take ownership of the GL resource, and the application must
    not destroy it while it is in use by the engine.
* `void IRenderDeviceGL::CreateBufferFromGLHandle(Uint32 GLHandle, const BufferDesc& BuffDesc, RESOURCE_STATE InitialState, IBuffer** ppBuffer)` -
    creates a diligent engine buffer from OpenGL handle. The method takes OpenGL handle `GLHandle`, buffer description `BuffDesc`,
    and writes the pointer to the created buffer object at the memory address pointed to by `ppBuffer`. The engine can automatically
    set the buffer size, but the rest of the fields need to be set by the client. Note that diligent engine buffer object does not
    take ownership of the GL resource, and the application must not destroy it while it is in use by the engine.

## Initializing the Engine by Attaching to Existing GL Context

The code snippet below shows how diligent engine can be attached to existing GL context that is active in the current thread.

```cpp
auto* pFactoryGL = GetEngineFactoryOpenGL();
EngineCreationAttribs EngineCI;
pFactoryGL->AttachToActiveGLContext(EngineCI, &m_Device, &m_Context);
```

-------------------

[diligentgraphics.com](http://diligentgraphics.com)

[![Diligent Engine on Twitter](https://github.com/DiligentGraphics/DiligentCore/blob/master/media/twitter.png)](https://twitter.com/diligentengine)
[![Diligent Engine on Facebook](https://github.com/DiligentGraphics/DiligentCore/blob/master/media/facebook.png)](https://www.facebook.com/DiligentGraphics/)
