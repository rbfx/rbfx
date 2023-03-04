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

#include "../../Monetization/Android/RbfxPurchasesUpdatedListener.h"

extern "C"
JNIEXPORT void JNICALL Java_io_rebelfork_RbfxPurchasesUpdatedListener_disposePtr(JNIEnv* env, jobject obj, jlong ptr) {
}

extern "C"
JNIEXPORT void JNICALL Java_io_rebelfork_RbfxPurchasesUpdatedListener_onPurchasesUpdated(JNIEnv *env, jobject obj, jobject billingResult, jobjectArray purchases)
{
}

namespace Urho3D {
namespace Platform {

jni::Local<jni::Object<RbfxPurchasesUpdatedListener> > RbfxPurchasesUpdatedListener::Create(jni::JNIEnv &env, void *userData)
{
    static auto &thisClass = jni::Class<RbfxPurchasesUpdatedListener>::Singleton(env);
    static auto constructor = thisClass.GetConstructor<jni::jlong>(env);
    return thisClass.New(env, constructor, jni::jlong(reinterpret_cast<long long>(userData)));
}
}
}
