//
// Copyright (c) 2017-2022 the Urho3D project.
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
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
#include <Urho3D/Network/LANDiscoveryManager.h>
#include <Urho3D/Network/NetworkEvents.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/Core/CoreEvents.h>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>

namespace Urho3D
{

static const StringHash MAGIC = "rbfx-LANDiscovery-v1";

static int GetLastNetworkError()
{
#if _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

LANDiscoveryManager::LANDiscoveryManager(Context* context)
    : Object(context)
{
}

bool LANDiscoveryManager::Start(unsigned short port, LANDiscoveryModeFlags mode)
{
    socket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_ == -1)
    {
        Log::GetLogger("LANDiscovery").Error("Failed create a socket: error {}", GetLastNetworkError());
        return false;
    }

#if defined __APPLE__
    int val = 1;
    setsockopt(socket_, SOL_SOCKET, SO_NOSIGPIPE, &val, sizeof(val));
#endif
#if defined _WIN32
    unsigned long reuse = 1;
    setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse));
    unsigned long broadcast = 1;
    if (setsockopt(socket_, SOL_SOCKET, SO_BROADCAST, (const char*)&broadcast, sizeof(broadcast)) == -1)
#else
    int reuse = 1;
    setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    int broadcast = 1;
    if (setsockopt(socket_, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) == -1)
#endif
    {
#ifdef _WIN32
        closesocket(socket_);
#else
        close(socket_);
#endif
        return false;
    }

    if (broadcastData_.empty())
    {
        struct sockaddr_in si_me = {};
        si_me.sin_family = AF_INET;
        si_me.sin_port = htons(port);
        si_me.sin_addr.s_addr = htonl((mode & LANDiscoveryMode::LAN) ? INADDR_ANY : INADDR_LOOPBACK);

        fcntl(socket_, F_SETFL, fcntl(socket_, F_GETFL) | O_NONBLOCK);
        if (bind(socket_, (struct sockaddr*)&si_me, sizeof(si_me)) == -1)
        {
            Log::GetLogger("LANDiscovery").Error("Failed to bind to port {}: error {}", port, GetLastNetworkError());
            close(socket_);
            socket_ = -1;
            return false;
        }

        buffer_.Resize(0xFFFF);
        SubscribeToEvent(E_UPDATE, [this](StringHash, VariantMap&)
        {
            if (timer_.GetMSec(false) < broadcastTimeMs_)
                return;

            timer_.Reset();
            struct sockaddr_in addr = {};
            socklen_t addrLen = sizeof(addr);
            int result = recvfrom(socket_, (void*)buffer_.GetData(), buffer_.GetSize(), 0, (sockaddr*)&addr, &addrLen);
            if (result > 0)
            {
                using namespace NetworkHostDiscovered;
                VariantMap& args = GetEventDataMap();
                char ipv4[16];
                inet_ntop(AF_INET, &addr.sin_addr, ipv4, sizeof(ipv4));
                args[P_ADDRESS] = ipv4;
                args[P_PORT] = ntohs(addr.sin_port);

                MemoryBuffer msg(buffer_);
                if (msg.ReadStringHash() != MAGIC)
                    return;
                args[P_BEACON] = msg.ReadVariantMap();
                SendEvent(E_NETWORKHOSTDISCOVERED, args);
            }
            else if (result < 0)
            {
                if (GetLastNetworkError() == EAGAIN)
                {
                    // Noop
                }
                else
                {
                    Log::GetLogger("LANDiscovery").Error("Failed receive data: result = {}, error = {}", result, GetLastNetworkError());
                }
            }
        });
    }
    else
    {
        SubscribeToEvent(E_UPDATE, [this, port, mode](StringHash, VariantMap&)
        {
            if (timer_.GetMSec(false) < broadcastTimeMs_)
                return;

            timer_.Reset();

            buffer_.Resize(0);
            buffer_.WriteStringHash(MAGIC);
            buffer_.WriteVariantMap(broadcastData_);

            sockaddr_in addr = {};
            addr.sin_family = AF_INET;
            addr.sin_port = htons(port);

            unsigned addresses[] = {
                (mode & LANDiscoveryMode::LAN) ? 0xFFFFFFFF : 0u,
                (mode & LANDiscoveryMode::Local) ? 0x7FFFFFFF : 0u
            };
            for (unsigned address : addresses)
            {
                if (!address)
                    continue;
                addr.sin_addr.s_addr = htonl(address);
                int result = sendto(socket_, buffer_.GetData(), buffer_.GetSize(), MSG_NOSIGNAL, (sockaddr*)&addr, sizeof(addr));
                if (result < 0)
                {
                    Log::GetLogger("LANDiscovery").Error("Failed to broadcast: result = {}, error = {}", result, GetLastNetworkError());
                    Stop();
                }
            }
        });
    }
    return true;
}

void LANDiscoveryManager::Stop()
{
    close(socket_);
    socket_ = -1;
    UnsubscribeFromAllEvents();
}

}
