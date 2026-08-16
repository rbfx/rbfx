#include "../Precompiled.h"

#include "LODGroup.h"

#include <EASTL/sort.h>
#include <cmath>

#include "../DebugNew.h"

namespace Urho3D
{

bool LODGroup::ValidateLevels(const ea::vector<LODLevel>& levels, ea::string* error) const
{
    for (unsigned i = 0; i < levels.size(); ++i)
    {
        if (levels[i].maxDistance < 0.0f)
        {
            if (error)
                *error = Format("LOD level {} has a negative max distance.", levels[i].index);
            return false;
        }
        for (unsigned j = 0; j < i; ++j)
        {
            if (levels[i].index == levels[j].index)
            {
                if (error)
                    *error = Format("LOD level index {} is duplicated.", levels[i].index);
                return false;
            }
        }
    }
    return true;
}

bool LODGroup::SetLevels(const ea::vector<LODLevel>& levels, ea::string* error)
{
    if (!ValidateLevels(levels, error))
        return false;

    levels_ = levels;
    ea::sort(levels_.begin(), levels_.end(), [](const LODLevel& lhs, const LODLevel& rhs)
    {
        if (lhs.maxDistance != rhs.maxDistance)
            return lhs.maxDistance < rhs.maxDistance;
        return lhs.index < rhs.index;
    });
    return true;
}

bool LODGroup::AddLevel(const LODLevel& level, ea::string* error)
{
    ea::vector<LODLevel> levels = levels_;
    levels.push_back(level);
    return SetLevels(levels, error);
}

void LODGroup::SetHysteresis(float hysteresis)
{
    hysteresis_ = Clamp(hysteresis, 0.0f, 0.5f);
}

unsigned LODGroup::FindNearestLevel(float distance) const
{
    if (levels_.empty())
        return M_MAX_UNSIGNED;
    distance = Max(distance, 0.0f);
    for (const LODLevel& level : levels_)
    {
        if (distance <= level.maxDistance)
            return level.index;
    }
    return levels_.back().index;
}

int LODGroup::FindLevelPosition(unsigned level) const
{
    for (unsigned i = 0; i < levels_.size(); ++i)
    {
        if (levels_[i].index == level)
            return static_cast<int>(i);
    }
    return -1;
}

unsigned LODGroup::Select(float distance, unsigned previousLevel) const
{
    const unsigned desiredLevel = FindNearestLevel(distance);
    if (desiredLevel == M_MAX_UNSIGNED)
        return M_MAX_UNSIGNED;

    const int desiredPosition = FindLevelPosition(desiredLevel);
    const int previousPosition = FindLevelPosition(previousLevel);
    if (previousPosition < 0 || previousPosition == desiredPosition)
        return desiredLevel;

    distance = Max(distance, 0.0f);
    const float boundary = levels_[desiredPosition < previousPosition ? desiredPosition : previousPosition].maxDistance;
    if (!std::isfinite(boundary))
        return previousLevel;

    if (desiredPosition > previousPosition)
    {
        const float enterDistance = boundary * (1.0f + hysteresis_);
        return distance > enterDistance ? desiredLevel : previousLevel;
    }

    const float leaveDistance = boundary * (1.0f - hysteresis_);
    return distance < leaveDistance ? desiredLevel : previousLevel;
}

} // namespace Urho3D
