#pragma once

#include <EASTL/vector.h>

#include "../Core/StringUtils.h"
#include "../Math/MathDefs.h"

namespace Urho3D
{

/// One distance-based level-of-detail band.
struct URHO3D_API LODLevel
{
    unsigned index{};
    float maxDistance{M_INFINITY};
};

/// Selects stable LOD levels using distance and configurable hysteresis.
class URHO3D_API LODGroup
{
public:
    /// Replace all levels. Levels are sorted by maxDistance, then by index.
    bool SetLevels(const ea::vector<LODLevel>& levels, ea::string* error = nullptr);
    /// Add one level and keep the deterministic order.
    bool AddLevel(const LODLevel& level, ea::string* error = nullptr);
    /// Remove every level.
    void ClearLevels() { levels_.clear(); }
    /// Return configured levels.
    const ea::vector<LODLevel>& GetLevels() const { return levels_; }
    /// Set hysteresis ratio in [0, 0.5].
    void SetHysteresis(float hysteresis);
    /// Return hysteresis ratio.
    float GetHysteresis() const { return hysteresis_; }
    /// Select a level. `previousLevel` may be M_MAX_UNSIGNED when there is no history.
    unsigned Select(float distance, unsigned previousLevel = M_MAX_UNSIGNED) const;

private:
    bool ValidateLevels(const ea::vector<LODLevel>& levels, ea::string* error) const;
    unsigned FindNearestLevel(float distance) const;
    int FindLevelPosition(unsigned level) const;

    ea::vector<LODLevel> levels_;
    float hysteresis_{0.05f};
};

} // namespace Urho3D
