// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2025 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Network/Connection.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/Core/Profiler.h"
#include "Urho3D/Core/WorkQueue.h"
#include "Urho3D/IO/File.h"
#include "Urho3D/IO/FileSystem.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/IO/MemoryBuffer.h"
#include "Urho3D/IO/PackageFile.h"
#include "Urho3D/IO/VirtualFileSystem.h"
#include "Urho3D/Network/ClockSynchronizer.h"
#include "Urho3D/Network/Connection.h"
#include "Urho3D/Network/MessageUtils.h"
#include "Urho3D/Network/Network.h"
#include "Urho3D/Network/NetworkEvents.h"
#include "Urho3D/Network/Protocol.h"
#include "Urho3D/Network/Transport/NetworkConnection.h"
#include "Urho3D/Replica/ReplicationManager.h"
#include "Urho3D/Resource/ResourceCache.h"
#include "Urho3D/Scene/Scene.h"
#include "Urho3D/Scene/SceneEvents.h"

#include <cstdio>

#include "Urho3D/DebugNew.h"

#ifdef SendMessage
    #undef SendMessage
#endif

namespace Urho3D
{

namespace
{

static const int STATS_INTERVAL_MSEC = 2000;

/// Size of package file fragment header.
static constexpr unsigned PackageFragmentHeaderSize = 8;
/// Package file fragment size.
static constexpr unsigned PACKAGE_FRAGMENT_SIZE =
    MaxNetworkPacketSize - PackageFragmentHeaderSize - NetworkMessageHeaderSize;

unsigned CalculateMaxPacketSize(unsigned requestedLimit, unsigned transportLimit)
{
    const unsigned effectiveRequestedLimit = requestedLimit != 0 ? requestedLimit : M_MAX_UNSIGNED;
    const unsigned effectiveTransportLimit = transportLimit != 0 ? transportLimit : MaxNetworkPacketSize;
    return ea::min(effectiveTransportLimit, effectiveRequestedLimit);
}

} // namespace

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

Connection::Connection(Context* context, NetworkConnection* connection)
    : AbstractConnection(context)
    , transportConnection_(connection)
{
    if (connection)
    {
        connection->onMessage_ = [this](ea::string_view msg)
        {
            MutexLock lock(packetQueueLock_);
            incomingPackets_.emplace_back(VectorBuffer(msg.data(), msg.size()));
        };
    }
}

Connection::~Connection()
{
    // Reset scene (remove possible owner references), as this connection is about to be destroyed
    SetScene(nullptr);
    Disconnect();
}

void Connection::Initialize()
{
    auto network = GetSubsystem<Network>();
    clock_ = ea::make_unique<ClockSynchronizer>(network->GetPingIntervalMs(), network->GetMaxPingIntervalMs(),
        network->GetClockBufferSize(), network->GetPingBufferSize());

    // Re-set the limit to apply transport limitations.
    const unsigned requestedMaxPacketSize = GetMaxPacketSize();
    SetMaxPacketSize(requestedMaxPacketSize);
}

void Connection::RegisterObject(Context* context)
{
    context->AddFactoryReflection<Connection>();
}

void Connection::SetMaxPacketSize(unsigned limit)
{
    const unsigned transportLimit = transportConnection_->GetMaxMessageSize();
    BaseClassName::SetMaxPacketSize(CalculateMaxPacketSize(limit, transportLimit));
}

void Connection::SendMessageInternal(NetworkMessageId messageId, const unsigned char* data, unsigned numBytes, PacketTypeFlags packetType)
{
    URHO3D_ASSERT(numBytes <= GetMaxMessageSize());
    URHO3D_ASSERT((data == nullptr && numBytes == 0) || (data != nullptr && numBytes > 0));

    VectorBuffer& buffer = outgoingBuffer_[packetType];

    // Flush buffer if it overflows on this message.
    if (buffer.GetSize() + NetworkMessageHeaderSize + numBytes > GetMaxPacketSize())
        SendBuffer(packetType);

    buffer.WriteUShort(messageId);
    buffer.WriteUShort(numBytes);
    if (numBytes)
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
        if (replicationManager_ && sceneLoaded_)
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
        SendMessage(MSG_LOADSCENE, msg_);
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

void Connection::Disconnect()
{
    if (transportConnection_)
        transportConnection_->Disconnect();
}

void Connection::SendRemoteEvents()
{
#ifdef URHO3D_LOGGING
    if (logStatistics_ && statsTimer_.GetMSec(false) > STATS_INTERVAL_MSEC)
    {
        statsTimer_.Reset();
        char statsBuffer[256];
        sprintf(statsBuffer, "PING %.3f ms Pkt in %i Pkt out %i Data in %.3f KB/s Data out %.3f KB/s", GetPing() / 1000.0f,
            GetPacketsInPerSec(),
            GetPacketsOutPerSec(),
            (float)GetBytesInPerSec(),
            (float)GetBytesOutPerSec());
        URHO3D_LOGINFO(statsBuffer);
    }
#endif

    if (remoteEvents_.empty())
        return;

    URHO3D_PROFILE("SendRemoteEvents");

    for (auto i = remoteEvents_.begin(); i != remoteEvents_.end(); ++i)
    {
        msg_.Clear();
        PacketTypeFlags packetType = PacketType::Reliable;
        if (i->inOrder_)
            packetType |= PacketType::Ordered;
        msg_.WriteStringHash(i->eventType_);
        msg_.WriteVariantMap(i->eventData_);
        SendMessage(MSG_REMOTEEVENT, msg_, packetType);
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
            SendMessage(MSG_PACKAGEDATA, msg_, PacketType::ReliableUnordered);

            // Check if upload finished
            if (upload.fragment_ == upload.totalFragments_)
                uploads_.erase(current);
        }
    }
}

void Connection::SendBuffer(PacketTypeFlags type, VectorBuffer& buffer)
{
    if (buffer.GetSize() < 1)
        return;

    if (transportConnection_)
    {
        packetCounterOutgoing_.AddSample(1);
        bytesCounterOutgoing_.AddSample(buffer.GetSize());
        transportConnection_->SendMessage({(const char*)buffer.GetData(), buffer.GetSize()}, type);
    }
    buffer.Clear();
}

void Connection::SendBuffer(PacketTypeFlags type)
{
    SendBuffer(type, outgoingBuffer_[type]);
}

void Connection::SendAllBuffers()
{
    // Send clock messages at the last time to have better precision
    if (clock_)
    {
        auto& buffer = outgoingBuffer_[PacketType::UnreliableUnordered];
        while (const auto clockMessage = clock_->PollMessage())
            WriteSerializedMessage(*this, MSG_CLOCK_SYNC, *clockMessage, PacketType::UnreliableUnordered);
    }

    SendBuffer(PacketType::ReliableOrdered);
    SendBuffer(PacketType::ReliableUnordered);
    SendBuffer(PacketType::UnreliableOrdered);
    SendBuffer(PacketType::UnreliableUnordered);
}

bool Connection::ProcessMessage(MemoryBuffer& buffer)
{
    int msgID;
    packetCounterIncoming_.AddSample(1);
    bytesCounterIncoming_.AddSample(buffer.GetSize());

    if (buffer.GetSize() < sizeof(msgID))
    {
        URHO3D_LOGERROR("Invalid network message size {}: too small.", buffer.GetSize());
        return false;
    }

    while (!buffer.IsEof())
    {
        msgID = buffer.ReadUShort();
        unsigned int packetSize = buffer.ReadUShort();
        MemoryBuffer msg(buffer.GetData() + buffer.GetPosition(), packetSize);
        buffer.Seek(buffer.GetPosition() + packetSize);

        Log::GetLogger().Write(GetMessageLogLevel((NetworkMessageId)msgID), "{}: Message #{} ({} bytes) received",
            ToString(),
            static_cast<unsigned>(msgID),
            packetSize);

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
    auto* vfs = GetSubsystem<VirtualFileSystem>();
    const ea::string& packageCacheDir = GetSubsystem<Network>()->GetPackageCacheDir();


    for (unsigned i = 0; i < vfs->NumMountPoints(); ++i)
    {
        const auto package = dynamic_cast<PackageFile*>(vfs->GetMountPoint(i));
        if (package && !package->GetName().find(packageCacheDir))
            vfs->Unmount(package);
    }

    // Now check which packages we have in the resource cache or in the download cache, and which we need to download
    unsigned numPackages = msg.ReadVLE();
    if (numPackages)
    {
        // Now check which packages we have in the resource cache or in the download cache, and which we need to download
        if (!RequestNeededPackages(numPackages, msg))
        {
            OnSceneLoadFailed();
            return;
        }
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
                GetSubsystem<VirtualFileSystem>()->MountPackageFile(download.file_->GetName());

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
                    SendMessage(MSG_REQUESTPACKAGE, msg_);
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
        SendMessage(MSG_SCENECHECKSUMERROR, msg_);
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
    return transportConnection_->GetState() == NetworkConnection::State::Connected;
}

unsigned long long Connection::GetBytesInPerSec() const
{
    return static_cast<int>(bytesCounterIncoming_.GetLast());
}

unsigned long long Connection::GetBytesOutPerSec() const
{
    return static_cast<int>(bytesCounterOutgoing_.GetLast());
}

int Connection::GetPacketsInPerSec() const
{
    return static_cast<int>(packetCounterIncoming_.GetLast());
}

int Connection::GetPacketsOutPerSec() const
{
    return static_cast<int>(packetCounterOutgoing_.GetLast());
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
    SendMessage(MSG_PACKAGEINFO, msg_);
}

void Connection::HandleAsyncLoadFinished()
{
    replicationManager_ = scene_->GetOrCreateComponent<ReplicationManager>();
    replicationManager_->StartClient(this);
    sceneLoaded_ = true;

    msg_.Clear();
    msg_.WriteUInt(scene_->GetChecksum());
    SendMessage(MSG_SCENELOADED, msg_);
}

bool Connection::RequestNeededPackages(unsigned numPackages, MemoryBuffer& msg)
{
    auto* vfs = GetSubsystem<VirtualFileSystem>();
    const ea::string& packageCacheDir = GetSubsystem<Network>()->GetPackageCacheDir();

    ea::vector<SharedPtr<PackageFile>> packages;
    for (unsigned i = 0; i<vfs->NumMountPoints(); ++i)
    {
        auto packageFile = dynamic_cast<PackageFile*>(vfs->GetMountPoint(i));
        if (packageFile)
            packages.push_back(SharedPtr<PackageFile>(packageFile));
    }
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

            GetSubsystem<FileSystem>()->ScanDir(downloadedPackages, packageCacheDir, "*.*", SCAN_FILES);
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
                    vfs->Mount(newPackage);
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
        SendMessage(MSG_REQUESTPACKAGE, msg_);
        download.initiated_ = true;
    }
}

void Connection::SendPackageError(const ea::string& name)
{
    msg_.Clear();
    msg_.WriteStringHash(name);
    SendMessage(MSG_PACKAGEDATA, msg_, PacketType::ReliableUnordered);
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
        SendMessage(MSG_SCENELOADED, msg_);
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
    if (transportConnection_)
        return transportConnection_->GetAddress();
    return "";
}

unsigned short Connection::GetPort() const
{
    if (transportConnection_)
        return transportConnection_->GetPort();
    return 0;
}

void Connection::ProcessPackets()
{
    MutexLock lock(packetQueueLock_);
    for (auto& packet : incomingPackets_)
    {
        MemoryBuffer msg(packet);
        if (!ProcessMessage(msg))
        {
            Disconnect();
            break;
        }
    }
    incomingPackets_.clear();
}

}
