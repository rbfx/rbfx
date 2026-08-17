// SPDX-License-Identifier: MIT

#pragma once

#include <Urho3D/Urho3D.h>
#include <Urho3D/Math/Color.h>
#include <Urho3D/Math/Vector3.h>

#include <EASTL/string.h>
#include <EASTL/vector.h>

namespace Urho3D
{

enum class VFXSimulationMode
{
    CPU,
    GPU
};

enum class VFXNodeType
{
    SpawnRate,
    InitialVelocity,
    Force,
    Drag,
    ColorOverLife,
    SizeOverLife,
    RibbonTrail,
    Output
};

struct URHO3D_API VFXNode
{
    unsigned id{};
    ea::string name;
    VFXNodeType type{VFXNodeType::SpawnRate};
    Vector3 vectorValue{Vector3::ZERO};
    float scalarValue{};
};

struct URHO3D_API VFXParticle
{
    Vector3 position{Vector3::ZERO};
    Vector3 velocity{Vector3::ZERO};
    Color color{Color::WHITE};
    float size{1.0f};
    float age{};
    float lifetime{1.0f};
};

struct URHO3D_API VFXRibbonPoint
{
    Vector3 position{Vector3::ZERO};
    Color color{Color::WHITE};
    float age{};
};

/// Production VFX graph data and bounded runtime simulation with ribbon trails.
class URHO3D_API VFXGraph
{
public:
    unsigned AddNode(const ea::string& name, VFXNodeType type);
    bool RemoveNode(unsigned nodeId);
    bool SetOutputNode(unsigned nodeId);
    VFXNode* GetNode(unsigned nodeId);
    const VFXNode* GetNode(unsigned nodeId) const;
    bool Compile(ea::string* error = nullptr) const;

    void SetSimulationMode(VFXSimulationMode mode) { simulationMode_ = mode; }
    VFXSimulationMode GetSimulationMode() const { return simulationMode_; }
    void SetMaxParticles(unsigned maxParticles);
    unsigned GetMaxParticles() const { return maxParticles_; }
    void SetSpawnRate(float particlesPerSecond);
    void SetParticleLifetime(float seconds);
    void SetInitialVelocity(const Vector3& velocity);
    void SetForce(const Vector3& force);
    void SetDrag(float drag);
    void SetRibbonTrailLength(unsigned points);
    unsigned GetRibbonTrailLength() const { return ribbonTrailLength_; }

    bool Play();
    bool Stop();
    void ClearParticles();
    void Update(float deltaSeconds);
    const ea::vector<VFXParticle>& GetParticles() const { return particles_; }
    const ea::vector<VFXRibbonPoint>& GetRibbonPoints() const { return ribbonPoints_; }
    bool IsPlaying() const { return playing_; }

private:
    void SpawnParticle();
    void UpdateRibbons(float deltaSeconds);

    ea::vector<VFXNode> nodes_;
    unsigned outputNodeId_{};
    VFXSimulationMode simulationMode_{VFXSimulationMode::CPU};
    unsigned maxParticles_{1024};
    float spawnRate_{};
    float spawnAccumulator_{};
    float particleLifetime_{1.0f};
    Vector3 initialVelocity_{Vector3::ZERO};
    Vector3 force_{Vector3::ZERO};
    float drag_{};
    unsigned ribbonTrailLength_{};
    bool playing_{};
    ea::vector<VFXParticle> particles_;
    ea::vector<VFXRibbonPoint> ribbonPoints_;
};

} // namespace Urho3D
