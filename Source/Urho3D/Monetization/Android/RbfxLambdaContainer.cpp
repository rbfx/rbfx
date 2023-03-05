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

#include "../../Monetization/Android/RbfxLambdaContainer.h"

#include <jni.h>

extern "C"
JNIEXPORT void JNICALL Java_io_rebelfork_RbfxLambdaContainer_disposePtr(JNIEnv* envPtr, jobject thiz, jlong ptr)
{
    using namespace Urho3D;
    using namespace Urho3D::Platform;
    jni::JNIEnv& env = *envPtr;

    static auto &thisClass = jni::Class<RbfxLambdaContainer>::Singleton(env);
    static auto field = thisClass.GetField<jni::jlong>(env, "lambdaPtr_");
    auto localRbfxLambdaContainer = jni::Local<jni::Object<Urho3D::Platform::RbfxLambdaContainer>>(env, jni::Wrap<jni::jobject*>(env.NewLocalRef(thiz)));
    jni::jlong lambdaPtr = localRbfxLambdaContainer.Get(env, field);
    delete reinterpret_cast<RbfxLambdaContainer*>((long long)lambdaPtr);
};

namespace Urho3D {
    namespace Platform {

        jni::Local<jni::Object<RbfxLambdaContainer> > RbfxLambdaContainer::Create(jni::JNIEnv &env, RbfxLambdaContainer* container)
        {
            static auto &thisClass = jni::Class<RbfxLambdaContainer>::Singleton(env);
            static auto constructor = thisClass.GetConstructor<jni::jlong>(env);
            return thisClass.New(env, constructor, jni::jlong(reinterpret_cast<long long>(container)));
        }
    }
}
