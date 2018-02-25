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

#include <Declarations/Class.hpp>
#include "FindBaseClassesPass.h"


namespace Urho3D
{

bool FindBaseClassesPass::Visit(Declaration* decl, CppApiPass::Event event)
{
    if (decl->kind_ == Declaration::Kind::Class)
    {
        if (decl->source_ == nullptr)
            return false;

        Class* cls = dynamic_cast<Class*>(decl);

        const cppast::cpp_class* astCls = dynamic_cast<const cppast::cpp_class*>(decl->source_);
        for (const auto& base : astCls->bases())
        {
            Class* baseClass = dynamic_cast<Class*>(generator->symbols_.Get(cppast::to_string(base.type())));
            if (baseClass != nullptr)
            {
                WeakPtr<Class> basePtr(baseClass);
                if (!cls->bases_.Contains(basePtr))
                    cls->bases_.Push(basePtr);
            }
        }
    }
    return true;
}

}
