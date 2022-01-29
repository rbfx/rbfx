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
#include <EASTL/tuple.h>

namespace Urho3D
{

namespace ParticleGraphNodes
{

struct PinPatternBase
{
    PinPatternBase(ParticleGraphPinFlags flags, ea::string_view name, VariantType type = VAR_NONE)
        : flags_(flags)
        , name_(name)
        , nameHash_(name)
        , type_(type)
    {
    }
    PinPatternBase(ea::string_view name, VariantType type = VAR_NONE)
        : PinPatternBase(ParticleGraphPinFlag::Input, name, type)
    {
    }
    ParticleGraphPinFlags flags_;
    ea::string_view name_;
    StringHash nameHash_;
    VariantType type_;
};

template <typename T> struct PinPattern : public PinPatternBase
{
    typedef T Type;

    PinPattern(ParticleGraphPinFlags flags, const char* name)
        : PinPatternBase(flags, name, GetVariantType<T>())
    {
    }
    PinPattern(const char* name)
        : PinPatternBase(name, GetVariantType<T>())
    {
    }
};

template <typename T>
struct GetPinType
{
    typedef T Type;
};
template <typename T> struct GetPinType<PinPattern<T>>
{
    typedef T Type;
};


/// Abstract update runner.
template <typename Instance, typename SpanTuple, typename Tuple>
void RunUpdate(UpdateContext& context, Instance& instance, bool scalar, SpanTuple& spanTuple, Tuple tuple)
{
    ea::apply(instance,
        ea::tuple_cat(
            ea::tie(context), ea::make_tuple(static_cast<unsigned>(scalar ? 1 : context.indices_.size())), tuple));
};

/// Abstract update runner.
template <typename Instance, typename SpanTuple, typename Tuple, typename Value0, typename... Values>
void RunUpdate(UpdateContext& context, Instance& instance, bool scalar, SpanTuple& spanTuple, Tuple tuple)
{
    typedef typename GetPinType<Value0>::Type ValueType;
    constexpr size_t index = ea::tuple_size<Tuple>();
    auto& span = ea::get<index>(spanTuple);
    switch (span.type_)
    {
    case ParticleGraphContainerType::Span:
    {
        auto nextTuple = ea::tuple_cat(std::move(tuple), ea::make_tuple(span.GetSpan()));
        RunUpdate<Instance, SpanTuple, decltype(nextTuple), Values...>(
            context, instance, false, spanTuple, nextTuple);
        return;
    }
    case ParticleGraphContainerType::Sparse:
    {
        auto nextTuple = ea::tuple_cat(std::move(tuple), ea::make_tuple(span.GetSparse()));
        RunUpdate<Instance, SpanTuple, decltype(nextTuple), Values...>(
            context, instance, false, spanTuple, nextTuple);
        return;
    }
    case ParticleGraphContainerType::Scalar:
    {
        auto nextTuple = ea::tuple_cat(std::move(tuple), ea::make_tuple(span.GetScalar()));
        RunUpdate<Instance, SpanTuple, decltype(nextTuple), Values...>(
            context, instance, scalar, spanTuple, nextTuple);
        return;
    }
    default:
        assert(!"Invalid pin container type");
        break;
    }
};

/// Abstract update runner.
template <typename Instance, typename... Values>
void RunUpdate(UpdateContext& context, Instance& instance, ParticleGraphPinRef* pinRefs)
{
    auto spans = SpanVariantTuple<Values...>::Make(context, pinRefs);
    RunUpdate<Instance, decltype(spans), ea::tuple<>, Values...>(
        context, instance, true, spans, ea::tuple<>());
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
    static auto Make(UpdateContext& context, ParticleGraphPinRef* pinRefs)
    {
        return ea::make_tuple(SpanVariant<typename ParticleGraphNodes::GetPinType<Value0>::Type>(context, pinRefs[0]));
    }
};
template <typename Value0, typename Value1> struct SpanVariantTuple<Value0, Value1>
{
    static auto Make(UpdateContext& context, ParticleGraphPinRef* pinRefs)
    {
        return ea::make_tuple(SpanVariant<typename ParticleGraphNodes::GetPinType<Value0>::Type>(context, pinRefs[0]),
            SpanVariant<typename ParticleGraphNodes::GetPinType<Value1>::Type>(context, pinRefs[1]));
    }
};
template <typename Value0, typename Value1, typename Value2> struct SpanVariantTuple<Value0, Value1, Value2>
{
    static auto Make(UpdateContext& context, ParticleGraphPinRef* pinRefs)
    {
        return ea::make_tuple(SpanVariant<typename ParticleGraphNodes::GetPinType<Value0>::Type>(context, pinRefs[0]),
            SpanVariant<typename ParticleGraphNodes::GetPinType<Value1>::Type>(context, pinRefs[1]),
            SpanVariant<typename ParticleGraphNodes::GetPinType<Value2>::Type>(context, pinRefs[2]));
    }
};
template <typename Value0, typename Value1, typename Value2, typename Value3>
struct SpanVariantTuple<Value0, Value1, Value2, Value3>
{
    static auto Make(UpdateContext& context, ParticleGraphPinRef* pinRefs)
    {
        return ea::make_tuple(SpanVariant<typename ParticleGraphNodes::GetPinType<Value0>::Type>(context, pinRefs[0]),
            SpanVariant<typename ParticleGraphNodes::GetPinType<Value1>::Type>(context, pinRefs[1]),
            SpanVariant<typename ParticleGraphNodes::GetPinType<Value2>::Type>(context, pinRefs[2]),
            SpanVariant<typename ParticleGraphNodes::GetPinType<Value3>::Type>(context, pinRefs[3]));
    }
};
template <typename Value0, typename Value1, typename Value2, typename Value3, typename Value4>
struct SpanVariantTuple<Value0, Value1, Value2, Value3, Value4>
{
    static auto Make(UpdateContext& context, ParticleGraphPinRef* pinRefs)
    {
        return ea::make_tuple(SpanVariant<typename ParticleGraphNodes::GetPinType<Value0>::Type>(context, pinRefs[0]),
            SpanVariant<typename ParticleGraphNodes::GetPinType<Value1>::Type>(context, pinRefs[1]),
            SpanVariant<typename ParticleGraphNodes::GetPinType<Value2>::Type>(context, pinRefs[2]),
            SpanVariant<typename ParticleGraphNodes::GetPinType<Value3>::Type>(context, pinRefs[3]),
            SpanVariant<typename ParticleGraphNodes::GetPinType<Value4>::Type>(context, pinRefs[4]));
    }
};
template <typename Value0, typename Value1, typename Value2, typename Value3, typename Value4, typename Value5>
struct SpanVariantTuple<Value0, Value1, Value2, Value3, Value4, Value5>
{
    static auto Make(UpdateContext& context, ParticleGraphPinRef* pinRefs)
    {
        return ea::make_tuple(SpanVariant<typename ParticleGraphNodes::GetPinType<Value0>::Type>(context, pinRefs[0]),
            SpanVariant<typename ParticleGraphNodes::GetPinType<Value1>::Type>(context, pinRefs[1]),
            SpanVariant<typename ParticleGraphNodes::GetPinType<Value2>::Type>(context, pinRefs[2]),
            SpanVariant<typename ParticleGraphNodes::GetPinType<Value3>::Type>(context, pinRefs[3]),
            SpanVariant<typename ParticleGraphNodes::GetPinType<Value4>::Type>(context, pinRefs[4]),
            SpanVariant<typename ParticleGraphNodes::GetPinType<Value5>::Type>(context, pinRefs[5]));
    }
};
template <typename Value0, typename Value1, typename Value2, typename Value3, typename Value4, typename Value5,
    typename Value6>
struct SpanVariantTuple<Value0, Value1, Value2, Value3, Value4, Value5, Value6>
{
    static auto Make(UpdateContext& context, ParticleGraphPinRef* pinRefs)
    {
        return ea::make_tuple(SpanVariant<typename ParticleGraphNodes::GetPinType<Value0>::Type>(context, pinRefs[0]),
            SpanVariant<typename ParticleGraphNodes::GetPinType<Value1>::Type>(context, pinRefs[1]),
            SpanVariant<typename ParticleGraphNodes::GetPinType<Value2>::Type>(context, pinRefs[2]),
            SpanVariant<typename ParticleGraphNodes::GetPinType<Value3>::Type>(context, pinRefs[3]),
            SpanVariant<typename ParticleGraphNodes::GetPinType<Value4>::Type>(context, pinRefs[4]),
            SpanVariant<typename ParticleGraphNodes::GetPinType<Value5>::Type>(context, pinRefs[5]),
            SpanVariant<typename ParticleGraphNodes::GetPinType<Value6>::Type>(context, pinRefs[6]));
    }
};

} // namespace Urho3D
