// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "../IO/MemoryBuffer.h"
#include "../IO/VectorBuffer.h"
#include "../Network/Protocol.h"
#include "../Replica/NetworkId.h"

namespace Urho3D
{

struct MsgConfigure
{
    unsigned magic_{};
    VariantMap settings_;

    void Save(VectorBuffer& dest) const;
    void Load(MemoryBuffer& src);
    ea::string ToString() const;
};

struct MsgSynchronized
{
    unsigned magic_{};

    void Save(VectorBuffer& dest) const;
    void Load(MemoryBuffer& src);
    ea::string ToString() const;
};

struct MsgSceneClock
{
    NetworkFrame latestFrame_{};
    unsigned latestFrameTime_{};
    unsigned inputDelay_{};

    void Save(VectorBuffer& dest) const;
    void Load(MemoryBuffer& src);
    ea::string ToString() const;
};

}
