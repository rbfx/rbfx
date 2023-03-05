//
// Copyright (c) 2023-2023 the rbfx project.
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

#include <jni/jni.hpp>

namespace Urho3D
{
namespace Platform
{

/// JNI.HPP wrapper for the RbfxLambdaContainer java class.
struct RbfxLambdaContainer {
    virtual ~RbfxLambdaContainer() = default;

    static constexpr auto Name() { return "io/rebelfork/RbfxLambdaContainer"; }

    static void registerNative(jni::JNIEnv &env) { jni::Class<RbfxLambdaContainer>::Singleton(env); }

    static jni::Local<jni::Object<RbfxLambdaContainer> > Create(jni::JNIEnv &env, RbfxLambdaContainer* container);

    template<typename T>
    static jni::Local<jni::Object<RbfxLambdaContainer>> Create(jni::JNIEnv &env, const ea::function<T>& function);

    template<class T, class... Args>
    static void Invoke(jni::JNIEnv &env, const jni::Object<RbfxLambdaContainer>& container, Args&... args);
};

template <typename T>
struct RbfxLambdaContainerImpl: public RbfxLambdaContainer {
    virtual ~RbfxLambdaContainerImpl() override = default;

    RbfxLambdaContainerImpl(const ea::function<T>& function): function_(function){}

    ea::function<T> function_;
};

template<typename T>
jni::Local<jni::Object<RbfxLambdaContainer>> RbfxLambdaContainer::Create(jni::JNIEnv &env,  const ea::function<T>& function)
{
    return RbfxLambdaContainer::Create(env, new RbfxLambdaContainerImpl<T>(function));
}

template<class T, class... Args>
void RbfxLambdaContainer::Invoke(jni::JNIEnv &env, const jni::Object<RbfxLambdaContainer>& container, Args&... args)
{
    static auto &thisClass = jni::Class<RbfxLambdaContainer>::Singleton(env);
    static auto field = thisClass.GetField<jni::jlong>(env, "lambdaPtr_");
    jni::jlong lambdaPtr = container.Get(env, field);
    reinterpret_cast<RbfxLambdaContainerImpl<T>*>((long long)lambdaPtr)->function_(args...);
}

}
}
