// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace Urho3DNet
{
    public partial class VertexBuffer
    {
        // TODO: This should be removed when C++ API starts using enums instead of uints for flags.
        public bool SetSize(int vertexCount, VertexMask mask, bool isDynamic)
        {
            return SetSize((uint)vertexCount, (uint)mask, isDynamic);
        }
    }
}
