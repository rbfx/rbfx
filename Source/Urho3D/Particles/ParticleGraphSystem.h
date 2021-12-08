//
// Copyright (c) 2021 the rbfx project.
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

#include "ParticleGraphNode.h"
#include "../Core/Object.h"
#include "../Core/Context.h"

namespace Urho3D
{
/// %Particle graph effect definition.
class URHO3D_API ParticleGraphSystem : public Object
{
    URHO3D_OBJECT(ParticleGraphSystem, Object);

    typedef ea::unordered_map<StringHash, SharedPtr<ObjectFactory>> FactoryMap;

public:
    ParticleGraphSystem(Context* context);

    ~ParticleGraphSystem() override;

    /// Create a particle graph node by type hash. Return pointer to it or null if no factory found.
    SharedPtr<ParticleGraphNode> CreateParticleGraphNode(StringHash objectType);

    /// Register a factory for an particle graph node type.
    void RegisterParticleGraphNodeFactory(ObjectFactory* factory);

    /// Template version of registering a particle node factory.
    template <class T = void, class... Rest> void RegisterParticleGraphNodeFactory();
private:
    /// Particle graph node factories.
    FactoryMap particleNodeFactories_;
};

// Helper function that terminate looping of argument list.
template <> inline void ParticleGraphSystem::RegisterParticleGraphNodeFactory() {}

template <class T, class... Rest> void ParticleGraphSystem::RegisterParticleGraphNodeFactory()
{
    RegisterParticleGraphNodeFactory(new ObjectFactoryImpl<T>(context_));
    RegisterParticleGraphNodeFactory<Rest...>();
}

/// Register Graphics library objects.
/// @nobind
void URHO3D_API RegisterParticleGraphLibrary(Context* context, ParticleGraphSystem* system);


}
