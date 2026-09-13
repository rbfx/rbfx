// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "ParticleGraphPin.h"
#include "UpdateContext.h"

namespace Urho3D
{

//template <typename T> SpanVariant<T>::SpanVariant(UpdateContext& context, ParticleGraphPinRef& pinRef)
//{
//    type_ = pinRef.type_;
//    if (type_ == ParticleGraphContainerType::Sparse)
//    {
//        data_ = context.layer_->GetAttributeValues<T>(pinRef.index_).data_;
//        indices_ = context.indices_.data();
//    }
//    else
//    {
//        const auto tempLocation = context.layer_->GetLayer()->GetIntermediateValues()[pinRef.index_];
//        data_ = reinterpret_cast<T*>(context.tempBuffer_.data() + tempLocation.offset_);
//        indices_ = nullptr;
//    }
//}

} // namespace Urho3D
