//
// Copyright (c) 2017-2021 the rbfx project.
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

namespace
{

class TestFooInterface
{
public:
    virtual void Foo() = 0;
};

class TestBarInterface
{
public:
    virtual void Bar() = 0;
};

class TestObject : public RefCounted, public TestFooInterface, public TestBarInterface
{
public:
    void Foo() override {}
    void Bar() override {}
};

}

TEST_CASE("SharedPtr is converted between types")
{
    // Create derived SharedPtr
    const auto objectPtr = MakeShared<TestObject>();
    REQUIRE(objectPtr != nullptr);
    REQUIRE(objectPtr.Refs() == 1);
    REQUIRE(objectPtr.WeakRefs() == 0);
    objectPtr->Foo();
    objectPtr->Bar();

    // Create base SharedPtrs
    SharedPtrT<RefCounted> refCountedPtr = objectPtr;
    SharedPtrT<TestFooInterface> fooPtr = objectPtr;
    const SharedPtrT<TestBarInterface> barPtr = objectPtr;

    REQUIRE(objectPtr.Refs() == 4);
    REQUIRE(objectPtr.WeakRefs() == 0);

    REQUIRE(refCountedPtr == objectPtr);
    REQUIRE(refCountedPtr.Refs() == 4);
    REQUIRE(refCountedPtr.WeakRefs() == 0);

    REQUIRE(fooPtr == objectPtr);
    REQUIRE(fooPtr.Refs() == 4);
    REQUIRE(fooPtr.WeakRefs() == 0);

    REQUIRE(barPtr == objectPtr);
    REQUIRE(barPtr.Refs() == 4);
    REQUIRE(barPtr.WeakRefs() == 0);

    fooPtr->Foo();
    barPtr->Bar();

    // Move SharedPtrs
    SharedPtrT<RefCounted> refCountedPtr2 = ea::move(refCountedPtr);
    SharedPtrT<TestFooInterface> fooPtr2 = ea::move(fooPtr);

    REQUIRE(objectPtr.Refs() == 4);
    REQUIRE(objectPtr.WeakRefs() == 0);

    REQUIRE(refCountedPtr == nullptr);
    REQUIRE(refCountedPtr.Refs() == 0);
    REQUIRE(refCountedPtr.WeakRefs() == 0);

    REQUIRE(refCountedPtr2 == objectPtr);
    REQUIRE(refCountedPtr2.Refs() == 4);
    REQUIRE(refCountedPtr2.WeakRefs() == 0);

    REQUIRE(fooPtr == nullptr);
    REQUIRE(fooPtr.Refs() == 0);
    REQUIRE(fooPtr.WeakRefs() == 0);

    REQUIRE(fooPtr2 == objectPtr);
    REQUIRE(fooPtr2.Refs() == 4);
    REQUIRE(fooPtr2.WeakRefs() == 0);

    REQUIRE(barPtr == objectPtr);
    REQUIRE(barPtr.Refs() == 4);
    REQUIRE(barPtr.WeakRefs() == 0);

    // Create WeakPtrs
    WeakPtrT<TestObject> weakObjectPtr = objectPtr;
    WeakPtrT<TestFooInterface> weakFooPtr = fooPtr;
    WeakPtrT<TestFooInterface> weakFooPtr2 = fooPtr2;
    WeakPtrT<TestBarInterface> weakBarPtr = barPtr;

    REQUIRE(objectPtr.Refs() == 4);
    REQUIRE(objectPtr.WeakRefs() == 3);
    REQUIRE(weakObjectPtr.Get() == objectPtr);
    REQUIRE(weakObjectPtr.Refs() == 4);
    REQUIRE(weakObjectPtr.WeakRefs() == 3);

    REQUIRE(refCountedPtr == nullptr);
    REQUIRE(refCountedPtr.Refs() == 0);
    REQUIRE(refCountedPtr.WeakRefs() == 0);

    REQUIRE(refCountedPtr2 == objectPtr);
    REQUIRE(refCountedPtr2.Refs() == 4);
    REQUIRE(refCountedPtr2.WeakRefs() == 3);

    REQUIRE(fooPtr == nullptr);
    REQUIRE(fooPtr.Refs() == 0);
    REQUIRE(fooPtr.WeakRefs() == 0);
    REQUIRE(weakFooPtr == nullptr);
    REQUIRE(weakFooPtr.Refs() == 0);
    REQUIRE(weakFooPtr.WeakRefs() == 0);

    REQUIRE(fooPtr2 == objectPtr);
    REQUIRE(fooPtr2.Refs() == 4);
    REQUIRE(fooPtr2.WeakRefs() == 3);
    REQUIRE(weakFooPtr2.Get() == objectPtr);
    REQUIRE(weakFooPtr2.Refs() == 4);
    REQUIRE(weakFooPtr2.WeakRefs() == 3);

    REQUIRE(barPtr == objectPtr);
    REQUIRE(barPtr.Refs() == 4);
    REQUIRE(barPtr.WeakRefs() == 3);
    REQUIRE(weakBarPtr.Get() == objectPtr);
    REQUIRE(weakBarPtr.Refs() == 4);
    REQUIRE(weakBarPtr.WeakRefs() == 3);

    // Lock WeakPtrs
    WeakPtrT<RefCounted> weakRefCountedPtr = refCountedPtr;
    WeakPtrT<RefCounted> weakRefCountedPtr2 = refCountedPtr2;
    auto lockedObjectPtr = weakObjectPtr.Lock();
    auto lockedRefCountedPtr = weakRefCountedPtr.Lock();
    auto lockedRefCountedPtr2 = weakRefCountedPtr2.Lock();

    REQUIRE(lockedObjectPtr == objectPtr);
    REQUIRE(lockedObjectPtr.Refs() == 6);
    REQUIRE(lockedObjectPtr.WeakRefs() == 4);

    REQUIRE(lockedRefCountedPtr == nullptr);
    REQUIRE(lockedRefCountedPtr.Refs() == 0);
    REQUIRE(lockedRefCountedPtr.WeakRefs() == 0);

    REQUIRE(lockedRefCountedPtr2 == objectPtr);
    REQUIRE(lockedRefCountedPtr2.Refs() == 6);
    REQUIRE(lockedRefCountedPtr2.WeakRefs() == 4);
}

TEST_CASE("WeakPtr is consistent on expiration")
{
    auto objectPtr = MakeShared<TestObject>();
    WeakPtr<TestObject> weakObjectPtr = objectPtr;

    objectPtr.Reset();

    REQUIRE(objectPtr == nullptr);
    REQUIRE(objectPtr.Refs() == 0);
    REQUIRE(objectPtr.WeakRefs() == 0);

    REQUIRE(weakObjectPtr == nullptr);
    REQUIRE(weakObjectPtr.Refs() == 0);
    REQUIRE(weakObjectPtr.WeakRefs() == 1);
}
