
# Shader Resource Variables

Diligent Engine uses three variable types:

- *Static* variables can be set only once in each pipeline state object or pipeline resource signature.
  Once bound, the resource can't be changed.
- *Mutable* variables can be set only once in each shader resource binding instance.
  Once bound, the resource can't be changed.
- *Dynamic* variables can be bound any number of times.

Internally, static and mutable variables are implemented in the same way. Static variable bindings are copied
from PSO or Signature to the SRB either when the SRB is created or when `IPipelineState::InitializeStaticSRBResources()`
method is called.

Dynamic variables introduce some overhead every time an SRB is committed (even if no bindings have changed). Prefer static/mutable
variables over dynamic ones whenever possible.


# Dynamic Buffers

`USAGE_DYNAMIC` buffers introduce some overhead in every draw or dispatch command that uses an SRB with dynamic buffers.
Note that extra work is performed even if none of the dynamic buffers have been mapped between the commands. If this
is the case, an application should use `DRAW_FLAG_DYNAMIC_RESOURCE_BUFFERS_INTACT` flag to tell the engine that
none of the dynamic buffers have been updated between the commands. Note that the first time an SRB is bound,
dynamic buffers are properly bound regardless of the `DRAW_FLAG_DYNAMIC_RESOURCE_BUFFERS_INTACT` flag.

Constant and structured buffers set with `IShaderResourceBinding::SetBufferRange` method count as
dynamic buffers when the specified range does not cover the entire buffer *regardless of the buffer usage*,
unless the variable was created with `NO_DYNAMIC_BUFFERS` flag. Note that in the latter case,
`IShaderResourceBinding::SetBufferOffset` method can't be used to set dynamic offset.
Similar to `USAGE_DYNAMIC` buffers, if an applications knows that none of the dynamic offsets have changed
between the draw calls, it may use `DRAW_FLAG_DYNAMIC_RESOURCE_BUFFERS_INTACT` flag.

An application should try to use as few dynamic buffers as possible. On some implementations, the number of dynamic
buffers that can be used by a PSO may be limited by as few as 8 buffers. If an application knows that no dynamic buffers
will be bound to a shader resource variable, it should use the `SHADER_VARIABLE_FLAG_NO_DYNAMIC_BUFFERS` flag when defining
the variables through the PSO layout or `PIPELINE_RESOURCE_FLAG_NO_DYNAMIC_BUFFERS` when defining the variable
through pipeline resource signature. It is an error to bind `USAGE_DYNAMIC` buffer to a variable that was
created with `NO_DYNAMIC_BUFFERS` flag. Likewise, it is an error to set dynamic offset for such variable.

Try to optimize dynamic buffers usage, but don't strive to avoid them as they are usually the fastest way
to upload frequently changing data to the GPU.


# Render pass

Use `IRenderPass` for better performance on mobile GPUs, including ARM-based laptops and desktops.

Vulkan backend natively supports render pass subpasses. The subpasses allow rendering multiple passes in on-chip memory
without the need to load and store data to the global memory. This is faster, saves memory bandwidth and reduces power consumption.

Metal backend and Vulkan on top of Metal don't natively support subpasses. To leverage the on-chip memory, use
[Imageblocks](https://developer.apple.com/documentation/metal/gpu_features/understanding_gpu_family_4/about_imageblocks?language=objc),
[Tile shader](https://developer.apple.com/documentation/metal/gpu_features/understanding_gpu_family_4/about_tile_shading?language=objc), and
[Raster order groups](https://developer.apple.com/documentation/metal/gpu_features/understanding_gpu_family_4/about_raster_order_groups?language=objc).


# Profilers

* [RenderDoc](https://renderdoc.org/) - Direct3D11/Direct3D12/OpenGL/Vulkan debugging tool
* [NVidia NSight](https://developer.nvidia.com/nsight-graphics) - Vulkan & Direct3D12 profiling and debugging tool for NVidia GPUs
* [AMD GPU Profiler](https://gpuopen.com/rgp/) - Vulkan & Direct3D12 profiling and debugging tool for AMD GPUs
* [Microsoft PIX](https://devblogs.microsoft.com/pix/download/) - Direct3D11/Direct3D12 profiler and debugger
* [XCode](https://developer.apple.com/xcode/) - GPU profiler & debugger for Metal API (including Vulkan on top of Metal)
* [ARM Mobile Studio](https://www.arm.com/products/development-tools/graphics/arm-mobile-Studio) - profiler for Mali GPUs
* [Snapdragon Profiler](https://developer.qualcomm.com/software/snapdragon-profiler) - profiler for Adreno GPUs
* [Android GPU Inspector](https://gpuinspector.dev/) - debugger & profiler by Google


# References

* [DirectX12 Do's And Don'ts](https://developer.nvidia.com/dx12-dos-and-donts) by NVidia
* [Vulkan Do's And Don'ts](https://developer.nvidia.com/blog/vulkan-dos-donts/) by NVidia
* [RDNA2 performance guide](https://gpuopen.com/performance/) by AMD
* [Arm Mali GPU Best Practices Developer Guide Version](https://developer.arm.com/documentation/101897/0201/Preface) by ARM
* [Arm Mali GPU Training Series](https://www.youtube.com/watch?v=tnR4mExVClY&list=PLKjl7IFAwc4QUTejaX2vpIwXstbgf8Ik7) video by ARM
* [Adreno GPU](https://developer.qualcomm.com/sites/default/files/docs/adreno-gpu/developer-guide//gpu/gpu.html) by Qualcomm
* [PowerVR](http://cdn.imgtec.com/sdk-documentation/PowerVR_Performance_Recommendations.pdf) by Imagination Technologies
* [Metal Best Practices Guide](https://developer.apple.com/library/archive/documentation/3DDrawing/Conceptual/MTLBestPracticesGuide/index.html) by Apple
* [Apple Tech Talks - Graphics & Games](https://developer.apple.com/videos/graphics-games)
