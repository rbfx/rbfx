//
// Copyright (c) 2017-2020 the rbfx project.
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
#include "../Graphics/Graphics.h"

namespace Urho3D
{
    ///@{
    /// This classes handles load and store
    /// compiled pipeline states on disk.
    /// All pipelines created is automatically added to PipelineStateCache
    /// This helps to improve performance when building PipelineState on
    /// modern backends like Vulkan, D3D12 and Metal.
    ///@}
    class URHO3D_API PipelineStateCache : public Object, public GPUObject {
        URHO3D_OBJECT(PipelineStateCache, Object);
    public:
        PipelineStateCache(Context* context);
        /// @{
        /// Start pipeline state cache, must called at start of engine
        /// This method will load cached pipelines and create GPU objects
        /// @}
        void Init();
    private:
        bool init_;
    };
}
