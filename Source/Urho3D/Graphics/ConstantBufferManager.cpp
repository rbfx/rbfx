#include "./ConstantBufferManager.h"

namespace Urho3D
{
    ConstantBufferManager::ConstantBufferManager(Context* context) : Object(context) {
        unsigned size = MAX_SHADER_PARAMETER_GROUPS;
        while (--size) {
            data_[size] = new ConstantBufferManagerData();
        }
    }
    ConstantBufferManagerTicket* ConstantBufferManager::GetTicket(ShaderParameterGroup grp)
    {
        assert(grp < MAX_SHADER_PARAMETER_GROUPS);
        unsigned nextTicket = data_[grp]->nextTicket_;
        if (nextTicket >= data_[grp]->tickets_.size()) {
            ConstantBufferManagerTicket ticket;
            ticket.group_ = grp;
            ticket.id_ = data_[grp]->tickets_.size();
            data_[grp]->tickets_.push_back(ticket);
        }
        return &data_[grp]->tickets_[nextTicket++];
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
            ConstantBufferManagerData* data = data_[i];
            for (unsigned j = 0; j < data->tickets_.size(); ++j)
                data_[i]->cbufferSize_ = Max(data_[i]->cbufferSize_, data_[i]->tickets_[j].data_.size());
            if (!data_[i]->cbuffer_) {
                hasChangedBuffers = true;
                data_[i]->cbuffer_ = new ConstantBuffer(context_);
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
        ConstantBufferManagerData* data = data_[grp];
        if (ticketId >= data->tickets_.size()) {
            URHO3D_LOGERROR("Invalid TicketId.");
            return;
        }
        if (data->cbuffer_->GetSize() == 0)
            return;
        // Don't write twice if previous ticket has been already executed.
        if (ticketId == data->prevTicketDispatched_)
            return;

        ConstantBufferManagerTicket ticket = data->tickets_[ticketId];
        // Write data into buffer
        data->cbuffer_->Update(ticket.data_.data());
        data->prevTicketDispatched_ = ticketId;
    }
    void ConstantBufferManager::Finalize()
    {
        unsigned i = MAX_SHADER_PARAMETER_GROUPS;
        while (--i)
            Reset((ShaderParameterGroup)i);
    }
}
