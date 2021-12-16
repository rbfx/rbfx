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
class URHO3D_API ParticleGraphSystem : public Object, public ObjectReflectionRegistry
{
    URHO3D_OBJECT(ParticleGraphSystem, Object);

public:
    ParticleGraphSystem(Context* context);

    ~ParticleGraphSystem() override;

    ///// Create a particle graph node by type hash. Return pointer to it or null if no factory found.
    //SharedPtr<ParticleGraphNode> CreateParticleGraphNode(StringHash objectType);

    ///// Register a factory for an particle graph node type.
    //void RegisterParticleGraphNodeFactory(ObjectFactory* factory);

    ///// Template version of registering a particle node factory.
    //template <class T = void, class... Rest> void RegisterParticleGraphNodeFactory();

    ///// Register object attribute.
    //void RegisterAttribute(StringHash objectType, const AttributeInfo& attr);
    ///// Template version of registering an object attribute.
    //template <class T> void RegisterAttribute(const AttributeInfo& attr);

    ///// Return a specific attribute description for an object, or null if not found.
    //AttributeInfo* GetAttribute(StringHash objectType, StringHash nameHash);
    ///// Template version of returning a specific attribute description.
    //template <class T> AttributeInfo* GetAttribute(StringHash nameHash);

    ///// Return attribute descriptions for an object type, or null if none defined.
    //const ea::vector<AttributeInfo>* GetAttributes(StringHash type) const
    //{
    //    auto i = attributes_.find(type);
    //    return i != attributes_.end() ? &i->second : nullptr;
    //}

private:
    /*/// Particle graph node factories.
    FactoryMap particleNodeFactories_;
    /// Attribute descriptions per object type.
    ea::unordered_map<StringHash, ea::vector<AttributeInfo>> attributes_;*/
};
//
//// Helper function that terminate looping of argument list.
//template <> inline void ParticleGraphSystem::RegisterParticleGraphNodeFactory() {}
//
//template <class T, class... Rest> void ParticleGraphSystem::RegisterParticleGraphNodeFactory()
//{
//    RegisterParticleGraphNodeFactory(new ObjectFactoryImpl<T>(context_));
//    RegisterParticleGraphNodeFactory<Rest...>();
//}
//
//template <class T> void ParticleGraphSystem::RegisterAttribute(const AttributeInfo& attr)
//{
//    //AttributeHandle
//    RegisterAttribute(T::GetTypeStatic(), attr);
//}
//template <class T> AttributeInfo* ParticleGraphSystem::GetAttribute(StringHash nameHash)
//{
//    return GetAttribute(T::GetTypeStatic(), nameHash);
//}


/// Register Graphics library objects.
/// @nobind
void URHO3D_API RegisterParticleGraphLibrary(Context* context, ParticleGraphSystem* system);

}
