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

#include "../../Monetization/Android/RbfxBillingClientStateListener.h"
#include "../../Monetization/Android/BillingManager.h"
#include "../../Monetization/Android/BillingResult.h"
#include <jni.h>

extern "C"
JNIEXPORT void JNICALL Java_io_rebelfork_RbfxBillingClientStateListener_disposePtr(JNIEnv* env, jobject obj, jlong ptr) {
}

extern "C"
JNIEXPORT void JNICALL Java_io_rebelfork_RbfxBillingClientStateListener_onBillingServiceDisconnected(JNIEnv *envPtr, jobject obj)
{
    using namespace Urho3D;
    using namespace Urho3D::Platform;
    jni::JNIEnv& env = *envPtr;

    static auto &thisClass = jni::Class<RbfxBillingClientStateListener>::Singleton(env);
    static auto method = thisClass.GetMethod<jni::jlong()>(env, "getPtr");

    auto thisObject = jni::Local<jni::Object<RbfxBillingClientStateListener>>(env, jni::Wrap<jni::jobject*>(env.NewLocalRef(obj)));
    long long savedPtr = thisObject.Call(env, method);
    auto billingManager = reinterpret_cast<BillingManagerAndroid*>(savedPtr);

    billingManager->BillingServiceDisconnected();
}

extern "C"
JNIEXPORT void JNICALL Java_io_rebelfork_RbfxBillingClientStateListener_onBillingSetupFinished(JNIEnv *envPtr, jobject obj, jobject billingResult)
{
    using namespace Urho3D;
    using namespace Urho3D::Platform;
    jni::JNIEnv& env = *envPtr;

    static auto &thisClass = jni::Class<RbfxBillingClientStateListener>::Singleton(env);
    static auto method = thisClass.GetMethod<jni::jlong()>(env, "getPtr");

    auto thisObject = jni::Local<jni::Object<RbfxBillingClientStateListener>>(env, jni::Wrap<jni::jobject*>(env.NewLocalRef(obj)));
    long long savedPtr = thisObject.Call(env, method);
    auto billingManager = reinterpret_cast<BillingManagerAndroid*>(savedPtr);

    auto result = jni::Local<jni::Object<BillingResult>>(env, jni::Wrap<jni::jobject*>(env.NewLocalRef(billingResult)));
    billingManager->BillingSetupFinished(BillingResult::GetResponseCode(env, result), BillingResult::GetDebugMessage(env, result));
}

namespace Urho3D {
namespace Platform {
    jni::Local<jni::Object<RbfxBillingClientStateListener> > RbfxBillingClientStateListener::Create(jni::JNIEnv &env, BillingManagerAndroid *userData)
    {
        static auto &thisClass = jni::Class<RbfxBillingClientStateListener>::Singleton(env);
        static auto constructor = thisClass.GetConstructor<jni::jlong>(env);
        return thisClass.New(env, constructor, jni::jlong(reinterpret_cast<long long>(userData)));
    }
}
}
