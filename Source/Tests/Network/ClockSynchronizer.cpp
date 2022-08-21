//
// Copyright (c) 2017-2021 the rbfx project.
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
#include "../NetworkUtils.h"

#include <Urho3D/Network/ClockSynchronizer.h>

namespace
{

class ClockSynchronizerSimulator
{
public:
    ea::vector<int> predictedServerToClientOffset_;
    ea::vector<int> predictedClientToServerOffset_;
    ea::vector<unsigned> pingOnServer_;
    ea::vector<unsigned> pingOnClient_;

    ClockSynchronizerSimulator(ea::function<unsigned()> getRandomDelay)
        : getRandomDelay_(getRandomDelay)
        , serverSync_{250, 10000, 40, 10, [&]{ return serverClock_; }}
        , clientSync_{250, 10000, 40, 10, [&]{ return clientClock_; }}
    {
    }

    void Simulate(unsigned totalTime, unsigned timeStep = 10)
    {
        predictedServerToClientOffset_.clear();
        predictedClientToServerOffset_.clear();
        pingOnServer_.clear();
        pingOnClient_.clear();

        const unsigned numSteps = ea::max(1u, totalTime / timeStep);
        for (unsigned i = 0; i < numSteps; ++i)
        {
            SimulateTimeStep(timeStep);
            if (serverSync_.IsReady())
            {
                predictedServerToClientOffset_.push_back(static_cast<int>(serverSync_.LocalToRemote(0)));
                pingOnClient_.push_back(serverSync_.GetPing());
            }
            if (clientSync_.IsReady())
            {
                predictedClientToServerOffset_.push_back(static_cast<int>(clientSync_.LocalToRemote(0)));
                pingOnServer_.push_back(clientSync_.GetPing());
            }
        }
    };

private:
    using MessageQueue = ea::vector<ea::pair<unsigned, ClockSynchronizerMessage>>;

    void SimulateTimeStep(unsigned timeStep)
    {
        ProcessAndRemove(serverSync_, serverClock_, clientToServerMessages_);
        ProcessAndRemove(clientSync_, clientClock_, serverToClientMessages_);

        serverClock_ += timeStep;
        clientClock_ += timeStep;

        while (const auto msg = serverSync_.PollMessage())
            serverToClientMessages_.emplace_back(clientClock_ + getRandomDelay_(), *msg);

        while (const auto msg = clientSync_.PollMessage())
            clientToServerMessages_.emplace_back(serverClock_ + getRandomDelay_(), *msg);
    };

    static void ProcessAndRemove(ClockSynchronizer& sync, unsigned currentTime, MessageQueue& queue)
    {
        ea::erase_if(queue, [&](const auto& timeAndMessage)
        {
            if (timeAndMessage.first <= currentTime)
            {
                sync.ProcessMessage(timeAndMessage.second);
                return true;
            }
            return false;
        });
    }

    ea::function<unsigned()> getRandomDelay_;

    unsigned serverClock_{10000};
    unsigned clientClock_{20000};

    ClockSynchronizer serverSync_;
    ClockSynchronizer clientSync_;

    MessageQueue serverToClientMessages_;
    MessageQueue clientToServerMessages_;
};

}

TEST_CASE("System clock is synchronized between client and server")
{
    const unsigned minDelay = 250;
    const unsigned maxDelay = 350;
    const unsigned throttleDelay = 100;
    const float throttleChance = 0.2f;

    const unsigned seed = GENERATE(0, 1, 2, 3, 4);
    RandomEngine re(seed);
    const auto getRandomDelay = [&]
    {
        const bool isThrottled = re.GetBool(throttleChance);
        const unsigned baseDelay = re.GetUInt(minDelay, maxDelay);
        return baseDelay + (isThrottled ? throttleDelay : 0);
    };

    ClockSynchronizerSimulator sim{getRandomDelay};

    // Expect time somewhat synchronized
    sim.Simulate(2000);

    REQUIRE(sim.predictedServerToClientOffset_.back() >= 9900);
    REQUIRE(sim.predictedServerToClientOffset_.back() <= 10100);

    REQUIRE(sim.predictedClientToServerOffset_.back() >= -10100);
    REQUIRE(sim.predictedClientToServerOffset_.back() <= -9900);

    REQUIRE(sim.pingOnClient_.back() >= 250);
    REQUIRE(sim.pingOnClient_.back() <= 400);

    REQUIRE(sim.pingOnServer_.back() >= 250);
    REQUIRE(sim.pingOnServer_.back() <= 400);

    sim.Simulate(2000);

    // Expect time to stay stable
    sim.Simulate(10000);

    const auto [minServerToClient, maxServerToClient] =
        ea::minmax_element(sim.predictedServerToClientOffset_.begin(), sim.predictedServerToClientOffset_.end());
    const auto [minClientToServer, maxClientToServer] =
        ea::minmax_element(sim.predictedClientToServerOffset_.begin(), sim.predictedClientToServerOffset_.end());

    const auto [minPingOnServer, maxPingOnServer] =
        ea::minmax_element(sim.pingOnServer_.begin(), sim.pingOnServer_.end());
    const auto [minPingOnClient, maxPingOnClient] =
        ea::minmax_element(sim.pingOnClient_.begin(), sim.pingOnClient_.end());

    REQUIRE(sim.predictedServerToClientOffset_.back() >= 9950);
    REQUIRE(sim.predictedServerToClientOffset_.back() <= 10050);

    REQUIRE(sim.predictedClientToServerOffset_.back() >= -10050);
    REQUIRE(sim.predictedClientToServerOffset_.back() <= -9950);

    REQUIRE(*maxServerToClient - *minServerToClient < 35);
    REQUIRE(*maxClientToServer - *minClientToServer < 35);

    REQUIRE(*minPingOnServer >= 250);
    REQUIRE(*maxPingOnServer <= 400);

    REQUIRE(*minPingOnClient >= 250);
    REQUIRE(*maxPingOnClient <= 400);
}

TEST_CASE("System clock is perfectly synchronized on good connection")
{
    const unsigned minDelay = 180;
    const unsigned maxDelay = 200;
    const unsigned throttleDelay = 100;
    const float throttleChance = 0.02f;

    const unsigned seed = GENERATE(0, 1, 2, 3, 4);
    RandomEngine re(seed);
    const auto getRandomDelay = [&]
    {
        const bool isThrottled = re.GetBool(throttleChance);
        const unsigned baseDelay = re.GetUInt(minDelay, maxDelay);
        return baseDelay + (isThrottled ? throttleDelay : 0);
    };

    ClockSynchronizerSimulator sim{getRandomDelay};
    sim.Simulate(2000);

    // Expect time to stay stable
    sim.Simulate(10000);

    const auto [minServerToClient, maxServerToClient] =
        ea::minmax_element(sim.predictedServerToClientOffset_.begin(), sim.predictedServerToClientOffset_.end());
    const auto [minClientToServer, maxClientToServer] =
        ea::minmax_element(sim.predictedClientToServerOffset_.begin(), sim.predictedClientToServerOffset_.end());

    const auto [minPingOnServer, maxPingOnServer] =
        ea::minmax_element(sim.pingOnServer_.begin(), sim.pingOnServer_.end());
    const auto [minPingOnClient, maxPingOnClient] =
        ea::minmax_element(sim.pingOnClient_.begin(), sim.pingOnClient_.end());

    REQUIRE(sim.predictedServerToClientOffset_.back() >= 9990);
    REQUIRE(sim.predictedServerToClientOffset_.back() <= 10010);

    REQUIRE(sim.predictedClientToServerOffset_.back() >= -10010);
    REQUIRE(sim.predictedClientToServerOffset_.back() <= -9990);

    REQUIRE(*maxServerToClient - *minServerToClient < 15);
    REQUIRE(*maxClientToServer - *minClientToServer < 15);

    REQUIRE(*minPingOnServer >= 180);
    REQUIRE(*maxPingOnServer <= 200);

    REQUIRE(*minPingOnClient >= 180);
    REQUIRE(*maxPingOnClient <= 200);
}
