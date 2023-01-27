//
// Copyright (c) 2017-2020 the rbfx project.
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

/// \file

#pragma once

#include <Urho3D/Replica/NetworkObject.h>

namespace Urho3D
{

class PrefabResource;

/// NetworkObject that is replicated on the client from prefab and is not updated afterwards.
/// Note: object position in the hierarchy of NetworkObject-s is still maintained.
class URHO3D_API StaticNetworkObject : public NetworkObject
{
    URHO3D_OBJECT(StaticNetworkObject, NetworkObject);

public:
    explicit StaticNetworkObject(Context* context);
    ~StaticNetworkObject() override;

    static void RegisterObject(Context* context);

    /// Attribute modification. Don't do that after replication!
    /// @{
    void SetClientPrefab(PrefabResource* prefab);
    /// @}

    /// Implement NetworkObject
    /// @{
    void InitializeOnServer() override;

    void WriteSnapshot(NetworkFrame frame, Serializer& dest) override;
    bool PrepareReliableDelta(NetworkFrame frame) override;
    void WriteReliableDelta(NetworkFrame frame, Serializer& dest) override;

    void InitializeFromSnapshot(NetworkFrame frame, Deserializer& src, bool isOwned) override;
    void ReadReliableDelta(NetworkFrame frame, Deserializer& src) override;
    /// @}

protected:
    ResourceRef GetClientPrefabAttr() const;
    void SetClientPrefabAttr(const ResourceRef& value);

private:
    SharedPtr<PrefabResource> clientPrefab_;

    NetworkId latestSentParentObject_{NetworkId::None};
};

}; // namespace Urho3D
