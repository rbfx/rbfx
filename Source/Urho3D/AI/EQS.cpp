// SPDX-License-Identifier: MIT

#include "EQS.h"

#include <algorithm>
#include <cmath>

namespace Urho3D
{

bool EQS::AddItem(const EQSItem& item)
{
    if (item.id.empty() || !std::isfinite(item.position.x_) || !std::isfinite(item.position.y_) || !std::isfinite(item.position.z_))
        return false;
    for (EQSItem& existing : items_)
    {
        if (existing.id == item.id)
        {
            existing = item;
            return true;
        }
    }
    items_.push_back(item);
    return true;
}

bool EQS::RemoveItem(const ea::string& id)
{
    const auto iter = std::find_if(items_.begin(), items_.end(), [&](const EQSItem& item) { return item.id == id; });
    if (iter == items_.end())
        return false;
    items_.erase(iter);
    return true;
}

void EQS::Clear()
{
    items_.clear();
}

EQSQueryResult EQS::Query(const Vector3& origin, float radius, const Blackboard* blackboard,
    EQSScoreFunction scoreFunction, unsigned maxResults) const
{
    EQSQueryResult result;
    if (radius < 0.0f)
        return result;
    const float radiusSquared = radius * radius;
    for (const EQSItem& item : items_)
    {
        const float distanceSquared = (item.position - origin).LengthSquared();
        if (distanceSquared > radiusSquared)
            continue;
        const float distance = std::sqrt(distanceSquared);
        const float defaultScore = item.baseScore * (radius > M_EPSILON ? 1.0f - distance / radius : 1.0f);
        const float score = scoreFunction ? scoreFunction(item, distance, blackboard) : defaultScore;
        if (std::isfinite(score))
            result.items.push_back({item, score, distance});
    }
    std::sort(result.items.begin(), result.items.end(), [](const EQSResultItem& lhs, const EQSResultItem& rhs)
    {
        if (lhs.score != rhs.score)
            return lhs.score > rhs.score;
        if (lhs.distance != rhs.distance)
            return lhs.distance < rhs.distance;
        return lhs.item.id < rhs.item.id;
    });
    if (maxResults != 0 && result.items.size() > maxResults)
        result.items.resize(maxResults);
    result.found = !result.items.empty();
    return result;
}

} // namespace Urho3D
