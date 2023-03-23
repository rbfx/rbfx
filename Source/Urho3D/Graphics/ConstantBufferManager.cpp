#include "./ConstantBufferManager.h"
#include "./GraphicsDefs.h"

namespace Urho3D
{
    ConstantBufferManager::ConstantBufferManager(Context* context) : Object(context) {
        unsigned size = MAX_SHADER_PARAMETER_GROUPS;
        while (size--) {
            data_[size] = ea::make_shared<ConstantBufferManagerData>();
        }
    }
    ConstantBufferManagerTicket* ConstantBufferManager::GetTicket(ShaderParameterGroup grp)
    {
        assert(grp < MAX_SHADER_PARAMETER_GROUPS);
        unsigned nextTicket = data_[grp]->nextTicket_;
        if (nextTicket >= data_[grp]->tickets_.size()) {
            ea::shared_ptr<ConstantBufferManagerTicket> ticket = ea::make_shared<ConstantBufferManagerTicket>();
            ticket->group_ = grp;
            ticket->id_ = data_[grp]->tickets_.size();
            data_[grp]->tickets_.push_back(ticket);
        }

        ConstantBufferManagerTicket* ticket = data_[grp]->tickets_[nextTicket++].get();
        data_[grp]->nextTicket_ = nextTicket;
        return ticket;
    }
    void ConstantBufferManager::Reset(ShaderParameterGroup grp)
    {
        assert(grp < MAX_SHADER_PARAMETER_GROUPS);
        data_[grp]->nextTicket_ = 0;
        data_[grp]->prevTicketDispatched_ = M_MAX_UNSIGNED;
    }
    bool ConstantBufferManager::PrepareBuffers()
    {
        bool hasChangedBuffers = false;
        for (unsigned i = 0; i < MAX_SHADER_PARAMETER_GROUPS; ++i) {
            ea::shared_ptr<ConstantBufferManagerData> data = data_[i];
            for (unsigned j = 0; j < data->tickets_.size(); ++j)
                data_[i]->cbufferSize_ = Max(data_[i]->cbufferSize_, data_[i]->tickets_[j]->data_.size());
            if (!data_[i]->cbuffer_) {
                hasChangedBuffers = true;
                data_[i]->cbuffer_ = new ConstantBuffer(context_);
#ifdef URHO3D_DEBUG
                data_[i]->cbuffer_->SetDbgName(ConstantBufferDebugNames[i]);
#endif
            }
            if (data_[i]->cbuffer_->GetSize() < data_[i]->cbufferSize_) {
                hasChangedBuffers = true;
                data_[i]->cbuffer_->SetSize(data_[i]->cbufferSize_);
            }
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
        const void* bufferData = ticket->data_.data();
        // Write data into buffer
        data->cbuffer_->Update(bufferData);
        data->prevTicketDispatched_ = ticketId;
    }
    void ConstantBufferManager::Finalize()
    {
        unsigned i = MAX_SHADER_PARAMETER_GROUPS;
        while (--i)
            Reset((ShaderParameterGroup)i);
    }
}
