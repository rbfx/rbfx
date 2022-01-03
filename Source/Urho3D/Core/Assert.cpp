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

#include "../Precompiled.h"

#include "../Core/Assert.h"
#include "../IO/Log.h"

#include <cassert>
#include <stdexcept>

#include "../DebugNew.h"

namespace Urho3D
{

void AssertFailure(bool isFatal, ea::string_view expression, ea::string_view message, ea::string_view file,
    int line, ea::string_view function)
{
    // Always log an error
    URHO3D_LOGERROR("Assertion failure!\nExpression:\t{}\nMessage:\t{}\nFile:\t{}\nLine:\t{}\nFunction:\t{}\n",
        expression, message, file, line, function);

    // Show standard assert popup in debug mode
    assert(0);

    // Throw exception in release
    if (isFatal)
        throw std::logic_error("Assertion failure");
}

} // namespace Urho3D
