//
// Copyright (c) 2021-2022 the rbfx project.
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

#include "../../Precompiled.h"

#include "Noise3DInstance.h"

#include "../ParticleGraphLayerInstance.h"
#include "../Span.h"
#include "../UpdateContext.h"

namespace Urho3D
{

namespace ParticleGraphNodes
{
Noise3DInstance::Noise3DInstance()
    : noise_(RandomEngine::GetDefaultEngine())
{
}

void Noise3DInstance::Init(ParticleGraphNode* node, ParticleGraphLayerInstance* layer)
{
    InstanceBase::Init(node, layer);
}

float Noise3DInstance::Generate(const Vector3& pos) const
{
    return noise_.GetDouble(pos.x_, pos.y_, pos.z_);
}

} // namespace ParticleGraphNodes

} // namespace Urho3D
