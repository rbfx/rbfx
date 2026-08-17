// SPDX-License-Identifier: MIT

#include "VFXGraph.h"

#include <algorithm>

namespace Urho3D
{

unsigned VFXGraph::AddNode(const ea::string& name, VFXNodeType type)
{
    if (name.empty())
        return 0;
    VFXNode node;
    node.id = nodes_.size() + 1;
    node.name = name;
    node.type = type;
    nodes_.push_back(node);
    if (type == VFXNodeType::Output)
        outputNodeId_ = node.id;
    return node.id;
}

bool VFXGraph::RemoveNode(unsigned nodeId)
{
    VFXNode* node = GetNode(nodeId);
    if (!node)
        return false;
    node->id = 0;
    node->name.clear();
    if (outputNodeId_ == nodeId)
        outputNodeId_ = 0;
    return true;
}

bool VFXGraph::SetOutputNode(unsigned nodeId)
{
    const VFXNode* node = GetNode(nodeId);
    if (!node || node->type != VFXNodeType::Output)
        return false;
    outputNodeId_ = nodeId;
    return true;
}

VFXNode* VFXGraph::GetNode(unsigned nodeId)
{
    return const_cast<VFXNode*>(static_cast<const VFXGraph*>(this)->GetNode(nodeId));
}

const VFXNode* VFXGraph::GetNode(unsigned nodeId) const
{
    if (nodeId == 0 || nodeId > nodes_.size() || nodes_[nodeId - 1].id != nodeId)
        return nullptr;
    return &nodes_[nodeId - 1];
}

bool VFXGraph::Compile(ea::string* error) const
{
    if (outputNodeId_ == 0 || !GetNode(outputNodeId_))
    {
        if (error)
            *error = "VFXGraph requires an Output node.";
        return false;
    }
    if (maxParticles_ == 0)
    {
        if (error)
            *error = "VFXGraph max particle count must be greater than zero.";
        return false;
    }
    return true;
}

void VFXGraph::SetMaxParticles(unsigned maxParticles)
{
    maxParticles_ = std::max(1u, maxParticles);
    if (particles_.size() > maxParticles_)
        particles_.resize(maxParticles_);
}

void VFXGraph::SetSpawnRate(float particlesPerSecond)
{
    spawnRate_ = std::max(0.0f, particlesPerSecond);
}

void VFXGraph::SetParticleLifetime(float seconds)
{
    particleLifetime_ = std::max(0.001f, seconds);
}

void VFXGraph::SetInitialVelocity(const Vector3& velocity)
{
    initialVelocity_ = velocity;
}

void VFXGraph::SetForce(const Vector3& force)
{
    force_ = force;
}

void VFXGraph::SetDrag(float drag)
{
    drag_ = std::max(0.0f, drag);
}

void VFXGraph::SetRibbonTrailLength(unsigned points)
{
    ribbonTrailLength_ = points;
    if (ribbonTrailLength_ == 0)
        ribbonPoints_.clear();
    else if (ribbonPoints_.size() > ribbonTrailLength_)
        ribbonPoints_.erase(ribbonPoints_.begin(), ribbonPoints_.end() - ribbonTrailLength_);
}

bool VFXGraph::Play()
{
    if (!Compile())
        return false;
    playing_ = true;
    return true;
}

bool VFXGraph::Stop()
{
    if (!playing_)
        return false;
    playing_ = false;
    return true;
}

void VFXGraph::ClearParticles()
{
    particles_.clear();
    ribbonPoints_.clear();
    spawnAccumulator_ = 0.0f;
}

void VFXGraph::Update(float deltaSeconds)
{
    if (!playing_ || deltaSeconds <= 0.0f)
        return;
    spawnAccumulator_ += spawnRate_ * deltaSeconds;
    while (spawnAccumulator_ >= 1.0f && particles_.size() < maxParticles_)
    {
        SpawnParticle();
        spawnAccumulator_ -= 1.0f;
    }
    for (VFXParticle& particle : particles_)
    {
        particle.velocity += force_ * deltaSeconds;
        const float damping = std::max(0.0f, 1.0f - drag_ * deltaSeconds);
        particle.velocity *= damping;
        particle.position += particle.velocity * deltaSeconds;
        particle.age += deltaSeconds;
        const float life = std::min(1.0f, particle.age / particle.lifetime);
        particle.color.a_ = 1.0f - life;
        particle.size = 1.0f - 0.5f * life;
    }
    particles_.erase(std::remove_if(particles_.begin(), particles_.end(),
        [](const VFXParticle& particle) { return particle.age >= particle.lifetime; }), particles_.end());
    UpdateRibbons(deltaSeconds);
}

void VFXGraph::SpawnParticle()
{
    VFXParticle particle;
    particle.velocity = initialVelocity_;
    particle.lifetime = particleLifetime_;
    particles_.push_back(particle);
}

void VFXGraph::UpdateRibbons(float deltaSeconds)
{
    if (ribbonTrailLength_ == 0 || particles_.empty())
        return;
    const VFXParticle& source = particles_.front();
    ribbonPoints_.push_back({source.position, source.color, 0.0f});
    for (VFXRibbonPoint& point : ribbonPoints_)
        point.age += deltaSeconds;
    while (ribbonPoints_.size() > ribbonTrailLength_)
        ribbonPoints_.erase(ribbonPoints_.begin());
}

} // namespace Urho3D
