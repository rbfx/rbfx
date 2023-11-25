
# GraphicsEngineD3D12

Implementation of Direct3D12 backend

# Interoperability with Direct3D12

Diligent Engine exposes methods to access internal Direct3D12 objects, is able to create diligent engine buffers
and textures from existing Direct3D12 resources, and can be initialized by attaching to existing Direct3D12
device and provide synchronization tools.

## Accessing Native Direct3D12 Resources

Below are some of the methods that provide access to internal Direct3D12 objects:

|                              Function                                  |                              Description                                                                      |
|------------------------------------------------------------------------|---------------------------------------------------------------------------------------------------------------|
| `IBufferD3D12::GetD3D12Buffer(size_t& DataStartByteOffset, ContextId)` | returns a pointer to the `ID3D12Resource` interface of the internal Direct3D12 buffer object. Note that dynamic buffers are suballocated from dynamic heap, and every context has its own dynamic heap. Offset from the beginning of the dynamic heap for a context identified by `ContextId` is returned in `DataStartByteOffset` parameter |
| `IBufferD3D12::SetD3D12ResourceState`                                  | sets the buffer usage state. This method should be used when an application transitions the buffer to inform the engine about the current usage state |
| `IBufferD3D12::GetD3D12ResourceState`                                  | returns current Direct3D12 buffer state |
| `IBufferViewD3D12::GetCPUDescriptorHandle`                             | returns CPU descriptor handle of the buffer view |
| `ITextureD3D12::GetD3D12Texture`                                       | returns a pointer to the `ID3D12Resource` interface of the internal Direct3D12 texture object |
| `ITextureD3D12::SetD3D12ResourceState`                                 | sets the texture usage state. This method should be used when an application transitions the texture to inform the engine about the current usage state |
| `ITextureD3D12::GetD3D12ResourceState`                                 | returns current Direct3D12 texture state |
| `ITextureViewD3D12::GetCPUDescriptorHandle`                            | returns CPU descriptor handle of the texture view |
| `IDeviceContextD3D12::TransitionTextureState`                          | transitions internal Direct3D12 texture object to the specified state |
| `IDeviceContextD3D12::TransitionBufferState`                           | transitions internal Direct3D12 buffer object to the specified state |
| `IDeviceContextD3D12::GetD3D12CommandList`                             | returns a pointer to Direct3D12 graphics command list that is currently being recorded |
| `IPipelineStateD3D12::GetD3D12PipelineState`                           | returns `ID3D12PipelineState` interface of the internal Direct3D12 pipeline state object object |
| `IPipelineStateD3D12::GetD3D12RootSignature`                           | returns a pointer to the root signature object associated with this pipeline state |
| `IPipelineStateD3D12::GetD3D12StateObject`                             | returns `ID3D12StateObject` interface of the internal D3D12 state object for ray tracing |
| `ISamplerD3D12::GetCPUDescriptorHandle`                                | returns a CPU descriptor handle of the Direct3D12 sampler object |
| `IRenderDeviceD3D12::GetD3D12Device`                                   | returns `ID3D12Device` interface of the internal Direct3D12 device object |
| `IFenceD3D12::GetD3D12Fence`                                           | returns `IFenceD3D12` interface of the internal fence object |
| `IQueryD3D12::GetD3D12QueryHeap`                                       | returns the Direct3D12 query heap that internal query object resides in |
| `IQueryD3D12::GetQueryHeapIndex`                                       | returns the index of a query object in Direct3D12 query heap |
| `IBottomLevelASD3D12::GetD3D12BLAS`                                    | returns `ID3D12Resource` interface of the internal Direct3D12 BLAS object |
| `ITopLevelASD3D12::GetD3D12TLAS`                                       | returns `ID3D12Resource` interface of the internal Direct3D12 TLAS object |
| `ITopLevelASD3D12::GetCPUDescriptorHandle`                             | returns a CPU descriptor handle of the D3D12 acceleration structure |
| `IShaderBindingTableD3D12::GetD3D12BindingTable`                       | returns the shader binding table structure that can be used with `ID3D12GraphicsCommandList4::DispatchRays` call |
| `ISwapChainD3D12::GetDXGISwapChain`                                    | returns a pointer to the IDXGISwapChain interface of the internal DXGI object |

## Synchronization Tools

|                              Function          |                              Description         |
|------------------------------------------------|--------------------------------------------------|
| `ICommandQueueD3D12::Submit`                   | submits a Direct3D12 command lists for execution |
| `ICommandQueueD3D12::GetD3D12CommandQueue`     | returns Direct3D12 command queue                 |
| `ICommandQueueD3D12::EnqueueSignal`            | signals the Direct3D12 fence                     |
| `ICommandQueueD3D12::WaitFence`                | waits for the Direct3D12 fence on the GPU        |

## Creating Diligent Engine Objects from D3D12 Resources

|                              Function              |                              Description                                                                      |
|----------------------------------------------------|---------------------------------------------------------------------------------------------------------------|
| `IRenderDeviceD3D12::CreateTextureFromD3DResource` | creates a Diligent Engine texture object from the native Direct3D12 resource |
| `IRenderDeviceD3D12::CreateBufferFromD3DResource`  | creates a Diligent Engine buffer object from the native Direct3D12 resource. The method takes a pointer to the native Direct3D12 resource `pd3d12Buffer`, buffer description `BuffDesc` and writes a pointer to the `IBuffer` interface at the memory location pointed to by `ppBuffer`. The system can recover buffer size, but the rest of the fields of `BuffDesc` structure need to be populated by the client as they cannot be recovered from Direct3D12 resource description. |
| `IRenderDeviceD3D12::CreateBLASFromD3DResource`    | creates a Diligent Engine bottom-level acceleration structure from the native Direct3D12 resource |
| `IRenderDeviceD3D12::CreateTLASFromD3DResource`    | creates a Diligent Engine top-level acceleration structure from the native Direct3D12 resource |


## Initializing the Engine by Attaching to Existing D3D12 Device

To attach diligent engine to existing D3D12 device, use the following factory function:

```cpp
void IEngineFactoryD3D12::AttachToD3D12Device(void*                           pd3d12NativeDevice,
                                              Uint32                          CommandQueueCount,
                                              struct ICommandQueueD3D12**     ppCommandQueues,
                                              const EngineD3D12CreateInfo REF EngineCI,
                                              IRenderDevice**                 ppDevice,
                                              IDeviceContext**                ppContexts);
```

The method takes a pointer to the native D3D12 device `pd3d12NativeDevice`, initialization parameters `EngineCI`,
and returns the Diligent Engine device in `ppDevice`, and contexts in `ppContexts` parameters. Pointers to the
immediate contexts go first. If `EngineCI.NumDeferredContexts` > 0, pointers to deferred contexts go afterwards.
The function also takes an array of command queue objects `ppCommandQueues`, which need to implement
`ICommandQueueD3D12` interface.

-------------------

[diligentgraphics.com](http://diligentgraphics.com)

[![Diligent Engine on Twitter](https://github.com/DiligentGraphics/DiligentCore/blob/master/media/twitter.png)](https://twitter.com/diligentengine)
[![Diligent Engine on Facebook](https://github.com/DiligentGraphics/DiligentCore/blob/master/media/facebook.png)](https://www.facebook.com/DiligentGraphics/)
