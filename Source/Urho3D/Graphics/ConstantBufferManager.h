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

#pragma once
#include "../Graphics/ConstantBuffer.h"
#include "../Graphics/Graphics.h"

namespace Urho3D
{
    struct ConstantBufferManagerTicket {
        unsigned id_{};
        ByteVector data_{};
        ShaderParameterGroup group_{MAX_SHADER_PARAMETER_GROUPS};
    };
    struct ConstantBufferManagerData {
        ea::vector<ea::shared_ptr<ConstantBufferManagerTicket>> tickets_{};
        SharedPtr<ConstantBuffer> cbuffer_{};
        unsigned nextTicket_{ 0 };
        unsigned cbufferSize_{0};
        unsigned prevTicketDispatched_{M_MAX_UNSIGNED};
    };
    /// @{
    /// This class is used by the DrawCommandQueue
    /// They handle the whole process of writes on ConstantBuffer
    /// 1. DrawCommandQueue insert a write command for a ShaderParameterGroup (aka constant buffer slot)
    /// 1.1 For each write command, a ticket is retrieved for DrawCommandQueue write into data field
    /// 1.2 Each ticket is reused every time, if DrawCommandQueue needs more memory, then they will call resize() method from data_ field.
    /// 2. At DrawCommandQueue::Execute() PrepareBuffers() will be called to Allocate or Resize the whole ConstantBuffers
    /// 3. At DrawCommandQueue::Execute()
    /// 3.1 For each command, Dispatch() with the ticket id will be called to execute write command on GPU.
    /// 3.2 If previous id is already executed, then Dispatch execution will be skipped.
    /// 4. At end of DrawCommandQueue::Execute(), Finalize() method will be called to reset cursors
    /// @}
    class URHO3D_API ConstantBufferManager : public Object {
        URHO3D_OBJECT(ConstantBufferManager, Object);
    public:
        ConstantBufferManager(Context* context);
        SharedPtr<ConstantBuffer> GetCBuffer(ShaderParameterGroup grp) { return data_[grp]->cbuffer_; }
        ConstantBufferManagerTicket* GetTicket(ShaderParameterGroup grp);
        /// @{
        /// Reset Ticket cursor to 0
        /// @}
        void Reset(ShaderParameterGroup grp);
        /// @{
        /// This method will calculate the whole cbuffer size from tickets
        /// And allocate or reallocate then.
        /// return: true or false if cbuffers has been changed.
        /// @}
        bool PrepareBuffers();
        /// @{
        /// Execute write command at ticketId
        /// @}
        void Dispatch(ShaderParameterGroup grp, unsigned ticketId);
        void Finalize();
    private:
        ea::array<ea::shared_ptr<ConstantBufferManagerData>, MAX_SHADER_PARAMETER_GROUPS> data_;
    };
}