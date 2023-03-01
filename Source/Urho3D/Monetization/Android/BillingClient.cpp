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

#include "../../Monetization/Android/AppContext.h"
#include "../../Monetization/Android/BillingClient.h"
#include "../../Monetization/Android/BillingClientBuilder.h"
#include "../../Monetization/Android/BillingClientStateListener.h"

namespace Urho3D
{
namespace Platform
{

jni::Local <jni::Object<BillingClientBuilder>> BillingClient::NewBuilder(jni::JNIEnv &env, const jni::Object <AppContext> &context) {
    static auto &thisClass = jni::Class<BillingClient>::Singleton(env);
    static auto newBuilderMethod = thisClass.GetStaticMethod<jni::Object<BillingClientBuilder>(jni::Object < AppContext > )>(env, "newBuilder");
    return thisClass.Call(env, newBuilderMethod, context);
}

void BillingClient::StartConnection(jni::JNIEnv &env, const jni::Object <BillingClient>& thisObject, const jni::Object <BillingClientStateListener>& listener)
{
    static auto &thisClass = jni::Class<BillingClient>::Singleton(env);
    static auto newBuilderMethod = thisClass.GetMethod<void(jni::Object < BillingClientStateListener > )>(env, "startConnection");
    return thisObject.Call(env, newBuilderMethod, listener);
}

}
}
