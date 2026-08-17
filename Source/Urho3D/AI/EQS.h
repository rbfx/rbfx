// SPDX-License-Identifier: MIT

#pragma once

#include <Urho3D/Urho3D.h>
#include <Urho3D/AI/Blackboard.h>
#include <Urho3D/Math/Vector3.h>

#include <EASTL/functional.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>

namespace Urho3D
{

struct URHO3D_API EQSItem
{
    ea::string id;
    Vector3 position{Vector3::ZERO};
    float baseScore{1.0f};
};

struct URHO3D_API EQSResultItem
{
    EQSItem item;
    float score{};
    float distance{};
};

struct URHO3D_API EQSQueryResult
{
    bool found{};
    ea::vector<EQSResultItem> items;

    const EQSResultItem* GetBest() const { return found && !items.empty() ? &items.front() : nullptr; }
};

using EQSScoreFunction = ea::function<float(const EQSItem&, float, const Blackboard*)>;

/// Environment Query System over registered spatial candidates.
class URHO3D_API EQS
{
public:
    bool AddItem(const EQSItem& item);
    bool RemoveItem(const ea::string& id);
    void Clear();
    const ea::vector<EQSItem>& GetItems() const { return items_; }

    EQSQueryResult Query(const Vector3& origin, float radius, const Blackboard* blackboard = nullptr,
        EQSScoreFunction scoreFunction = {}, unsigned maxResults = 0) const;

private:
    ea::vector<EQSItem> items_;
};

} // namespace Urho3D
