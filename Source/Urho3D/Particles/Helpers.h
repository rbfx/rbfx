//
// Copyright (c) 2021-2022 the rbfx project.
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

#include "../Core/Context.h"
#include "ParticleGraphEffect.h"
#include "ParticleGraphLayerInstance.h"
#include "ParticleGraphNode.h"
#include "ParticleGraphNodeInstance.h"
#include "SpanVariants.h"
#include <EASTL/tuple.h>

namespace Urho3D
{

namespace ParticleGraphNodes
{

template <typename T>
struct GetPinType
{
    typedef T Type;
};
template <typename T> struct GetPinType<ParticleGraphTypedPin<T>>
{
    typedef T Type;
};

/// Abstract update runner.
template <typename Instance, typename... Values>
void RunUpdate(const UpdateContext& context, Instance& instance, ParticleGraphPinRef* pinRefs)
{
    auto spans = SpanVariantTuple<Values...>::Make(context, pinRefs);
    ea::apply(instance, ea::tuple_cat(ea::tie(context), ea::make_tuple(static_cast<unsigned>(context.indices_.size())), spans));
};

template <template <typename> typename T, typename ... Args>
void SelectByVariantType(VariantType variantType, Args... args)
{
    switch (variantType)
    {
    case VAR_INT: { T<int> fn; fn(args...); break; }
    case VAR_INT64: { T<long long> fn; fn(args...); break; }
    case VAR_BOOL: { T<bool> fn; fn(args...); break; }
    case VAR_FLOAT: { T<float> fn; fn(args...); break; }
    case VAR_DOUBLE: { T<double> fn; fn(args...); break; }
    case VAR_VECTOR2: { T<Vector2> fn; fn(args...); break; }
    case VAR_VECTOR3: { T<Vector3> fn; fn(args...); break; }
    case VAR_VECTOR4: { T<Vector4> fn; fn(args...); break; }
    case VAR_QUATERNION: { T<Quaternion> fn; fn(args...); break; }
    case VAR_COLOR: { T<Color> fn; fn(args...); break; }
    case VAR_STRING: { T<ea::string> fn; fn(args...); break; }
    case VAR_BUFFER: { T<VariantBuffer> fn; fn(args...); break; }
    case VAR_RESOURCEREF: { T<ResourceRef> fn; fn(args...); break; }
    case VAR_RESOURCEREFLIST: { T<ResourceRefList> fn; fn(args...); break; }
    case VAR_INTVECTOR2: { T<IntVector2> fn; fn(args...); break; }
    case VAR_INTVECTOR3: { T<IntVector2> fn; fn(args...); break; }
    default:
        assert(!"Not implemented");
    }
}

} // namespace ParticleGraphNodes

template <typename Value0> struct SpanVariantTuple<Value0>
{
    static auto Make(const UpdateContext& context, ParticleGraphPinRef* pinRefs)
    {
        return ea::make_tuple(context.GetSpan<typename ParticleGraphNodes::GetPinType<Value0>::Type>(pinRefs[0]));
    }
};
template <typename Value0, typename Value1> struct SpanVariantTuple<Value0, Value1>
{
    static auto Make(const UpdateContext& context, ParticleGraphPinRef* pinRefs)
    {
        return ea::make_tuple(context.GetSpan<typename ParticleGraphNodes::GetPinType<Value0>::Type>(pinRefs[0]),
            context.GetSpan<typename ParticleGraphNodes::GetPinType<Value1>::Type>(pinRefs[1]));
    }
};
template <typename Value0, typename Value1, typename Value2> struct SpanVariantTuple<Value0, Value1, Value2>
{
    static auto Make(const UpdateContext& context, ParticleGraphPinRef* pinRefs)
    {
        return ea::make_tuple(context.GetSpan<typename ParticleGraphNodes::GetPinType<Value0>::Type>(pinRefs[0]),
            context.GetSpan<typename ParticleGraphNodes::GetPinType<Value1>::Type>(pinRefs[1]),
            context.GetSpan<typename ParticleGraphNodes::GetPinType<Value2>::Type>(pinRefs[2]));
    }
};
template <typename Value0, typename Value1, typename Value2, typename Value3>
struct SpanVariantTuple<Value0, Value1, Value2, Value3>
{
    static auto Make(const UpdateContext& context, ParticleGraphPinRef* pinRefs)
    {
        return ea::make_tuple(context.GetSpan<typename ParticleGraphNodes::GetPinType<Value0>::Type>(pinRefs[0]),
            context.GetSpan<typename ParticleGraphNodes::GetPinType<Value1>::Type>(pinRefs[1]),
            context.GetSpan<typename ParticleGraphNodes::GetPinType<Value2>::Type>(pinRefs[2]),
            context.GetSpan<typename ParticleGraphNodes::GetPinType<Value3>::Type>(pinRefs[3]));
    }
};
template <typename Value0, typename Value1, typename Value2, typename Value3, typename Value4>
struct SpanVariantTuple<Value0, Value1, Value2, Value3, Value4>
{
    static auto Make(const UpdateContext& context, ParticleGraphPinRef* pinRefs)
    {
        return ea::make_tuple(context.GetSpan<typename ParticleGraphNodes::GetPinType<Value0>::Type>(pinRefs[0]),
            context.GetSpan<typename ParticleGraphNodes::GetPinType<Value1>::Type>(pinRefs[1]),
            context.GetSpan<typename ParticleGraphNodes::GetPinType<Value2>::Type>(pinRefs[2]),
            context.GetSpan<typename ParticleGraphNodes::GetPinType<Value3>::Type>(pinRefs[3]),
            context.GetSpan<typename ParticleGraphNodes::GetPinType<Value4>::Type>(pinRefs[4]));
    }
};
template <typename Value0, typename Value1, typename Value2, typename Value3, typename Value4, typename Value5>
struct SpanVariantTuple<Value0, Value1, Value2, Value3, Value4, Value5>
{
    static auto Make(const UpdateContext& context, ParticleGraphPinRef* pinRefs)
    {
        return ea::make_tuple(context.GetSpan<typename ParticleGraphNodes::GetPinType<Value0>::Type>(pinRefs[0]),
            context.GetSpan<typename ParticleGraphNodes::GetPinType<Value1>::Type>(pinRefs[1]),
            context.GetSpan<typename ParticleGraphNodes::GetPinType<Value2>::Type>(pinRefs[2]),
            context.GetSpan<typename ParticleGraphNodes::GetPinType<Value3>::Type>(pinRefs[3]),
            context.GetSpan<typename ParticleGraphNodes::GetPinType<Value4>::Type>(pinRefs[4]),
            context.GetSpan<typename ParticleGraphNodes::GetPinType<Value5>::Type>(pinRefs[5]));
    }
};
template <typename Value0, typename Value1, typename Value2, typename Value3, typename Value4, typename Value5,
    typename Value6>
struct SpanVariantTuple<Value0, Value1, Value2, Value3, Value4, Value5, Value6>
{
    static auto Make(const UpdateContext& context, ParticleGraphPinRef* pinRefs)
    {
        return ea::make_tuple(context.GetSpan<typename ParticleGraphNodes::GetPinType<Value0>::Type>(pinRefs[0]),
            context.GetSpan<typename ParticleGraphNodes::GetPinType<Value1>::Type>(pinRefs[1]),
            context.GetSpan<typename ParticleGraphNodes::GetPinType<Value2>::Type>(pinRefs[2]),
            context.GetSpan<typename ParticleGraphNodes::GetPinType<Value3>::Type>(pinRefs[3]),
            context.GetSpan<typename ParticleGraphNodes::GetPinType<Value4>::Type>(pinRefs[4]),
            context.GetSpan<typename ParticleGraphNodes::GetPinType<Value5>::Type>(pinRefs[5]),
            context.GetSpan<typename ParticleGraphNodes::GetPinType<Value6>::Type>(pinRefs[6]));
    }
};

} // namespace Urho3D
