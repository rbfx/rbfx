//
// Copyright (c) 2008-2016 the Urho3D project.
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

#include <Urho3D/Scene/LogicComponent.h>

using namespace Urho3D;

namespace Urho3D
{
class Controls;
}

//=============================================================================
//=============================================================================
enum CollisionLayerType
{
    ColLayer_None       = (0),

    ColLayer_Static     = (1<<0), // 1
    ColLayer_Unused     = (1<<1), // 2 -- previously thought Bullet used this as kinematic layer, turns out Bullet has a kinematic collision flag=2

    ColLayer_Character  = (1<<2), // 4

    ColLayer_Projectile = (1<<3), // 8

    ColLayer_Platform   = (1<<4), // 16
    ColLayer_Trigger    = (1<<5), // 32

    ColLayer_Ragdoll    = (1<<6), // 64
    ColLayer_Kinematic  = (1<<7), // 128

    ColLayer_All        = (0xffff)
};

enum CollisionMaskType
{
    ColMask_Static     = ~(ColLayer_Platform | ColLayer_Trigger),       // ~(16|32) = 65487
    ColMask_Character  = ~(ColLayer_Ragdoll | ColLayer_Kinematic),                           // ~(64)    = 65471
    ColMask_Kinematic  = ~(ColLayer_Ragdoll | ColLayer_Character),                           // ~(64)    = 65471
    ColMask_Projectile = ~(ColLayer_Trigger),                           // ~(32)    = 65503
    ColMask_Platform   = ~(ColLayer_Static | ColLayer_Trigger),         // ~(1|32)  = 65502
    ColMask_Trigger    = ~(ColLayer_Projectile | ColLayer_Platform),    // ~(8|16)  = 65511
    ColMask_Ragdoll    = ~(ColLayer_Character),                         // ~(4)     = 65531

    ColMask_Camera     = ~(ColLayer_Character | ColLayer_Projectile | ColLayer_Trigger) // ~(4|8|32) = 65491
};


