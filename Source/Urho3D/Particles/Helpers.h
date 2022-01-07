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
template <typename Instance, typename Tuple>
void RunUpdate(UpdateContext& context, Instance& instance, bool scalar,
               ParticleGraphPinRef* pinRefs, Tuple tuple)
{
    ea::apply(instance,
        ea::tuple_cat(
            ea::tie(context), ea::make_tuple(static_cast<unsigned>(scalar ? 1 : context.indices_.size())), tuple));
};

/// Abstract update runner.
template <typename Instance, typename Tuple, typename Value0, typename... Values>
void RunUpdate(UpdateContext& context, Instance& instance, bool scalar,
               ParticleGraphPinRef* pinRefs, Tuple tuple)
{
    typedef typename GetPinType<Value0>::Type ValueType;
    switch (pinRefs[0].type_)
    {
    case ParticleGraphContainerType::Span:
    {
        auto nextTuple = ea::tuple_cat(std::move(tuple), ea::make_tuple(context.GetSpan<ValueType>(*pinRefs)));
        RunUpdate<Instance, decltype(nextTuple), Values...>(context, instance, false, pinRefs + 1,
                                                                  nextTuple);
        return;
    }
    case ParticleGraphContainerType::Sparse:
    {
        auto nextTuple = ea::tuple_cat(std::move(tuple), ea::make_tuple(context.GetSparse<ValueType>(*pinRefs)));
        RunUpdate<Instance, decltype(nextTuple), Values...>(context, instance, false, pinRefs + 1,
                                                                  nextTuple);
        return;
    }
    case ParticleGraphContainerType::Scalar:
    {
        auto nextTuple = ea::tuple_cat(std::move(tuple), ea::make_tuple(context.GetScalar<ValueType>(*pinRefs)));
        RunUpdate<Instance, decltype(nextTuple), Values...>(context, instance, scalar, pinRefs + 1,
                                                                  nextTuple);
        return;
    }
    default:
        assert(!"Invalid pin container type permutation");
        break;
    }
};

/// Abstract update runner.
template <typename Instance, typename Value0, typename... Values>
void RunUpdate(UpdateContext& context, Instance& instance, ParticleGraphPinRef* pinRefs)
{
    typedef typename GetPinType<Value0>::Type ValueType;
    switch (pinRefs[0].type_)
    {
    case ParticleGraphContainerType::Span:
    {
        ea::tuple<ea::span<ValueType>> nextTuple = ea::make_tuple(context.GetSpan<ValueType>(*pinRefs));
        RunUpdate<Instance, ea::tuple<ea::span<ValueType>>, Values...>(
            context, instance, false, pinRefs + 1, std::move(nextTuple));
        return;
    }
    case ParticleGraphContainerType::Sparse:
    {
        ea::tuple<SparseSpan<ValueType>> nextTuple = ea::make_tuple(context.GetSparse<ValueType>(*pinRefs));
        RunUpdate<Instance, ea::tuple<SparseSpan<ValueType>>, Values...>(
            context, instance, false, pinRefs + 1, std::move(nextTuple));
        return;
    }
    case ParticleGraphContainerType::Scalar:
    {
        ea::tuple<ScalarSpan<ValueType>> nextTuple = ea::make_tuple(context.GetScalar<ValueType>(*pinRefs));
        RunUpdate<Instance, ea::tuple<ScalarSpan<ValueType>>, Values...>(
            context, instance, true, pinRefs + 1, std::move(nextTuple));
        return;
    }
    default: assert(!"Invalid pin container type permutation"); break;
    }
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

} // namespace Urho3D
