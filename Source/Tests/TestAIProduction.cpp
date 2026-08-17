// SPDX-License-Identifier: MIT

#include <Urho3D/AI/BehaviorTree.h>
#include <Urho3D/AI/Blackboard.h>
#include <Urho3D/AI/EQS.h>
#include <Urho3D/AI/PerceptionSystem.h>
#include <Urho3D/AI/StateTree.h>
#include <Urho3D/Blueprint/BlueprintRuntime.h>

#include <catch2/catch_amalgamated.hpp>

using namespace Urho3D;

TEST_CASE("Blackboard stores typed values and emits deterministic changes", "[ai][blackboard]")
{
    Blackboard blackboard;
    ea::vector<ea::string> changedKeys;
    const unsigned callback = blackboard.BindOnChanged([&](const BlackboardChange& change) { changedKeys.push_back(change.key); });
    REQUIRE(callback != 0);
    REQUIRE(blackboard.Set("health", Variant(100)));
    CHECK_FALSE(blackboard.Set("health", Variant(100)));
    REQUIRE(blackboard.Set("health", Variant(75)));
    REQUIRE(blackboard.Remove("health"));
    CHECK(blackboard.Get("health").IsEmpty());
    CHECK(changedKeys == ea::vector<ea::string>{"health", "health", "health"});
    CHECK(blackboard.UnbindOnChanged(callback));
}

TEST_CASE("BehaviorTree executes sequence and selector composites", "[ai][behavior-tree]")
{
    BehaviorTree tree;
    unsigned calls = 0;
    const unsigned sequence = tree.AddNode("Sequence", BehaviorNodeType::Sequence);
    const unsigned first = tree.AddNode("First", BehaviorNodeType::Task,
        [&](BehaviorTreeTickContext&) { ++calls; return calls == 1 ? BehaviorStatus::Running : BehaviorStatus::Success; });
    const unsigned second = tree.AddNode("Second", BehaviorNodeType::Task,
        [](BehaviorTreeTickContext&) { return BehaviorStatus::Success; });
    REQUIRE(tree.AddChild(sequence, first));
    REQUIRE(tree.AddChild(sequence, second));
    REQUIRE(tree.SetRoot(sequence));

    BehaviorTreeTickContext context;
    CHECK(tree.Tick(context) == BehaviorStatus::Running);
    CHECK(tree.Tick(context) == BehaviorStatus::Success);
    CHECK(calls == 2);

    BehaviorTree selector;
    const unsigned selectorRoot = selector.AddNode("Selector", BehaviorNodeType::Selector);
    const unsigned failure = selector.AddNode("Failure", BehaviorNodeType::Task,
        [](BehaviorTreeTickContext&) { return BehaviorStatus::Failure; });
    const unsigned success = selector.AddNode("Success", BehaviorNodeType::Task,
        [](BehaviorTreeTickContext&) { return BehaviorStatus::Success; });
    REQUIRE(selector.AddChild(selectorRoot, failure));
    REQUIRE(selector.AddChild(selectorRoot, success));
    REQUIRE(selector.SetRoot(selectorRoot));
    CHECK(selector.Tick(context) == BehaviorStatus::Success);
}

TEST_CASE("PerceptionSystem filters stimuli by sense, radius and TTL", "[ai][perception]")
{
    PerceptionSystem perception;
    perception.SetObserverPosition(Vector3::ZERO);
    perception.SetSightRadius(10.0f);
    perception.SetHearingRadius(20.0f);
    REQUIRE(perception.AddStimulus({"enemy", PerceptionSense::Sight, "Enemy", Vector3(5.0f, 0.0f, 0.0f), 0.5f, 0.0f, 2.0f, true}));
    REQUIRE(perception.AddStimulus({"noise", PerceptionSense::Hearing, "Door", Vector3(15.0f, 0.0f, 0.0f), 1.0f, 0.0f, 0.5f, true}));
    REQUIRE(perception.AddStimulus({"far", PerceptionSense::Sight, "Far", Vector3(50.0f, 0.0f, 0.0f), 2.0f, 0.0f, 2.0f, true}));
    CHECK(perception.Query(PerceptionSense::Sight).size() == 1);
    CHECK(perception.Query(PerceptionSense::Hearing).size() == 1);
    CHECK(perception.Update(0.6f) == 1);
    CHECK(perception.Query(PerceptionSense::Hearing).empty());
}

TEST_CASE("EQS ranks candidates by score and distance", "[ai][eqs]")
{
    EQS eqs;
    REQUIRE(eqs.AddItem({"near", Vector3(2.0f, 0.0f, 0.0f), 1.0f}));
    REQUIRE(eqs.AddItem({"far", Vector3(8.0f, 0.0f, 0.0f), 1.0f}));
    REQUIRE(eqs.AddItem({"outside", Vector3(20.0f, 0.0f, 0.0f), 10.0f}));
    Blackboard blackboard;
    REQUIRE(blackboard.Set("bias", Variant(2.0f)));
    const EQSQueryResult result = eqs.Query(Vector3::ZERO, 10.0f, &blackboard,
        [](const EQSItem& item, float distance, const Blackboard* values)
        {
            const float bias = values ? values->Get("bias").GetFloat() : 0.0f;
            return item.baseScore * bias - distance;
        });
    REQUIRE(result.found);
    REQUIRE(result.items.size() == 2);
    CHECK(result.GetBest()->item.id == "near");
}

TEST_CASE("StateTree enters hierarchy and transitions from leaf conditions", "[ai][state-tree]")
{
    StateTree tree;
    ea::vector<ea::string> events;
    REQUIRE(tree.AddState({"Combat", {},
        [&](Blackboard&, float) { events.push_back("enter-combat"); },
        [&](Blackboard&, float) { events.push_back("exit-combat"); }, {}}));
    REQUIRE(tree.AddState({"Attack", "Combat",
        [&](Blackboard&, float) { events.push_back("enter-attack"); },
        [&](Blackboard&, float) { events.push_back("exit-attack"); },
        [&](Blackboard&, float) { events.push_back("tick-attack"); }}));
    REQUIRE(tree.AddState({"Recover", "Combat",
        [&](Blackboard&, float) { events.push_back("enter-recover"); }, {}, {}}));
    REQUIRE(tree.AddTransition({"Attack", "Recover", [](const Blackboard& blackboard)
    {
        return blackboard.Get("done").GetBool();
    }}));
    REQUIRE(tree.SetInitialState("Attack"));

    Blackboard blackboard;
    REQUIRE(tree.Start(blackboard));
    CHECK(tree.GetCurrentState() == "Attack");
    CHECK(events == ea::vector<ea::string>{"enter-combat", "enter-attack"});
    REQUIRE(tree.Tick(blackboard, 0.016f));
    REQUIRE(blackboard.Set("done", Variant(true)));
    REQUIRE(tree.Tick(blackboard, 0.016f));
    CHECK(tree.GetCurrentState() == "Recover");
    CHECK(events.back() == "enter-recover");
}

TEST_CASE("Blueprint runtime exposes gameplay AI services and nodes", "[ai][blueprint]")
{
    BlueprintRuntime runtime;
    Blackboard blackboard;
    BehaviorTree behaviorTree;
    EQS eqs;
    PerceptionSystem perception;
    StateTree stateTree;
    runtime.SetBlackboard(&blackboard);
    runtime.SetBehaviorTree(&behaviorTree);
    runtime.SetEQS(&eqs);
    runtime.SetPerceptionSystem(&perception);
    runtime.SetStateTree(&stateTree);

    CHECK(runtime.GetBlackboard() == &blackboard);
    CHECK(runtime.GetBehaviorTree() == &behaviorTree);
    CHECK(runtime.GetEQS() == &eqs);
    CHECK(runtime.GetPerceptionSystem() == &perception);
    CHECK(runtime.GetStateTree() == &stateTree);
    CHECK(runtime.GetRegistry().Find("AI.RunBehaviorTree") != nullptr);
    CHECK(runtime.GetRegistry().Find("AI.SetBlackboardValue") != nullptr);
    CHECK(runtime.GetRegistry().Find("AI.GetBlackboardValue") != nullptr);
    CHECK(runtime.GetRegistry().Find("AI.QueryEQS") != nullptr);
}
