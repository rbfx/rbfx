//
// Copyright (c) 2008-2018 the Urho3D project.
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

#include "../Core/Context.h"
#include "../Engine/PluginApplication.h"
#include "../IO/Log.h"
#if !defined(URHO3D_STATIC) && defined(URHO3D_PLUGINS)
#   if !defined(NDEBUG) && defined(URHO3D_LOGGING)
#       define CR_DEBUG 1
#       define CR_ERROR(format, ...) Urho3D::Log::Write(Urho3D::LOG_ERROR, Urho3D::ToString(format, ##__VA_ARGS__).Trimmed())
#       define CR_LOG(format, ...)   Urho3D::Log::Write(Urho3D::LOG_DEBUG, Urho3D::ToString(format, ##__VA_ARGS__).Trimmed())
#       define CR_TRACE
#   endif
#   include <cr/cr.h>
#endif

namespace Urho3D
{

PluginApplication::~PluginApplication()
{
    for (const auto& pair : registeredTypes_)
    {
        if (!pair.second_.Empty())
            context_->RemoveFactory(pair.first_, pair.second_.CString());
        else
            context_->RemoveFactory(pair.first_);
        context_->RemoveAllAttributes(pair.first_);
        context_->RemoveSubsystem(pair.first_);
    }
}

void PluginApplication::RecordPluginFactory(StringHash type, const char* category)
{
    registeredTypes_.Push({type, category});
}

#if !defined(URHO3D_STATIC) && defined(URHO3D_PLUGINS)
int PluginApplication::PluginMain(void* ctx_, size_t operation, PluginApplication*(*factory)(Context*))
{
    assert(ctx_);
    auto* ctx = static_cast<cr_plugin*>(ctx_);

    switch (operation)
    {
    case CR_LOAD:
    {
        auto* context = static_cast<Context*>(ctx->userdata);
        auto* application = factory(context);
        application->type_ = PLUGIN_NATIVE;
        application->Load();
        ctx->userdata = application;
        return 0;
    }
    case CR_UNLOAD:
    case CR_CLOSE:
    {
        auto* application = static_cast<PluginApplication*>(ctx->userdata);
        application->Unload();
        ctx->userdata = application->GetContext();
        if (application->Refs() != 1)
        {
            URHO3D_LOGERRORF("Plugin application '%s' has more than one reference remaining. "
                             "This may lead to memory leaks or crashes.",
                             application->GetTypeName().CString());
            assert(false);
        }
        application->ReleaseRef();
        return 0;
    }
    case CR_STEP:
    {
        return 0;
    }
    default:
		break;
    }
	assert(false);
	return -3;
}
#endif

}
