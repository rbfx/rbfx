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

#include "ParticleGraphSystem.h"

#include "ParticleGraphNode.h"
#include "ParticleGraphEmitter.h"
#include "ParticleGraphLayer.h"
#include "../IO/Log.h"
#include "../Graphics/Material.h"
#include "../Graphics/Model.h"

namespace Urho3D
{
namespace ParticleGraphNodes
{

void RegisterGraphNodes(ParticleGraphSystem* system);

}

ParticleGraphSystem::ParticleGraphSystem(Context* context)
    : Object(context)
    , ObjectReflectionRegistry(context)
{
    RegisterParticleGraphLibrary(context, this);
}

ParticleGraphSystem::~ParticleGraphSystem()
{
}

//ParticleGraphSystem::~ParticleGraphSystem() { particleNodeFactories_.clear(); }
//
//SharedPtr<ParticleGraphNode> ParticleGraphSystem::CreateParticleGraphNode(StringHash objectType)
//{
//    auto i = particleNodeFactories_.find(objectType);
//    if (i != particleNodeFactories_.end())
//    {
//        SharedPtr<ParticleGraphNode> node;
//        node.StaticCast(i->second->CreateObject());
//        return node;
//    }
//    return SharedPtr<ParticleGraphNode>();
//}
//
//void ParticleGraphSystem::RegisterParticleGraphNodeFactory(ObjectFactory* factory)
//{
//    if (!factory)
//        return;
//
//    auto it = particleNodeFactories_.find(factory->GetType());
//    if (it != particleNodeFactories_.end())
//    {
//        URHO3D_LOGERRORF("Failed to register '%s' because type '%s' is already registered with same type hash.",
//                         factory->GetTypeName().c_str(), it->second->GetTypeName().c_str());
//        assert(false);
//        return;
//    }
//    particleNodeFactories_[factory->GetType()] = factory;
//}
//
//void ParticleGraphSystem::RegisterAttribute(StringHash objectType, const AttributeInfo& attr)
//{
//    ea::vector<AttributeInfo>& objectAttributes = attributes_[objectType];
//    objectAttributes.push_back(attr);
//}
//
//AttributeInfo* ParticleGraphSystem::GetAttribute(StringHash objectType, StringHash nameHash)
//{
//    auto i = attributes_.find(objectType);
//    if (i == attributes_.end())
//        return nullptr;
//
//    ea::vector<AttributeInfo>& infos = i->second;
//
//    for (auto j = infos.begin(); j != infos.end(); ++j)
//    {
//        if (j->nameHash_ == nameHash)
//            return &(*j);
//    }
//
//    return nullptr;
//}

void RegisterParticleGraphLibrary(Context* context, ParticleGraphSystem* system)
{
    ParticleGraphEffect::RegisterObject(context);
    ParticleGraphLayer::RegisterObject(context);
    ParticleGraphEmitter::RegisterObject(context);

    ParticleGraphNodes::RegisterGraphNodes(system);
}

} // namespace Urho3D
