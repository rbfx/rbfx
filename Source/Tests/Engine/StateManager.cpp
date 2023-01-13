//
// Copyright (c) 2022-2022 the rbfx project.
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
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR rhs
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR rhsWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR rhs DEALINGS IN
// THE SOFTWARE.
//


#include "../CommonUtils.h"
#include <Urho3D/Engine/StateManager.h>
#include <Urho3D/Engine/StateManagerEvents.h>

namespace
{
struct EventMatcher
{
    EventMatcher(StringHash eventType, StringHash from, StringHash to)
    {
        eventType_ = eventType;
        from_ = from;
        to_ = to;
    }
    EventMatcher(StringHash eventType, VariantMap& data)
    {
        eventType_ = eventType;
        using namespace LeavingApplicationState;
        from_ = data[P_FROM].GetStringHash();
        to_ = data[P_TO].GetStringHash();
    }

    bool operator==(const EventMatcher& rhs) const
    {
        return eventType_ == rhs.eventType_ && from_ == rhs.from_ && to_ == rhs.to_;
    }
    bool operator!=(const EventMatcher& rhs) const { return !(*this == rhs); }

    StringHash eventType_;
    StringHash from_;
    StringHash to_;
};
class State1: public ApplicationState
{
    URHO3D_OBJECT(State1, ApplicationState);
public:
    State1(Context* context)
        : BaseClassName(context)
    {
    }
};

class State2 : public ApplicationState
{
    URHO3D_OBJECT(State2, ApplicationState);
public:
    State2(Context* context)
        : BaseClassName(context)
    {
    }
};

template <typename T, typename AllocComp, typename AllocMatch>
struct EaEqualsMatcher final : Catch::Matchers::MatcherBase<ea::vector<T, AllocMatch>>
{

    EaEqualsMatcher(ea::vector<T, AllocComp> const& comparator)
        : m_comparator(comparator)
    {
    }

    bool match(ea::vector<T, AllocMatch> const& v) const override
    {
        // !TBD: This currently works if all elements can be compared using !=
        // - a more general approach would be via a compare template that defaults
        // to using !=. but could be specialised for, e.g. std::vector<T> etc
        // - then just call that directly
        if (m_comparator.size() != v.size())
            return false;
        for (std::size_t i = 0; i < v.size(); ++i)
            if (!(m_comparator[i] == v[i]))
                return false;
        return true;
    }
    std::string describe() const override { return "Equals: " + ::Catch::Detail::stringify(m_comparator); }
    ea::vector<T, AllocComp> const& m_comparator;
};

template <typename T, typename AllocComp = std::allocator<T>, typename AllocMatch = AllocComp>
EaEqualsMatcher<T, AllocComp, AllocMatch> EaEquals(ea::vector<T, AllocComp> const& comparator)
{
    return EaEqualsMatcher<T, AllocComp, AllocMatch>(comparator);
}

}

TEST_CASE("StateManager: Enque state, step and reset")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto guard = Tests::MakeScopedReflection<State1, State2>(context);

    auto* stateManager = context->GetSubsystem<StateManager>();
    stateManager->Reset();

    Serializable subscriber(context);
    ea::vector<EventMatcher> events;
    auto callback = [&](StringHash eventType, VariantMap& data) { events.push_back(EventMatcher(eventType, data)); };
    subscriber.SubscribeToEvent(E_STATETRANSITIONSTARTED, callback);
    subscriber.SubscribeToEvent(E_ENTERINGAPPLICATIONSTATE, callback);
    subscriber.SubscribeToEvent(E_LEAVINGAPPLICATIONSTATE, callback);
    subscriber.SubscribeToEvent(E_STATETRANSITIONCOMPLETE, callback);

    stateManager->EnqueueState(State1::GetTypeStatic());

    Tests::RunFrame(context, 0.1f);

    CHECK_THAT(events,
        EaEquals(ea::vector<EventMatcher>{
            EventMatcher(E_STATETRANSITIONSTARTED, StringHash::Empty, State1::GetTypeStatic()),
            EventMatcher(E_ENTERINGAPPLICATIONSTATE, StringHash::Empty, State1::GetTypeStatic()),
            EventMatcher(E_STATETRANSITIONCOMPLETE, StringHash::Empty, State1::GetTypeStatic())
        }));

    events.clear();
    stateManager->Reset();

    CHECK_THAT(events,
        EaEquals(
            ea::vector<EventMatcher>{
            EventMatcher(E_STATETRANSITIONSTARTED, State1::GetTypeStatic(), StringHash::Empty),
            EventMatcher(E_LEAVINGAPPLICATIONSTATE, State1::GetTypeStatic(), StringHash::Empty),
            EventMatcher(E_STATETRANSITIONCOMPLETE, State1::GetTypeStatic(), StringHash::Empty)
        }));
}


TEST_CASE("StateManager: Enque two states")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto guard = Tests::MakeScopedReflection<State1, State2>(context);

    auto* stateManager = context->GetSubsystem<StateManager>();
    stateManager->Reset();
    REQUIRE(!stateManager->GetState());

    Serializable subscriber(context);
    ea::vector<EventMatcher> events;
    auto callback = [&](StringHash eventType, VariantMap& data) { events.push_back(EventMatcher(eventType, data)); };
    subscriber.SubscribeToEvent(E_STATETRANSITIONSTARTED, callback);
    subscriber.SubscribeToEvent(E_ENTERINGAPPLICATIONSTATE, callback);
    subscriber.SubscribeToEvent(E_LEAVINGAPPLICATIONSTATE, callback);
    subscriber.SubscribeToEvent(E_STATETRANSITIONCOMPLETE, callback);

    stateManager->EnqueueState(State1::GetTypeStatic());
    stateManager->EnqueueState(State2::GetTypeStatic());

    Tests::RunFrame(context, 0.1f);

    CHECK_THAT(events,
        EaEquals(ea::vector<EventMatcher>{
            EventMatcher(E_STATETRANSITIONSTARTED, StringHash::Empty, State1::GetTypeStatic()),
            EventMatcher(E_ENTERINGAPPLICATIONSTATE, StringHash::Empty, State1::GetTypeStatic()),
            EventMatcher(E_STATETRANSITIONCOMPLETE, StringHash::Empty, State1::GetTypeStatic()),
            EventMatcher(E_STATETRANSITIONSTARTED, State1::GetTypeStatic(), State2::GetTypeStatic()),
            EventMatcher(E_LEAVINGAPPLICATIONSTATE, State1::GetTypeStatic(), State2::GetTypeStatic()),
            EventMatcher(E_ENTERINGAPPLICATIONSTATE, State1::GetTypeStatic(), State2::GetTypeStatic()),
            EventMatcher(E_STATETRANSITIONCOMPLETE, State1::GetTypeStatic(), State2::GetTypeStatic())
        }));
}

TEST_CASE("StateManager: Skip unknown state")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto guard = Tests::MakeScopedReflection<State1, State2>(context);

    auto* stateManager = context->GetSubsystem<StateManager>();
    stateManager->Reset();

    Serializable subscriber(context);
    ea::vector<EventMatcher> events;
    auto callback = [&](StringHash eventType, VariantMap& data) { events.push_back(EventMatcher(eventType, data)); };
    subscriber.SubscribeToEvent(E_STATETRANSITIONSTARTED, callback);
    subscriber.SubscribeToEvent(E_ENTERINGAPPLICATIONSTATE, callback);
    subscriber.SubscribeToEvent(E_LEAVINGAPPLICATIONSTATE, callback);
    subscriber.SubscribeToEvent(E_STATETRANSITIONCOMPLETE, callback);

    StringHash unknownState("BlaBla");
    stateManager->EnqueueState(State1::GetTypeStatic());
    stateManager->EnqueueState(unknownState);
    stateManager->EnqueueState(State2::GetTypeStatic());

    Tests::RunFrame(context, 0.1f);

    CHECK_THAT(events,
        EaEquals(
            ea::vector<EventMatcher>{EventMatcher(E_STATETRANSITIONSTARTED, StringHash::Empty, State1::GetTypeStatic()),
                EventMatcher(E_ENTERINGAPPLICATIONSTATE, StringHash::Empty, State1::GetTypeStatic()),
                EventMatcher(E_STATETRANSITIONCOMPLETE, StringHash::Empty, State1::GetTypeStatic()),
                EventMatcher(E_STATETRANSITIONSTARTED, State1::GetTypeStatic(), unknownState),
                EventMatcher(E_LEAVINGAPPLICATIONSTATE, State1::GetTypeStatic(), unknownState),
                EventMatcher(E_ENTERINGAPPLICATIONSTATE, State1::GetTypeStatic(), State2::GetTypeStatic()),
                EventMatcher(E_STATETRANSITIONCOMPLETE, State1::GetTypeStatic(), State2::GetTypeStatic())}));
}

TEST_CASE("StateManager: Last state is unknown")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto guard = Tests::MakeScopedReflection<State1, State2>(context);

    auto* stateManager = context->GetSubsystem<StateManager>();
    stateManager->Reset();

    Serializable subscriber(context);
    ea::vector<EventMatcher> events;
    auto callback = [&](StringHash eventType, VariantMap& data) { events.push_back(EventMatcher(eventType, data)); };
    subscriber.SubscribeToEvent(E_STATETRANSITIONSTARTED, callback);
    subscriber.SubscribeToEvent(E_ENTERINGAPPLICATIONSTATE, callback);
    subscriber.SubscribeToEvent(E_LEAVINGAPPLICATIONSTATE, callback);
    subscriber.SubscribeToEvent(E_STATETRANSITIONCOMPLETE, callback);

    StringHash unknownState("BlaBla");
    stateManager->EnqueueState(State1::GetTypeStatic());
    stateManager->EnqueueState(unknownState);

    Tests::RunFrame(context, 0.1f);

    CHECK_THAT(events,
        EaEquals(
            ea::vector<EventMatcher>{EventMatcher(E_STATETRANSITIONSTARTED, StringHash::Empty, State1::GetTypeStatic()),
                EventMatcher(E_ENTERINGAPPLICATIONSTATE, StringHash::Empty, State1::GetTypeStatic()),
                EventMatcher(E_STATETRANSITIONCOMPLETE, StringHash::Empty, State1::GetTypeStatic()),
                EventMatcher(E_STATETRANSITIONSTARTED, State1::GetTypeStatic(), unknownState),
                EventMatcher(E_LEAVINGAPPLICATIONSTATE, State1::GetTypeStatic(), unknownState),
                EventMatcher(E_STATETRANSITIONCOMPLETE, State1::GetTypeStatic(), StringHash::Empty)}));
}
