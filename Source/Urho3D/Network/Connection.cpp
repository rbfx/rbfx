//
// Copyright (c) 2008-2022 the Urho3D project.
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

#include "../Precompiled.h"

#include "../Core/Context.h"
#include "../Core/Profiler.h"
#include "../IO/File.h"
#include "../IO/FileSystem.h"
#include "../IO/Log.h"
#include "../IO/MemoryBuffer.h"
#include "../IO/PackageFile.h"
#include "../Network/ClockSynchronizer.h"
#include "../Network/Connection.h"
#include "../Network/Network.h"
#include "../Network/NetworkEvents.h"
#include "../Network/Protocol.h"
#include "../Replica/ReplicationManager.h"
#include "../Resource/ResourceCache.h"
#include "../Scene/Scene.h"
#include "../Scene/SceneEvents.h"

#include <slikenet/MessageIdentifiers.h>
#include <slikenet/peerinterface.h>
#include <slikenet/statistics.h>

#ifdef SendMessage
#undef SendMessage
#endif

#include "../DebugNew.h"
#include "Connection.h"


#include <cstdio>

namespace Urho3D
{

static const int STATS_INTERVAL_MSEC = 2000;

PackageDownload::PackageDownload() :
    totalFragments_(0),
    checksum_(0),
    initiated_(false)
{
}

PackageUpload::PackageUpload() :
    fragment_(0),
    totalFragments_(0)
{
}

Connection::Connection(Context* context) :
    AbstractConnection(context),
    peer_(nullptr),
    isClient_(false),
    connectPending_(false),
    sceneLoaded_(false),
    logStatistics_(false),
    address_(nullptr),
    packedMessageLimit_(1024)
{
}

Connection::~Connection()
{
    // Reset scene (remove possible owner references), as this connection is about to be destroyed
    SetScene(nullptr);

    delete address_;
    address_ = nullptr;
}

void Connection::Initialize(bool isClient, const SLNet::AddressOrGUID& address, SLNet::RakPeerInterface* peer)
{
    assert(peer_ == nullptr);
    peer_ = peer;
    isClient_ = isClient;
    port_ = address.systemAddress.GetPort();
    SetAddressOrGUID(address);

    auto network = GetSubsystem<Network>();
    clock_ = ea::make_unique<ClockSynchronizer>(network->GetPingIntervalMs(), network->GetMaxPingIntervalMs(),
        network->GetClockBufferSize(), network->GetPingBufferSize());
}

void Connection::RegisterObject(Context* context)
{
    context->AddFactoryReflection<Connection>();
}

PacketType Connection::GetPacketType(bool reliable, bool inOrder)
{
    if (reliable && inOrder)
        return PT_RELIABLE_ORDERED;

    if (reliable && !inOrder)
        return PT_RELIABLE_UNORDERED;

    if (!reliable && inOrder)
        return PT_UNRELIABLE_ORDERED;

    return PT_UNRELIABLE_UNORDERED;
}

void Connection::SendMessageInternal(NetworkMessageId messageId, bool reliable, bool inOrder, const unsigned char* data, unsigned numBytes)
{
    if (numBytes && !data)
    {
        URHO3D_LOGERROR("Null pointer supplied for network message data");
        return;
    }

    PacketType type = GetPacketType(reliable, inOrder);
    VectorBuffer& buffer = outgoingBuffer_[type];

    if (buffer.GetSize() + numBytes >= packedMessageLimit_)
        SendBuffer(type);

    if (buffer.GetSize() == 0)
    {
        buffer.WriteUByte((unsigned char)DefaultMessageIDTypes::ID_USER_PACKET_ENUM);
        buffer.WriteUInt((unsigned int)MSG_PACKED_MESSAGE);
    }

    buffer.WriteUInt((unsigned int)messageId);
    buffer.WriteUInt(numBytes);
    buffer.Write(data, numBytes);
}

void Connection::SendRemoteEvent(StringHash eventType, bool inOrder, const VariantMap& eventData)
{
    RemoteEvent queuedEvent;
    queuedEvent.eventType_ = eventType;
    queuedEvent.eventData_ = eventData;
    queuedEvent.inOrder_ = inOrder;
    remoteEvents_.push_back(queuedEvent);
}

void Connection::SetScene(Scene* newScene)
{
    if (scene_)
    {
        // Remove replication states and owner references from the previous scene
        if (replicationManager_)
            replicationManager_->DropConnection(this);
        replicationManager_ = nullptr;
    }

    scene_ = newScene;
    sceneLoaded_ = false;
    UnsubscribeFromEvent(E_ASYNCLOADFINISHED);

    if (!scene_)
        return;

    if (isClient_)
    {
        replicationManager_ = scene_->GetOrCreateComponent<ReplicationManager>();
        if (!replicationManager_->IsServer())
            replicationManager_->StartServer();

        // When scene is assigned on the server, instruct the client to load it. This may require downloading packages
        const ea::vector<SharedPtr<PackageFile> >& packages = scene_->GetRequiredPackageFiles();
        unsigned numPackages = packages.size();
        msg_.Clear();
        msg_.WriteString(scene_->GetFileName());
        msg_.WriteVLE(numPackages);
        for (unsigned i = 0; i < numPackages; ++i)
        {
            PackageFile* package = packages[i];
            msg_.WriteString(GetFileNameAndExtension(package->GetName()));
            msg_.WriteUInt(package->GetTotalSize());
            msg_.WriteUInt(package->GetChecksum());
        }
        SendMessage(MSG_LOADSCENE, true, true, msg_);
    }
    else
    {
        // Make sure there is no existing async loading
        scene_->StopAsyncLoading();
        SubscribeToEvent(scene_, E_ASYNCLOADFINISHED, URHO3D_HANDLER(Connection, HandleAsyncLoadFinished));
    }
}

void Connection::SetIdentity(const VariantMap& identity)
{
    identity_ = identity;
}

void Connection::SetConnectPending(bool connectPending)
{
    connectPending_ = connectPending;
}

void Connection::SetLogStatistics(bool enable)
{
    logStatistics_ = enable;
}

void Connection::Disconnect(int waitMSec)
{
    peer_->CloseConnection(*address_, true);
}

void Connection::SendRemoteEvents()
{
#ifdef URHO3D_LOGGING
    if (logStatistics_ && statsTimer_.GetMSec(false) > STATS_INTERVAL_MSEC)
    {
        statsTimer_.Reset();
        char statsBuffer[256];
        sprintf(statsBuffer, "RTT %.3f ms Pkt in %i Pkt out %i Data in %.3f KB/s Data out %.3f KB/s", GetRoundTripTime(),
            GetPacketsInPerSec(),
            GetPacketsOutPerSec(),
            (float)GetBytesInPerSec(),
            (float)GetBytesOutPerSec());
        URHO3D_LOGINFO(statsBuffer);
    }
#endif

    if (packetCounterTimer_.GetMSec(false) > 1000)
    {
        packetCounterTimer_.Reset();
        packetCounter_ = tempPacketCounter_;
        tempPacketCounter_ = IntVector2::ZERO;
    }

    if (remoteEvents_.empty())
        return;

    URHO3D_PROFILE("SendRemoteEvents");

    for (auto i = remoteEvents_.begin(); i != remoteEvents_.end(); ++i)
    {
        msg_.Clear();
        msg_.WriteStringHash(i->eventType_);
        msg_.WriteVariantMap(i->eventData_);
        SendMessage(MSG_REMOTEEVENT, true, i->inOrder_, msg_);
    }

    remoteEvents_.clear();
}

void Connection::SendPackages()
{
    while (!uploads_.empty())
    {
        unsigned char buffer[PACKAGE_FRAGMENT_SIZE];

        for (auto i = uploads_.begin(); i != uploads_.end();)
        {
            auto current = i++;
            PackageUpload& upload = current->second;
            auto fragmentSize =
                (unsigned)Min((int)(upload.file_->GetSize() - upload.file_->GetPosition()), (int)PACKAGE_FRAGMENT_SIZE);
            upload.file_->Read(buffer, fragmentSize);

            msg_.Clear();
            msg_.WriteStringHash(current->first);
            msg_.WriteUInt(upload.fragment_++);
            msg_.Write(buffer, fragmentSize);
            SendMessage(MSG_PACKAGEDATA, true, false, msg_);

            // Check if upload finished
            if (upload.fragment_ == upload.totalFragments_)
                uploads_.erase(current);
        }
    }
}

void Connection::SendBuffer(PacketType type)
{
    VectorBuffer& buffer = outgoingBuffer_[type];
    if (buffer.GetSize() == 0)
        return;

    PacketReliability reliability = PacketReliability::UNRELIABLE;
    if (type == PT_UNRELIABLE_ORDERED)
        reliability = PacketReliability::UNRELIABLE_SEQUENCED;

    if (type == PT_RELIABLE_ORDERED)
        reliability = PacketReliability::RELIABLE_ORDERED;

    if (type == PT_RELIABLE_UNORDERED)
        reliability = PacketReliability::RELIABLE;

    if (peer_) {
        peer_->Send((const char *) buffer.GetData(), (int) buffer.GetSize(), HIGH_PRIORITY, reliability, (char) 0,
                    *address_, false);
        tempPacketCounter_.y_++;
    }

    buffer.Clear();
}

void Connection::SendAllBuffers()
{
    // Send clock messages at the last time to have better precision
    if (clock_)
    {
        auto& buffer = outgoingBuffer_[PT_UNRELIABLE_UNORDERED];
        while (const auto clockMessage = clock_->PollMessage())
        {
            SendGeneratedMessage(MSG_CLOCK_SYNC, PT_UNRELIABLE_UNORDERED,
                [&](VectorBuffer& msg, ea::string* debugInfo)
            {
                clockMessage->Save(msg);
                return true;
            });
        }
    }

    SendBuffer(PT_RELIABLE_ORDERED);
    SendBuffer(PT_RELIABLE_UNORDERED);
    SendBuffer(PT_UNRELIABLE_ORDERED);
    SendBuffer(PT_UNRELIABLE_UNORDERED);
}

bool Connection::ProcessMessage(int msgID, MemoryBuffer& buffer)
{
    tempPacketCounter_.x_++;
    if (buffer.GetSize() == 0)
        return false;

    if (msgID != MSG_PACKED_MESSAGE)
    {
        ProcessUnknownMessage(msgID, buffer);
        return true;
    }

    while (!buffer.IsEof()) {
        msgID = buffer.ReadUInt();
        unsigned int packetSize = buffer.ReadUInt();
        MemoryBuffer msg(buffer.GetData() + buffer.GetPosition(), packetSize);
        buffer.Seek(buffer.GetPosition() + packetSize);

        switch (msgID)
        {
            case MSG_IDENTITY:
                ProcessIdentity(msgID, msg);
                break;

            case MSG_SCENELOADED:
                ProcessSceneLoaded(msgID, msg);
                break;

            case MSG_REQUESTPACKAGE:
            case MSG_PACKAGEDATA:
                ProcessPackageDownload(msgID, msg);
                break;

            case MSG_LOADSCENE:
                ProcessLoadScene(msgID, msg);
                break;

            case MSG_SCENECHECKSUMERROR:
                ProcessSceneChecksumError(msgID, msg);
                break;

            case MSG_REMOTEEVENT:
                ProcessRemoteEvent(msgID, msg);
                break;

            case MSG_PACKAGEINFO:
                ProcessPackageInfo(msgID, msg);
                break;

            case MSG_CLOCK_SYNC:
                if (clock_)
                {
                    ClockSynchronizerMessage clockMessage;
                    clockMessage.Load(msg);
                    clock_->ProcessMessage(clockMessage);
                }
                break;

            default:
                if (replicationManager_ && replicationManager_->ProcessMessage(this, static_cast<NetworkMessageId>(msgID), msg))
                    break;

                ProcessUnknownMessage(msgID, msg);
                break;
        }
    }
    return true;
}

void Connection::Ban()
{
    if (peer_)
    {
        peer_->AddToBanList(address_->ToString(false), 0);
    }
}

void Connection::ProcessLoadScene(int msgID, MemoryBuffer& msg)
{
    if (IsClient())
    {
        URHO3D_LOGWARNING("Received unexpected LoadScene message from client " + ToString());
        return;
    }

    if (!scene_)
    {
        URHO3D_LOGERROR("Can not handle LoadScene message without an assigned scene");
        return;
    }

    // Store the scene file name we need to eventually load
    sceneFileName_ = msg.ReadString();

    // Clear previous pending latest data and package downloads if any
    downloads_.clear();

    // In case we have joined other scenes in this session, remove first all downloaded package files from the resource system
    // to prevent resource conflicts
    auto* cache = GetSubsystem<ResourceCache>();
    const ea::string& packageCacheDir = GetSubsystem<Network>()->GetPackageCacheDir();

    ea::vector<SharedPtr<PackageFile> > packages = cache->GetPackageFiles();
    for (unsigned i = 0; i < packages.size(); ++i)
    {
        PackageFile* package = packages[i];
        if (!package->GetName().find(packageCacheDir))
            cache->RemovePackageFile(package, true);
    }

    // Now check which packages we have in the resource cache or in the download cache, and which we need to download
    unsigned numPackages = msg.ReadVLE();
    if (!RequestNeededPackages(numPackages, msg))
    {
        OnSceneLoadFailed();
        return;
    }

    // If no downloads were queued, can load the scene directly
    if (downloads_.empty())
        OnPackagesReady();
}

void Connection::ProcessSceneChecksumError(int msgID, MemoryBuffer& msg)
{
    if (IsClient())
    {
        URHO3D_LOGWARNING("Received unexpected SceneChecksumError message from client " + ToString());
        return;
    }

    URHO3D_LOGERROR("Scene checksum error");
    OnSceneLoadFailed();
}

void Connection::ProcessPackageDownload(int msgID, MemoryBuffer& msg)
{
    switch (msgID)
    {
    case MSG_REQUESTPACKAGE:
        if (!IsClient())
        {
            URHO3D_LOGWARNING("Received unexpected RequestPackage message from server");
            return;
        }
        else
        {
            ea::string name = msg.ReadString();

            if (!scene_)
            {
                URHO3D_LOGWARNING("Received a RequestPackage message without an assigned scene from client " + ToString());
                return;
            }

            // The package must be one of those required by the scene
            const ea::vector<SharedPtr<PackageFile> >& packages = scene_->GetRequiredPackageFiles();
            for (unsigned i = 0; i < packages.size(); ++i)
            {
                PackageFile* package = packages[i];
                const ea::string& packageFullName = package->GetName();
                if (!GetFileNameAndExtension(packageFullName).comparei(name))
                {
                    StringHash nameHash(name);

                    // Do not restart upload if already exists
                    if (uploads_.contains(nameHash))
                    {
                        URHO3D_LOGWARNING("Received a request for package " + name + " already in transfer");
                        return;
                    }

                    // Try to open the file now
                    const auto file = MakeShared<File>(context_, packageFullName);
                    if (!file->IsOpen())
                    {
                        URHO3D_LOGERROR("Failed to transmit package file " + name);
                        SendPackageError(name);
                        return;
                    }

                    URHO3D_LOGINFO("Transmitting package file " + name + " to client " + ToString());

                    uploads_[nameHash].file_ = file;
                    uploads_[nameHash].fragment_ = 0;
                    uploads_[nameHash].totalFragments_ = (file->GetSize() + PACKAGE_FRAGMENT_SIZE - 1) / PACKAGE_FRAGMENT_SIZE;
                    return;
                }
            }

            URHO3D_LOGERROR("Client requested an unexpected package file " + name);
            // Send the name hash only to indicate a failed download
            SendPackageError(name);
            return;
        }
        break;

    case MSG_PACKAGEDATA:
        if (IsClient())
        {
            URHO3D_LOGWARNING("Received unexpected PackageData message from client");
            return;
        }
        else
        {
            StringHash nameHash = msg.ReadStringHash();

            auto i = downloads_.find(nameHash);
            // In case of being unable to create the package file into the cache, we will still receive all data from the server.
            // Simply disregard it
            if (i == downloads_.end())
                return;

            PackageDownload& download = i->second;

            // If no further data, this is an error reply
            if (msg.IsEof())
            {
                OnPackageDownloadFailed(download.name_);
                return;
            }

            // If file has not yet been opened, try to open now. Prepend the checksum to the filename to allow multiple versions
            if (!download.file_)
            {
                download.file_ = MakeShared<File>(context_,
                    GetSubsystem<Network>()->GetPackageCacheDir() + ToStringHex(download.checksum_) + "_" + download.name_,
                    FILE_WRITE);
                if (!download.file_->IsOpen())
                {
                    OnPackageDownloadFailed(download.name_);
                    return;
                }
            }

            // Write the fragment data to the proper index
            unsigned char buffer[PACKAGE_FRAGMENT_SIZE];
            unsigned index = msg.ReadUInt();
            const unsigned fragmentSize = msg.GetSize() - msg.GetPosition();

            msg.Read(buffer, fragmentSize);
            download.file_->Seek(index * PACKAGE_FRAGMENT_SIZE);
            download.file_->Write(buffer, fragmentSize);
            download.receivedFragments_.insert(index);

            // Check if all fragments received
            if (download.receivedFragments_.size() == download.totalFragments_)
            {
                URHO3D_LOGINFO("Package " + download.name_ + " downloaded successfully");

                // Instantiate the package and add to the resource system, as we will need it to load the scene
                download.file_->Close();
                GetSubsystem<ResourceCache>()->AddPackageFile(download.file_->GetName(), 0);

                // Then start the next download if there are more
                downloads_.erase(i);
                if (downloads_.empty())
                    OnPackagesReady();
                else
                {
                    PackageDownload& nextDownload = downloads_.begin()->second;

                    URHO3D_LOGINFO("Requesting package " + nextDownload.name_ + " from server");
                    msg_.Clear();
                    msg_.WriteString(nextDownload.name_);
                    SendMessage(MSG_REQUESTPACKAGE, true, true, msg_);
                    nextDownload.initiated_ = true;
                }
            }
        }
        break;

    default: break;
    }
}

void Connection::ProcessIdentity(int msgID, MemoryBuffer& msg)
{
    if (!IsClient())
    {
        URHO3D_LOGWARNING("Received unexpected Identity message from server");
        return;
    }

    identity_ = msg.ReadVariantMap();

    using namespace ClientIdentity;

    VariantMap eventData = identity_;
    eventData[P_CONNECTION] = this;
    eventData[P_ALLOW] = true;
    SendEvent(E_CLIENTIDENTITY, eventData);

    // If connection was denied as a response to the identity event, disconnect now
    if (!eventData[P_ALLOW].GetBool())
        Disconnect();
}

void Connection::ProcessSceneLoaded(int msgID, MemoryBuffer& msg)
{
    if (!IsClient())
    {
        URHO3D_LOGWARNING("Received unexpected SceneLoaded message from server");
        return;
    }

    if (!scene_ || !replicationManager_ || !replicationManager_->IsServer())
    {
        URHO3D_LOGWARNING("Received a SceneLoaded message without an assigned scene from client " + ToString());
        return;
    }

    unsigned checksum = msg.ReadUInt();

    if (checksum != scene_->GetChecksum())
    {
        URHO3D_LOGINFO("Scene checksum error from client " + ToString());
        msg_.Clear();
        SendMessage(MSG_SCENECHECKSUMERROR, true, true, msg_);
        OnSceneLoadFailed();
    }
    else
    {
        replicationManager_->GetServerReplicator()->AddConnection(this);
        sceneLoaded_ = true;

        using namespace ClientSceneLoaded;

        VariantMap& eventData = GetEventDataMap();
        eventData[P_CONNECTION] = this;
        SendEvent(E_CLIENTSCENELOADED, eventData);
    }
}

void Connection::ProcessRemoteEvent(int msgID, MemoryBuffer& msg)
{
    using namespace RemoteEventData;

    StringHash eventType = msg.ReadStringHash();
    if (!GetSubsystem<Network>()->CheckRemoteEvent(eventType))
    {
        URHO3D_LOGWARNING("Discarding not allowed remote event " + eventType.ToString());
        return;
    }

    VariantMap eventData = msg.ReadVariantMap();
    eventData[P_CONNECTION] = this;
    SendEvent(eventType, eventData);
}

Scene* Connection::GetScene() const
{
    return scene_;
}

bool Connection::IsConnected() const
{
    return peer_ && peer_->IsActive();
}

float Connection::GetRoundTripTime() const
{
    if (peer_)
    {
        SLNet::RakNetStatistics stats{};
        if (peer_->GetStatistics(address_->systemAddress, &stats))
            return (float)peer_->GetAveragePing(*address_);
    }
    return 0.0f;
}

unsigned long long Connection::GetBytesInPerSec() const
{
    if (peer_)
    {
        SLNet::RakNetStatistics stats{};
        if (peer_->GetStatistics(address_->systemAddress, &stats))
            return stats.valueOverLastSecond[SLNet::ACTUAL_BYTES_RECEIVED];
    }
    return 0;
}

unsigned long long Connection::GetBytesOutPerSec() const
{
    if (peer_)
    {
        SLNet::RakNetStatistics stats{};
        if (peer_->GetStatistics(address_->systemAddress, &stats))
            return stats.valueOverLastSecond[SLNet::ACTUAL_BYTES_SENT];
    }
    return 0;
}

int Connection::GetPacketsInPerSec() const
{
    return packetCounter_.x_;
}

int Connection::GetPacketsOutPerSec() const
{
    return packetCounter_.y_;
}

ea::string Connection::ToString() const
{
    return Format("#{} {}:{}", GetObjectID(), GetAddress(), GetPort());
}

bool Connection::IsClockSynchronized() const
{
    return clock_ && clock_->IsReady();
}

unsigned Connection::RemoteToLocalTime(unsigned time) const
{
    return clock_ ? clock_->RemoteToLocal(time) : time;
}

unsigned Connection::LocalToRemoteTime(unsigned time) const
{
    return clock_ ? clock_->LocalToRemote(time) : time;
}

unsigned Connection::GetLocalTime() const
{
    return Time::GetSystemTime();
}

unsigned Connection::GetLocalTimeOfLatestRoundtrip() const
{
    return clock_ ? clock_->GetLocalTimeOfLatestRoundtrip() : 0;
}

unsigned Connection::GetPing() const
{
    return clock_ ? clock_->GetPing() : 0;
}

unsigned Connection::GetNumDownloads() const
{
    return downloads_.size();
}

const ea::string& Connection::GetDownloadName() const
{
    for (auto i = downloads_.begin(); i !=
        downloads_.end(); ++i)
    {
        if (i->second.initiated_)
            return i->second.name_;
    }
    return EMPTY_STRING;
}

float Connection::GetDownloadProgress() const
{
    for (auto i = downloads_.begin(); i !=
        downloads_.end(); ++i)
    {
        if (i->second.initiated_)
            return (float)i->second.receivedFragments_.size() / (float)i->second.totalFragments_;
    }
    return 1.0f;
}

void Connection::SendPackageToClient(PackageFile* package)
{
    if (!scene_)
        return;

    if (!IsClient())
    {
        URHO3D_LOGERROR("SendPackageToClient can be called on the server only");
        return;
    }
    if (!package)
    {
        URHO3D_LOGERROR("Null package specified for SendPackageToClient");
        return;
    }

    msg_.Clear();

    ea::string filename = GetFileNameAndExtension(package->GetName());
    msg_.WriteString(filename);
    msg_.WriteUInt(package->GetTotalSize());
    msg_.WriteUInt(package->GetChecksum());
    SendMessage(MSG_PACKAGEINFO, true, true, msg_);
}

void Connection::ConfigureNetworkSimulator(int latencyMs, float packetLoss)
{
    if (peer_)
        peer_->ApplyNetworkSimulator(packetLoss, latencyMs, 0);
}

void Connection::SetPacketSizeLimit(int limit)
{
    packedMessageLimit_ = limit;
}

void Connection::HandleAsyncLoadFinished(StringHash eventType, VariantMap& eventData)
{
    replicationManager_ = scene_->GetOrCreateComponent<ReplicationManager>();
    replicationManager_->StartClient(this);
    sceneLoaded_ = true;

    msg_.Clear();
    msg_.WriteUInt(scene_->GetChecksum());
    SendMessage(MSG_SCENELOADED, true, true, msg_);
}

bool Connection::RequestNeededPackages(unsigned numPackages, MemoryBuffer& msg)
{
    auto* cache = GetSubsystem<ResourceCache>();
    const ea::string& packageCacheDir = GetSubsystem<Network>()->GetPackageCacheDir();

    ea::vector<SharedPtr<PackageFile> > packages = cache->GetPackageFiles();
    ea::vector<ea::string> downloadedPackages;
    bool packagesScanned = false;

    for (unsigned i = 0; i < numPackages; ++i)
    {
        ea::string name = msg.ReadString();
        unsigned fileSize = msg.ReadUInt();
        unsigned checksum = msg.ReadUInt();
        ea::string checksumString = ToStringHex(checksum);
        bool found = false;

        // Check first the resource cache
        for (unsigned j = 0; j < packages.size(); ++j)
        {
            PackageFile* package = packages[j];
            if (!GetFileNameAndExtension(package->GetName()).comparei(name) && package->GetTotalSize() == fileSize &&
                package->GetChecksum() == checksum)
            {
                found = true;
                break;
            }
        }

        if (found)
            continue;

        if (!packagesScanned)
        {
            if (packageCacheDir.empty())
            {
                URHO3D_LOGERROR("Can not check/download required packages, as package cache directory is not set");
                return false;
            }

            GetSubsystem<FileSystem>()->ScanDir(downloadedPackages, packageCacheDir, "*.*", SCAN_FILES, false);
            packagesScanned = true;
        }

        // Then the download cache
        for (unsigned j = 0; j < downloadedPackages.size(); ++j)
        {
            const ea::string& fileName = downloadedPackages[j];
            // In download cache, package file name format is checksum_packagename
            if (!fileName.find(checksumString) && !fileName.substr(9).comparei(name))
            {
                // Name matches. Check file size and actual checksum to be sure
                SharedPtr<PackageFile> newPackage(new PackageFile(context_, packageCacheDir + fileName));
                if (newPackage->GetTotalSize() == fileSize && newPackage->GetChecksum() == checksum)
                {
                    // Add the package to the resource system now, as we will need it to load the scene
                    cache->AddPackageFile(newPackage, 0);
                    found = true;
                    break;
                }
            }
        }

        // Package not found, need to request a download
        if (!found)
            RequestPackage(name, fileSize, checksum);
    }

    return true;
}

void Connection::RequestPackage(const ea::string& name, unsigned fileSize, unsigned checksum)
{
    StringHash nameHash(name);
    if (downloads_.contains(nameHash))
        return; // Download already exists

    PackageDownload& download = downloads_[nameHash];
    download.name_ = name;
    download.totalFragments_ = (fileSize + PACKAGE_FRAGMENT_SIZE - 1) / PACKAGE_FRAGMENT_SIZE;
    download.checksum_ = checksum;

    // Start download now only if no existing downloads, else wait for the existing ones to finish
    if (downloads_.size() == 1)
    {
        URHO3D_LOGINFO("Requesting package " + name + " from server");
        msg_.Clear();
        msg_.WriteString(name);
        SendMessage(MSG_REQUESTPACKAGE, true, true, msg_);
        download.initiated_ = true;
    }
}

void Connection::SendPackageError(const ea::string& name)
{
    msg_.Clear();
    msg_.WriteStringHash(name);
    SendMessage(MSG_PACKAGEDATA, true, false, msg_);
}

void Connection::OnSceneLoadFailed()
{
    sceneLoaded_ = false;

    using namespace NetworkSceneLoadFailed;

    VariantMap& eventData = GetEventDataMap();
    eventData[P_CONNECTION] = this;
    SendEvent(E_NETWORKSCENELOADFAILED, eventData);
}

void Connection::OnPackageDownloadFailed(const ea::string& name)
{
    URHO3D_LOGERROR("Download of package " + name + " failed");
    // As one package failed, we can not join the scene in any case. Clear the downloads
    downloads_.clear();
    OnSceneLoadFailed();
}

void Connection::OnPackagesReady()
{
    if (!scene_)
        return;

    // If sceneLoaded_ is true, we may have received additional package downloads while already joined in a scene.
    // In that case the scene should not be loaded.
    if (sceneLoaded_)
        return;

    if (sceneFileName_.empty())
    {
        replicationManager_ = scene_->GetOrCreateComponent<ReplicationManager>();
        replicationManager_->StartClient(this);
        sceneLoaded_ = true;

        msg_.Clear();
        msg_.WriteUInt(scene_->GetChecksum());
        SendMessage(MSG_SCENELOADED, true, true, msg_);
    }
    else
    {
        // Otherwise start the async loading process
        ea::string extension = GetExtension(sceneFileName_);
        AbstractFilePtr file = GetSubsystem<ResourceCache>()->GetFile(sceneFileName_);
        bool success;

        if (extension == ".xml")
            success = scene_->LoadAsyncXML(file);
        else if (extension == ".json")
            success = scene_->LoadAsyncJSON(file);
        else
            success = scene_->LoadAsync(file);

        if (!success)
            OnSceneLoadFailed();
    }
}

void Connection::ProcessPackageInfo(int msgID, MemoryBuffer& msg)
{
    if (!scene_)
        return;

    if (IsClient())
    {
        URHO3D_LOGWARNING("Received unexpected packages info message from client");
        return;
    }

    RequestNeededPackages(1, msg);
}

void Connection::ProcessUnknownMessage(int msgID, MemoryBuffer& msg)
{
    // If message was not handled internally, forward as an event
    using namespace NetworkMessage;

    VariantMap& eventData = GetEventDataMap();
    eventData[P_CONNECTION] = this;
    eventData[P_MESSAGEID] = (int)msgID;
    eventData[P_DATA].SetBuffer(msg.GetData(), msg.GetSize());
    SendEvent(E_NETWORKMESSAGE, eventData);
}

ea::string Connection::GetAddress() const
{
    return ea::string(address_->ToString(false /*write port*/));
}

void Connection::SetAddressOrGUID(const SLNet::AddressOrGUID& addr)
{
    delete address_;
    address_ = nullptr;
    address_ = new SLNet::AddressOrGUID(addr);
}

}
