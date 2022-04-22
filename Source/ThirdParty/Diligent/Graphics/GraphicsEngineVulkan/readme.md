
# GraphicsEngineVulkan

Implementation of Vulkan backend

# Interoperability with Vulkan

Diligent Engine exposes methods to access internal Vulkan objects, is able to create diligent engine buffers
and textures from existing Vulkan handles.

## Accessing Native Vulkan handles

Below are some of the methods that provide access to internal Vulkan objects:

|                  Function                   |                              Description                                 |
|---------------------------------------------|--------------------------------------------------------------------------|
| `IBufferVk::GetVkBuffer`                    | returns a Vulkan handle of the internal buffer object                    |
| `IBufferVk::SetAccessFlags`                 | sets the access flags for the buffer                                     |
| `IBufferVk::GetAccessFlags`                 | returns the buffer access flags                                          |
| `IBufferVk::GetVkDeviceAddress`             | returns a Vulkan device address of the internal buffer object            |
| `IBufferViewVk::GetVkBufferView`            | returns a Vulkan handle of the internal buffer view object               |
| `ITextureVk::GetVkImage`                    | returns a Vulkan handle of the internal image object                     |
| `ITextureVk::SetLayout`                     | sets the Vulkan image resource layout                                    |
| `ITextureVk::GetLayout`                     | returns the Vulkan image resource layout                                 |
| `ITextureViewVk::GetVulkanImageView`        | returns Vulkan handle of the internal image view object                  |
| `IDeviceContextVk::TransitionImageLayout`   | transitions the internal Vulkan image to the specified layout            |
| `IDeviceContextVk::BufferMemoryBarrier`     | transitions the internal Vulkan buffer object to the specified state     |
| `IDeviceContextVk::GetVkCommandBuffer`      | returns the Vulkan handle of the command buffer currently being recorded |
| `IPipelineStateVk::GetRenderPass`           | returns a pointer to the internal render pass object                     |
| `IPipelineStateVk::GetVkPipeline`           | returns a Vulkan handle of the internal pipeline state object            |
| `IRenderPassVk::GetVkRenderPass`            | returns a Vulkan handle of the internal render pass object               |
| `ISamplerVk::GetVkSampler`                  | returns a Vulkan handle of the internal sampler object                   |
| `IRenderDeviceVk:GetVkDevice`               | returns a handle of the logical Vulkan device                            |
| `IRenderDeviceVk:GetVkPhysicalDevice`       | returns a handle of the physical Vulkan device                           |
| `IRenderDeviceVk:GetVkInstance`             | returns Vulkan instance handle                                           |
| `IRenderDeviceVk:GetVkVersion`              | returns Vulkan API version                                               |
| `IFenceVk::GetVkSemaphore`                  | returns the handle of the internal timeline semaphore object             |
| `IBottomLevelASVk:GetVkBLAS`                | returns a Vulkan handle of the internal bottom-level AS object           |
| `IBottomLevelASVk:GetVkDeviceAddress`       | returns a Vulkan bottom-level AS device address                          |
| `ITopLevelASVk::GetVkTLAS`                  | returns a Vulkan handle of the internal top-level AS object              |
| `ITopLevelASVk::GetVkDeviceAddress`         | returns a Vulkan handle of the internal top-level AS object              |
| `IShaderBindingTableVk::GetVkBindingTable`  | returns the data that can be used with vkCmdTraceRaysKHR call            |
| `ISwapChainVk::GetVkSwapChain`              | returns a Vulkan handle to the internal swap chain object                |
| `IFramebuffer::GetVkFramebuffer`            | returns Vulkan framebuffer object handle                                 |


## Synchronization Tools

|                              Function          |                              Description         |
|------------------------------------------------|--------------------------------------------------|
| `ICommandQueueVk::SubmitCmdBuffer`             | submits a given Vulkan command buffer to the command queue       |
| `ICommandQueueVk::Submit`                      | submits a given work batch to the internal Vulkan command queue  |
| `ICommandQueueVk::GetVkQueue`                  | returns a Vulkan handle of the internal command queue object     |
| `ICommandQueueVk::GetQueueFamilyIndex`         | returns Vulkan command queue family index                        |
| `ICommandQueueVk::EnqueueSignalFence`          | signals the given fence                                          |
| `ICommandQueueVk::EnqueueSignal`               | signals the given timeline semaphore.                            |

## Creating Diligent Engine Objects from Vk Resources

|                              Function              |                              Description                                                    |
|----------------------------------------------------|---------------------------------------------------------------------------------------------|
| `IRenderDeviceVk::CreateTextureFromVulkanImage`    | creates a Diligent Engine texture object from the native Vulkan handle                      |
| `IRenderDeviceVk::CreateBufferFromVulkanResource`  | creates a Diligent Engine buffer object from the native Vukan handle                        |
| `IRenderDeviceVk::CreateBLASFromVulkanResource`    | creates a Diligent Engine bottom-level acceleration structure from the native Vulkan handle |
| `IRenderDeviceVk::CreateTLASFromVulkanResource`    | creates a Diligent Engine top-level acceleration structure from the native Vulkan handle    |
| `IRenderDeviceVk::CreateFenceFromVulkanResource`   | creates a Diligent Engine fence from the native Vulkan handle                               |



-------------------

[diligentgraphics.com](http://diligentgraphics.com)

[![Diligent Engine on Twitter](https://github.com/DiligentGraphics/DiligentCore/blob/master/media/twitter.png)](https://twitter.com/diligentengine)
[![Diligent Engine on Facebook](https://github.com/DiligentGraphics/DiligentCore/blob/master/media/facebook.png)](https://www.facebook.com/DiligentGraphics/)
