#include "./ConstantBufferManager.h"
#include "./GraphicsDefs.h"

namespace Urho3D
{
    ConstantBufferManagerTicket::ConstantBufferManagerTicket(ConstantBufferManagerTicketDesc desc) :
        id_(desc.id_),
        size_(desc.size_),
        offset_(desc.offset_),
        group_(desc.group_),
        manager_(desc.manager_)
    {
    }

    unsigned char* ConstantBufferManagerTicket::GetPointerData() const {
        return manager_->GetBufferData(group_) + offset_;
    }

    ConstantBufferManager::ConstantBufferManager(Context* context) : Object(context) {
        unsigned size = MAX_SHADER_PARAMETER_GROUPS - 1;
        do{
            data_[size] = ea::make_shared<ConstantBufferManagerData>();
            buffer_[size] = ea::make_pair(0, ea::shared_array<uint8_t>(new uint8_t[0]));
        } while (size--);
    }
    ConstantBufferManagerTicket* ConstantBufferManager::GetTicket(ShaderParameterGroup grp, size_t size)
    {
        assert(grp < MAX_SHADER_PARAMETER_GROUPS);
        if (size == 0)
            return nullptr;
        ea::shared_ptr<ConstantBufferManagerData> mgrData = data_[grp];
        const unsigned nextTicket = data_[grp]->nextTicket_;
        ++mgrData->nextTicket_;

        ConstantBufferManagerTicketDesc desc = {};
        desc.group_ = grp;
        desc.id_ = nextTicket;
        desc.offset_ = mgrData->lastOffset_;
        desc.size_ = size;
        desc.manager_ = this;

        if (nextTicket >= data_[grp]->tickets_.size()) {
            ea::shared_ptr<ConstantBufferManagerTicket> ticket = ea::make_shared<ConstantBufferManagerTicket>(desc);
            data_[grp]->tickets_.push_back(ticket);
        }

        size_t totalSize = mgrData->lastOffset_ + size;
        auto bufferPair = buffer_[grp];
        if (totalSize > bufferPair.first) {
            ea::shared_array<uint8_t> newBuffer(new uint8_t[totalSize]);
            if (bufferPair.first > 0)
                memcpy_s(newBuffer.get(), bufferPair.first, bufferPair.second.get(), bufferPair.first);
            buffer_[grp] = ea::make_pair(totalSize, newBuffer);
        }

        mgrData->cbufferSize_ = Max(size, mgrData->cbufferSize_);
        mgrData->lastOffset_ = totalSize;

        ea::shared_ptr<ConstantBufferManagerTicket> ticket = data_[grp]->tickets_[nextTicket];
        // If ticket has different size from last frame, we must reallocate then with new sizes
        if (ticket->GetSize() != size)
            mgrData->tickets_[nextTicket] = ticket = ea::make_shared<ConstantBufferManagerTicket>(desc);

        return ticket.get();
    }
    void ConstantBufferManager::Reset(ShaderParameterGroup grp)
    {
        assert(grp < MAX_SHADER_PARAMETER_GROUPS);
        data_[grp]->nextTicket_ = 0;
        data_[grp]->lastOffset_ = 0;
        data_[grp]->cbufferSize_ = 0;
        data_[grp]->prevTicketDispatched_ = M_MAX_UNSIGNED;
    }
    bool ConstantBufferManager::PrepareBuffers()
    {
        bool hasChangedBuffers = false;
        for (unsigned i = 0; i < MAX_SHADER_PARAMETER_GROUPS; ++i) {
            ea::shared_ptr<ConstantBufferManagerData> data = data_[i];
            if (!data_[i]->cbuffer_) {
                data_[i]->cbuffer_ = new ConstantBuffer(context_);
#ifdef URHO3D_DEBUG
                data_[i]->cbuffer_->SetDbgName(ConstantBufferDebugNames[i]);
#endif
            }
            if (data_[i]->cbufferSize_ > data_[i]->cbuffer_->GetSize())
                data_[i]->cbuffer_->SetSize(data_[i]->cbufferSize_);
        }

        return hasChangedBuffers;
    }
    void ConstantBufferManager::Dispatch(ShaderParameterGroup grp, unsigned ticketId)
    {
        assert(grp < MAX_SHADER_PARAMETER_GROUPS);
        if (ticketId == M_MAX_UNSIGNED)
            return;
        auto data = data_[grp];
        if (ticketId >= data->tickets_.size()) {
            URHO3D_LOGERROR("Invalid TicketId.");
            return;
        }
        size_t cbufferSize = data->cbuffer_->GetSize();
        // Unallocated CBuffer, this must never occurs
        if (cbufferSize == 0)
            return;
        // Don't write twice if previous ticket has been already executed.
        if (ticketId == data->prevTicketDispatched_)
            return;

        auto ticket = data->tickets_[ticketId];
        const void* bufferData = ticket->GetPointerData();
        // Write data into buffer
        data->cbuffer_->Update(bufferData);
        data->prevTicketDispatched_ = ticketId;
    }
    void ConstantBufferManager::Finalize()
    {
        unsigned i = MAX_SHADER_PARAMETER_GROUPS - 1;
        do {
            Reset((ShaderParameterGroup)i);
        } while (i--);
    }
}
