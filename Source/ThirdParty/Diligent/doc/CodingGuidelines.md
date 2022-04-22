# Includes

## Header files

Includes in the source file should go in the following order:
1. System includes and standard headers
2. Diligent Engine interface headers
3. Header for the base object implementation, if any
4. Headers of the object implementations
5. Other headers required by this header file

Example:

```cpp
// DeviceContextD3D12Impl.hpp

// 1 - System includes and standard headers
#include <unordered_map>
#include <vector>

// 2 - System includes and standard headers
#include "DeviceContextD3D12.h"

// 3 - Header for the base object implementation
#include "DeviceContextNextGenBase.hpp"

// 4 - Headers of the object implementations
#include "BufferD3D12Impl.hpp"
#include "TextureD3D12Impl.hpp"
#include "QueryD3D12Impl.hpp"
#include "FramebufferD3D12Impl.hpp"
#include "RenderPassD3D12Impl.hpp"
#include "PipelineStateD3D12Impl.hpp"
#include "D3D12DynamicHeap.hpp"
#include "BottomLevelASD3D12Impl.hpp"
#include "TopLevelASD3D12Impl.hpp"

// 5 - Other headers
#include "D3D12DynamicHeap.hpp"
```

## Source files

Includes in the source file should go in the following order:

1. Precompiled header (if any)
2. The source file's header
3. System includes and standard headers
4. Diligent Engine interface headers
5. Headers of the object implementations
6. Other headers required by this source file

Example:

```cpp
// DeviceContextD3D12Impl.cpp


// 1 - Precompiled header
#include "pch.h"

// 2 - The source file's header
#include "DeviceContextD3D12Impl.hpp"

// 3 - System includes and standard headers
#include <sstream>

// 4 - Interface headers
#include "RenderDeviceD3D12.hpp"
#include "DeviceContextD3D12.hpp"

// 5 - Headers of the object implementations
#include "RenderDeviceD3D12Impl.hpp" // Render device always goes first, if any
#include "PipelineStateD3D12Impl.hpp"
#include "TextureD3D12Impl.hpp"
#include "BufferD3D12Impl.hpp"
#include "FenceD3D12Impl.hpp"
#include "ShaderBindingTableD3D12Impl.hpp"
#include "ShaderResourceBindingD3D12Impl.hpp"
#include "CommandListD3D12Impl.hpp"

// 6 - Other headers required by this source file
#include "CommandContext.hpp"
#include "D3D12TypeConversions.hpp"
#include "d3dx12_win.h"
#include "D3D12DynamicHeap.hpp"
#include "DXGITypeConversions.hpp"
```

When there is more than one header in each group, it is recommended to separate the groups with blank lines.


# Exceptions


# Naming conventions


# Debug macros
