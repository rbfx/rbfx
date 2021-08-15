//
// Copyright (c) 2017-2021 the rbfx project.
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

#include "CommonUtils.h"

#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Engine/EngineDefs.h>

namespace Tests
{

SharedPtr<Context> CreateCompleteTestContext()
{
    auto context = MakeShared<Context>();
    auto engine = new Engine(context);
    VariantMap parameters;
    parameters[EP_HEADLESS] = true;
    parameters[EP_LOG_QUIET] = true;
    const bool engineInitialized = engine->Initialize(parameters);
    REQUIRE(engineInitialized);
    return context;
}

void RunFrame(Context* context, float timeStep, float maxTimeStep)
{
    auto engine = context->GetSubsystem<Engine>();
    do
    {
        const float subTimeStep = ea::min(timeStep, maxTimeStep);
        engine->SetNextTimeStep(subTimeStep);
        engine->RunFrame();
        timeStep -= subTimeStep;
    }
    while (timeStep > 0.0f);
}

}
