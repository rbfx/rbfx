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

TEST_CASE("System clock is synchronized between client and server")
{
    const unsigned minDelay = 250;
    const unsigned maxDelay = 350;
    const unsigned throttleDelay = 150;
    const float throttleChance = 0.25f;

    const unsigned seed = GENERATE(0, 1, 2, 3, 4);
    RandomEngine re(seed);
    const auto getRandomDelay = [&] { return re.GetUInt(minDelay, maxDelay) + (re.GetBool(throttleChance) ? throttleDelay : 0); };

    unsigned serverClock = 10000;
    ServerClockSynchronizer serverSync{250, 10000, 40, [&]{ return serverClock; }};

    unsigned clientClock = 20000;
    ClientClockSynchronizer clientSync{40, [&]{ return clientClock; }};

    ea::vector<ea::pair<unsigned, ClockSynchronizerMessage>> serverToClientMessages;
    ea::vector<ea::pair<unsigned, ClockSynchronizerMessage>> clientToServerMessages;

    const auto processAndRemove = [](ClockSynchronizer& sync, unsigned now, auto& messages)
    {
        messages.erase(ea::remove_if(messages.begin(), messages.end(),
            [&](const auto& timeAndMessage)
        {
            if (timeAndMessage.first <= now)
            {
                sync.ProcessMessage(timeAndMessage.second);
                return true;
            }
            return false;
        }), messages.end());
    };

    const auto simulateTimeStep = [&](unsigned elapsedMs)
    {
        processAndRemove(serverSync, serverClock, clientToServerMessages);
        processAndRemove(clientSync, clientClock, serverToClientMessages);

        serverClock += elapsedMs;
        clientClock += elapsedMs;

        while (const auto msg = serverSync.PollMessage())
            serverToClientMessages.emplace_back(clientClock + getRandomDelay(), *msg);

        while (const auto msg = clientSync.PollMessage())
            clientToServerMessages.emplace_back(serverClock + getRandomDelay(), *msg);
    };

    ea::vector<int> predictedServerToClientOffset;
    ea::vector<int> predictedClientToServerOffset;
    const auto simulateTime = [&](unsigned totalMs)
    {
        predictedServerToClientOffset.clear();
        predictedClientToServerOffset.clear();

        const unsigned timeStep = 10;
        const unsigned numSteps = ea::max(1u, totalMs / timeStep);
        for (unsigned i = 0; i < numSteps; ++i)
        {
            simulateTimeStep(timeStep);
            if (serverSync.IsReady())
                predictedServerToClientOffset.push_back(static_cast<int>(serverSync.LocalToRemote(0)));
            if (clientSync.IsReady())
                predictedClientToServerOffset.push_back(static_cast<int>(clientSync.LocalToRemote(0)));
        }
    };

    // Expect time somewhat synchronized
    simulateTime(2000);

    REQUIRE(predictedServerToClientOffset.back() >= 9900);
    REQUIRE(predictedServerToClientOffset.back() <= 10100);

    REQUIRE(predictedClientToServerOffset.back() >= -10100);
    REQUIRE(predictedClientToServerOffset.back() <= -9900);

    simulateTime(2000);

    // Expect time to stay stable
    simulateTime(10000);
    const auto [minServerToClient, maxServerToClient] =
        ea::minmax_element(predictedServerToClientOffset.begin(), predictedServerToClientOffset.end());
    const auto [minClientToServer, maxClientToServer] =
        ea::minmax_element(predictedClientToServerOffset.begin(), predictedClientToServerOffset.end());

    REQUIRE(predictedServerToClientOffset.back() >= 9950);
    REQUIRE(predictedServerToClientOffset.back() <= 10050);

    REQUIRE(predictedClientToServerOffset.back() >= -10050);
    REQUIRE(predictedClientToServerOffset.back() <= -9950);

    REQUIRE(*maxServerToClient - *minServerToClient < 35);
    REQUIRE(*maxClientToServer - *minClientToServer < 35);
}
