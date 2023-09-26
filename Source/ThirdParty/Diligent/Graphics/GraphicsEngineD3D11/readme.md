
# GraphicsEngineD3D11

Implementation of Direct3D11 backend

# Interoperability with Direct3D11

Diligent Engine exposes methods to access internal D3D11 objects, is able to create diligent engine buffers
and textures from existing Direct3D11 buffers and textures, and can be initialized by attaching to existing D3D11
device and immediate context.

## Accessing Native Direct3D11 objects

Below are some of the methods that provide access to internal D3D11 objects:

|                              Function                                     |                              Description                                                                        |
|---------------------------------------------------------------------------|-----------------------------------------------------------------------------------------------------------------|
| `ID3D11Buffer* IBufferD3D11::GetD3D11Buffer`                              | returns a pointer to the `ID3D11Buffer` interface of the internal Direct3D11 buffer object                      |
| `ID3D11Resource* ITextureD3D11::GetD3D11Texture`                          | returns a pointer to the `ID3D11Resource` interface of the internal Direct3D11 texture object                   |
| `ID3D11View* IBufferViewD3D11()::GetD3D11View`                            | returns a pointer to the `ID3D11View` interface of the internal Direct3D11 object representing the buffer view  |
| `ID3D11View* ITextureViewD3D11::GetD3D11View`                             | returns a pointer to the `ID3D11View` interface of the internal Direct3D11 object representing the texture view |
| `ID3D11Device* IRenderDeviceD3D11::GetD3D11Device`                        | returns a pointer to the native Direct3D11 device object                                                        |
| `ID3D11DeviceContext* IDeviceContextD3D11::GetD3D11DeviceContext`         | returns a pointer to the native `ID3D11DeviceContext` object                                                    |
| `ID3D11BlendState* IPipelineStateD3D11::GetD3D11BlendState`               | returns a pointer to the native `ID3D11BlendState` object                                                       |
| `ID3D11RasterizerState* IPipelineStateD3D11::GetD3D11RasterizerState`     | returns a pointer to the native `ID3D11RasterizerState` object                                                  |
| `ID3D11DepthStencilState* IPipelineStateD3D11::GetD3D11DepthStencilState` | returns a pointer to the native `ID3D11DepthStencilState` object                                                |
| `ID3D11InputLayout* IPipelineStateD3D11::GetD3D11InputLayout`             | returns a pointer to the native `ID3D11InputLayout` object                                                      |
| `ID3D11VertexShader* IPipelineStateD3D11::GetD3D11VertexShader`           | returns a pointer to the native `ID3D11VertexShader` object                                                     |
| `ID3D11PixelShader* IPipelineStateD3D11::GetD3D11PixelShader`             | returns a pointer to the native `ID3D11PixelShader` object                                                      |
| `ID3D11GeometryShader* IPipelineStateD3D11::GetD3D11GeometryShader`       | returns a pointer to the native `ID3D11GeometryShader` object                                                   |
| `ID3D11DomainShader* IPipelineStateD3D11::GetD3D11DomainShader`           | returns a pointer to the native `ID3D11DomainShader` object                                                     |
| `ID3D11HullShader* IPipelineStateD3D11::GetD3D11HullShader`               | returns a pointer to the native `ID3D11HullShader` object                                                       |
| `ID3D11ComputeShader* IPipelineStateD3D11::GetD3D11ComputeShader`         | returns a pointer to the native `ID3D11ComputeShader` object                                                    |
| `ID3D11Query* IQueryD3D11::GetD3D11Query`                                 | returns a pointer to the native `ID3D11Query` object                                                            |
| `ID3D11SamplerState* ISamplerD3D11::GetD3D11SamplerState`                 | returns a pointer to the native `ID3D11SamplerState` object                                                     |
| `ID3D11DeviceChild* IShaderD3D11::GetD3D11Shader`                         | returns a pointer to the native `ID3D11DeviceChild` object representing the shader                              |
| `ITextureViewD3D11* ISwapChainD3D11::GetCurrentBackBufferRTV`             | returns a pointer to the native `ITextureViewD3D11` object representing current back buffer render target view  |
| `ITextureViewD3D11* ISwapChainD3D11::GetDepthBufferDSV`                   | returns a pointer to the native `ITextureViewD3D11` object representing depth-stencil view                      |
| `IDXGISwapChain* ISwapChainD3D11::GetDXGISwapChain`                       | returns a pointer to the native `IDXGISwapChain` object                                                         |

## Creating Diligent Engine Objects from Direct3D11 Resources

|                              Function                 |                              Description                                        |
|-------------------------------------------------------|---------------------------------------------------------------------------------|
| `IRenderDeviceD3D11::CreateBufferFromD3DResource`     | creates a Diligent Engine buffer object from the native Direct3D11 buffer       |
| `IRenderDeviceD3D11::CreateTexture1DFromD3DResource`  | creates a Diligent Engine texture object from the native Direct3D11 1D texture  |
| `IRenderDeviceD3D11::CreateTexture2DFromD3DResource`  | creates a Diligent Engine texture object from the native Direct3D11 2D texture  |
| `IRenderDeviceD3D11::CreateTexture3DFromD3DResource`  | creates a Diligent Engine texture object from the native Direct3D11 3D texture  |
 

## Initializing the Engine by Attaching to Existing Direct3D11 Device and Immediate Context

You can use `IEngineFactoryD3D11::AttachToD3D11Device` to attach the engine to existing Direct3D11 device and immediate context.
For example, the code snippet below shows how Diligent Engine can be attached to Direct3D11 device returned by Unity:

```cpp
IUnityGraphicsD3D11* d3d = interfaces->Get<IUnityGraphicsD3D11>();
ID3D11Device* d3d11NativeDevice = d3d->GetDevice();
CComPtr<ID3D11DeviceContext> d3d11ImmediateContext;
d3d11NativeDevice->GetImmediateContext(&d3d11ImmediateContext);
auto* pFactoryD3d11 = GetEngineFactoryD3D11();
EngineD3D11CreateInfo EngineCI;
pFactoryD3d11->AttachToD3D11Device(d3d11NativeDevice, d3d11ImmediateContext, EngineCI, &m_Device, &m_Context, 0);
```

-------------------

[diligentgraphics.com](http://diligentgraphics.com)

[![Diligent Engine on Twitter](https://github.com/DiligentGraphics/DiligentCore/blob/master/media/twitter.png)](https://twitter.com/diligentengine)
[![Diligent Engine on Facebook](https://github.com/DiligentGraphics/DiligentCore/blob/master/media/facebook.png)](https://www.facebook.com/DiligentGraphics/)
