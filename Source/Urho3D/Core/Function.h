//
// Copyright (c) 2017-2019 the rbfx project.
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


#include <type_traits>


namespace Urho3D
{

template <int StorageSize, typename T>
class URHO3D_API FunctionImpl;

template <int StorageSize, typename Return, typename... Args>
class URHO3D_API FunctionImpl<StorageSize, Return(Args...)>
{
    /// Internal operation enum.
    enum class Operation
    {
        /// Destroy internal storage.
        Destruct,
        /// Move internal storage contents to another memory location.
        MoveAndDestruct,
        /// Copy-construct internal storage.
        Copy,
    };
public:
    /// Construct.
    FunctionImpl() noexcept = default;

    /// Construct.
    template <class T, typename = typename std::enable_if<std::is_invocable<T, Args...>::value>::type>
    FunctionImpl(T functor) noexcept
    {
        invoker_ = [](const void* storage, Args... args) -> Return
        {
            const T& function = *static_cast<const T*>(storage);
            return function(static_cast<Args&&>(args)...);
        };

        operator_ = [](Operation op, void* storage, void* other)
        {
            switch (op)
            {
            case Operation::Destruct:
            {
                T& functor = *static_cast<T*>(storage);
                functor.~T();
                break;
            }
            case Operation::MoveAndDestruct:
            {
                T& otherFunctor = *static_cast<T*>(other);
                new(storage) T(ea::move(otherFunctor));
                otherFunctor.~T();
                break;
            }
            case Operation::Copy:
            {
                const T& otherFunctor = *static_cast<const T*>(other);
                new(storage) T(otherFunctor);
            }
            default:
                assert(false);
            }
        };

        static_assert(sizeof(T) <= sizeof(storage_));
        new(storage_) T(ea::move(functor));
    }

    /// Copy-construct.
    FunctionImpl(const FunctionImpl& other) noexcept
    {
        operator=(other);
    }

    /// Copy-assign.
    FunctionImpl& operator=(const FunctionImpl& other) noexcept
    {
        Reset();
        invoker_ = other.invoker_;
        operator_ = other.operator_;
        if (operator_)
            operator_(Operation::Copy, storage_, other.storage_);
        return *this;
    }

    /// Move-construct.
    FunctionImpl(FunctionImpl&& other) noexcept
    {
        operator=(ea::move(other));
    }

    /// Move-assign.
    FunctionImpl& operator=(FunctionImpl&& other) noexcept
    {
        Reset();
        ea::swap(invoker_, other.invoker_);
        ea::swap(operator_, other.operator_);
        if (operator_)
            operator_(Operation::MoveAndDestruct, storage_, other.storage_);
        return *this;
    }

    /// Resets object state.
    void Reset() noexcept
    {
        if(operator_)
        {
            operator_(Operation::Destruct, storage_, nullptr);
            operator_ = nullptr;
        }
        invoker_ = nullptr;
    }

    /// Destruct.
    ~FunctionImpl()
    {
        Reset();
    }

    /// Invoke.
    Return operator()(Args... args) const
    {
        assert(invoker_ != nullptr);
        return invoker_(storage_, static_cast<Args&&>(args)...);
    }

    /// Returns true when function is invokable.
    operator bool() noexcept { return invoker_ != nullptr; }

private:
    /// Functor invoker type.
    using Invoker = Return(*)(const void*, Args...);
    /// Functor operator type.
    using Operator = void(*)(Operation, void*, void*);

    /// Functor invoker instance.
    Invoker invoker_ = nullptr;
    /// Functor destructor instance.
    Operator operator_ = nullptr;
    /// Storage for functor instance.
    void* storage_[StorageSize]{};
};

template <typename T> using Function = FunctionImpl<4, T>;

}
