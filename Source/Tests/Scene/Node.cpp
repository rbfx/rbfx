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
#include "../SceneUtils.h"
#include <Urho3D/Graphics/StaticModel.h>

TEST_CASE("Load node from XML node file")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto scene = MakeShared<Scene>(context);

    auto child = scene->CreateChild("Child");

    XMLFile file(context);
    auto nodeElement = file.GetOrCreateRoot("node");
    auto constraint2Attr = nodeElement.CreateChild("attribute");
    constraint2Attr.SetAttribute("name", "Name");
    constraint2Attr.SetAttribute("value", "NodeName");
    auto componentElement = nodeElement.CreateChild("component");
    componentElement.SetAttribute("type", "StaticModel");

    child->LoadXML(nodeElement);

    CHECK(child->GetName() == "NodeName");
    CHECK(child->GetComponent<StaticModel>());
};
