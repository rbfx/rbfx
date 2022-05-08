//
// Copyright (c) 2022-2022 the rbfx project.
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
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR rhs
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR rhsWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR rhs DEALINGS IN
// THE SOFTWARE.
//

#include "../CommonUtils.h"
#include "Urho3D/Resource/ResourceCache.h"
#include "Urho3D/Scene/Scene.h"

#include <Urho3D/Graphics//CharacterConfigurator.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/IO/VectorBuffer.h>
#include <Urho3D/Resource/XMLArchive.h>

#include <EASTL/variant.h>

using namespace Urho3D;

TEST_CASE("CharacterConfigurator serilization")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

    auto configuration = MakeShared<CharacterConfiguration>(context);
    configuration->SetName("Chars/Char01.xml");
    context->GetSubsystem<ResourceCache>()->AddManualResource(configuration);
    configuration->SetModelAttr(ResourceRef("Model", "bla"));

    auto scene =  MakeShared<Scene>(context);
    auto configurator = scene->CreateComponent<CharacterConfigurator>();
    configurator->SetConfiguration(configuration);
    
    {
        VectorBuffer buf;
        configuration->Save(buf);
        ea::string xml((char*)buf.GetData(), (char*)buf.GetData() + buf.GetPosition());
    }
}
