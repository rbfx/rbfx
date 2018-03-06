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
#include <cr/cr.h>


namespace Urho3D
{

static const StringHash contextKey("PluginApplication");

int PluginMain(void* ctx_, size_t operation, PluginApplication*(*factory)(Context*),
    void(*destroyer)(PluginApplication*))
{
    assert(ctx_);
    auto* ctx = static_cast<cr_plugin*>(ctx_);
    auto context = static_cast<Context*>(ctx->userdata);
    auto application = dynamic_cast<PluginApplication*>(context->GetGlobalVar(contextKey).GetPtr());

    switch (operation)
    {
    case CR_LOAD:
    {
        application = factory(context);
        context->SetGlobalVar(contextKey, application);
        application->OnLoad();
        return 0;
    }
    case CR_UNLOAD:
    case CR_CLOSE:
    {
        context->SetGlobalVar(contextKey, Variant::EMPTY);
        application->OnUnload();
        destroyer(application);
        return 0;
    }
    case CR_STEP:
    {
        application->OnUpdate();
        return 0;
    }
    default:
		break;
    }
	assert(false);
	return -3;
}

}
